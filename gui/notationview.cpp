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


#include <qstring.h>
#include <qregexp.h>
#include <qpaintdevicemetrics.h>
#include <qpixmap.h>
#include <qtimer.h>

#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kapp.h>
#include <kprinter.h>

#include "NotationTypes.h"
#include "Quantizer.h"
#include "Selection.h"
#include "BaseProperties.h"

#include "Profiler.h"

#include "notationview.h"
#include "constants.h"
#include "colours.h"
#include "notationstrings.h"
#include "notepixmapfactory.h"
#include "notefont.h"
#include "notestyle.h"
#include "notationtool.h"
#include "notationstrings.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "rosedebug.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "widgets.h"
#include "chordnameruler.h"
#include "temporuler.h"
#include "rawnoteruler.h"
#include "studiocontrol.h"
#include "notationhlayout.h"
#include "notationvlayout.h"
#include "ktmpstatusmsg.h"
#include "scrollbox.h"
#include "rosegardenguiview.h"
#include "trackeditor.h"
#include "trackbuttons.h"

//!!! TESTING:
#include "qcanvassimplesprite.h"
#include <qpixmap.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>


using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::Quantizer;
using Rosegarden::timeT;
using Rosegarden::Mark;
using namespace Rosegarden::Marks;


class NoteActionData
{
public:
    NoteActionData();
    NoteActionData(const QString& _title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   bool _rest,
		   Note::Type _noteType,
		   int _dots);
    
    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    bool rest;
    Note::Type noteType;
    int dots;
};

NoteActionData::NoteActionData()
    : title(0),
      actionName(0),
      pixmapName(0),
      keycode(0),
      rest(false),
      noteType(0),
      dots(0)
{
}

NoteActionData::NoteActionData(const QString& _title,
			       QString _actionName,
			       QString _pixmapName,
			       int _keycode,
			       bool _rest,
			       Note::Type _noteType,
			       int _dots)
    : title(_title),
      actionName(_actionName),
      pixmapName(_pixmapName),
      keycode(_keycode),
      rest(_rest),
      noteType(_noteType),
      dots(_dots)
{
}


class MarkActionData
{
public:
    MarkActionData() :
	title(0),
	actionName(0),
	pixmapName(0),
	keycode(0) { }

    MarkActionData(const QString &_title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   Mark _mark) :
	title(_title),
	actionName(_actionName),
	pixmapName(_pixmapName),
	keycode(_keycode),
	mark(_mark) { }

    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    Mark mark;
};




//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
			   bool showProgressive) :
    EditView(doc, segments, 1, parent, "notationview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_selectionCounter(0),
    m_insertModeLabel(0),
    m_annotationsLabel(0),
    m_progressBar(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_currentStaff(-1),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_deferredCursorMove(NoCursorMoveNeeded),
    m_currentAccidental(Rosegarden::Accidentals::NoAccidental),
    m_fontName(NoteFontFactory::getDefaultFontName()),
    m_fontSize(NoteFontFactory::getDefaultSize(m_fontName)),
    m_pageMode(LinedStaff::LinearMode),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
                                  m_properties, this)),
    m_vlayout(new NotationVLayout(&doc->getComposition(),
                                  m_properties, this)),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_rawNoteRuler(0),
    m_annotationsVisible(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeCombo(0),
    m_spacingCombo(0),
    m_fontSizeActionMenu(0),
    m_pannerDialog(new ScrollBoxDialog(this, ScrollBox::FixHeight)),
    m_renderTimer(0),
    m_progressDisplayer(PROGRESS_NONE),
    m_inhibitRefresh(true),
    m_ok(false),
    m_printMode(false),
    m_printSize(8) // set in positionStaffs
{
    initActionDataMaps(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationView::ConfigGroup);

    m_fontName = qstrtostr(m_config->readEntry
			   ("notefont",
			    strtoqstr(NoteFontFactory::getDefaultFontName())));

    try {
	(void)NoteFontFactory::getFont
	    (m_fontName,
	     NoteFontFactory::getDefaultSize(m_fontName));
    } catch (Rosegarden::Exception e) {
	m_fontName = NoteFontFactory::getDefaultFontName();
    }

    m_fontSize = m_config->readUnsignedNumEntry
	((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
	 NoteFontFactory::getDefaultSize(m_fontName));

    int defaultSpacing = m_config->readNumEntry("spacing", 100);
    m_hlayout->setSpacing(defaultSpacing);

    int defaultProportion = m_config->readNumEntry("proportion", 60);
    m_hlayout->setProportion(defaultProportion);

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    
    setupActions();
//     setupAddControlRulerMenu(); - too early for notation, moved to end of ctor.
    initLayoutToolbar();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    //!!! TESTING
    {
/*!!!
	int y = 0;

	QPixmap *pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	QFont font("opus");
	font.setPixelSize(16);
	QFontMetrics metrics(font);
	QPainter painter;
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	int x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(0xf000 + c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	QCanvasSimpleSprite *sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	QString s("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	painter.drawText(x, 50, s);
	x += metrics.width(s);
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	font = QFont("maestro");
	font.setPixelSize(16);
	metrics = QFontMetrics(font);
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(0xf000 + c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	s = QString("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	painter.drawText(x, 50, s);
	x += metrics.width(s);
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	font = QFont("xinfonia");
	font.setPixelSize(16);
	metrics = QFontMetrics(font);
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(0xf000 + c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	for (int c = 32; c < 256; ++c) {
	    QChar cc(c);
	    painter.drawText(x, 50, cc);
	    x += metrics.width(cc);
	}
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;

	pixmap = new QPixmap(3000, 100);
	pixmap->fill();
	painter.begin(pixmap);
	painter.setFont(font);
	painter.drawLine(0, 50, 3000, 50);
	x = 0;
	s = QString("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	painter.drawText(x, 50, s);
	x += metrics.width(s);
	painter.end();
	sprite = new QCanvasSimpleSprite
	    (pixmap, tCanvas);
	sprite->move(0, y);
	sprite->show();
	y += 100;
*/
    }
    

    setCanvasView(new NotationCanvasView(*this, tCanvas, getCentralWidget()));

    if (segments.size() == 1) {
        setCaption(QString("%1 - Segment Track #%2 - Notation")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    } else if (segments.size() == doc->getComposition().getNbSegments()) {
        setCaption(QString("%1 - All Segments - Notation")
                   .arg(doc->getTitle()));
    } else {
        setCaption(QString("%1 - %2 Segments - Notation")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    setTopBarButtons(new BarButtons(getDocument(),
                                    m_hlayout, 20.0, 25,
				    false, getCentralWidget()));

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(m_hlayout, doc, segments, 20.0, 20, getCentralWidget());
    addRuler(m_chordNameRuler);
    if (showProgressive) m_chordNameRuler->show();

    m_tempoRuler = new TempoRuler
	(m_hlayout, doc, 20.0, 20, false, getCentralWidget());
    addRuler(m_tempoRuler);
    m_tempoRuler->hide();

    m_rawNoteRuler = new RawNoteRuler
	(m_hlayout, segments[0], 20.0, 20, getCentralWidget());
    addRuler(m_rawNoteRuler);
    m_rawNoteRuler->show();

    // All toolbars should be created before this is called
    setAutoSaveSettings("NotationView", true);

    // All rulers must have been created before this is called,
    // or the program will crash
    readOptions();


    setBottomBarButtons(new BarButtons(getDocument(), m_hlayout, 20.0, 25,
				       true, getBottomWidget()));

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff
			   (canvas(), segments[i], 0, // snap
			    i, this,
			    m_fontName, m_fontSize));
    }

    //
    // layout
    //
    RosegardenProgressDialog* progressDlg = 0;

    if (showProgressive) {
	show();
        RosegardenProgressDialog::processEvents();

        NOTATION_DEBUG << "NotationView : setting up progress dialog" << endl;

        progressDlg = new RosegardenProgressDialog(i18n("Starting..."),
                                                   100, this);
	progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    m_chordNameRuler->setStudio(&getDocument()->getStudio());

    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    m_config->setGroup(NotationView::ConfigGroup);
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

    try {

	LinedStaff::PageMode mode = LinedStaff::LinearMode;
	if (layoutMode == 1) mode = LinedStaff::ContinuousPageMode;
	else if (layoutMode == 2) mode = LinedStaff::MultiPageMode;

        setPageMode(mode);

	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
	}

        m_ok = true;

    } catch (ProgressReporter::Cancelled c) {
	// when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled" << endl;
    }
    
    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;
    
    delete progressDlg;

    // at this point we can return if operation was cancelled
    if (!isOK()) { setOutOfCtor(); return; }
    

    // otherwise, carry on
    setupDefaultProgress();

    //
    // Connect signals
    //

    QObject::connect
	(getCanvasView(), SIGNAL(renderRequired(double, double)),
	 this,            SLOT(slotCheckRendered(double, double)));

    QObject::connect
	(m_topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    m_bottomBarButtons->connectRulerToDocPointer(doc);

    QObject::connect
        (getCanvasView(), SIGNAL(itemPressed(int, int, QMouseEvent*, NotationElement*)),
         this,            SLOT  (slotItemPressed(int, int, QMouseEvent*, NotationElement*)));

    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (slotActiveItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(nonNotationItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (slotNonNotationItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseMoved(QMouseEvent*)),
         this,         SLOT  (slotMouseMoved(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseReleased(QMouseEvent*)),
         this,         SLOT  (slotMouseReleased(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this,         SLOT  (slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(m_pannerDialog->scrollbox(), SIGNAL(valueChanged(const QPoint &)),
	 getCanvasView(), SLOT(slotSetScrollPos(const QPoint &)));

    QObject::connect
	(getCanvasView()->horizontalScrollBar(), SIGNAL(valueChanged(int)),
	 m_pannerDialog->scrollbox(), SLOT(setViewX(int)));

    QObject::connect
	(getCanvasView()->verticalScrollBar(), SIGNAL(valueChanged(int)),
	 m_pannerDialog->scrollbox(), SLOT(setViewY(int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_multiple_staffs",
		 (m_staffs.size() > 1 ? KXMLGUIClient::StateNoReverse :
		                        KXMLGUIClient::StateReverse));
    stateChanged("rest_insert_tool_current", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    if (getSegmentsOnlyRests()) {
        m_selectDefaultNote->activate();
	stateChanged("note_insert_tool_current", 
		     KXMLGUIClient::StateNoReverse);
    } else {
        actionCollection()->action("select")->activate();
	stateChanged("note_insert_tool_current",
		     KXMLGUIClient::StateReverse);
    }

    slotSetInsertCursorPosition(0);
    slotSetPointerPosition(doc->getComposition().getPosition());
    setCurrentSelection(0, false, true);
    slotUpdateInsertModeStatus();
    m_chordNameRuler->repaint();
    m_rawNoteRuler->repaint();
    m_inhibitRefresh = false;

//    slotCheckRendered(0, getCanvasView()->visibleWidth());
//    getCanvasView()->repaintContents();
    updateView();

    QObject::connect
	(this,            SIGNAL(renderComplete()),
	 getCanvasView(), SLOT(slotRenderComplete()));

    if (parent) {
	const TrackButtons * trackLabels = 
            ((RosegardenGUIView*)parent)->getTrackEditor()->getTrackButtons();
        QObject::connect
            (trackLabels,             SIGNAL(nameChanged()),
             this, SLOT(slotUpdateStaffName()));
    }

    setConfigDialogPageIndex(1);
    setOutOfCtor();

    // Property and Control Rulers
    //
    if (getCurrentSegment()->getViewFeatures()) slotShowVelocityControlRuler();
    setupControllerTabs();

    setupAddControlRulerMenu();
    setRewFFwdToAutoRepeat();
    
    NOTATION_DEBUG << "NotationView ctor exiting" << endl;
}

//
// Notation Print mode
//
NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
			   QWidget *parent,
			   NotationView *referenceView)
    : EditView(doc, segments, 1, 0, "printview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_selectionCounter(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_deferredCursorMove(NoCursorMoveNeeded),
    m_currentAccidental(Rosegarden::Accidentals::NoAccidental),
    m_fontName(NoteFontFactory::getDefaultFontName()),
    m_fontSize(NoteFontFactory::getDefaultSize(m_fontName)),
    m_pageMode(LinedStaff::LinearMode),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
				  m_properties, this)),
    m_vlayout(new NotationVLayout(&doc->getComposition(),
                                  m_properties, this)),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_rawNoteRuler(0),
    m_annotationsVisible(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeCombo(0),
    m_spacingCombo(0),
    m_fontSizeActionMenu(0),
    m_pannerDialog(0),
    m_renderTimer(0),
    m_progressDisplayer(PROGRESS_NONE),
    m_inhibitRefresh(true),
    m_ok(false),
    m_printMode(true),
    m_printSize(8) // set in positionStaffs
{
    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView print ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationView::ConfigGroup);

    if (referenceView) {
	m_fontName = referenceView->m_fontName;
    } else {
	m_fontName = qstrtostr(m_config->readEntry
			       ("notefont",
				strtoqstr(NoteFontFactory::getDefaultFontName())));
    }


    // Force largest font size
    std::vector<int> sizes = NoteFontFactory::getAllSizes(m_fontName);
    m_fontSize = sizes[sizes.size()-1];

    if (referenceView) {
	m_hlayout->setSpacing(referenceView->m_hlayout->getSpacing());
    } else {
	int defaultSpacing = m_config->readNumEntry("spacing", 100);
	m_hlayout->setSpacing(defaultSpacing);
    }

    delete m_notePixmapFactory;
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);
    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);
    
    setBackgroundMode(PaletteBase);
    m_config->setGroup(NotationView::ConfigGroup);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);//!!! 

    setCanvasView(new NotationCanvasView(*this, tCanvas, getCentralWidget()));

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0, // snap
                                             i, this,
                                             m_fontName, m_fontSize));
    }

    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    RosegardenProgressDialog* progressDlg = 0;

    if (parent) {

        RosegardenProgressDialog::processEvents();

	NOTATION_DEBUG << "NotationView : setting up progress dialog" << endl;

        progressDlg = new RosegardenProgressDialog(i18n("Preparing to print..."),
                                                   100, parent);
	progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    try {

        setPageMode(LinedStaff::MultiPageMode); // also positions and renders the staffs!

	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
	}

	m_ok = true;

    } catch (ProgressReporter::Cancelled c) {
	// when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled" << endl;
    }

    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;
    
    delete progressDlg;

    if (!isOK()) {
        setOutOfCtor();
        return; // In case more code is added there later
    }

    setOutOfCtor(); // keep this as last call in the ctor
}


NotationView::~NotationView()
{
    NOTATION_DEBUG << "-> ~NotationView()" << endl;

    if (!m_printMode && m_ok) slotSaveOptions();

    delete m_chordNameRuler;

    delete m_renderTimer;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	for (Segment::iterator j = m_staffs[i]->getSegment().begin();
	     j != m_staffs[i]->getSegment().end(); ++j) {
	    removeViewLocalProperties(*j);
	}
	delete m_staffs[i]; // this will erase all "notes" canvas items
    }
    
    PixmapArrayGC::deleteAll();
    Rosegarden::Profiles::getInstance()->dump();

    NOTATION_DEBUG << "<- ~NotationView()" << endl;
}

void
NotationView::removeViewLocalProperties(Rosegarden::Event *e)
{
    Event::PropertyNames names(e->getPropertyNames());
    std::string prefix(getViewLocalPropertyPrefix());

    for (Event::PropertyNames::iterator i = names.begin();
	 i != names.end(); ++i) {
	if (i->getName().substr(0, prefix.size()) == prefix) {
	    e->unset(*i);
	}
    }
}
    
const NotationProperties &
NotationView::getProperties() const 
{
    return m_properties;
}

void NotationView::positionStaffs()
{
    m_config->setGroup(NotationView::ConfigGroup);
    m_printSize = m_config->readUnsignedNumEntry("printingnotesize", 5);

    int minTrack = 0, maxTrack = 0;
    bool haveMinTrack = false;
    typedef std::map<int, int> TrackIntMap;
    TrackIntMap trackHeights;
    TrackIntMap trackCoords;

    int pageWidth, pageHeight, leftMargin, topMargin;
    int accumulatedHeight;
    int rowsPerPage = 1;
    int legerLines = 8;
    if (m_pageMode != LinedStaff::LinearMode) legerLines = 6;
    int rowGapPercent = (m_staffs.size() > 1 ? 40 : 10);
    int aimFor = -1;

    bool done = false;

    while (1) {

	pageWidth = getPageWidth();
	pageHeight = getPageHeight();
	leftMargin = 0, topMargin = 0;
	getPageMargins(leftMargin, topMargin);

	accumulatedHeight = 0;
	int maxTrackHeight = 0;

	trackHeights.clear();

	for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	    m_staffs[i]->setLegerLineCount(legerLines);

	    int height = m_staffs[i]->getHeightOfRow();
	    Rosegarden::TrackId trackId = m_staffs[i]->getSegment().getTrack();
	    Rosegarden::Track *track =
		m_staffs[i]->getSegment().getComposition()->
		getTrackById(trackId);

	    if (!track) continue; // This Should Not Happen, My Friend

	    int trackPosition = track->getPosition();

	    TrackIntMap::iterator hi = trackHeights.find(trackPosition);
	    if (hi == trackHeights.end()) {
		trackHeights.insert(TrackIntMap::value_type
				    (trackPosition, height));
	    } else if (height > hi->second) {
		hi->second = height;
	    }
	
	    if (height > maxTrackHeight) maxTrackHeight = height;

	    if (trackPosition < minTrack || !haveMinTrack) {
		minTrack = trackPosition;
		haveMinTrack = true;
	    }
	    if (trackPosition > maxTrack) {
		maxTrack = trackPosition;
	    }
	}

	for (int i = minTrack; i <= maxTrack; ++i) {
	    TrackIntMap::iterator hi = trackHeights.find(i);
	    if (hi != trackHeights.end()) {
		trackCoords[i] = accumulatedHeight;
		accumulatedHeight += hi->second;
	    }
	}

	accumulatedHeight += maxTrackHeight * rowGapPercent / 100;

	if (done) break;

	if (m_pageMode != LinedStaff::MultiPageMode) {

	    rowsPerPage = 0;
	    done = true;
	    break;

	} else {

	    // Check how well all this stuff actually fits on the
	    // page.  If things don't fit as well as we'd like, modify
	    // at most one parameter so as to save some space, then
	    // loop around again and see if it worked.  This iterative
	    // approach is inefficient but the time spent here is
	    // neglible in context, and it's a simple way to code it.

	    int staffPageHeight = pageHeight - topMargin * 2;
	    rowsPerPage = staffPageHeight / accumulatedHeight;

	    if (rowsPerPage < 1) {

		if (legerLines > 5) --legerLines;
		else if (rowGapPercent > 20) rowGapPercent -= 10;
		else if (legerLines > 4) --legerLines;
		else if (rowGapPercent > 0) rowGapPercent -= 10;
		else if (legerLines > 3) --legerLines;
		else if (m_printSize > 3) --m_printSize;
		else { // just accept that we'll have to overflow
		    rowsPerPage = 1;
		    done = true;
		}

	    } else {

		if (aimFor == rowsPerPage) {
		    done = true;
		} else {

		    if (aimFor == -1) aimFor = rowsPerPage + 1;

		    // we can perhaps accommodate another row, with care
		    if (legerLines > 5) --legerLines;
		    else if (rowGapPercent > 20) rowGapPercent -= 10;
		    else if (legerLines > 3) --legerLines;
		    else if (rowGapPercent > 0) rowGapPercent -= 10;
		    else { // no, we can't
			rowGapPercent = 0;
			legerLines = 8;
			done = true;
		    }
		}
	    }
	}
    }

    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	Rosegarden::TrackId trackId = m_staffs[i]->getSegment().getTrack();
	Rosegarden::Track *track =
	    m_staffs[i]->getSegment().getComposition()->
	    getTrackById(trackId);

	if (!track) continue; // Once Again, My Friend, You Should Never See Me Here
	
	int trackPosition = track->getPosition();

        m_staffs[i]->setRowSpacing(accumulatedHeight);
	
        if (trackPosition < maxTrack) {
            m_staffs[i]->setConnectingLineLength(trackHeights[trackPosition]);
        }

	if (trackPosition == minTrack &&
	    m_pageMode != LinedStaff::LinearMode) {
	    m_staffs[i]->setBarNumbersEvery(5);
	} else {
	    m_staffs[i]->setBarNumbersEvery(0);
	}
        
	m_staffs[i]->setX(20);
	m_staffs[i]->setY((m_pageMode == LinedStaff::MultiPageMode ? 20 : 0) +
			  trackCoords[trackPosition] + topMargin);
	m_staffs[i]->setPageWidth(pageWidth - leftMargin * 2);
	m_staffs[i]->setRowsPerPage(rowsPerPage);
        m_staffs[i]->setPageMode(m_pageMode);
	m_staffs[i]->setMargin(leftMargin);

	NOTATION_DEBUG << "NotationView::positionStaffs: set staff's page width to "
		       << (pageWidth - leftMargin * 2) << endl;

    }
}    

void NotationView::positionPages()
{
    if (m_printMode) return;

    QPixmap background;
    QPixmap deskBackground;
    bool haveBackground = false;
    
    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (m_config->readBoolEntry("backgroundtextures", false)) {
	QString pixmapDir =
	    KGlobal::dirs()->findResource("appdata", "pixmaps/");
	if (background.load(QString("%1/misc/bg-paper-white.xpm").
			    arg(pixmapDir))) {
	    haveBackground = true;
	}
	// we're happy to ignore errors from this one:
	deskBackground.load(QString("%1/misc/bg-desktop.xpm").arg(pixmapDir));
    }

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();
    int leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);
    int maxPageCount = 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	int pageCount = m_staffs[i]->getPageCount();
	if (pageCount > maxPageCount) maxPageCount = pageCount;
    }
    
    for (unsigned int i = 0; i < m_pages.size(); ++i) {
	delete m_pages[i];
	delete m_pageNumbers[i];
    }
    m_pages.clear();
    m_pageNumbers.clear();
    
    if (m_pageMode != LinedStaff::MultiPageMode) {
	if (haveBackground) canvas()->setBackgroundPixmap(background);
    } else {

	QFont pageNumberFont;
	pageNumberFont.setPixelSize(m_fontSize * 2);
	QFontMetrics metrics(pageNumberFont);

	canvas()->setBackgroundPixmap(deskBackground);
	
	int thumbScale = 20;
	QPixmap thumbnail(canvas()->width() / thumbScale,
			  canvas()->height() / thumbScale);
	thumbnail.fill(Qt::white);
	QPainter thumbPainter(&thumbnail);
	thumbPainter.setPen(Qt::black);

	for (int page = 0; page < maxPageCount; ++page) {

	    int x = 20 + pageWidth * page + leftMargin/4;
	    int y = 20;
	    int w = pageWidth - leftMargin/2;
	    int h = pageHeight;

	    QCanvasRectangle *rect = new QCanvasRectangle(x, y, w, h, canvas());
	    if (haveBackground) rect->setBrush(QBrush(Qt::white, background));
	    rect->setPen(Qt::black);
	    rect->setZ(-1000);
	    rect->show();
	    m_pages.push_back(rect);

	    QString str = QString("%1").arg(page + 1);
	    QCanvasText *text = new QCanvasText(str, pageNumberFont, canvas());
	    text->setX(x + w - metrics.width(str) - leftMargin/2);
	    text->setY(y + h - metrics.descent() - topMargin);
	    text->setZ(-999);
	    text->show();
	    m_pageNumbers.push_back(text);

	    thumbPainter.drawRect(x / thumbScale, y / thumbScale,
				  w / thumbScale, h / thumbScale);

	    int tx = (x + w/2) / thumbScale, ty = (y + h/2) / thumbScale;
	    tx -= thumbPainter.fontMetrics().width(str)/2;
	    thumbPainter.drawText(tx, ty, str);
	}

	thumbPainter.end();
	if (m_pannerDialog) m_pannerDialog->scrollbox()->setThumbnail(thumbnail);
    }

    m_config->setGroup(NotationView::ConfigGroup);
}


void NotationView::slotUpdateStaffName()
{
    NotationStaff *staff = getStaff(m_currentStaff);
    staff->drawStaffName();
}

void NotationView::slotSaveOptions()
{
    m_config->setGroup(NotationView::ConfigGroup);

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Raw Note Ruler", getToggleAction("show_raw_note_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler",      getToggleAction("show_tempo_ruler")->isChecked());
    m_config->writeEntry("Show Annotations", m_annotationsVisible);

    m_config->sync();
}

void NotationView::setOneToolbar(const char *actionName,
				 const char *toolbarName)
{
    KToggleAction *action = getToggleAction(actionName);
    if (!action) {
	std::cerr << "WARNING: No such action as " << actionName << std::endl;
	return;
    }
    QWidget *toolbar = toolBar(toolbarName);
    if (!toolbar) {
	std::cerr << "WARNING: No such toolbar as " << toolbarName << std::endl;
	return;
    }
    action->setChecked(!toolbar->isHidden());
}
	

void NotationView::readOptions()
{
    EditView::readOptions();

    setOneToolbar("show_tools_toolbar", "Tools Toolbar");
    setOneToolbar("show_notes_toolbar", "Notes Toolbar");
    setOneToolbar("show_rests_toolbar", "Rests Toolbar");
    setOneToolbar("show_clefs_toolbar", "Clefs Toolbar");
    setOneToolbar("show_group_toolbar", "Group Toolbar");
    setOneToolbar("show_marks_toolbar", "Marks Toolbar");
    setOneToolbar("show_layout_toolbar", "Layout Toolbar");
    setOneToolbar("show_transport_toolbar", "Transport Toolbar");
    setOneToolbar("show_accidentals_toolbar", "Accidentals Toolbar");
    setOneToolbar("show_meta_toolbar", "Meta Toolbar");

    m_config->setGroup(NotationView::ConfigGroup);

    bool opt;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", false);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = m_config->readBoolEntry("Show Raw Note Ruler", true);
    getToggleAction("show_raw_note_ruler")->setChecked(opt);
    slotToggleRawNoteRuler();

    opt = m_config->readBoolEntry("Show Tempo Ruler", false);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = m_config->readBoolEntry("Show Annotations", true);
    m_annotationsVisible = opt;
    getToggleAction("show_annotations")->setChecked(opt);
    slotUpdateAnnotationsStatus();
//    slotToggleAnnotations();
}

void NotationView::setupActions()
{
    KStdAction::print(this, SLOT(slotFilePrint()), actionCollection());
    KStdAction::printPreview(this, SLOT(slotFilePrintPreview()),
			     actionCollection());

    EditViewBase::setupActions("notation.rc");
    EditView::setupActions();

    KRadioAction* noteAction = 0;
    
    // View menu stuff
    
    KActionMenu *fontActionMenu =
	new KActionMenu(i18n("Note &Font"), this, "note_font_actionmenu");

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());
    
    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

	QString fontQName(strtoqstr(*i));

	KToggleAction *fontAction = 
	    new KToggleAction
	    (fontQName, 0, this, SLOT(slotChangeFontFromAction()),
	     actionCollection(), "note_font_" + fontQName);

	fontAction->setChecked(*i == m_fontName);
	fontActionMenu->insert(fontAction);
    }

    actionCollection()->insert(fontActionMenu);

    m_fontSizeActionMenu =
	new KActionMenu(i18n("Si&ze"), this, "note_font_size_actionmenu");
    setupFontSizeMenu();

    actionCollection()->insert(m_fontSizeActionMenu);

    
    KActionMenu *spacingActionMenu =
	new KActionMenu(i18n("S&pacing"), this, "stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    for (std::vector<int>::iterator i = spacings.begin();
	 i != spacings.end(); ++i) {
	
	KToggleAction *spacingAction =
	    new KToggleAction
	    (QString("%1%").arg(*i), 0, this,
	     SLOT(slotChangeSpacingFromAction()),
	     actionCollection(), QString("spacing_%1").arg(*i));

	spacingAction->setExclusiveGroup("spacing");
	spacingAction->setChecked(*i == defaultSpacing);
	spacingActionMenu->insert(spacingAction);
    }

    actionCollection()->insert(spacingActionMenu);
    
    KActionMenu *proportionActionMenu =
	new KActionMenu(i18n("Du&ration Factor"), this, "proportion_actionmenu");

    int defaultProportion = m_hlayout->getProportion();
    std::vector<int> proportions = NotationHLayout::getAvailableProportions();

    for (std::vector<int>::iterator i = proportions.begin();
	 i != proportions.end(); ++i) {
	
	QString name = QString("%1%").arg(*i);
	if (*i == 0) name = i18n("None");

	KToggleAction *proportionAction =
	    new KToggleAction
	    (name, 0, this,
	     SLOT(slotChangeProportionFromAction()),
	     actionCollection(), QString("proportion_%1").arg(*i));

	proportionAction->setExclusiveGroup("proportion");
	proportionAction->setChecked(*i == defaultProportion);
	proportionActionMenu->insert(proportionAction);
    }

    actionCollection()->insert(proportionActionMenu);

    KActionMenu *styleActionMenu =
	new KActionMenu(i18n("Note &Style"), this, "note_style_actionmenu");

    std::vector<NoteStyleName> styles
	    (NoteStyleFactory::getAvailableStyleNames());
    
    for (std::vector<NoteStyleName>::iterator i = styles.begin();
	 i != styles.end(); ++i) {

	QString styleQName(strtoqstr(*i));

	KAction *styleAction = 
	    new KAction
	    (styleQName, 0, this, SLOT(slotSetStyleFromAction()),
	     actionCollection(), "style_" + styleQName);

	styleActionMenu->insert(styleAction);
    }

    actionCollection()->insert(styleActionMenu);


    new KAction
	(i18n("Insert Rest"), Key_P, this, SLOT(slotInsertRest()),
	 actionCollection(), QString("insert_rest"));

    new KAction
	(i18n("Switch from Note to Rest"), Key_T, this,
	 SLOT(slotSwitchFromNoteToRest()),
	 actionCollection(), QString("switch_from_note_to_rest"));

    new KAction
	(i18n("Switch from Rest to Note"), Key_Y, this,
	 SLOT(slotSwitchFromRestToNote()),
	 actionCollection(), QString("switch_from_rest_to_note"));


    // setup Notes menu & toolbar
    QIconSet icon;
 
    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
	 actionDataIter != m_noteActionDataMap->end();
	 ++actionDataIter) {

        NoteActionData noteActionData = *actionDataIter;
        
        icon = QIconSet
	    (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
	     (noteActionData.pixmapName)));
        noteAction = new KRadioAction(noteActionData.title,
				      icon,
				      noteActionData.keycode,
				      this,
                                      SLOT(slotNoteAction()),
                                      actionCollection(),
				      noteActionData.actionName);
        noteAction->setExclusiveGroup("notes");

        if (noteActionData.noteType == Note::Crotchet &&
	    noteActionData.dots == 0 && !noteActionData.rest) {
            m_selectDefaultNote = noteAction;
	}
    }
    
    //
    // Accidentals
    //
    static QString actionsAccidental[][4] = 
        {
            { i18n("No accidental"),  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
            { i18n("Sharp"),          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
            { i18n("Flat"),           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
            { i18n("Natural"),        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
            { i18n("Double sharp"),   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
            { i18n("Double flat"),    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsAccidental)/sizeof(actionsAccidental[0]); ++i) {

        icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                        (actionsAccidental[i][3])));
        noteAction = new KRadioAction(actionsAccidental[i][0], icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-treble")));
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-tenor")));
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-alto")));
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-bass")));
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    noteAction = new KRadioAction(i18n("&Text"), icon, 0, this,
                                  SLOT(slotText()),
                                  actionCollection(), "text");
    noteAction->setExclusiveGroup("notes");


    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("&Erase"), "eraser", Key_F3,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("select")));
    noteAction = new KRadioAction(i18n("&Select"), icon, Key_F1,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("step_by_step")));
    new KToggleAction(i18n("Ste&p Recording"), icon, 0, this,
                SLOT(slotToggleStepByStep()), actionCollection(),
                "toggle_step_by_step");


    // Edit menu
    new KAction(i18n("Select from Sta&rt"), 0, this,
		SLOT(slotEditSelectFromStart()), actionCollection(),
		"select_from_start");

    new KAction(i18n("Select to &End"), 0, this,
		SLOT(slotEditSelectToEnd()), actionCollection(),
		"select_to_end");

    new KAction(i18n("Select Whole St&aff"), 0, this,
		SLOT(slotEditSelectWholeStaff()), actionCollection(),
		"select_whole_staff");

    new KAction(i18n("C&ut and Close"), CTRL + SHIFT + Key_X, this,
		SLOT(slotEditCutAndClose()), actionCollection(),
		"cut_and_close");

    new KAction(i18n("Pa&ste..."), CTRL + SHIFT + Key_V, this,
		SLOT(slotEditGeneralPaste()), actionCollection(),
		"general_paste");

    new KAction(i18n("De&lete"), Key_Delete, this,
		SLOT(slotEditDelete()), actionCollection(),
		"delete");

    //
    // Settings menu
    //
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

    KRadioAction *linearModeAction = new KRadioAction
        (i18n("&Linear Layout"), 0, this, SLOT(slotLinearMode()),
         actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 0) linearModeAction->setChecked(true);

    KRadioAction *continuousPageModeAction = new KRadioAction
        (i18n("&Continuous Page Layout"), 0, this, SLOT(slotContinuousPageMode()),
         actionCollection(), "continuous_page_mode");
    continuousPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 1) continuousPageModeAction->setChecked(true);

    KRadioAction *multiPageModeAction = new KRadioAction
        (i18n("&Multiple Page Layout"), 0, this, SLOT(slotMultiPageMode()),
         actionCollection(), "multi_page_mode");
    multiPageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 2) multiPageModeAction->setChecked(true);

    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show Ra&w Note Ruler"), 0, this,
                      SLOT(slotToggleRawNoteRuler()),
                      actionCollection(), "show_raw_note_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");

    new KToggleAction(i18n("Show &Annotations"), 0, this,
                      SLOT(slotToggleAnnotations()),
                      actionCollection(), "show_annotations");

    new KAction(i18n("Open L&yric Editor"), 0, this, SLOT(slotEditLyrics()),
		actionCollection(), "lyric_editor");

    //
    // Group menu
    //
    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-beam")));

    new KAction(GroupMenuBeamCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(GroupMenuAutoBeamCommand::getGlobalName(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-unbeam")));

    new KAction(GroupMenuBreakCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");
    
    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-simple-tuplet")));

    new KAction(GroupMenuTupletCommand::getGlobalName(true), icon, 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-tuplet")));

    new KAction(GroupMenuTupletCommand::getGlobalName(false), icon, 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    new KAction(GroupMenuUnTupletCommand::getGlobalName(), 0, this,
                SLOT(slotGroupUnTuplet()), actionCollection(), "break_tuplets");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("triplet")));
    (new KToggleAction(i18n("Trip&let Insert Mode"), icon, Key_G,
		       this, SLOT(slotUpdateInsertModeStatus()),
                       actionCollection(), "triplet_mode"))->
	setChecked(false);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("chord")));
    (new KToggleAction(i18n("C&hord Insert Mode"), icon, Key_H,
		       this, SLOT(slotUpdateInsertModeStatus()),
		      actionCollection(), "chord_mode"))->
	setChecked(false);

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-grace")));

    new KAction(GroupMenuGraceCommand::getGlobalName(), icon, 0, this,
		SLOT(slotGroupGrace()), actionCollection(), "grace");

    new KAction(GroupMenuUnGraceCommand::getGlobalName(), 0, this,
		SLOT(slotGroupUnGrace()), actionCollection(), "ungrace");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-slur")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Slur), icon, 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-crescendo")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Crescendo), icon, 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-decrescendo")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Decrescendo), icon, 0, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("group-chord")));
    new KAction(GroupMenuMakeChordCommand::getGlobalName(), icon, 0, this,
		SLOT(slotGroupMakeChord()), actionCollection(), "make_chord");

    // setup Transforms menu
    new KAction(TransformsMenuNormalizeRestsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(TransformsMenuCollapseRestsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(TransformsMenuCollapseNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("transforms-tie")));

    new KAction(TransformsMenuTieNotesCommand::getGlobalName(), icon, 0, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(TransformsMenuUntieNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(TransformsMenuMakeNotesViableCommand::getGlobalName(), 0, this,
		SLOT(slotTransformsMakeNotesViable()), actionCollection(),
		"make_notes_viable");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("transforms-decounterpoint")));

    new KAction(TransformsMenuDeCounterpointCommand::getGlobalName(), icon, 0, this,
		SLOT(slotTransformsDeCounterpoint()), actionCollection(),
		"de_counterpoint");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(true),
		0, Key_PageUp + CTRL, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(false),
		0, Key_PageDown + CTRL, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(TransformsMenuRestoreStemsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::DoubleFlat),
		0, this,
		SLOT(slotRespellDoubleFlat()), actionCollection(),
                "respell_doubleflat");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::Flat),
		0, this,
		SLOT(slotRespellFlat()), actionCollection(),
                "respell_flat");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::Natural),
		0, this,
		SLOT(slotRespellNatural()), actionCollection(),
                "respell_natural");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::Sharp),
		0, this,
		SLOT(slotRespellSharp()), actionCollection(),
                "respell_sharp");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::DoubleSharp),
		0, this,
		SLOT(slotRespellDoubleSharp()), actionCollection(),
                "respell_doublesharp");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Up, Rosegarden::Accidentals::NoAccidental),
		Key_Up + CTRL + SHIFT, this,
		SLOT(slotRespellUp()), actionCollection(),
                "respell_up");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Down, Rosegarden::Accidentals::NoAccidental),
		Key_Down + CTRL + SHIFT, this,
		SLOT(slotRespellDown()), actionCollection(),
                "respell_down");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Restore, Rosegarden::Accidentals::NoAccidental),
		0, this,
		SLOT(slotRespellRestore()), actionCollection(),
                "respell_restore");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("quantize")));

    new KAction(EventQuantizeCommand::getGlobalName(), icon, 0, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(TransformsMenuFixNotationQuantizeCommand::getGlobalName(), 0,
		this, SLOT(slotTransformsFixQuantization()), actionCollection(),
                "fix_quantization");

    new KAction(TransformsMenuInterpretCommand::getGlobalName(), 0,
		this, SLOT(slotTransformsInterpret()), actionCollection(),
		"interpret");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
		SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    for (MarkActionDataMap::Iterator i = m_markActionDataMap->begin();
	 i != m_markActionDataMap->end(); ++i) {

        const MarkActionData &markActionData = *i;
        
        icon = QIconSet
	    (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
	     (markActionData.pixmapName)));

	new KAction(markActionData.title,
		    icon,
		    markActionData.keycode,
		    this,
		    SLOT(slotAddMark()),
		    actionCollection(),
		    markActionData.actionName);
    }

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
        ("text-mark")));

    new KAction(MarksMenuAddTextMarkCommand::getGlobalName(), icon, 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(MarksMenuRemoveMarksCommand::getGlobalName(), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    static QString slashTitles[] = {
	i18n("&None"), "&1", "&2", "&3", "&4", "&5"
    };
    for (int i = 0; i <= 5; ++i) {
	new KAction(slashTitles[i], 0, this,
		    SLOT(slotAddSlashes()), actionCollection(),
		    QString("slashes_%1").arg(i));
    }

    new KAction(ClefInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(KeyInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddKeySignature()), actionCollection(),
                "add_key_signature");

    // setup Settings menu
    static QString actionsToolbars[][4] = 
        {
            { i18n("Show T&ools Toolbar"),  "1slotToggleToolsToolBar()",  "show_tools_toolbar",                    "palette-tools" },
            { i18n("Show &Notes Toolbar"),  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { i18n("Show &Rests Toolbar"),  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { i18n("Show &Accidentals Toolbar"),   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { i18n("Show Cle&fs Toolbar"),  "1slotToggleClefsToolBar()",  "show_clefs_toolbar",
          "palette-clefs" },
            { i18n("Show &Marks Toolbar"), "1slotToggleMarksToolBar()",   "show_marks_toolbar",
          "palette-marks" },
            { i18n("Show &Group Toolbar"), "1slotToggleGroupToolBar()",   "show_group_toolbar",
          "palette-group" },            
            { i18n("Show &Layout Toolbar"), "1slotToggleLayoutToolBar()", "show_layout_toolbar",
          "palette-font" },
            { i18n("Show Trans&port Toolbar"), "1slotToggleTransportToolBar()", "show_transport_toolbar",
	  "palette-transport" },
            { i18n("Show M&eta Toolbar"), "1slotToggleMetaToolBar()", "show_meta_toolbar",
          "palette-meta" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsToolbars)/sizeof(actionsToolbars[0]); ++i) {

        icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap(actionsToolbars[i][3])));

        new KToggleAction(actionsToolbars[i][0], icon, 0,
                          this, actionsToolbars[i][1],
                          actionCollection(), actionsToolbars[i][2]);
    }

    new KAction(i18n("Cursor &Back"), 0, Key_Left, this,
		SLOT(slotStepBackward()), actionCollection(),
		"cursor_back");

    new KAction(i18n("Cursor &Forward"), 0, Key_Right, this,
		SLOT(slotStepForward()), actionCollection(),
		"cursor_forward");

    new KAction(i18n("Cursor Ba&ck Bar"), 0, Key_Left + CTRL, this,
		SLOT(slotJumpBackward()), actionCollection(),
		"cursor_back_bar");

    new KAction(i18n("Cursor For&ward Bar"), 0, Key_Right + CTRL, this,
		SLOT(slotJumpForward()), actionCollection(),
		"cursor_forward_bar");

    new KAction(i18n("Cursor Back and Se&lect"), SHIFT + Key_Left, this,
		SLOT(slotExtendSelectionBackward()), actionCollection(),
		"extend_selection_backward");

    new KAction(i18n("Cursor Forward and &Select"), SHIFT + Key_Right, this,
		SLOT(slotExtendSelectionForward()), actionCollection(),
		"extend_selection_forward");

    new KAction(i18n("Cursor Back Bar and Select"), SHIFT + CTRL + Key_Left, this,
		SLOT(slotExtendSelectionBackwardBar()), actionCollection(),
		"extend_selection_backward_bar");

    new KAction(i18n("Cursor Forward Bar and Select"), SHIFT + CTRL + Key_Right, this,
		SLOT(slotExtendSelectionForwardBar()), actionCollection(),
		"extend_selection_forward_bar");

/*!!! not here yet
    new KAction(i18n("Move Selection Left"), Key_Minus, this,
		SLOT(slotMoveSelectionLeft()), actionCollection(),
		"move_selection_left");
*/

    new KAction(i18n("Cursor to St&art"), 0, Key_A + CTRL, this,
		SLOT(slotJumpToStart()), actionCollection(),
		"cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
		SLOT(slotJumpToEnd()), actionCollection(),
		"cursor_end");

    new KAction(i18n("Cursor &Up Staff"), 0, Key_Up + SHIFT, this,
		SLOT(slotCurrentStaffUp()), actionCollection(),
		"cursor_up_staff");

    new KAction(i18n("Cursor &Down Staff"), 0, Key_Down + SHIFT, this,
		SLOT(slotCurrentStaffDown()), actionCollection(),
		"cursor_down_staff");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
		    ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                SLOT(slotToggleSolo()), actionCollection(),
                "toggle_solo");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
		SLOT(slotPreviewSelection()), actionCollection(),
		"preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
		SLOT(slotClearLoop()), actionCollection(),
		"clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

    QString pixmapDir =
	KGlobal::dirs()->findResource("appdata", "pixmaps/");
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), icon, 0, this,
                SLOT(slotFilterSelection()), actionCollection(),
                "filter_selection");

    createGUI(getRCFileName(), false);
}

bool
NotationView::isInChordMode()
{
    return ((KToggleAction *)actionCollection()->action("chord_mode"))->
	isChecked();
}

bool
NotationView::isInTripletMode()
{
    return ((KToggleAction *)actionCollection()->action("triplet_mode"))->
	isChecked();
}

void
NotationView::setupFontSizeMenu(std::string oldFontName)
{
    if (oldFontName != "") {

	std::vector<int> sizes = NoteFontFactory::getScreenSizes(oldFontName);
	
	for (unsigned int i = 0; i < sizes.size(); ++i) {
	    KAction *action =
		actionCollection()->action
		(QString("note_font_size_%1").arg(sizes[i]));
	    m_fontSizeActionMenu->remove(action);

	    // Don't delete -- that could cause a crash when this
	    // function is called from the action itself.  Instead
	    // we reuse and reinsert existing actions below.
	}
    }

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);

    for (unsigned int i = 0; i < sizes.size(); ++i) {

	QString actionName = QString("note_font_size_%1").arg(sizes[i]);

	KToggleAction *sizeAction = dynamic_cast<KToggleAction *>
	    (actionCollection()->action(actionName));

	if (!sizeAction) {
	    sizeAction = 
		new KToggleAction
		(sizes[i] == 1 ? i18n("%1 pixel").arg(sizes[i]) :
		                 i18n("%1 pixels").arg(sizes[i]),
		 0, this,
		 SLOT(slotChangeFontSizeFromAction()),
		 actionCollection(), actionName);
	}

	sizeAction->setChecked(sizes[i] == m_fontSize);
	m_fontSizeActionMenu->insert(sizeAction);
    }
}


NotationStaff *
NotationView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment) return m_staffs[i];
    }
    return 0;
}

void NotationView::initLayoutToolbar()
{
    KToolBar *layoutToolbar = toolBar("Layout Toolbar");
    
    if (!layoutToolbar) {
	std::cerr
            << "NotationView::initLayoutToolbar() : layout toolbar not found"
	    << std::endl;
        return;
    }

    new QLabel(i18n("  Font:  "), layoutToolbar, "font label");

    //
    // font combo
    //
    m_fontCombo = new KComboBox(layoutToolbar);
    m_fontCombo->setEditable(false);

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

	QString fontQName(strtoqstr(*i));

        m_fontCombo->insertItem(fontQName);
        if (fontQName.lower() == strtoqstr(m_fontName).lower()) {
            m_fontCombo->setCurrentItem(m_fontCombo->count() - 1);
	    foundFont = true;
        }
    }

    if (!foundFont) {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font \"%1\", using default")).arg
	     (strtoqstr(m_fontName)));
	m_fontName = NoteFontFactory::getDefaultFontName();
    }
    
    connect(m_fontCombo, SIGNAL(activated(const QString &)),
            this,        SLOT(slotChangeFont(const QString &)));

    new QLabel(i18n("  Size:  "), layoutToolbar, "size label");

    QString value;

    //
    // font size combo
    //
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeCombo = new KComboBox(layoutToolbar, "font size combo");

    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {

        value.setNum(*i);
        m_fontSizeCombo->insertItem(value);
    }
    // set combo's current value to default
    value.setNum(m_fontSize);
    m_fontSizeCombo->setCurrentText(value);

    connect(m_fontSizeCombo, SIGNAL(activated(const QString&)),
            this, SLOT(slotChangeFontSizeFromStringValue(const QString&)));

    new QLabel(i18n("  Spacing:  "), layoutToolbar, "spacing label");

    //
    // spacing combo
    //
    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    m_spacingCombo = new KComboBox(layoutToolbar, "spacing combo");
    for (std::vector<int>::iterator i = spacings.begin(); i != spacings.end(); ++i) {

        value.setNum(*i);
        value += "%";
        m_spacingCombo->insertItem(value);
    }
    // set combo's current value to default
    value.setNum(defaultSpacing); value += "%";
    m_spacingCombo->setCurrentText(value);
    
    connect(m_spacingCombo, SIGNAL(activated(const QString&)),
            this, SLOT(slotChangeSpacingFromStringValue(const QString&)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    m_hoveredOverNoteName = new QLabel(sb);
    m_hoveredOverNoteName->setMinimumWidth(32);

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    QHBox *hbox = new QHBox(sb);
    m_currentNotePixmap = new QLabel(hbox);
    m_currentNotePixmap->setMinimumWidth(20);
    m_insertModeLabel = new QLabel(hbox);
    m_annotationsLabel = new QLabel(hbox);
    sb->addWidget(hbox);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar = new RosegardenProgressBar(100, true, sb);
    m_progressBar->setMinimumWidth(100);
    sb->addWidget(m_progressBar);
}

QSize NotationView::getViewSize()
{
    return canvas()->size();
}

void NotationView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());
}


void
NotationView::setPageMode(LinedStaff::PageMode pageMode)
{
    m_pageMode = pageMode;

    if (pageMode != LinedStaff::LinearMode) {
	if (m_topBarButtons) m_topBarButtons->hide();
	if (m_bottomBarButtons) m_bottomBarButtons->hide();
	if (m_chordNameRuler) m_chordNameRuler->hide();
	if (m_rawNoteRuler) m_rawNoteRuler->hide();
	if (m_tempoRuler) m_tempoRuler->hide();
    } else {
	if (m_topBarButtons) m_topBarButtons->show();
	if (m_bottomBarButtons) m_bottomBarButtons->show();
	if (m_chordNameRuler && getToggleAction("show_chords_ruler")->isChecked())
	    m_chordNameRuler->show();
	if (m_rawNoteRuler && getToggleAction("show_raw_note_ruler")->isChecked())
	    m_rawNoteRuler->show();
	if (m_tempoRuler && getToggleAction("show_tempo_ruler")->isChecked())
	    m_tempoRuler->show();
    }

    stateChanged("linear_mode",
		 (pageMode == LinedStaff::LinearMode ? KXMLGUIClient::StateNoReverse :
		                                       KXMLGUIClient::StateReverse));

    int pageWidth = getPageWidth();
    int topMargin = 0, leftMargin = 0;
    getPageMargins(leftMargin, topMargin);

    m_hlayout->setPageMode(pageMode != LinedStaff::LinearMode);
    m_hlayout->setPageWidth(pageWidth - leftMargin * 2);

    NOTATION_DEBUG << "NotationView::setPageMode: set layout's page width to "
		   << (pageWidth - leftMargin * 2) << endl;

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->markChanged();
        }
    }

    positionPages();
    if (!m_printMode) updateView();

    Rosegarden::Profiles::getInstance()->dump();
}   

int
NotationView::getPageWidth()
{
    if (m_pageMode != LinedStaff::MultiPageMode) {

	if (isInPrintMode() && getCanvasView() && getCanvasView()->canvas())
	    return getCanvasView()->canvas()->width();
	
	return width() - 50;

    } else {

	//!!! For the moment we use A4 for this calculation
	
	double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
	double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
	return (int)(210.0 / mmPerPixel);
    }
}

int
NotationView::getPageHeight()
{
    if (m_pageMode != LinedStaff::MultiPageMode) {
	return 0;
    } else {

	//!!! For the moment we use A4 for this calculation
	
	double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
	double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
	return (int)(297.0 / mmPerPixel);
    }
}

void
NotationView::getPageMargins(int &left, int &top)
{
    if (m_pageMode != LinedStaff::MultiPageMode) {

	left = 0;
	top = 0;

    } else {

	//!!! For the moment we use A4 for this calculation
	
	double printSizeMm = 25.4 * ((double)m_printSize / 72.0);
	double mmPerPixel = printSizeMm / (double)m_notePixmapFactory->getSize();
	left = (int)(20.0 / mmPerPixel);
	top  = (int)(15.0 / mmPerPixel);
    }
}

/// Scrolls the view such that the given time is centered
void
NotationView::scrollToTime(timeT t) {

    double notationViewLayoutCoord = m_hlayout->getXForTime(t);

    // Doesn't appear to matter which staff we use
    //!!! actually it probably does matter, if they don't have the same extents
    double notationViewCanvasCoord =
	getStaff(0)->getCanvasCoordsForLayoutCoords
	(notationViewLayoutCoord, 0).first;

    // HK: I could have sworn I saw a hard-coded scroll happen somewhere
    // (i.e. a default extra scroll to make up for the staff not beginning on
    // the left edge) but now I can't find it.
    getCanvasView()->slotScrollHorizSmallSteps
	(int(notationViewCanvasCoord));// + DEFAULT_STAFF_OFFSET);
}

Rosegarden::RulerScale*
NotationView::getHLayout()
{
    return m_hlayout;
}

void
NotationView::paintEvent(QPaintEvent *e)
{
    NOTATION_DEBUG << "NotationView::paintEvent: m_hlayout->isPageMode() returns " << m_hlayout->isPageMode() << endl;

    m_inPaintEvent = true;

    // This is duplicated here from EditViewBase, because (a) we need
    // to know about the segment being removed before we try to check
    // the staff names etc., and (b) it's not safe to call close()
    // from EditViewBase::paintEvent if we're then going to try to do
    // some more work afterwards in this function

    if (isCompositionModified()) {
      
        // Check if one of the segments we display has been removed
        // from the composition.
        //
	// For the moment we'll have to close the view if any of the
	// segments we handle has been deleted.

	for (unsigned int i = 0; i < m_segments.size(); ++i) {

	    if (!m_segments[i]->getComposition()) {
		// oops, I think we've been deleted
		close();
		return;
	    }
	}
    }

    // relayout if the window width changes significantly in continuous page mode
    if (m_pageMode == LinedStaff::ContinuousPageMode) {
	int diff = int(getPageWidth() - m_hlayout->getPageWidth());
	NOTATION_DEBUG << "NotationView::paintEvent: diff is " << diff <<endl;
	if (diff < -10 || diff > 10) {
	    setPageMode(m_pageMode);
	    refreshSegment(0, 0, 0);
	}
    }

    // check for staff name changes
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	if (!m_staffs[i]->isStaffNameUpToDate()) {
	    refreshSegment(0);
	    break;
	}
    }

    m_inPaintEvent = false;

    EditView::paintEvent(e);

    m_inPaintEvent = false;

    // now deal with any backlog of insertable notes that appeared
    // during paint (because it's not safe to modify a segment from
    // within a sub-event-loop in a processEvents call from a paint)
    if (!m_pendingInsertableNotes.empty()) {
	std::vector<std::pair<int, int> > notes = m_pendingInsertableNotes;
	m_pendingInsertableNotes.clear();
	for (unsigned int i = 0; i < notes.size(); ++i) {
	    slotInsertableNoteEventReceived(notes[i].first, notes[i].second, true);
	}
    }
}

bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    slotSetOperationNameAndStatus(i18n("Laying out score..."));
    RosegardenProgressDialog::processEvents();

    m_hlayout->setStaffCount(m_staffs.size());

    Rosegarden::Profiler profiler("NotationView::applyLayout");
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        slotSetOperationNameAndStatus(i18n("Laying out staff %1...").arg(i + 1));
        RosegardenProgressDialog::processEvents();

        m_hlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_hlayout->scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->scanStaff(*m_staffs[i], startTime, endTime);
    }

    slotSetOperationNameAndStatus(i18n("Reconciling staffs..."));
    RosegardenProgressDialog::processEvents();

    m_hlayout->finishLayout(startTime, endTime);
    m_vlayout->finishLayout(startTime, endTime);

    // find the last finishing staff for future use

    timeT lastFinishingStaffEndTime = 0;
    bool haveEndTime = false;
    m_lastFinishingStaff = -1;

    timeT firstStartingStaffStartTime = 0;
    bool haveStartTime = false;
    int firstStartingStaff = -1;

    for (i = 0; i < m_staffs.size(); ++i) {

	timeT thisStartTime = m_staffs[i]->getSegment().getStartTime();
	if (thisStartTime < firstStartingStaffStartTime || !haveStartTime) {
	    firstStartingStaffStartTime = thisStartTime;
	    haveStartTime = true;
	    firstStartingStaff = i;
	}

        timeT thisEndTime = m_staffs[i]->getSegment().getEndTime();
        if (thisEndTime > lastFinishingStaffEndTime || !haveEndTime) {
            lastFinishingStaffEndTime = thisEndTime;
	    haveEndTime = true;
            m_lastFinishingStaff = i;
        }
    }

    readjustCanvasSize();
    if (m_topBarButtons) {
	m_topBarButtons->update();
    }
    if (m_bottomBarButtons) {
	m_bottomBarButtons->update();
    }
    if (m_rawNoteRuler && m_rawNoteRuler->isVisible()) {
	m_rawNoteRuler->update();
    }

    return true;
}


void NotationView::setCurrentSelectedNote(const char *pixmapName,
                                          bool rest, Note::Type n, int dots)
{
    NoteInserter* inserter = 0;

    if (rest)
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(RestInserter::ToolName));
    else
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(NoteInserter::ToolName));

    inserter->slotSetNote(n);
    inserter->slotSetDots(dots);

    setTool(inserter);

    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap(pixmapName)));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelectedNote(NoteActionData noteAction)
{
    setCurrentSelectedNote(noteAction.pixmapName,
                           noteAction.rest,
                           noteAction.noteType,
                           noteAction.dots);
}


void NotationView::setCurrentSelection(EventSelection* s, bool preview,
				       bool redrawNow)
{
    //!!! rather too much here shared with matrixview -- could much of
    // this be in editview?

    if (m_currentEventSelection == s) return;
    NOTATION_DEBUG << "XXX " << endl;

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    // positionElements is overkill here, but we hope it's not too
    // much overkill (if that's not a contradiction)

    timeT startA, endA, startB, endB;

    if (oldSelection) {
	startA = oldSelection->getStartTime();
	endA   = oldSelection->getEndTime();
	startB = s ? s->getStartTime() : startA;
	endB   = s ? s->getEndTime()   : endA;
    } else {
	// we know they can't both be null -- first thing we tested above
	startA = startB = s->getStartTime();
	endA   = endB   = s->getEndTime();
    }

    // refreshSegment takes start==end to mean refresh everything
    if (startA == endA) ++endA;
    if (startB == endB) ++endB;

    bool updateRequired = true;

    // play previews if appropriate -- also permits an optimisation
    // for the case where the selection is unchanged (quite likely
    // when sweeping) 

    if (s && preview) {

	bool foundNewEvent = false;

	for (EventSelection::eventcontainer::iterator i =
		 s->getSegmentEvents().begin();
	     i != s->getSegmentEvents().end(); ++i) {
	    
	    if (oldSelection && oldSelection->getSegment() == s->getSegment()
		&& oldSelection->contains(*i)) continue;
		
	    foundNewEvent = true;

	    long pitch;
	    if (!(*i)->get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH,
					    pitch)) continue;

	    long velocity = -1;
	    (void)(*i)->get<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY,
					     velocity);

	    playNote(s->getSegment(), pitch, velocity);
	}

	if (!foundNewEvent) {
	    if (oldSelection &&
		oldSelection->getSegment() == s->getSegment() &&
		oldSelection->getSegmentEvents().size() ==
		s->getSegmentEvents().size()) updateRequired = false;
	}
    }

    if (updateRequired) {

	if ((endA >= startB && endB >= startA) &&
	    (!s || !oldSelection ||
	     oldSelection->getSegment() == s->getSegment())) {
	    
	    // the regions overlap: use their union and just do one refresh

	    Segment &segment(s ? s->getSegment() :
			     oldSelection->getSegment());

	    if (redrawNow) {
		// recolour the events now
		getStaff(segment)->positionElements(std::min(startA, startB),
						    std::max(endA, endB));
	    } else {
		// mark refresh status and then request a repaint
		segment.getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(segment)->getId()]).
		    push(std::min(startA, startB), std::max(endA, endB));
	    }

	} else {
	    // do two refreshes, one for each -- here we know neither is null

	    if (redrawNow) {
		// recolour the events now
		getStaff(oldSelection->getSegment())->positionElements(startA,
								       endA);
		
		getStaff(s->getSegment())->positionElements(startB, endB);
	    } else {
		// mark refresh status and then request a repaint

		oldSelection->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(oldSelection->getSegment())->getId()]).
		    push(startA, endA);
		
		s->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(s->getSegment())->getId()]).
		    push(startB, endB);
	    }
	}

	if (s) {
	    // make the staff containing the selection current
	    int staffId = getStaff(s->getSegment())->getId();
	    if (staffId != m_currentStaff) slotSetCurrentStaff(staffId);
	}
    }

    delete oldSelection;

    statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(),
			    KTmpStatusMsg::getDefaultId());

    if (s) {
	int eventsSelected = s->getSegmentEvents().size();
        m_selectionCounter->setText
	    (i18n("  1 event selected ",
		  "  %n events selected ", eventsSelected));
    } else {
	m_selectionCounter->setText(i18n("  No selection "));
    }
    m_selectionCounter->update();

    setMenuStates();

    if (redrawNow) updateView();
    else update();

    NOTATION_DEBUG << "XXX " << endl;
}

void NotationView::setSingleSelectedEvent(int staffNo, Event *event,
					  bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
			   preview, redrawNow);
}

void NotationView::setSingleSelectedEvent(Segment &segment, Event *event,
					  bool preview, bool redrawNow)
{
    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection, preview, redrawNow);
}

bool NotationView::canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
	sinceLastCutOff = 0;
	lastCutOff = now;
	NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: reset" << endl;
    } else {
	if (sinceLastCutOff >= 20) {
	    // don't permit more than 20 notes per second or so, to
	    // avoid gungeing up the sound drivers
	    NOTATION_DEBUG << "Rejecting preview (too busy)" << endl;
	    return false;
	}
	NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: ok" << endl;
    }

    return true;
}

void NotationView::playNote(Rosegarden::Segment &s, int pitch, int velocity)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(s.getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0) return;

    if (!canPreviewAnotherNote()) return;

    if (velocity < 0) velocity = Rosegarden::MidiMaxValue;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNoteOneShot,
                               pitch,
                               velocity,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime(0, 500000000),
                               Rosegarden::RealTime::zeroTime);

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void NotationView::showPreviewNote(int staffNo, double layoutX,
				   int pitch, int height,
				   const Rosegarden::Note &note,
				   int velocity)
{ 
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note);
    playNote(m_staffs[staffNo]->getSegment(), pitch, velocity);
}

void NotationView::clearPreviewNote()
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	m_staffs[i]->clearPreviewNote();
    }
}

void NotationView::setNotePixmapFactory(NotePixmapFactory* f)
{
    delete m_notePixmapFactory;
    m_notePixmapFactory = f;
}


NotationCanvasView* NotationView::getCanvasView()
{
    return dynamic_cast<NotationCanvasView *>(m_canvasView);
}

Rosegarden::Segment *
NotationView::getCurrentSegment()
{
    NotationStaff *staff = getStaff(m_currentStaff);
    return (staff ? &staff->getSegment() : 0);
}

Rosegarden::Staff *
NotationView::getCurrentStaff()
{
    return getStaff(m_currentStaff);
}

timeT
NotationView::getInsertionTime()
{
    return m_insertionTime;
}


timeT
NotationView::getInsertionTime(Rosegarden::Clef &clef,
			       Rosegarden::Key &key)
{
    // This fuss is solely to recover the clef and key: we already
    // set m_insertionTime to the right value when we first placed
    // the insert cursor.  We could get clef and key directly from
    // the segment but the staff has a more efficient lookup

    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);
    
    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);
    else clef = Rosegarden::Clef();

    if (keyEvt) key = Rosegarden::Key(*keyEvt);
    else key = Rosegarden::Key();

    return m_insertionTime;
}


LinedStaff*
NotationView::getStaffForCanvasCoords(int x, int y) const
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	NotationStaff *s = m_staffs[i];

//	NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): looking at staff " << i << endl;

        if (s->containsCanvasCoords(x, y)) {
	    
	    NotationStaff::LinedStaffCoords coords = 
		s->getLayoutCoordsForCanvasCoords(x, y);

//	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): layout coords are (" << coords.first << "," << coords.second << ")" << endl;

	    int barNo = m_hlayout->getBarForX(coords.first);
//	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): bar number " << barNo << endl;
	    // 931067: < instead of <= in conditional:
	    if (barNo >= m_hlayout->getFirstVisibleBarOnStaff(*s) &&
		barNo < m_hlayout->getLastVisibleBarOnStaff(*s)) {
//		NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): it's within range for staff " << i << m_hlayout->getFirstVisibleBarOnStaff(*s) << "->" << m_hlayout->getLastVisibleBarOnStaff(*s) << ", returning true" << endl;
		return m_staffs[i];
	    }
//	    NOTATION_DEBUG << "NotationView::getStaffForCanvasCoords(" << x << "," << y << "): out of range for this staff " << i << " (" << m_hlayout->getFirstVisibleBarOnStaff(*s) << "->" << m_hlayout->getLastVisibleBarOnStaff(*s) << ")" << endl;
	}
    }

    return 0;
}

void NotationView::updateView()
{
    slotCheckRendered
	(getCanvasView()->contentsX(),
	 getCanvasView()->contentsX() + getCanvasView()->visibleWidth());
    canvas()->update();
}

void NotationView::print(bool previewOnly)
{
    if (m_staffs.size() == 0) {
        KMessageBox::error(0, "Nothing to print");
        return;
    }

    Rosegarden::Profiler profiler("NotationView::print");

    // We need to be in multi-page mode at this point

    int pageWidth = getPageWidth();
    int pageHeight = getPageHeight();
    int leftMargin = 0, topMargin = 0;
    getPageMargins(leftMargin, topMargin);
    int maxPageCount = 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	int pageCount = m_staffs[i]->getPageCount();
	NOTATION_DEBUG << "NotationView::print(): staff " << i << " reports " << pageCount << " pages " << endl;
	if (pageCount > maxPageCount) maxPageCount = pageCount;
    }

    KPrinter printer(true, QPrinter::HighResolution);

    printer.setPageSelection(KPrinter::ApplicationSide);
    printer.setMinMax(1, maxPageCount + 1);

    if (previewOnly) printer.setPreviewOnly(true);
    else if (!printer.setup((QWidget *)parent())) return;
    
    QPaintDeviceMetrics pdm(&printer);
    QPainter printpainter(&printer);
    printpainter.scale((double)pdm.width()  / (double)pageWidth,
		       (double)pdm.height() / (double)pageHeight);

    QValueList<int> pages = printer.pageList();

    for (QValueList<int>::Iterator pli = pages.begin();
	 pli != pages.end(); ) { // incremented just below
	
	int page = *pli - 1;
	++pli;
	if (page < 0 || page >= maxPageCount) continue;

	NOTATION_DEBUG << "Printing page " << page << endl;

	QRect pageRect(20 + pageWidth * page, topMargin, pageWidth, pageHeight);
	
	for (size_t i = 0; i < m_staffs.size(); ++i) {

	    NotationStaff *staff = m_staffs[i];
	    
	    LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
		(pageRect.x(), pageRect.y());

	    LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
		(pageRect.x() + pageRect.width(), pageRect.y() + pageRect.height());

	    timeT t0 = m_hlayout->getTimeForX(cc0.first);
	    timeT t1 = m_hlayout->getTimeForX(cc1.first);

	    m_staffs[i]->setPrintPainter(&printpainter);
	    m_staffs[i]->checkRendered(t0, t1);
	}

	// Supplying doublebuffer==true to this method appears to
        // slow down printing considerably but without it we get
	// all sorts of horrible artifacts (possibly related to
	// mishandling of pixmap masks?) in qt-3.0.  Let's permit
	// it as a "hidden" option.

	m_config->setGroup(NotationView::ConfigGroup);

	NOTATION_DEBUG << "NotationView::print: calling QCanvas::drawArea" << endl;

	{
	    Rosegarden::Profiler profiler("NotationView::print(QCanvas::drawArea)");

	if (m_config->readBoolEntry("forcedoublebufferprinting", false)) {
	    getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
	} else {
#if QT_VERSION >= 0x030100
	    getCanvasView()->canvas()->drawArea(pageRect, &printpainter, false);
#else
	    getCanvasView()->canvas()->drawArea(pageRect, &printpainter, true);
#endif
	}

	}

	NOTATION_DEBUG << "NotationView::print: QCanvas::drawArea done" << endl;

	//!!! experimental stuff this
	for (size_t i = 0; i < m_staffs.size(); ++i) {

	    NotationStaff *staff = m_staffs[i];
	    
	    LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
		(pageRect.x(), pageRect.y());

	    LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
		(pageRect.x() + pageRect.width(), pageRect.y() + pageRect.height());

	    timeT t0 = m_hlayout->getTimeForX(cc0.first);
	    timeT t1 = m_hlayout->getTimeForX(cc1.first);

	    m_staffs[i]->renderPrintable(t0, t1);
	}

        printpainter.translate(-pageWidth, 0);

	if (pli != pages.end() && *pli - 1 < maxPageCount) printer.newPage();
	
	for (size_t i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->markChanged(); // recover any memory used for this page
	    PixmapArrayGC::deleteAll();
	}
    }

    for (size_t i = 0; i < m_staffs.size(); ++i) {
	for (Segment::iterator j = m_staffs[i]->getSegment().begin();
	     j != m_staffs[i]->getSegment().end(); ++j) {
	    removeViewLocalProperties(*j);
	}
	delete m_staffs[i];
    }
    m_staffs.clear();
    
    printpainter.end();

    Rosegarden::Profiles::getInstance()->dump();
}

void NotationView::refreshSegment(Segment *segment,
				  timeT startTime, timeT endTime)
{
    NOTATION_DEBUG << "*** " << endl;

    if (m_inhibitRefresh) return;
    NOTATION_DEBUG << "NotationView::refreshSegment(" << segment << "," << startTime << "," << endTime << ")" << endl;
    Rosegarden::Profiler foo("NotationView::refreshSegment");

    emit usedSelection();

    if (segment) {
        NotationStaff *staff = getStaff(*segment);
        if (staff) applyLayout(staff->getId(), startTime, endTime);
    } else {
        applyLayout(-1, startTime, endTime);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Segment *ssegment = &m_staffs[i]->getSegment();
        bool thisStaff = (ssegment == segment || segment == 0);
	m_staffs[i]->markChanged(startTime, endTime, !thisStaff);
    }

    PixmapArrayGC::deleteAll();

    statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(),
			    KTmpStatusMsg::getDefaultId());

    Event::dumpStats(std::cerr);
    slotSetPointerPosition(getDocument()->getComposition().getPosition(), false);

    if (m_currentEventSelection &&
	m_currentEventSelection->getSegmentEvents().size() == 0) {
	delete m_currentEventSelection;
	m_currentEventSelection = 0;
	//!!!??? was that the right thing to do?
    }

    setMenuStates();

    NOTATION_DEBUG << "*** " << endl;
}


void NotationView::setMenuStates()
{
    // 1. set selection-related states

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (m_currentEventSelection) {

	NOTATION_DEBUG << "NotationView::setMenuStates: Have selection; it's " << m_currentEventSelection << " covering range from " << m_currentEventSelection->getStartTime() << " to " << m_currentEventSelection->getEndTime() << " (" << m_currentEventSelection->getSegmentEvents().size() << " events)" << endl;

	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventType)) {
	    stateChanged("have_notes_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventRestType)) {
	    stateChanged("have_rests_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
    }

    // 2. set inserter-related states

    if (dynamic_cast<NoteInserter *>(m_tool)) {
	NOTATION_DEBUG << "Have note inserter " << endl;
	stateChanged("note_insert_tool_current", StateNoReverse);
	stateChanged("rest_insert_tool_current", StateReverse);
    } else if (dynamic_cast<RestInserter *>(m_tool)) {
	NOTATION_DEBUG << "Have rest inserter " << endl;
	stateChanged("note_insert_tool_current", StateReverse);
	stateChanged("rest_insert_tool_current", StateNoReverse);
    } else {
	NOTATION_DEBUG << "Have neither inserter " << endl;
	stateChanged("note_insert_tool_current", StateReverse);
	stateChanged("rest_insert_tool_current", StateReverse);
    }
}


#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (progressTotal > 0) { \
	    emit setProgress(progressCount * 100 / progressTotal); \
	    RosegardenProgressDialog::processEvents(); \
	}

    
void NotationView::readjustCanvasSize()
{
    Rosegarden::Profiler profiler("NotationView::readjustCanvasSize");

    double maxWidth = 0.0;
    int maxHeight = 0;

    slotSetOperationNameAndStatus(i18n("Sizing and allocating canvas..."));
    RosegardenProgressDialog::processEvents();

    int progressTotal = m_staffs.size() + 2;
    int progressCount = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff &staff = *m_staffs[i];

        staff.sizeStaff(*m_hlayout);
	UPDATE_PROGRESS(1);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }
    }

    NOTATION_DEBUG << "NotationView::readjustCanvasSize: maxHeight is "
		   << maxHeight << ", page height is " << getPageHeight() << endl;

    if (maxWidth  < getPageWidth()  + 40) maxWidth  = getPageWidth()  + 40;
    if (maxHeight < getPageHeight() + 40) maxHeight = getPageHeight() + 40;

    // now get the EditView to do the biz
    readjustViewSize(QSize(int(maxWidth), maxHeight));
    UPDATE_PROGRESS(2);

    if (m_pannerDialog) {
    
	if (m_pageMode != LinedStaff::MultiPageMode) {
	    m_pannerDialog->hide();

	} else {

	    m_pannerDialog->show();

	    m_pannerDialog->setPageSize
		(QSize(canvas()->width(),
		       canvas()->height()));

	    m_pannerDialog->scrollbox()->setViewSize
		(QSize(getCanvasView()->width(),
		       getCanvasView()->height()));
	}
    }
}


// Slots: These are here because they use the note action data map
// or mark action data map


void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct =
	m_noteActionDataMap->find(sigSender->name());
    
    if (noteAct != m_noteActionDataMap->end()) {
	m_lastNoteAction = sigSender->name();
        setCurrentSelectedNote(*noteAct);
	setMenuStates();
    } else {
        std::cerr << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
                             << sigSender->name() << "'\n";
    }
}
    
// Reactivate the last note that was activated
void NotationView::slotLastNoteAction()
{
    KAction *action = actionCollection()->action(m_lastNoteAction);
    if (!action) action = actionCollection()->action("crotchet");

    if (action) {
	action->activate();
    } else {
        std::cerr << "NotationView::slotNoteAction() : couldn't find action named '"
                             << m_lastNoteAction << "' or 'crotchet'\n";
    }
}

void NotationView::slotAddMark()
{
    const QObject *s = sender();
    if (!m_currentEventSelection) return;

    MarkActionDataMap::Iterator i = m_markActionDataMap->find(s->name());

    if (i != m_markActionDataMap->end()) {
	addCommandToHistory(new MarksMenuAddMarkCommand
			    ((*i).mark, *m_currentEventSelection));
    }
}

void NotationView::initActionDataMaps()
{
    static bool called = false;
    static int keys[] =
    { Key_0, Key_3, Key_6, Key_8, Key_4, Key_2, Key_1, Key_5 };
    
    if (called) return;
    called = true;

    m_noteActionDataMap = new NoteActionDataMap;

    Note referenceNote(Note::Crotchet); // type doesn't matter
    for (int rest = 0; rest < 2; ++rest) {
	for (int dots = 0; dots < 2; ++dots) {
	    for (int type = Note::Longest; type >= Note::Shortest; --type) {
		if (dots && (type == Note::Longest)) continue;

		QString refName
		    (NotationStrings::getReferenceName(Note(type, dots), rest == 1));

		QString shortName(refName);
		shortName.replace(QRegExp("-"), "_");

		QString titleName
		    (NotationStrings::getNoteName(Note(type, dots)));

		titleName = titleName.left(1).upper() +
		            titleName.right(titleName.length()-1);

		if (rest) {
		    titleName.replace(QRegExp(i18n("note")), i18n("rest"));
		}

		int keycode = keys[type - Note::Shortest];
		if (dots) keycode += CTRL;
		if (rest) // keycode += SHIFT; -- can't do shift+numbers
		    keycode = 0;

		m_noteActionDataMap->insert
		    (shortName, NoteActionData
		     (titleName, shortName, refName, keycode,
		      rest > 0, type, dots));
	    }
	}
    }

    m_markActionDataMap = new MarkActionDataMap;

    std::vector<Mark> marks = Rosegarden::Marks::getStandardMarks();
    for (unsigned int i = 0; i < marks.size(); ++i) {

	Mark mark = marks[i];
	QString markName(strtoqstr(mark));
	QString actionName = QString("add_%1").arg(markName);

	m_markActionDataMap->insert
	    (actionName, MarkActionData
	     (MarksMenuAddMarkCommand::getGlobalName(mark),
	      actionName, markName, 0, mark));
    }
	     
}

void NotationView::setupProgress(KProgress* bar)
{
    if (bar) {
        NOTATION_DEBUG << "NotationView::setupProgress(bar)\n";

        connect(m_hlayout, SIGNAL(setProgress(int)),
                bar,       SLOT(setValue(int)));

        connect(m_hlayout, SIGNAL(incrementProgress(int)),
                bar,       SLOT(advance(int)));

        connect(this,      SIGNAL(setProgress(int)),
                bar,       SLOT(setValue(int)));

        connect(this,      SIGNAL(incrementProgress(int)),
                bar,       SLOT(advance(int)));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setProgress(int)),
                    bar,         SLOT(setValue(int)));

            connect(m_staffs[i], SIGNAL(incrementProgress(int)),
                    bar,         SLOT(advance(int)));
        }
    }

}

void NotationView::setupProgress(RosegardenProgressDialog* dialog)
{
    NOTATION_DEBUG << "NotationView::setupProgress(dialog)" << endl;
    disconnectProgress();
    
    if (dialog) {
        setupProgress(dialog->progressBar());

	connect(dialog, SIGNAL(cancelClicked()),
		m_hlayout, SLOT(slotCancel()));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setOperationName(QString)),
                    this,        SLOT(slotSetOperationNameAndStatus(QString)));

            connect(dialog, SIGNAL(cancelClicked()),
                    m_staffs[i], SLOT(slotCancel()));
        }
    
        connect(this, SIGNAL(setOperationName(QString)),
                dialog, SLOT(slotSetOperationName(QString)));
        m_progressDisplayer = PROGRESS_DIALOG;
    }
    
}

void NotationView::slotSetOperationNameAndStatus(QString name)
{
    emit setOperationName(name);
    statusBar()->changeItem(QString("  %1").arg(name),
			    KTmpStatusMsg::getDefaultId());
}

void NotationView::disconnectProgress()
{
    NOTATION_DEBUG << "NotationView::disconnectProgress()" << endl;

    m_hlayout->disconnect();
    disconnect(SIGNAL(setProgress(int)));
    disconnect(SIGNAL(incrementProgress(int)));
    disconnect(SIGNAL(setOperationName(QString)));

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->disconnect();
    }
}

void NotationView::setupDefaultProgress()
{
    if (m_progressDisplayer != PROGRESS_BAR) {
        NOTATION_DEBUG << "NotationView::setupDefaultProgress()" << endl;
        disconnectProgress();
        setupProgress(m_progressBar);
        m_progressDisplayer = PROGRESS_BAR;
    }
}

void
NotationView::keyPressEvent(QKeyEvent *event)
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
NotationView::keyReleaseEvent(QKeyEvent *event)
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

    
NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;
NotationView::MarkActionDataMap* NotationView::m_markActionDataMap = 0;
const char* const NotationView::ConfigGroup = "Notation Options";


