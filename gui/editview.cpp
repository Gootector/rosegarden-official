// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qlayout.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kstatusbar.h>

#include "Profiler.h"
#include "SegmentNotationHelper.h"
#include "AnalysisTypes.h"
#include "CompositionTimeSliceAdapter.h"

#include "editview.h"
#include "rosestrings.h"
#include "rosegardencanvasview.h"
#include "edittool.h"
#include "qcanvasgroupableitem.h"
#include "basiccommand.h"
#include "rosegardenguidoc.h"
#include "multiviewcommandhistory.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "segmentcommands.h"
#include "controlruler.h"

#include "rosedebug.h"

//----------------------------------------------------------------------
const unsigned int EditView::CONTROLS_ROW         = 0;
const unsigned int EditView::RULERS_ROW           = CONTROLS_ROW + 1;
const unsigned int EditView::TOPBARBUTTONS_ROW    = RULERS_ROW + 1;
const unsigned int EditView::CANVASVIEW_ROW       = TOPBARBUTTONS_ROW + 1;
const unsigned int EditView::BOTTOMBARBUTTONS_ROW = CANVASVIEW_ROW + 1;
const unsigned int EditView::HSCROLLBAR_ROW       = BOTTOMBARBUTTONS_ROW + 1;

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *> segments,
                   unsigned int cols,
                   QWidget *parent, const char *name) :
    EditViewBase(doc, segments, cols, parent, name),
    m_currentEventSelection(0),
    m_activeItem(0),
    m_canvasView(0),
    m_horizontalScrollBar(new QScrollBar(Horizontal, m_centralFrame)),
    m_rulerBox(new QVBoxLayout), // top ruler box - added to grid later on
    m_controlBox(new QVBoxLayout), // top control ruler box - added to grid later on
    m_bottomBox(new QVBoxLayout), // bottom box - added to grid later on
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_controlRuler(0)
{
    m_grid->addWidget(m_horizontalScrollBar, HSCROLLBAR_ROW,       m_mainCol);
    m_grid->addLayout(m_bottomBox,           BOTTOMBARBUTTONS_ROW, m_mainCol);

    m_grid->addLayout(m_rulerBox, RULERS_ROW, m_mainCol);
    m_grid->addMultiCellLayout(m_controlBox, CONTROLS_ROW, CONTROLS_ROW, 0, 1);
    m_controlBox->setAlignment(AlignRight);
}

EditView::~EditView()
{
    delete m_currentEventSelection;
    m_currentEventSelection = 0;
}

void EditView::paintEvent(QPaintEvent* e)
{
    EditViewBase::paintEvent(e);
    
    if (m_needUpdate)  {
        RG_DEBUG << "EditView::paintEvent() - calling updateView\n";
        updateView();
        getCanvasView()->slotUpdate();

        // update rulers
        QLayoutIterator it = m_rulerBox->iterator();
        QLayoutItem *child;
        while ( (child = it.current()) != 0 ) {
            if (child->widget()) child->widget()->update();
            ++it;
        }

        // update control ruler
        getControlRuler()->slotUpdate();

    } else {

        getCanvasView()->slotUpdate();
        getControlRuler()->slotUpdate();

    }

    m_needUpdate = false;
}

void EditView::setTopBarButtons(BarButtons* w)
{
    delete m_topBarButtons;
    m_topBarButtons = w;
    m_grid->addWidget(w, TOPBARBUTTONS_ROW, m_mainCol);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::setBottomBarButtons(BarButtons* w)
{
    delete m_bottomBarButtons;
    m_bottomBarButtons = w;

    m_bottomBox->insertWidget(0, w);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
}

void EditView::addRuler(QWidget* w)
{
    m_rulerBox->addWidget(w);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            w, SLOT(slotScrollHoriz(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            w, SLOT(slotScrollHoriz(int)));
}


void EditView::addPropertyBox(QWidget *w)
{
    m_controlBox->addWidget(w);
}

ControlRuler* EditView::makeControlRuler(Rosegarden::Staff* staff,
                                         Rosegarden::RulerScale* rulerScale)
{
    if (m_controlRuler) return m_controlRuler;

    QCanvas* controlRulerCanvas = new QCanvas(this);
    QSize viewSize = getViewSize();
    controlRulerCanvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight); // TODO - keep it in sync with main canvas size
    m_controlRuler = new ControlRuler(staff, rulerScale,
                                      m_horizontalScrollBar,
                                      controlRulerCanvas, getCentralFrame());

    m_bottomBox->addWidget(m_controlRuler);

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_controlRuler->horizontalScrollBar(), SIGNAL(valueChanged(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_controlRuler->horizontalScrollBar(), SIGNAL(sliderMoved(int)));


    return m_controlRuler;
}

void EditView::readjustViewSize(QSize requestedSize, bool exact)
{
    Rosegarden::Profiler profiler("EditView::readjustViewSize", true);

    if (exact) {
        RG_DEBUG << "EditView::readjustViewSize: exact size requested ("
                 << requestedSize.width() << ", " << requestedSize.height()
                 << ")\n";

        setViewSize(requestedSize);
        getCanvasView()->slotUpdate();
        return;
    }

    int requestedWidth  = requestedSize.width(),
        requestedHeight = requestedSize.height(),
        windowWidth     = width(),
        windowHeight        = height();

    QSize newSize;

    newSize.setWidth(((requestedWidth / windowWidth) + 1) * windowWidth);
    newSize.setHeight(((requestedHeight / windowHeight) + 1) * windowHeight);

    RG_DEBUG << "EditView::readjustViewSize: requested ("
			 << requestedSize.width() << ", " << requestedSize.height() 
			 << "), getting (" << newSize.width() <<", "
			 << newSize.height() << ")" << endl;

    setViewSize(newSize);

    getCanvasView()->slotUpdate();
}

void EditView::setCanvasView(RosegardenCanvasView *canvasView)
{
    delete m_canvasView;
    m_canvasView = canvasView;
    m_grid->addWidget(m_canvasView, CANVASVIEW_ROW, m_mainCol);

    m_horizontalScrollBar->setRange(m_canvasView->horizontalScrollBar()->minValue(),
                                    m_canvasView->horizontalScrollBar()->maxValue());

    m_horizontalScrollBar->setSteps(m_canvasView->horizontalScrollBar()->lineStep(),
                                    m_canvasView->horizontalScrollBar()->pageStep());

    connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
            m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)));
    connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
            m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)));

}

RosegardenCanvasView* EditView::getCanvasView()
{
    return m_canvasView;
}


Rosegarden::timeT
EditView::getInsertionTime(Rosegarden::Clef &clef,
			   Rosegarden::Key &key)
{
    Rosegarden::timeT t = getInsertionTime();
    Rosegarden::Segment *segment = getCurrentSegment();

    if (segment) {
	clef = segment->getClefAtTime(t);
	key = segment->getKeyAtTime(t);
    } else {
	clef = Rosegarden::Clef();
	key = Rosegarden::Key();
    }

    return t;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////


void EditView::slotActiveItemPressed(QMouseEvent* e,
                                 QCanvasItem* item)
{
    if (!item) return;

    // Check if it's a groupable item, if so get its group
    //
    QCanvasGroupableItem *gitem = dynamic_cast<QCanvasGroupableItem*>(item);
    if (gitem) item = gitem->group();
        
    // Check if it's an active item
    //
    ActiveItem *activeItem = dynamic_cast<ActiveItem*>(item);
        
    if (activeItem) {

        setActiveItem(activeItem);
        activeItem->handleMousePress(e);
        updateView();

    }
}

void
EditView::slotStepBackward()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = getInsertionTime();
    Rosegarden::Segment::iterator i = segment->findTime(time);

    while (i != segment->begin() &&
	   (i == segment->end() || (*i)->getAbsoluteTime() >= time)) --i;

    if (i != segment->end()) slotSetInsertCursorPosition((*i)->getAbsoluteTime());
}

void
EditView::slotStepForward()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = getInsertionTime();
    Rosegarden::Segment::iterator i = segment->findTime(time);

    while (segment->isBeforeEndMarker(i) &&
	   (*i)->getAbsoluteTime() <= time) ++i;

    if (!segment->isBeforeEndMarker(i)) {
	slotSetInsertCursorPosition(segment->getEndMarkerTime());
    } else {
	slotSetInsertCursorPosition((*i)->getAbsoluteTime());
    }
}

void
EditView::slotJumpBackward()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = getInsertionTime();
    time = segment->getBarStartForTime(time - 1);
    slotSetInsertCursorPosition(time);
}

void
EditView::slotJumpForward()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = getInsertionTime();
    time = segment->getBarEndForTime(time);
    slotSetInsertCursorPosition(time);
}

void
EditView::slotJumpToStart()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = segment->getStartTime();
    slotSetInsertCursorPosition(time);
}    

void
EditView::slotJumpToEnd()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::timeT time = segment->getEndMarkerTime();
    slotSetInsertCursorPosition(time);
}    


void EditView::slotExtendSelectionBackward()
{
    slotExtendSelectionBackward(false);
}

void EditView::slotExtendSelectionBackwardBar()
{
    slotExtendSelectionBackward(true);
}

void EditView::slotExtendSelectionBackward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the right of the cursor, move the cursor left and add to the
    // selection

    Rosegarden::timeT oldTime = getInsertionTime();
    if (bar) slotJumpBackward();
    else slotStepBackward();
    Rosegarden::timeT newTime = getInsertionTime();

    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::EventSelection *es = new Rosegarden::EventSelection(*segment);
    if (m_currentEventSelection) es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getStartTime() >= oldTime) {

	Rosegarden::Segment::iterator extendFrom = segment->findTime(oldTime);

	while (extendFrom != segment->begin() &&
	       (*--extendFrom)->getAbsoluteTime() >= newTime) {
	    if ((*extendFrom)->isa(Rosegarden::Note::EventType)) {
		es->addEvent(*extendFrom);
	    }
	}

    } else { // remove an event

	Rosegarden::EventSelection::eventcontainer::iterator i =
	    es->getSegmentEvents().end();

	std::vector<Rosegarden::Event *> toErase;

	while (i != es->getSegmentEvents().begin() &&
	       (*--i)->getAbsoluteTime() >= newTime) {
	    toErase.push_back(*i);
	}

	for (unsigned int j = 0; j < toErase.size(); ++j) {
	    es->removeEvent(toErase[j]);
	}
    }
    
    setCurrentSelection(es);
}

void EditView::slotExtendSelectionForward()
{
    slotExtendSelectionForward(false);
}

void EditView::slotExtendSelectionForwardBar()
{
    slotExtendSelectionForward(true);
}

void EditView::slotExtendSelectionForward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the left of the cursor, move the cursor right and add to the
    // selection

    Rosegarden::timeT oldTime = getInsertionTime();
    if (bar) slotJumpForward();
    else slotStepForward();
    Rosegarden::timeT newTime = getInsertionTime();

    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::EventSelection *es = new Rosegarden::EventSelection(*segment);
    if (m_currentEventSelection) es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getEndTime() <= oldTime) {

	Rosegarden::Segment::iterator extendFrom = segment->findTime(oldTime);

	while (extendFrom != segment->end() &&
	       (*extendFrom)->getAbsoluteTime() < newTime) {
	    if ((*extendFrom)->isa(Rosegarden::Note::EventType)) {
		es->addEvent(*extendFrom);
	    }
	    ++extendFrom;
	}

    } else { // remove an event

	Rosegarden::EventSelection::eventcontainer::iterator i =
	    es->getSegmentEvents().begin();

	std::vector<Rosegarden::Event *> toErase;

	while (i != es->getSegmentEvents().end() &&
	       (*i)->getAbsoluteTime() < newTime) {
	    toErase.push_back(*i);
	    ++i;
	}

	for (unsigned int j = 0; j < toErase.size(); ++j) {
	    es->removeEvent(toErase[j]);
	}
    }
    
    setCurrentSelection(es);
}


void
EditView::setupActions()
{
    createInsertPitchActionMenu();

    new KAction(AddTempoChangeCommand::getGlobalName(), 0, this,
                SLOT(slotAddTempo()), actionCollection(),
                "add_tempo");

    new KAction(AddTimeSignatureCommand::getGlobalName(), 0, this,
                SLOT(slotAddTimeSignature()), actionCollection(),
                "add_time_signature");

    new KToggleAction(i18n("Show Velocity Control Ruler"), 0, this,
                      SLOT(slotShowControlRuler()), actionCollection(),
                      "show_control_ruler");
}

void
EditView::createInsertPitchActionMenu()
{
    QString notePitchNames[] = {
	i18n("I"), i18n("II"), i18n("III"), i18n("IV"),
	i18n("V"), i18n("VI"), i18n("VII"), i18n("VIII")
    };
    QString flat  = i18n("%1 flat");
    QString sharp = i18n("%1 sharp");

    const Key notePitchKeys[3][7] = {
	{
	    Key_A, Key_S, Key_D, Key_F, Key_J, Key_K, Key_L,
	},
	{
	    Key_Q, Key_W, Key_E, Key_R, Key_U, Key_I, Key_O,
	},
	{
	    Key_Z, Key_X, Key_C, Key_V, Key_B, Key_N, Key_M,
	},
    };

    KActionMenu *insertPitchActionMenu =
	new KActionMenu(i18n("&Insert Note"), this, "insert_note_actionmenu");

    for (int octave = 0; octave <= 2; ++octave) {

	KActionMenu *menu = insertPitchActionMenu;
	if (octave == 1) {
	    menu = new KActionMenu(i18n("&Upper Octave"), this,
				   "insert_note_actionmenu_upper_octave");
	    insertPitchActionMenu->insert(new KActionSeparator(this));
	    insertPitchActionMenu->insert(menu);
	} else if (octave == 2) {
	    menu = new KActionMenu(i18n("&Lower Octave"), this,
				   "insert_note_actionmenu_lower_octave");
	    insertPitchActionMenu->insert(menu);
	}

	for (unsigned int i = 0; i < 7; ++i) {

	    KAction *insertPitchAction = 0;

	    QString octaveSuffix;
	    if (octave == 1) octaveSuffix = "_high";
	    else if (octave == 2) octaveSuffix = "_low";

	    // do and fa lack a flat

	    if (i != 0 && i != 3) {
      
		insertPitchAction =
		    new KAction
		    (flat.arg(notePitchNames[i]),
		     CTRL + SHIFT + notePitchKeys[octave][i],
		     this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		     QString("insert_%1_flat%2").arg(i).arg(octaveSuffix));

		menu->insert(insertPitchAction);
	    }

	    insertPitchAction =
		new KAction
		(notePitchNames[i],
		 notePitchKeys[octave][i],
		 this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		 QString("insert_%1%2").arg(i).arg(octaveSuffix));

	    menu->insert(insertPitchAction);

	    // and mi and ti lack a sharp

	    if (i != 2 && i != 6) {

		insertPitchAction =
		    new KAction
		    (sharp.arg(notePitchNames[i]),
		     SHIFT + notePitchKeys[octave][i],
		     this, SLOT(slotInsertNoteFromAction()), actionCollection(),
		     QString("insert_%1_sharp%2").arg(i).arg(octaveSuffix));

		menu->insert(insertPitchAction);
	    }

	    if (i < 6) menu->insert(new KActionSeparator(this));
	}
    }

    actionCollection()->insert(insertPitchActionMenu);
}

int
EditView::getPitchFromNoteInsertAction(QString name,
				       Rosegarden::Accidental &accidental)
{
    using namespace Rosegarden::Accidentals;

    accidental = NoAccidental;

    Rosegarden::Key key;
    Rosegarden::Clef clef;

    //!!! modify to use Pitch class

    if (name.left(7) == "insert_") {

	name = name.right(name.length()-7);
	
	int modify = 0;
	int octave = 0;

	if (name.right(5) == "_high") {
	    
	    octave = 1;
	    name = name.left(name.length()-5);
	
	} else if (name.right(4) == "_low") {

	    octave = -1;
	    name = name.left(name.length()-4);
	}

	if (name.right(6) == "_sharp") {

	    modify = 1;
	    accidental = Sharp;
	    name = name.left(name.length()-6);

	} else if (name.right(5) == "_flat") {

	    modify = -1;
	    accidental = Flat;
	    name = name.left(name.length()-5);
	}

	int scalePitch = name.toInt();
	
	if (scalePitch < 0 || scalePitch > 7) {
	    NOTATION_DEBUG << "EditView::getPitchFromNoteInsertAction: pitch "
			   << scalePitch << " out of range, using 0" <<endl;
	    scalePitch = 0;
	}

	int pitch =
	    key.getTonicPitch() + 60 + 12*(octave + clef.getOctave()) + modify;

	static int scale[] = { 0, 2, 4, 5, 7, 9, 11 };
	pitch += scale[scalePitch];

/*!!!
	if (accidental != NoAccidental) {
	    Rosegarden::NotationDisplayPitch ndp(pitch, clef, key);
	    int height = ndp.getHeightOnStaff();
	    Rosegarden::Accidental defaultAcc = key.getAccidentalAtHeight(height, clef);
	    if ((defaultAcc == Sharp || defaultAcc == Flat) &&
		defaultAcc != accidental) accidental = Natural;
	    else if (defaultAcc == Sharp) accidental = DoubleSharp;
	    else                          accidental = DoubleFlat;
	}
*/
	return pitch;

    } else {

	throw Rosegarden::Exception("Not an insert action",
				    __FILE__, __LINE__);
    }
}

void EditView::slotAddTempo()
{
    Rosegarden::timeT insertionTime = getInsertionTime();

    TempoDialog tempoDlg(this, getDocument());

    connect(&tempoDlg,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)),
            this,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               double, TempoDialog::TempoDialogAction)));
        
    tempoDlg.setTempoPosition(insertionTime);
    tempoDlg.exec();
}

void EditView::slotAddTimeSignature()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return;
    Rosegarden::Composition *composition = segment->getComposition();
    Rosegarden::timeT insertionTime = getInsertionTime();

    int barNo = composition->getBarNumber(insertionTime);
    bool atStartOfBar = (insertionTime == composition->getBarStart(barNo));

    Rosegarden::CompositionTimeSliceAdapter adapter
        (&getDocument()->getComposition(), insertionTime,
         getDocument()->getComposition().getDuration());
    Rosegarden::AnalysisHelper helper;
    Rosegarden::TimeSignature timeSig = helper.guessTimeSignature(adapter);

    TimeSignatureDialog *dialog = new TimeSignatureDialog
        (this, timeSig, barNo, atStartOfBar,
         i18n("Estimated time signature shown"));
    
    if (dialog->exec() == QDialog::Accepted) {

        TimeSignatureDialog::Location location = dialog->getLocation();
        if (location == TimeSignatureDialog::StartOfBar) {
            insertionTime = composition->getBarStartForTime(insertionTime);
        }
        
        if (dialog->shouldNormalizeRests()) {
            
            addCommandToHistory(new AddTimeSignatureAndNormalizeCommand
                                (composition, insertionTime,
                                 dialog->getTimeSignature()));
            
        } else {
            
            addCommandToHistory(new AddTimeSignatureCommand
                                (composition, insertionTime,
                                 dialog->getTimeSignature()));
        }
    }
    
    delete dialog;
}                       

void EditView::slotShowControlRuler()
{
    // the control ruler should be created by the overriding implementation
    // of this slot
    //
    if (m_controlRuler) {
        if (m_controlRuler->isVisible())
            m_controlRuler->hide();
        else
            m_controlRuler->show();
    }
}
