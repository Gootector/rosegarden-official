// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <qpopupmenu.h>
#include <qinputdialog.h>
#include <qobjectlist.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kstatusbar.h>
#include <klistbox.h>

#if KDE_VERSION >= 197120 // 320
#include <ktabwidget.h>
#else
#include "kde32_ktabwidget.h"
#endif

#include <kiconloader.h>

#include "BaseProperties.h"
#include "Profiler.h"
#include "SegmentNotationHelper.h"
#include "AnalysisTypes.h"
#include "CompositionTimeSliceAdapter.h"
#include "MidiTypes.h"

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
#include "loopruler.h"

#include "rosedebug.h"

using Rosegarden::PropertyName;
using Rosegarden::ControlParameter;

//----------------------------------------------------------------------
const unsigned int EditView::CONTROLS_ROW         = 0;
const unsigned int EditView::RULERS_ROW           = CONTROLS_ROW + 1;
const unsigned int EditView::TOPBARBUTTONS_ROW    = RULERS_ROW + 1;
const unsigned int EditView::CANVASVIEW_ROW       = TOPBARBUTTONS_ROW + 1;
const unsigned int EditView::CONTROLRULER_ROW     = CANVASVIEW_ROW + 1;

// Just some simple features we might want to show - make them bit maskable
//
static int FeatureShowVelocity = 0x00001; // show the velocity ruler


EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Rosegarden::Segment *> segments,
                   unsigned int cols,
                   QWidget *parent, const char *name) :
    EditViewBase(doc, segments, cols, parent, name),
    m_currentEventSelection(0),
    m_activeItem(0),
    m_canvasView(0),
    m_rulerBox(new QVBoxLayout), // top ruler box - added to grid later on
    m_controlBox(new QVBoxLayout), // top control ruler box - added to grid later on
    m_bottomBox(new QVBox(this, "bottomframe")), // bottom box - added to bottom of canvas view by setCanvasView()
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_controlRuler(0),
#if KDE_VERSION < 197120
    m_controlRulers(new KDE32Backport::KTabWidget(getBottomWidget(), "controlrulers"))
#else
    m_controlRulers(new KTabWidget(getBottomWidget(), "controlrulers"))
#endif
{
    m_controlRulers->setHoverCloseButton(true);
    m_controlRulers->setHoverCloseButtonDelayed(false); 
    connect(m_controlRulers, SIGNAL(closeRequest(QWidget*)),
            this, SLOT(slotRemoveControlRuler(QWidget*)));

    (dynamic_cast<QBoxLayout*>(m_bottomBox->layout()))->setDirection(QBoxLayout::BottomToTop);

    m_grid->addLayout(m_rulerBox, RULERS_ROW, m_mainCol);
    m_grid->addMultiCellLayout(m_controlBox, CONTROLS_ROW, CONTROLS_ROW, 0, 1);
    m_controlBox->setAlignment(AlignRight);
//     m_grid->addWidget(m_controlRulers, CONTROLRULER_ROW, 2);
    
    m_controlRulers->hide();
    m_controlRulers->setTabPosition(QTabWidget::Bottom);

}

EditView::~EditView()
{
    delete m_currentEventSelection;
    m_currentEventSelection = 0;
}

void EditView::updateBottomWidgetGeometry()
{
    getBottomWidget()->layout()->invalidate();
    getBottomWidget()->updateGeometry();
    getCanvasView()->updateBottomWidgetGeometry();
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

        updateControlRulers();
        
    } else {

        getCanvasView()->slotUpdate();
        updateControlRulers();

    }

    m_needUpdate = false;
}

void EditView::updateControlRulers(bool updateHPos)
{
    for(int i = 0; i < m_controlRulers->count(); ++i) {
        ControlRuler* ruler = dynamic_cast<ControlRuler*>(m_controlRulers->page(i));
        if (ruler) {
            if (updateHPos)
                ruler->slotUpdateElementsHPos();
            else
                ruler->slotUpdate();
        }
    }
}

void EditView::setControlRulersZoom(QWMatrix zoomMatrix)
{
    m_currentRulerZoomMatrix = zoomMatrix;
    
    for(int i = 0; i < m_controlRulers->count(); ++i) {
        ControlRuler* ruler = dynamic_cast<ControlRuler*>(m_controlRulers->page(i));
        if (ruler) ruler->setWorldMatrix(zoomMatrix);
    }
}


void EditView::setTopBarButtons(BarButtons* w)
{
    delete m_topBarButtons;
    m_topBarButtons = w;
    m_grid->addWidget(w, TOPBARBUTTONS_ROW, m_mainCol);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                m_topBarButtons, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                m_topBarButtons, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::setBottomBarButtons(BarButtons* w)
{
    delete m_bottomBarButtons;
    m_bottomBarButtons = w;

//     m_bottomBox->insertWidget(0, w);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::setRewFFwdToAutoRepeat()
{
    QWidget* transportToolbar = factory()->container("Transport Toolbar", this);

    if (transportToolbar) {
        QObjectList *l = transportToolbar->queryList();
        QObjectListIt it(*l); // iterate over the buttons
        QObject *obj;

        while ( (obj = it.current()) != 0 ) {
            // for each found object...
            ++it;
//             RG_DEBUG << "EditView::setRewFFwdToAutoRepeat() : obj name : " << obj->name() << endl;
            QString objName = obj->name();
            
            if (objName.endsWith("playback_pointer_back_bar") || objName.endsWith("playback_pointer_forward_bar")) {
                QButton* btn = dynamic_cast<QButton*>(obj);
                if (!btn) {
                    RG_DEBUG << "Very strange - found widgets in Transport Toolbar which aren't buttons\n";
                    
                    continue;
                }
                btn->setAutoRepeat(true);
            }
            
            
        }
        delete l;

    } else {
        RG_DEBUG << "transportToolbar == 0\n";
    }
 
}

void EditView::addRuler(QWidget* w)
{
    m_rulerBox->addWidget(w);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                w, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                w, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::addPropertyBox(QWidget *w)
{
    m_controlBox->addWidget(w);
}

PropertyControlRuler* EditView::makePropertyControlRuler(PropertyName propertyName)
{
    QCanvas* controlRulerCanvas = new QCanvas(this);
    QSize viewSize = getViewSize();
    controlRulerCanvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight); // TODO - keep it in sync with main canvas size

//     QCanvas* controlRulerCanvas = ControlRulerCanvasRepository::getCanvas(getCurrentSegment(), propertyName,
//                                                                           getViewSize());

    PropertyControlRuler* controlRuler = new PropertyControlRuler
	(propertyName, getCurrentStaff(), getHLayout(), this,
	 controlRulerCanvas, m_controlRulers);

    return controlRuler;
}

ControllerEventsRuler* EditView::makeControllerEventRuler(ControlParameter *controller)
{
    QCanvas* controlRulerCanvas = new QCanvas(this);
    QSize viewSize = getViewSize();
    controlRulerCanvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight); // TODO - keep it in sync with main canvas size
//     QCanvas* controlRulerCanvas = ControlRulerCanvasRepository::getCanvas(getCurrentSegment(), controller,
//                                                                           getViewSize());
    

    ControllerEventsRuler* controlRuler = new ControllerEventsRuler
	(*getCurrentSegment(), getHLayout(), this,
	 controlRulerCanvas, m_controlRulers, controller);

    return controlRuler;
}

void EditView::addControlRuler(ControlRuler* ruler)
{
    ruler->setWorldMatrix(m_currentRulerZoomMatrix);
    m_controlRulers->addTab(ruler, KGlobal::iconLoader()->loadIconSet("fileclose", KIcon::Small),
                            ruler->getName());
    m_controlRulers->showPage(ruler);
    
    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                ruler->horizontalScrollBar(), SIGNAL(valueChanged(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                ruler->horizontalScrollBar(), SIGNAL(sliderMoved(int)));
    }
    

    stateChanged("have_control_ruler", KXMLGUIClient::StateReverse);
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
        windowHeight    = height();

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
    m_canvasView->setBottomFixedWidget(m_bottomBox);

    // TODO : connect canvas view's horiz. scrollbar to top/bottom bars and rulers

//     m_horizontalScrollBar->setRange(m_canvasView->horizontalScrollBar()->minValue(),
//                                     m_canvasView->horizontalScrollBar()->maxValue());

//     m_horizontalScrollBar->setSteps(m_canvasView->horizontalScrollBar()->lineStep(),
//                                     m_canvasView->horizontalScrollBar()->pageStep());

//     connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
//             m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)));
//     connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
//             m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)));

}

RosegardenCanvasView* EditView::getCanvasView()
{
    return m_canvasView;
}


Rosegarden::Device *
EditView::getCurrentDevice()
{
    Rosegarden::Segment *segment = getCurrentSegment();
    if (!segment) return 0;

    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Instrument *instrument =
	studio.getInstrumentById
	(segment->getComposition()->getTrackById(segment->getTrack())->
	 getInstrument());
    if (!instrument) return 0;
    
    return instrument->getDevice();
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
    Rosegarden::Staff *staff = getCurrentStaff();
    if (!staff) return;
    Rosegarden::ViewElementList *vel = staff->getViewElementList();

    Rosegarden::timeT time = getInsertionTime();
    Rosegarden::ViewElementList::iterator i = vel->findTime(time);

    while (i != vel->begin() &&
	   (i == vel->end() || (*i)->getViewAbsoluteTime() >= time)) --i;

    if (i != vel->end()) slotSetInsertCursorPosition((*i)->getViewAbsoluteTime());
}

void
EditView::slotStepForward()
{
    Rosegarden::Staff *staff = getCurrentStaff();
    if (!staff) return;
    Rosegarden::ViewElementList *vel = staff->getViewElementList();

    Rosegarden::timeT time = getInsertionTime();
    Rosegarden::ViewElementList::iterator i = vel->findTime(time);

    while (i != vel->end() &&
	   (*i)->getViewAbsoluteTime() <= time) ++i;

    if (i == vel->end()) {
	slotSetInsertCursorPosition(staff->getSegment().getEndMarkerTime());
    } else {
	slotSetInsertCursorPosition((*i)->getViewAbsoluteTime());
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

    Rosegarden::Staff *staff = getCurrentStaff();
    if (!staff) return;
    Rosegarden::Segment *segment = &staff->getSegment();
    Rosegarden::ViewElementList *vel = staff->getViewElementList();

    Rosegarden::EventSelection *es = new Rosegarden::EventSelection(*segment);
    if (m_currentEventSelection &&
	&m_currentEventSelection->getSegment() == segment)
	es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getStartTime() >= oldTime) {

	Rosegarden::ViewElementList::iterator extendFrom = vel->findTime(oldTime);

	while (extendFrom != vel->begin() &&
	       (*--extendFrom)->getViewAbsoluteTime() >= newTime) {
	    if ((*extendFrom)->event()->isa(Rosegarden::Note::EventType)) {
		es->addEvent((*extendFrom)->event());
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

    Rosegarden::Staff *staff = getCurrentStaff();
    if (!staff) return;
    Rosegarden::Segment *segment = &staff->getSegment();
    Rosegarden::ViewElementList *vel = staff->getViewElementList();

    Rosegarden::EventSelection *es = new Rosegarden::EventSelection(*segment);
    if (m_currentEventSelection &&
	&m_currentEventSelection->getSegment() == segment)
	es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getEndTime() <= oldTime) {

	Rosegarden::ViewElementList::iterator extendFrom = vel->findTime(oldTime);

	while (extendFrom != vel->end() &&
	       (*extendFrom)->getViewAbsoluteTime() < newTime) {
	    if ((*extendFrom)->event()->isa(Rosegarden::Note::EventType)) {
		es->addEvent((*extendFrom)->event());
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

    //
    // Transforms
    //
    new KAction(TransposeCommand::getGlobalName(1), 0,
		Key_Up, this,
                SLOT(slotTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(TransposeCommand::getGlobalName(12), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(TransposeCommand::getGlobalName(-1), 0,
		Key_Down, this,
                SLOT(slotTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(TransposeCommand::getGlobalName(-12), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(TransposeCommand::getGlobalName(0), 0, this,
                SLOT(slotTranspose()), actionCollection(),
                "general_transpose");

    new KAction(i18n("Jog &Left"), Key_Left + ALT, this,
                SLOT(slotJogLeft()), actionCollection(),
                "jog_left");

    new KAction(i18n("Jog &Right"), Key_Right + ALT, this,
                SLOT(slotJogRight()), actionCollection(),
                "jog_right");

    // Control rulers
    //
    new KAction(i18n("Show Velocity Property Ruler"), 0, this,
                SLOT(slotShowVelocityControlRuler()), actionCollection(),
                "show_velocity_control_ruler");

    /*
    new KAction(i18n("Show Controllers Events Ruler"), 0, this,
                SLOT(slotShowControllerEventsRuler()), actionCollection(),
                "show_controller_events_ruler");
                */

    // Disabled for now
    //
//     new KAction(i18n("Add Control Ruler..."), 0, this,
//                 SLOT(slotShowPropertyControlRuler()), actionCollection(),
//                 "add_control_ruler");

    //
    // Control Ruler context menu
    //
    new KAction(i18n("Insert item"), 0, this,
		SLOT(slotInsertControlRulerItem()), actionCollection(),
		"insert_control_ruler_item");

    // This was on Key_Delete, but that conflicts with existing Delete commands
    // on individual edit views
    new KAction(i18n("Erase selected items"), 0, this,
		SLOT(slotEraseControlRulerItem()), actionCollection(),
		"erase_control_ruler_item");

    new KAction(i18n("Clear ruler"), 0, this,
		SLOT(slotClearControlRulerItem()), actionCollection(),
		"clear_control_ruler_item");

    new KAction(i18n("Insert line of controllers"), 0, this,
		SLOT(slotStartControlLineItem()), actionCollection(),
		"start_control_line_item");

    new KAction(i18n("Flip forward"), Key_BracketRight, this,
		SLOT(slotFlipForwards()), actionCollection(),
		"flip_control_events_forward");

    new KAction(i18n("Flip backwards"), Key_BracketLeft, this,
		SLOT(slotFlipBackwards()), actionCollection(),
		"flip_control_events_back");

    new KAction(i18n("Draw property line"), 0, this,
                SLOT(slotDrawPropertyLine()), actionCollection(),
                "draw_property_line");

    new KAction(i18n("Select all property values"), 0, this,
                SLOT(slotSelectAllProperties()), actionCollection(),
                "select_all_properties");
}

void
EditView::setupAddControlRulerMenu()
{
    QPopupMenu* addControlRulerMenu = dynamic_cast<QPopupMenu*>
        (factory()->container("add_control_ruler", this));

    if (addControlRulerMenu) {

	//!!! problem here with notation view -- current segment can
	// change after construction, but this function isn't used again

	Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>
	    (getCurrentDevice());
	if (!md) return;

        const Rosegarden::ControlList &list = md->getControlParameters();

        int i = 0;
        QString itemStr;

        for (Rosegarden::ControlList::const_iterator it = list.begin();
	     it != list.end(); ++it)
        {
            if (it->getType() == Rosegarden::Controller::EventType)
            {
                QString hexValue;
                hexValue.sprintf("(0x%x)", it->getControllerValue());

                itemStr = i18n("%1 Controller %2 %3").arg(strtoqstr(it->getName()))
                                                     .arg(it->getControllerValue())
                                                     .arg(hexValue);

            } else if (it->getType() == Rosegarden::PitchBend::EventType)
                itemStr = i18n("Pitch Bend");
            else
                itemStr = i18n("Unsupported Event Type");

            addControlRulerMenu->insertItem(itemStr, i++);
        }

        connect(addControlRulerMenu, SIGNAL(activated(int)),
                SLOT(slotAddControlRuler(int)));
    }

}

void
EditView::setupControllerTabs()
{
    // Setup control rulers the Segment already has some stored against it.
    //
    Rosegarden::Segment *segment = getCurrentSegment();
    Rosegarden::Segment::EventRulerList list = segment->getEventRulerList();

    RG_DEBUG << "EditView::setupControllerTabs - got " << list.size() << " EventRulers" << endl;

    if (list.size())
    {
	Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>
	    (getCurrentDevice());
	if (!md) return;

        Rosegarden::Segment::EventRulerListIterator it;

        for (it = list.begin(); it != list.end(); ++it)
        {
            // Get ControlParameter object from controller value
            //
            ControlParameter *controlParameter = 
                md->getControlParameter((*it)->m_type, Rosegarden::MidiByte((*it)->m_controllerValue));

            RG_DEBUG << "EditView::setupControllerTabs - " 
                     << "Control Parameter type = " << (*it)->m_type << endl;

            if (controlParameter)
            {
                ControllerEventsRuler* controlRuler = makeControllerEventRuler(controlParameter);
                addControlRuler(controlRuler);
                RG_DEBUG << "EditView::setupControllerTabs - adding Ruler" << endl;
            }
        }

        if (!m_controlRulers->isVisible())
            m_controlRulers->show();

        updateBottomWidgetGeometry();
    }
}



void
EditView::slotAddControlRuler(int controller)
{
    RG_DEBUG << "EditView::slotAddControlRuler - item = " 
             << controller << endl;

    Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>
	(getCurrentDevice());
    if (!md) return;

    const Rosegarden::ControlList &list = md->getControlParameters();
    ControlParameter control = list[controller];

    int index = 0;
    
    ControlRuler* existingRuler = findRuler(control, index);

    if (existingRuler) {

        m_controlRulers->setCurrentPage(index);

    } else {

        // Create control ruler to a specific controller.  This duplicates
        // the control parameter in the supplied pointer.
        ControllerEventsRuler* controlRuler = makeControllerEventRuler(&control);
    
        addControlRuler(controlRuler);
    }
    
    if (!m_controlRulers->isVisible()) {
	m_controlRulers->show();
    }
    
    updateBottomWidgetGeometry();
    
    // Add the controller to the segment so the views can
    // remember what we've opened against it.
    //
    Rosegarden::Staff *staff = getCurrentStaff();
    staff->getSegment().addEventRuler(control.getType(), control.getControllerValue());

    getDocument()->slotDocumentModified();
}

void EditView::slotRemoveControlRuler(QWidget* w)
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(w);

    if (ruler) {
        ControlParameter *controller = ruler->getControlParameter();

        // remove the control parameter from the "showing controllers" list on the segment
        //
        if (controller) {
            Rosegarden::Staff *staff = getCurrentStaff();
            bool value = staff->getSegment().
                deleteEventRuler(controller->getType(), controller->getControllerValue());

            if (value) 
                RG_DEBUG << "slotRemoveControlRuler : removed controller from segment\n";
            else
                RG_DEBUG << "slotRemoveControlRuler : couldn't remove controller from segment - " 
                         << int(controller->getControllerValue())
                         << endl;

        }
    } else { // else it's probably a velocity ruler
        PropertyControlRuler *propertyRuler = dynamic_cast<PropertyControlRuler*>(w);

        if (propertyRuler) {
            Rosegarden::Segment &seg = getCurrentStaff()->getSegment();
            seg.setViewFeatures(0); // for the moment we only have one view feature so
                                    // we can just blank it out

            RG_DEBUG << "slotRemoveControlRuler : removed velocity ruler" << endl;
        }
    }
    
    delete w;

    if (m_controlRulers->count() == 0) {
        m_controlRulers->hide();
        updateBottomWidgetGeometry();
    }
    
    getDocument()->slotDocumentModified();
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

    //!!! need to be able to get clef & key -- how?
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

	//!!! needs testing before we remove old stuff
	Rosegarden::Pitch pitch
	    (scalePitch, 5 + octave + clef.getOctave(), key, accidental);
	return pitch.getPerformancePitch();

/*!!!
	int pitch =
	    key.getTonicPitch() + 60 + 12*(octave + clef.getOctave()) + modify;

	static int scale[] = { 0, 2, 4, 5, 7, 9, 11 };
	pitch += scale[scalePitch];
*/
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
//!!!	return pitch;

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

    TimeSignatureDialog *dialog = 0;
    int timeSigNo = composition->getTimeSignatureNumberAt(insertionTime);

    if (timeSigNo >= 0) {

	dialog = new TimeSignatureDialog
	    (this, composition, insertionTime,
	     composition->getTimeSignatureAt(insertionTime));

    } else {

	Rosegarden::timeT endTime = composition->getDuration();
	if (composition->getTimeSignatureCount() > 0) {
	    endTime = composition->getTimeSignatureChange(0).first;
	}

	Rosegarden::CompositionTimeSliceAdapter adapter
	    (composition, insertionTime, endTime);
	Rosegarden::AnalysisHelper helper;
	Rosegarden::TimeSignature timeSig = helper.guessTimeSignature(adapter);
	
	dialog = new TimeSignatureDialog
	    (this, composition, insertionTime, timeSig, false,
	     i18n("Estimated time signature shown"));
    }
    
    if (dialog->exec() == QDialog::Accepted) {

	insertionTime = dialog->getTime();
        
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

ControlRuler* EditView::findRuler(PropertyName propertyName, int &index)
{
    for(index = 0; index < m_controlRulers->count(); ++index) {
        PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>(m_controlRulers->page(index));
        if (ruler && ruler->getPropertyName() == propertyName) return ruler;
    }

    return 0;
}

ControlRuler* EditView::findRuler(const ControlParameter& controller, int &index)
{
    for(index = 0; index < m_controlRulers->count(); ++index) {
        ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(m_controlRulers->page(index));
        if (ruler && *(ruler->getControlParameter()) == controller) return ruler;
    }

    return 0;
}

void EditView::showPropertyControlRuler(PropertyName propertyName)
{
    int index = 0;
    
    ControlRuler* existingRuler = findRuler(propertyName, index);

    if (existingRuler) {

        m_controlRulers->setCurrentPage(index);

    } else {

        PropertyControlRuler* controlRuler = makePropertyControlRuler(propertyName);
        addControlRuler(controlRuler);
    }
    
    if (!m_controlRulers->isVisible()) {
        m_controlRulers->show();
    }

    updateBottomWidgetGeometry();
}

ControlRuler* EditView::getCurrentControlRuler()
{
    return dynamic_cast<ControlRuler*>(m_controlRulers->currentPage());
}

void EditView::slotShowVelocityControlRuler()
{
    showPropertyControlRuler(Rosegarden::BaseProperties::VELOCITY);
    Rosegarden::Segment &seg = getCurrentStaff()->getSegment();
    seg.setViewFeatures(seg.getViewFeatures() | FeatureShowVelocity);
    getDocument()->slotDocumentModified();
}

/* 
 * This is the "generic case" control ruler which we probably don't even need 
 *
 */
void EditView::slotShowControllerEventsRuler()
{

//     int index = 0;
    
//     ControlRuler* existingRuler = findRuler(propertyName, index);

//     if (existingRuler) {

//         m_controlRulers->setCurrentPage(index);

//     } else {

//         ControllerEventsRuler* controlRuler = makeControllerEventRuler();
//         addControlRuler(controlRuler);
//     }
    
//     if (!m_controlRulers->isVisible()) {
//         m_controlRulers->show();
//     }

//     updateBottomWidgetGeometry();
}

class QListBoxRGProperty : public QListBoxText
{
public:
    QListBoxRGProperty(QListBox* parent, PropertyName propertyName);
    const PropertyName& getPropertyName() { return m_propertyName; }
protected:
    PropertyName m_propertyName;
};

QListBoxRGProperty::QListBoxRGProperty(QListBox* parent, PropertyName propertyName)
    : QListBoxText(parent, propertyName.c_str()),
      m_propertyName(propertyName)
{
}



void EditView::slotShowPropertyControlRuler()
{
/*
    KDialogBase propChooserDialog(this, "propertychooserdialog", true, i18n("Select event property"),
                                  KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok);
    
    KListBox* propList = new KListBox(propChooserDialog.makeVBoxMainWidget());
    new QListBoxRGProperty(propList, Rosegarden::BaseProperties::VELOCITY.c_str());

    int rc = propChooserDialog.exec();
    if (rc == QDialog::Accepted) {
        // fix for KDE 3.0
        //QListBoxRGProperty* item = dynamic_cast<QListBoxRGProperty*>(propList->selectedItem());
        QListBoxRGProperty* item = dynamic_cast<QListBoxRGProperty*>
            (propList->item(propList->currentItem()));

        if (item) {
            PropertyName property = item->getPropertyName();
            showPropertyControlRuler(property);
        }
    }
*/
}

void
EditView::slotInsertControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->insertControllerEvent();
}

void
EditView::slotEraseControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->eraseControllerEvent();
}

void
EditView::slotStartControlLineItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->startControlLine();
}

void
EditView::slotDrawPropertyLine()
{
    int index = 0;
    PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>
        (findRuler(Rosegarden::BaseProperties::VELOCITY, index));

    if (ruler) ruler->startPropertyLine();
}

void
EditView::slotSelectAllProperties()
{
    int index = 0;
    PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>
        (findRuler(Rosegarden::BaseProperties::VELOCITY, index));

    if (ruler) ruler->selectAllProperties();
}




void
EditView::slotClearControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->clearControllerEvents();
}

void EditView::slotTranspose()
{
    if (!m_currentEventSelection) return;

    bool ok = false;
    int semitones = QInputDialog::getInteger
	(i18n("Transpose"),
	 i18n("Enter the number of semitones to transpose by:"),
	 0, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    KTmpStatusMsg msg(i18n("Transposing..."), this);
    addCommandToHistory(new TransposeCommand
                        (semitones, *m_currentEventSelection));
}

void EditView::slotTransposeUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), this);

    addCommandToHistory(new TransposeCommand(1, *m_currentEventSelection));
}

void EditView::slotTransposeUpOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), this);

    addCommandToHistory(new TransposeCommand(12, *m_currentEventSelection));
}

void EditView::slotTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), this);

    addCommandToHistory(new TransposeCommand(-1, *m_currentEventSelection));
}

void EditView::slotTransposeDownOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), this);

    addCommandToHistory(new TransposeCommand(-12, *m_currentEventSelection));
}

void EditView::slotJogLeft()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Jogging left..."), this);

    RG_DEBUG << "EditView::slotJogLeft" << endl;

    addCommandToHistory(
            new MoveCommand(*getCurrentSegment(),
                -Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration(),
                false, // don't use notation timings
                *m_currentEventSelection));
}

void EditView::slotJogRight()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Jogging right..."), this);

    RG_DEBUG << "EditView::slotJogRight" << endl;

    addCommandToHistory(
            new MoveCommand(*getCurrentSegment(),
                Rosegarden::Note(Rosegarden::Note::Demisemiquaver).getDuration(),
                false, // don't use notation timings
                *m_currentEventSelection));
}

void
EditView::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:
            m_shiftDown = true;

            break;

        case Key_Control:
            m_controlDown = true;
            break;

        default:
            event->ignore();
            break;
    }

    if (m_bottomBarButtons)
        m_bottomBarButtons->getLoopRuler()->slotSetLoopingMode(m_shiftDown);
}



void
EditView::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:
            m_shiftDown = false;
            break;

        case Key_Control:
            m_controlDown = false;
            break;

        default:
            event->ignore();
            break;
    }

    if (m_bottomBarButtons)
        m_bottomBarButtons->getLoopRuler()->slotSetLoopingMode(m_shiftDown);
}

void
EditView::slotFlipForwards()
{
    RG_DEBUG << "EditView::slotFlipForwards" << endl;
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->flipForwards();
}

void
EditView::slotFlipBackwards()
{
    RG_DEBUG << "EditView::slotFlipBackwards" << endl;
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler) ruler->flipBackwards();
}



