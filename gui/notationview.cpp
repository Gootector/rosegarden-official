// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <sys/times.h>

#include <qcanvas.h>

#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kapp.h>

#include "rosegardenguidoc.h"
#include "notationview.h"
#include "notationelement.h"
#include "notationproperties.h"

#include "notationstaff.h"
#include "notepixmapfactory.h"
#include "qcanvaslinegroupable.h"
#include "ktmpstatusmsg.h"

#include "rosedebug.h"

#include "NotationTypes.h"
#include "TrackNotationHelper.h"
#include "Quantizer.h"
#include "staffline.h"

using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::NoAccidental;
using Rosegarden::Note;
using Rosegarden::Track;
using Rosegarden::TrackNotationHelper;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::TimeSignature;
using Rosegarden::Quantizer;
using Rosegarden::timeT;

using std::vector;

#define ID_STATUS_MSG 1



//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc* doc,
			   vector<Track *> tracks,
                           QWidget *parent,
                           int resolution) :
    KMainWindow(parent),
    m_config(kapp->config()),
    m_document(doc),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_canvasView(new NotationCanvasView(new QCanvas(width() * 2,
                                                    height() * 2),
                                        this)),
    m_notePixmapFactory(resolution),
    m_toolbarNotePixmapFactory(5),
    m_hlayout(0),
    m_vlayout(0),
    m_tool(0),
    m_selectDefaultNote(0),
    m_pointer(0)
{
    assert(tracks.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    setCentralWidget(m_canvasView);

    QObject::connect
	(m_canvasView, SIGNAL(itemClicked(int,const QPoint&, NotationElement*)),
	 this,         SLOT  (itemClicked(int,const QPoint&, NotationElement*)));

    QObject::connect
	(m_canvasView, SIGNAL(mouseMove(QMouseEvent*)),
	 this,         SLOT  (mouseMove(QMouseEvent*)));

    QObject::connect
	(m_canvasView, SIGNAL(mouseRelease(QMouseEvent*)),
	 this,         SLOT  (mouseRelease(QMouseEvent*)));

    QObject::connect
	(m_canvasView, SIGNAL(hoveredOverNoteChange (const QString&)),
	 this,         SLOT  (hoveredOverNoteChanged(const QString&)));

    QObject::connect
	(m_canvasView, SIGNAL(hoveredOverAbsoluteTimeChange(unsigned int)),
	 this,         SLOT  (hoveredOverAbsoluteTimeChange(unsigned int)));

    readOptions();

    if (tracks.size() == 1) {
	setCaption(QString("%1 - Track Instrument #%2")
		   .arg(doc->getTitle())
		   .arg(tracks[0]->getInstrument()));
    } else if (tracks.size() == doc->getComposition().getNbTracks()) {
	setCaption(QString("%1 - All Tracks")
		   .arg(doc->getTitle()));
    } else {
	setCaption(QString("%1 - %2-Track Partial View")
		   .arg(doc->getTitle())
		   .arg(tracks.size()));
    }

    NotePixmapFactory *npf;

    for (unsigned int i = 0; i < tracks.size(); ++i) {
	m_staffs.push_back(new NotationStaff(canvas(), tracks[i], resolution));
	m_staffs[i]->move(20, m_staffs[i]->getStaffHeight() * i + 15);
	m_staffs[i]->show();
	npf = &m_staffs[i]->getNotePixmapFactory();
    }
    m_currentStaff = 0;

    m_vlayout = new NotationVLayout();
    m_hlayout = new NotationHLayout(*npf);

    // Position pointer
    //
    m_pointer = new QCanvasLine(canvas());
    m_pointer->setPen(Qt::darkBlue);
    m_pointer->setPoints(0, 0, 0, canvas()->height());
    // m_pointer->show();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->showElements();
	    showBars(i);
	}
    }

    m_selectDefaultNote->activate();
}

NotationView::~NotationView()
{
    kdDebug(KDEBUG_AREA) << "-> ~NotationView()\n";

    saveOptions();

    delete m_hlayout;
    delete m_vlayout;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;
    delete canvas();

    kdDebug(KDEBUG_AREA) << "<- ~NotationView()\n";
}

void NotationView::saveOptions()
{	
    m_config->setGroup("Notation Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
}

void NotationView::readOptions()
{
    m_config->setGroup("Notation Options");
	
    QSize size(m_config->readSizeEntry("Geometry"));

    if (!size.isEmpty()) {
        resize(size);
    }
}

void NotationView::setupActions()
{   
    KRadioAction* noteAction = 0;
    
    // setup Notes menu & toolbar
    QIconSet icon;
 
    //
    // Notes
    //
    static const char* actionsNote[][4] = 
        {   // i18n,     slotName,         action name,     pixmap
            { "Breve",   "1slotBreve()",   "breve",         "breve" },
            { "Whole",   "1slotWhole()",   "whole_note",    "semibreve" },
            { "Half",    "1slotHalf()",    "half",          "minim" },
            { "Quarter", "1slotQuarter()", "quarter",       "crotchet" },
            { "8th",     "1slot8th()",     "8th",           "quaver" },
            { "16th",    "1slot16th()",    "16th",          "semiquaver" },
            { "32nd",    "1slot32nd()",    "32nd",          "demisemi" },
            { "64th",    "1slot64th()",    "64th",          "hemidemisemi" }
        };
   
    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsNote[i][3]));
        noteAction = new KRadioAction(i18n(actionsNote[i][0]), icon, 0, this,
                                      actionsNote[i][1],
                                      actionCollection(), actionsNote[i][2]);
        noteAction->setExclusiveGroup("notes");

        if (i == 3)
            m_selectDefaultNote = noteAction; // quarter is the default selected note

    }

    //
    // Dotted Notes
    //
    static const char* actionsDottedNote[][4] = 
        {
            { "Dotted Breve",   "1slotDottedBreve()",   "dotted_breve",      "dotted-breve" },
            { "Dotted Whole",   "1slotDottedWhole()",   "dotted_whole_note", "dotted-semibreve" },
            { "Dotted Half",    "1slotDottedHalf()",    "dotted_half",       "dotted-minim" },
            { "Dotted Quarter", "1slotDottedQuarter()", "dotted_quarter",    "dotted-crotchet" },
            { "Dotted 8th",     "1slotDotted8th()",     "dotted_8th",        "dotted-quaver" },
            { "Dotted 16th",    "1slotDotted16th()",    "dotted_16th",       "dotted-semiquaver" },
            { "Dotted 32nd",    "1slotDotted32nd()",    "dotted_32nd",       "dotted-demisemi" },
            { "Dotted 64th",    "1slotDotted64th()",    "dotted_64th",       "dotted-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsDottedNote[i][3]));
        noteAction = new KRadioAction(i18n(actionsDottedNote[i][0]), icon, 0, this,
                                      actionsDottedNote[i][1],
                                      actionCollection(), actionsDottedNote[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Rests
    //
    static const char* actionsRest[][4] = 
        {
            { "Breve Rest",   "1slotRBreve()",   "breve_rest",      "rest-breve" },
            { "Whole Rest",   "1slotRWhole()",   "whole_note_rest", "rest-semibreve" },
            { "Half Rest",    "1slotRHalf()",    "half_rest",       "rest-minim" },
            { "Quarter Rest", "1slotRQuarter()", "quarter_rest",    "rest-crotchet" },
            { "8th Rest",     "1slotR8th()",     "8th_rest",        "rest-quaver" },
            { "16th Rest",    "1slotR16th()",    "16th_rest",       "rest-semiquaver" },
            { "32nd Rest",    "1slotR32nd()",    "32nd_rest",       "rest-demisemi" },
            { "64th Rest",    "1slotR64th()",    "64th_rest",       "rest-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsRest[i][3]));
        noteAction = new KRadioAction(i18n(actionsRest[i][0]), icon, 0, this,
                                      actionsRest[i][1],
                                      actionCollection(), actionsRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Dotted Rests
    //
    static const char* actionsDottedRest[][4] = 
        {
            { "Dotted Breve Rest",   "1slotDottedRBreve()",   "dotted_breve_rest",      "dotted-rest-breve" },
            { "Dotted Whole Rest",   "1slotDottedRWhole()",   "dotted_whole_note_rest", "dotted-rest-semibreve" },
            { "Dotted Half Rest",    "1slotDottedRHalf()",    "dotted_half_rest",       "dotted-rest-minim" },
            { "Dotted Quarter Rest", "1slotDottedRQuarter()", "dotted_quarter_rest",    "dotted-rest-crotchet" },
            { "Dotted 8th Rest",     "1slotDottedR8th()",     "dotted_8th_rest",        "dotted-rest-quaver" },
            { "Dotted 16th Rest",    "1slotDottedR16th()",    "dotted_16th_rest",       "dotted-rest-semiquaver" },
            { "Dotted 32nd Rest",    "1slotDottedR32nd()",    "dotted_32nd_rest",       "dotted-rest-demisemi" },
            { "Dotted 64th Rest",    "1slotDottedR64th()",    "dotted_64th_rest",       "dotted-rest-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8 && noteType > 0; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsDottedRest[i][3]));
        noteAction = new KRadioAction(i18n(actionsDottedRest[i][0]), icon, 0, this,
                                      actionsDottedRest[i][1],
                                      actionCollection(), actionsDottedRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Accidentals
    //
    static const char* actionsAccidental[][4] = 
        {
            { "No accidental",  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
            { "Sharp",          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
            { "Flat",           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
            { "Natural",        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
            { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
            { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
        };

    for (unsigned int i = 0, accidental = NoAccidental;
         i < 6; ++i, ++accidental) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsAccidental[i][3]));
        noteAction = new KRadioAction(i18n(actionsAccidental[i][0]), icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble"));
    noteAction = new KRadioAction(i18n("Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    noteAction = new KRadioAction(i18n("Tenor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    noteAction = new KRadioAction(i18n("Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    noteAction = new KRadioAction(i18n("Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");

    //
    // Edition tools (at the moment, there's only an eraser)
    //
    noteAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    noteAction = new KRadioAction(i18n("Select"), "misc", 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");
    

    // File menu
    KStdAction::close (this, SLOT(closeWindow()),          actionCollection());

   // setup edit menu
    KStdAction::undo    (this, SLOT(slotEditUndo()),       actionCollection());
    KStdAction::redo    (this, SLOT(slotEditRedo()),       actionCollection());
    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Settings menu
    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());

    static const char* actionsToolbars[][4] = 
        {
            { "Show Notes Toolbar",  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { "Show Rests Toolbar",  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { "Show Accidentals Toolbar",   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { "Show Clefs Toolbar",         "1slotToggleClefsToolBar()",        "show_clefs_toolbar",       "palette-clefs" }
        };

    for (unsigned int i = 0; i < 4; ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap(actionsToolbars[i][3]));

        KToggleAction* toolbarAction = new KToggleAction
            (i18n(actionsToolbars[i][0]), icon, 0,
             this, actionsToolbars[i][1],
             actionCollection(), actionsToolbars[i][2]);

/*
        KToggleAction* toolbarAction = new KToggleAction(i18n(actionsToolbars[i][0]), 0,
                                                         this, actionsToolbars[i][1],
                                                         actionCollection(), actionsToolbars[i][2]);
*/
        toolbarAction->setChecked(true);
    }
    
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
}


void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(), KTmpStatusMsg::getDefaultId());
    m_currentNotePixmap       = new QLabel(sb);
    m_hoveredOverNoteName     = new QLabel(sb);
    m_hoveredOverAbsoluteTime = new QLabel(sb);
    
    sb->addWidget(m_currentNotePixmap);
    sb->addWidget(m_hoveredOverNoteName);
    sb->addWidget(m_hoveredOverAbsoluteTime);
}


//!!! This should probably be unnecessary here and done in
//NotationStaff instead (it should be intelligent enough to query the
//notationhlayout itself)

bool NotationView::showBars(int staffNo)
{
    NotationStaff &staff = *m_staffs[staffNo];
    staff.deleteBars(0);

    for (unsigned int i = 0; i < m_hlayout->getBarLineCount(staff); ++i) {

        if (m_hlayout->isBarLineVisible(staff, i)) {
            staff.insertBar(unsigned(m_hlayout->getBarLineX(staff, i)),
                            m_hlayout->isBarLineCorrect(staff, i));
        }
    }

    return true;
}


bool NotationView::applyLayout(int staffNo)
{
    kdDebug(KDEBUG_AREA) << "NotationView::applyLayout() : entering; we have " << m_staffs.size() << " staffs" << endl;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        m_hlayout->resetStaff(*m_staffs[i]);
        m_vlayout->resetStaff(*m_staffs[i]);

        m_hlayout->scanStaff(*m_staffs[i]);
        m_vlayout->scanStaff(*m_staffs[i]);
    }

    m_hlayout->finishLayout();
    m_vlayout->finishLayout();

    readjustCanvasSize();

    kdDebug(KDEBUG_AREA) << "NotationView::applyLayout() : done" << endl;
    return true;
}


void NotationView::setCurrentSelectedNote(const char *pixmapName,
                                          bool rest, Note::Type n, int dots)
{
    if (rest) setTool(new RestInserter(n, dots, *this));
    else      setTool(new NoteInserter(n, dots, *this));

    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap(pixmapName));

    emit changeCurrentNote(rest, n);
}

void NotationView::setTool(NotationTool* tool)
{
    delete m_tool;
    m_tool = tool;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void NotationView::closeWindow()
{
    close();
}

void NotationView::slotEditUndo()
{
    KTmpStatusMsg msg(i18n("Undo..."), statusBar());
}

void NotationView::slotEditRedo()
{
    KTmpStatusMsg msg(i18n("Redo..."), statusBar());
}

void NotationView::slotEditCut()
{
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());
}

void NotationView::slotEditCopy()
{
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());
}

void NotationView::slotEditPaste()
{
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), statusBar());
}

void NotationView::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("notesToolBar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("restsToolBar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("accidentalsToolBar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("clefsToolBar");
}

void NotationView::toggleNamedToolBar(const QString& toolBarName)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        kdDebug(KDEBUG_AREA) << "NotationView::toggleNamedToolBar() : toolBar "
                             << toolBarName << " not found" << endl;
        return;
    }
    
    if (namedToolBar->isVisible())
        namedToolBar->hide();
    else
        namedToolBar->show();
}

void NotationView::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}


void NotationView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void NotationView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}


// Code required to work out where to put the pointer
// (i.e. where is the nearest note) and also if indeed
// it should be currently shown at all for this view
// (is it within scope)
// 
void
NotationView::setPositionPointer(const int& /*position*/)
{
}

//////////////////////////////////////////////////////////////////////

//----------------------------------------
// Notes
//----------------------------------------

void NotationView::slotBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotBreve()\n";
    setCurrentSelectedNote("breve", false, Note::Breve);
}

void NotationView::slotWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotWhole()\n";
    setCurrentSelectedNote("semibreve", false, Note::WholeNote);
}

void NotationView::slotHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotHalf()\n";
    setCurrentSelectedNote("minim", false, Note::HalfNote);
}

void NotationView::slotQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotQuarter()\n";
    setCurrentSelectedNote("crotchet", false, Note::QuarterNote);
}

void NotationView::slot8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot8th()\n";
    setCurrentSelectedNote("quaver", false, Note::EighthNote);
}

void NotationView::slot16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot16th()\n";
    setCurrentSelectedNote("semiquaver", false, Note::SixteenthNote);
}

void NotationView::slot32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot32nd()\n";
    setCurrentSelectedNote("demisemi", false, Note::ThirtySecondNote);
}

void NotationView::slot64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot64th()\n";
    setCurrentSelectedNote("hemidemisemi", false, Note::SixtyFourthNote);
}

void NotationView::slotDottedBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedBreve()\n";
    setCurrentSelectedNote("dotted-breve", false, Note::Breve, 1);
}

void NotationView::slotDottedWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedWhole()\n";
    setCurrentSelectedNote("dotted-semibreve", false, Note::WholeNote, 1);
}

void NotationView::slotDottedHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedHalf()\n";
    setCurrentSelectedNote("dotted-minim", false, Note::HalfNote, 1);
}

void NotationView::slotDottedQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedQuarter()\n";
    setCurrentSelectedNote("dotted-crotchet", false, Note::QuarterNote, 1);
}

void NotationView::slotDotted8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted8th()\n";
    setCurrentSelectedNote("dotted-quaver", false, Note::EighthNote, 1);
}

void NotationView::slotDotted16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted16th()\n";
    setCurrentSelectedNote("dotted-semiquaver", false, Note::SixteenthNote, 1);
}

void NotationView::slotDotted32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted32nd()\n";
    setCurrentSelectedNote("dotted-demisemi", false, Note::ThirtySecondNote, 1);
}

void NotationView::slotDotted64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted64th()\n";
    setCurrentSelectedNote("dotted-hemidemisemi", false, Note::SixtyFourthNote, 1);
}

//----------------------------------------
// Rests
//----------------------------------------

void NotationView::slotRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRBreve()\n";
    setCurrentSelectedNote("rest-breve", true, Note::Breve);
}

void NotationView::slotRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRWhole()\n";
    setCurrentSelectedNote("rest-semibreve", true, Note::WholeNote);
}

void NotationView::slotRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRHalf()\n";
    setCurrentSelectedNote("rest-minim", true, Note::HalfNote);
}

void NotationView::slotRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRQuarter()\n";
    setCurrentSelectedNote("rest-crotchet", true, Note::QuarterNote);
}

void NotationView::slotR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR8th()\n";
    setCurrentSelectedNote("rest-quaver", true, Note::EighthNote);
}

void NotationView::slotR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR16th()\n";
    setCurrentSelectedNote("rest-semiquaver", true, Note::SixteenthNote);
}

void NotationView::slotR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR32nd()\n";
    setCurrentSelectedNote("rest-demisemi", true, Note::ThirtySecondNote);
}

void NotationView::slotR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR64th()\n";
    setCurrentSelectedNote("rest-hemidemisemi", true, Note::SixtyFourthNote);
}

void NotationView::slotDottedRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRBreve()\n";
    setCurrentSelectedNote("dotted-rest-breve", true, Note::Breve, 1);
}

void NotationView::slotDottedRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRWhole()\n";
    setCurrentSelectedNote("dotted-rest-semibreve", true, Note::WholeNote, 1);
}

void NotationView::slotDottedRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRHalf()\n";
    setCurrentSelectedNote("dotted-rest-minim", true, Note::HalfNote, 1);
}

void NotationView::slotDottedRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRQuarter()\n";
    setCurrentSelectedNote("dotted-rest-crotchet", true, Note::QuarterNote, 1);
}

void NotationView::slotDottedR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR8th()\n";
    setCurrentSelectedNote("dotted-rest-quaver", true, Note::EighthNote, 1);
}

void NotationView::slotDottedR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR16th()\n";
    setCurrentSelectedNote("dotted-rest-semiquaver", true, Note::SixteenthNote, 1);
}

void NotationView::slotDottedR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR32nd()\n";
    setCurrentSelectedNote("dotted-rest-demisemi", true, Note::ThirtySecondNote, 1);
}

void NotationView::slotDottedR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR64th()\n";
    setCurrentSelectedNote("dotted-rest-hemidemisemi", true, Note::SixtyFourthNote, 1);
}

//----------------------------------------
// Accidentals
//----------------------------------------
void NotationView::slotNoAccidental()
{
    NoteInserter::setAccidental(Rosegarden::NoAccidental);
}

void NotationView::slotSharp()
{
    NoteInserter::setAccidental(Rosegarden::Sharp);
}

void NotationView::slotFlat()
{
    NoteInserter::setAccidental(Rosegarden::Flat);
}

void NotationView::slotNatural()
{
    NoteInserter::setAccidental(Rosegarden::Natural);
}

void NotationView::slotDoubleSharp()
{
    NoteInserter::setAccidental(Rosegarden::DoubleSharp);
}

void NotationView::slotDoubleFlat()
{
    NoteInserter::setAccidental(Rosegarden::DoubleFlat);
}


//----------------------------------------
// Clefs
//----------------------------------------
void NotationView::slotTrebleClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble"));
    setTool(new ClefInserter(Clef::Treble, *this));
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    setTool(new ClefInserter(Clef::Tenor, *this));
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    setTool(new ClefInserter(Clef::Alto, *this));
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    setTool(new ClefInserter(Clef::Bass, *this));
}


//----------------------------------------
// Time sigs.
//----------------------------------------

//----------------------------------------
// Edition Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotEraseSelected()\n";
    setTool(new NotationEraser(*this));
}

void NotationView::slotSelectSelected()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotSelectSelected()\n";
    setTool(new NotationSelector(*this));
}

//----------------------------------------------------------------------

void NotationView::itemClicked(int height, const QPoint &eventPos,
                               NotationElement* el)
{
    m_tool->handleMousePress(height, eventPos, el);
}

void NotationView::mouseMove(QMouseEvent *e)
{
    m_tool->handleMouseMove(e);
}

void NotationView::mouseRelease(QMouseEvent *e)
{
    m_tool->handleMouseRelease(e);
}



int
NotationView::findClosestStaff(double eventY)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	int top = m_staffs[i]->getStaffHeight() * i;
	int bottom = top + m_staffs[i]->getStaffHeight() + 15;
	if (eventY >= top && eventY < bottom) return i;
    }
    return -1;
}


NotationElementList::iterator
NotationView::findClosestNote(double eventX, Event *&timeSignature,
			      Event *&clef, Event *&key, int staffNo,
			      unsigned int proximityThreshold)
{
    double minDist = 10e9,
        prevDist = 10e9;

    NotationElementList *notes = m_staffs[staffNo]->getViewElementList();

    NotationElementList::iterator it, res;

    // TODO: this is grossly inefficient
    //
    for (it = notes->begin();
         it != notes->end(); ++it) 
{
        if (!(*it)->isNote() && !(*it)->isRest()) {
            if ((*it)->event()->isa(Clef::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found clef: type is "
                                     << (*it)->event()->get<String>(Clef::ClefPropertyName) << endl;
		clef = (*it)->event();
            } else if ((*it)->event()->isa(TimeSignature::EventType)) {
		kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found time sig " << endl;
		timeSignature = (*it)->event();
            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found key: type is "
                                     << (*it)->event()->get<String>(Rosegarden::Key::KeyPropertyName) << endl;
		key = (*it)->event();
            }
            continue;
        }

        double dist;
        
        if ( (*it)->getEffectiveX() >= eventX )
            dist = (*it)->getEffectiveX() - eventX;
        else
            dist = eventX - (*it)->getEffectiveX();

        if (dist < minDist) {
            kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : minDist was "
                                 << minDist << " now = " << dist << endl;
            minDist = dist;
            res = it;
        }
        
        // not sure about this
        if (dist > prevDist) break; // we can stop right now

        prevDist = dist;
    }

    if (minDist > proximityThreshold) {
        kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : element is too far away : "
                             << minDist << endl;
        return notes->end();
    }
        
    return res;
}


void NotationView::redoLayout(int staffNo, timeT startTime, timeT endTime)
{
    applyLayout(staffNo);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Track &track = m_staffs[i]->getTrack();

	NotationElementList *notes = m_staffs[i]->getViewElementList();
        NotationElementList::iterator starti = notes->begin();
        NotationElementList::iterator endi = notes->end();

        if (startTime > 0) {
            timeT barStartTime = track.findBarStartTime(startTime);
            starti = notes->findTime(barStartTime);
        }

        if (endTime >= 0) {
            timeT barEndTime = track.findBarEndTime(endTime);
            endi = notes->findTime(barEndTime);
        }

	m_staffs[i]->showElements(starti, endi,
				  (staffNo >= 0 && (int)i != staffNo));
	showBars(i);
    }
}


//!!! Some of this should be in NotationStaff (at the very least it
//should be able to report how much width and height it needs based on
//its own bar line positions which it's calculated by querying the
//notationhlayout)

void NotationView::readjustCanvasSize()
{
    double totalHeight = 0.0, totalWidth = m_hlayout->getTotalWidth();

    kdDebug(KDEBUG_AREA) << "NotationView::readjustCanvasWidth() : totalWidth = "
                         << totalWidth << endl;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        double xleft = 0, xright = totalWidth;

        NotationStaff &staff = *m_staffs[i];
        unsigned int barCount = m_hlayout->getBarLineCount(staff);
        
        for (unsigned int j = 0; j < barCount; ++j) {
            if (m_hlayout->isBarLineVisible(staff, j)) {
                xleft = m_hlayout->getBarLineX(staff, j);
                break;
            }
        }
        
        if (barCount > 0) {
            xright = m_hlayout->getBarLineX(staff, barCount - 1);
        }

        kdDebug(KDEBUG_AREA) << "Setting staff lines to " << xleft <<
            " -> " << xright << endl;
        staff.setLines(xleft, xright);

        totalHeight += staff.getStaffHeight() + 15;
    }

    if (canvas()->width()  < totalWidth ||
        canvas()->height() < totalHeight) {

        kdDebug(KDEBUG_AREA) << "NotationView::readjustCanvasWidth() to "
                             << totalWidth << endl;

        canvas()->resize(int(totalWidth) + 50, int(totalHeight) + 50);
    }
}

void
NotationView::hoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(noteName);
}

void
NotationView::hoveredOverAbsoluteTimeChange(unsigned int time)
{
    m_hoveredOverAbsoluteTime->setText(QString("Time : %1").arg(time));
}

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////
NotationTool::NotationTool(NotationView& view)
    : m_parentView(view)
{
}

NotationTool::~NotationTool()
{
}

void NotationTool::handleMouseMove(QMouseEvent*)
{
}

void NotationTool::handleMouseRelease(QMouseEvent*)
{
}

//------------------------------

NoteInserter::NoteInserter(Rosegarden::Note::Type type,
                           unsigned int dots,
                           NotationView& view)
    : NotationTool(view),
      m_noteType(type),
      m_noteDots(dots)
{
}
    
void    
NoteInserter::handleMousePress(int height, const QPoint &eventPos,
                               NotationElement*)
{
    if (height == StaffLine::NoHeight) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    int staffNo = m_parentView.findClosestStaff(eventPos.y());
    if (staffNo < 0) return;

    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote(eventPos.x(), tsig, clef, key, staffNo);

    //!!! Could be nicer! Likewise the other inserters.

    if (closestNote ==
	m_parentView.getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }


    kdDebug(KDEBUG_AREA) << "NoteInserter::handleMousePress() : accidental = "
                         << m_accidental << endl;

    int pitch = Rosegarden::NotationDisplayPitch(height, m_accidental).
        getPerformancePitch(clef ? Clef(*clef) : Clef::DefaultClef,
                            key ? Rosegarden::Key(*key) :
                            Rosegarden::Key::DefaultKey);

    // We are going to modify the document so mark it as such
    //
    m_parentView.getDocument()->setModified();

    Note note(m_noteType, m_noteDots);
    TrackNotationHelper nt
	(m_parentView.getStaff(staffNo)->getTrack());

    timeT time = (*closestNote)->getAbsoluteTime();
    timeT endTime = time + note.getDuration(); //???
    doInsert(nt, time, note, pitch, m_accidental);

    m_parentView.redoLayout(staffNo, time, endTime);
}

void NoteInserter::doInsert(TrackNotationHelper& nt,
                            Rosegarden::timeT absTime,
                            const Note& note, int pitch,
                            Accidental accidental)
{
    nt.insertNote(absTime, note, pitch, accidental);
}

void NoteInserter::setAccidental(Rosegarden::Accidental accidental)
{
    m_accidental = accidental;
}

Rosegarden::Accidental NoteInserter::m_accidental = NoAccidental;

//------------------------------

RestInserter::RestInserter(Rosegarden::Note::Type type,
                           unsigned int dots, NotationView& view)
    : NoteInserter(type, dots, view)
{
}

void RestInserter::doInsert(TrackNotationHelper& nt,
                            Rosegarden::timeT absTime,
                            const Note& note, int,
                            Accidental)
{
    nt.insertRest(absTime, note);
}

//------------------------------

ClefInserter::ClefInserter(std::string clefType, NotationView& view)
    : NotationTool(view),
      m_clef(clefType)
{
}
    
void ClefInserter::handleMousePress(int /*height*/, const QPoint &eventPos,
                                    NotationElement*)
{
    Event *tsig = 0, *clef = 0, *key = 0;

    int staffNo = m_parentView.findClosestStaff(eventPos.y());
    if (staffNo < 0) return;

    NotationElementList::iterator closestNote =
        m_parentView.findClosestNote
	(eventPos.x(), tsig, clef, key, staffNo, 100);

    if (closestNote ==
	m_parentView.getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();
    TrackNotationHelper nt
	(m_parentView.getStaff(staffNo)->getTrack());
    nt.insertClef(time, m_clef);

    m_parentView.redoLayout(staffNo, time, time + 1);
}


//------------------------------

NotationEraser::NotationEraser(NotationView& view)
    : NotationTool(view)
{
}

void NotationEraser::handleMousePress(int, const QPoint&,
                                      NotationElement* element)
{
    bool needLayout = false;
    if (!element) return;

    // discover (slowly) which staff contains element
    int staffNo = -1;
    for (int i = 0; i < m_parentView.getStaffCount(); ++i) {

	NotationElementList *nel =
	    m_parentView.getStaff(i)->getViewElementList(); 

	if (nel->findSingle(element) != nel->end()) {
	    staffNo = i;
	    break;
	}
    }
    if (staffNo == -1) return;

    TrackNotationHelper nt
	(m_parentView.getStaff(staffNo)->getTrack());

    timeT absTime = 0;

    if (element->isNote()) {

        absTime = element->getAbsoluteTime();
	nt.deleteNote(element->event());
	needLayout = true;

    } else if (element->isRest()) {

        absTime = element->getAbsoluteTime();
	nt.deleteRest(element->event());
	needLayout = true;

    } else {
        // we don't know what it is
        KMessageBox::sorry(0, "Not Implemented Yet");

    }
    
    if (needLayout)
        m_parentView.redoLayout(staffNo, absTime, absTime);
}

//------------------------------

NotationSelector::NotationSelector(NotationView& view)
    : NotationTool(view),
      m_selectionRect(new QCanvasRectangle(m_parentView.canvas())),
      m_updateRect(false)
{
    m_selectionRect->hide();
    m_selectionRect->setPen(Qt::red);
}

NotationSelector::~NotationSelector()
{
    delete m_selectionRect;
    m_parentView.canvas()->update();
}

void NotationSelector::handleMousePress(int, const QPoint& p,
                                        NotationElement* element)
{
    m_selectionRect->setX(p.x());
    m_selectionRect->setY(p.y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;
}

void NotationSelector::handleMouseMove(QMouseEvent* e)
{
    if (!m_updateRect) return;

    int w = e->x() - m_selectionRect->x();
    int h = e->y() - m_selectionRect->y();

    m_selectionRect->setSize(w,h);

    m_parentView.canvas()->update();
}

void NotationSelector::handleMouseRelease(QMouseEvent*)
{
    m_updateRect = false;
}
