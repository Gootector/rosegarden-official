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

#include <qpopupmenu.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>

#include "Event.h"
#include "Segment.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"

#include "eventselection.h"
#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "notationview.h"
#include "staffline.h"
#include "qcanvassimplesprite.h"

#include "notationcommands.h"

#include "rosedebug.h"
#include "colours.h"

using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;

//////////////////////////////////////////////////////////////////////
//               Notation Tools
//////////////////////////////////////////////////////////////////////

NotationToolBox::NotationToolBox(NotationView *parent)
    : QObject(parent),
      m_parentView(parent),
      m_tools(17, false)
{
    //m_tools.setAutoDelete(true);
}


NotationTool* NotationToolBox::getTool(const QString& toolName)
{
    NotationTool* tool = m_tools[toolName];

    if (!tool) tool = createTool(toolName);
    
    return tool;
}


NotationTool* NotationToolBox::createTool(const QString& toolName)
{
    NotationTool* tool = 0;

    QString toolNamelc = toolName.lower();
    
    if (toolNamelc == NoteInserter::ToolName)

        tool = new NoteInserter(m_parentView);

    else if (toolNamelc == RestInserter::ToolName)

        tool = new RestInserter(m_parentView);

    else if (toolNamelc == ClefInserter::ToolName)

        tool = new ClefInserter(m_parentView);

    else if (toolNamelc == NotationEraser::ToolName)

        tool = new NotationEraser(m_parentView);

    else if (toolNamelc == NotationSelector::ToolName)

        tool = new NotationSelector(m_parentView);

    else {
        KMessageBox::error(0, QString("NotationToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
}


NotationTool::NotationTool(const QString& menuName, NotationView* view)
    : QObject(view),
      m_menuName(menuName),
      m_parentView(view),
      m_menu(0)
{
}

NotationTool::~NotationTool()
{
    kdDebug(KDEBUG_AREA) << "NotationTool::~NotationTool()\n";

//     delete m_menu;
//     m_parentView->factory()->removeClient(this);
//    m_instance = 0;
}

void NotationTool::ready()
{
    m_parentView->setCanvasCursor(Qt::arrowCursor);
    m_parentView->setPositionTracking(false);
}

void NotationTool::stow()
{
}

void NotationTool::createMenu(const QString& rcFileName)
{
    setXMLFile(rcFileName);
    m_parentView->factory()->addClient(this);

    QWidget* tmp =  m_parentView->factory()->container(m_menuName, this);

    m_menu = dynamic_cast<QPopupMenu*>(tmp);
}

void NotationTool::handleMousePress(int height, int staffNo,
                                    QMouseEvent* e,
                                    NotationElement* el)
{
    kdDebug(KDEBUG_AREA) << "NotationTool::handleMousePress : mouse button = "
                         << e->button() << endl;

    switch (e->button()) {

    case Qt::LeftButton:
        handleLeftButtonPress(height, staffNo, e, el);
        break;

    case Qt::RightButton:
        handleRightButtonPress(height, staffNo, e, el);
        break;

    case Qt::MidButton:
        handleMidButtonPress(height, staffNo, e, el);
        break;

    default:
        kdDebug(KDEBUG_AREA) << "NotationTool::handleMousePress : no button mouse press\n";
        break;
    }
}

void NotationTool::handleMidButtonPress(int, int,
                                        QMouseEvent*,
                                        NotationElement*)
{
}

void NotationTool::handleRightButtonPress(int, int,
                                          QMouseEvent*,
                                          NotationElement*)
{
    showMenu();
}

void NotationTool::handleMouseDblClick(int, int,
                                       QMouseEvent*,
                                       NotationElement*)
{
    // nothing
}


void NotationTool::handleMouseMove(QMouseEvent*)
{
}

void NotationTool::handleMouseRelease(QMouseEvent*)
{
}

void NotationTool::showMenu()
{
    if (m_menu)
        m_menu->exec(QCursor::pos());
    else
        kdDebug(KDEBUG_AREA) << "NotationTool::showMenu() : no menu to show\n";
}

void NotationTool::setParentView(NotationView* view)
{
    m_parentView = view;
}


//------------------------------


NoteInserter::NoteInserter(NotationView* view)
    : NotationTool("NoteInserter", view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_accidental(Accidentals::NoAccidental)
{
    QIconSet icon;

    for (unsigned int i = 0; i < 6; ++i) {

	icon = QIconSet
	    (m_parentView->getToolbarNotePixmapFactory()->
	     makeToolbarPixmap(m_actionsAccidental[i][3]));
        KRadioAction* noteAction = new KRadioAction(i18n(m_actionsAccidental[i][0]),
                                                    icon, 0, this,
                                                    m_actionsAccidental[i][1],
                                                    actionCollection(),
                                                    m_actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }

    icon = QIconSet
	(m_parentView->getToolbarNotePixmapFactory()->
	 makeToolbarPixmap("dotted-crotchet"));
    new KToggleAction(i18n("Dotted note"), icon, 0, this,
                      SLOT(slotToggleDot()), actionCollection(),
                      "toggle_dot");

    icon = QIconSet(m_parentView->getToolbarNotePixmapFactory()->
		    makeToolbarPixmap("select"));
    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    createMenu("noteinserter.rc");
}

NoteInserter::NoteInserter(const QString& menuName, NotationView* view)
    : NotationTool(menuName, view),
      m_noteType(Rosegarden::Note::Quaver),
      m_noteDots(0),
      m_accidental(Accidentals::NoAccidental)
{
    connect(m_parentView, SIGNAL(changeAccidental(Rosegarden::Accidental)),
            this,         SLOT(setAccidental(Rosegarden::Accidental)));

}

NoteInserter::~NoteInserter()
{
}

void NoteInserter::ready()
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
    m_parentView->setPositionTracking(true);
}



void    
NoteInserter::handleLeftButtonPress(int height, int staffNo,
				    QMouseEvent* e,
				    NotationElement*)
{
    if (height == NotationStaff::NoHeight || staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    //!!! wrong in page mode

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote(e->x(), e->y(), tsig, clef, key, staffNo);

    //!!! Could be nicer! Likewise the other inserters.

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    int pitch = Rosegarden::NotationDisplayPitch(height, m_accidental).
        getPerformancePitch(clef ? Clef(*clef) : Clef::DefaultClef,
                            key ? Rosegarden::Key(*key) :
                            Rosegarden::Key::DefaultKey);

    Note note(m_noteType, m_noteDots);
    Segment &segment = m_parentView->getStaff(staffNo)->getSegment();

    timeT time = (*closestNote)->getAbsoluteTime();
    timeT endTime = time + note.getDuration();

    Segment::iterator realEnd = segment.findTime(endTime);
    if (realEnd == segment.end() || ++realEnd == segment.end()) {
	endTime = segment.getEndIndex();
    } else {
	endTime = std::max(endTime, (*realEnd)->getAbsoluteTime());
    }

    Event *lastInsertedEvent =
	doAddCommand(segment, time, endTime, note, pitch, m_accidental);

    if (lastInsertedEvent) {
	m_parentView->setSingleSelectedEvent(staffNo, lastInsertedEvent);
    }
}

Event *
NoteInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int pitch, Accidental accidental)
{
    NoteInsertionCommand *command = 
	new NoteInsertionCommand
	 (segment, time, endTime, note, pitch, accidental);
    m_parentView->getCommandHistory()->addCommand(command);
    return command->getLastInsertedEvent();
} 

void NoteInserter::setNote(Rosegarden::Note::Type nt)
{
    m_noteType = nt;
}

void NoteInserter::setDots(unsigned int dots)
{
    m_noteDots = dots;
}

void NoteInserter::setAccidental(Rosegarden::Accidental accidental)
{
    m_accidental = accidental;
}

void NoteInserter::setAccidentalSync(Rosegarden::Accidental accidental)
{
    setAccidental(accidental);

    int i;
    for (i = 0; i < 6; ++i) {
	if (accidental == m_actionsAccidental[i][4]) break;
    }
    if (i == 6) return;

    // Get the parent view toolbar in sync - check the corresponding action
    KAction* action = actionCollection()->action(m_actionsAccidental[i][2]);

    KToggleAction* tAction = 0;
    
    if ((tAction = dynamic_cast<KToggleAction*>(action)))
        tAction->setChecked(true);
    else
        KMessageBox::error(0, "NoteInserter::setAccidental : couldn't find action");
}

void NoteInserter::slotNoAccidental()
{
    m_parentView->actionCollection()->action("no_accidental")->activate();
}

void NoteInserter::slotSharp()
{
    m_parentView->actionCollection()->action("sharp_accidental")->activate();
}

void NoteInserter::slotFlat()
{
    m_parentView->actionCollection()->action("flat_accidental")->activate();
}

void NoteInserter::slotNatural()
{
    m_parentView->actionCollection()->action("natural_accidental")->activate();
}

void NoteInserter::slotDoubleSharp()
{
    m_parentView->actionCollection()->action("double_sharp_accidental")->activate();
}

void NoteInserter::slotDoubleFlat()
{
    m_parentView->actionCollection()->action("double_flat_accidental")->activate();
}

void NoteInserter::slotToggleDot()
{
    // TODO : sync. this with the NotationView toolbars
    m_noteDots = (m_noteDots) ? 0 : 1;
}

void NoteInserter::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void NoteInserter::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}

const char* NoteInserter::m_actionsAccidental[][5] = 
    {
        { "No accidental",  "1slotNoAccidental()",  "no_accidental",
	      "accidental-none", Accidentals::NoAccidental.c_str() },
        { "Sharp",          "1slotSharp()",         "sharp_accidental",
	      "accidental-sharp", Accidentals::Sharp.c_str() },
        { "Flat",           "1slotFlat()",          "flat_accidental",
	      "accidental-flat", Accidentals::Flat.c_str() },
        { "Natural",        "1slotNatural()",       "natural_accidental",
	      "accidental-natural", Accidentals::Natural.c_str() },
        { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental",
	      "accidental-doublesharp", Accidentals::DoubleSharp.c_str() },
        { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",
	      "accidental-doubleflat", Accidentals::DoubleFlat.c_str() }
    };

//------------------------------

RestInserter::RestInserter(NotationView* view)
    : NoteInserter("RestInserter", view)
{
}

Event *
RestInserter::doAddCommand(Segment &segment, timeT time, timeT endTime,
			   const Note &note, int, Accidental)
{
    NoteInsertionCommand *command = 
	new RestInsertionCommand(segment, time, endTime, note);
    m_parentView->getCommandHistory()->addCommand(command);
    return command->getLastInsertedEvent();
} 


//------------------------------

ClefInserter::ClefInserter(NotationView* view)
    : NotationTool("ClefInserter", view),
      m_clef(Rosegarden::Clef::Treble)
{
}

void ClefInserter::ready()
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
    m_parentView->setPositionTracking(false);
}

void ClefInserter::setClef(std::string clefType)
{
    m_clef = clefType;
}

void ClefInserter::handleLeftButtonPress(int, int staffNo,
					 QMouseEvent* e,
					 NotationElement*)
{
    Event *tsig = 0, *clef = 0, *key = 0;

    if (staffNo < 0) return;

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote
        (e->x(), e->y(), tsig, clef, key, staffNo, 100);

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();

    ClefInsertionCommand *command = 
	new ClefInsertionCommand(m_parentView->getStaff(staffNo)->getSegment(),
				 time, m_clef);

    m_parentView->getCommandHistory()->addCommand(command);

    Event *event = command->getLastInsertedEvent();
    if (event) m_parentView->setSingleSelectedEvent(staffNo, event);
}


//------------------------------

NotationEraser::NotationEraser(NotationView* view)
    : NotationTool("NotationEraser", view),
      m_collapseRest(false)
{
    new KToggleAction(i18n("Collapse rests after erase"), 0, this,
                      SLOT(toggleRestCollapse()), actionCollection(),
                      "toggle_rest_collapse");

    createMenu("notationeraser.rc");
}

void NotationEraser::ready()
{
    m_parentView->setCanvasCursor(Qt::pointingHandCursor);
    m_parentView->setPositionTracking(false);
}

void NotationEraser::handleLeftButtonPress(int, int staffNo,
                                          QMouseEvent*,
                                          NotationElement* element)
{
    if (!element || staffNo < 0) return;

    long pitch = 0;
    (void)element->event()->get<Int>(Rosegarden::BaseProperties::PITCH, pitch);

    EraseCommand *command =
	new EraseCommand(m_parentView->getStaff(staffNo)->getSegment(),
			 element->getAbsoluteTime(),
			 element->event()->getType(),
			 (int)pitch, m_collapseRest);

    m_parentView->getCommandHistory()->addCommand(command);
}

void NotationEraser::toggleRestCollapse()
{
    m_collapseRest = !m_collapseRest;
}


//------------------------------

NotationSelector::NotationSelector(NotationView* view)
    : NotationTool("NotationSelector", view),
      m_selectionRect(0),
      m_updateRect(false),
      m_clickedStaff(-1),
      m_clickedElement(0)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this,         SLOT(hideSelection()));
}

void NotationSelector::handleLeftButtonPress(int, int staffNo,
                                            QMouseEvent* e,
                                            NotationElement *element)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMousePress" << endl;
    m_clickedStaff = staffNo;
    m_clickedElement = element;

    m_selectionRect->setX(e->x());
    m_selectionRect->setY(e->y());
    m_selectionRect->setSize(0,0);

    m_selectionRect->show();
    m_updateRect = true;

    //m_parentView->setCursorPosition(p.x());
}

void NotationSelector::handleMouseDblClick(int, int staffNo,
                                           QMouseEvent* e,
                                           NotationElement *element)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMouseDblClick" << endl;
    m_clickedStaff = staffNo;
    m_clickedElement = element;
    
    NotationStaff *staff = m_parentView->getStaff(staffNo);
    if (!staff) return;

    QRect rect = staff->getBarExtents(e->x(), e->y());

    m_selectionRect->setX(rect.x() + 1);
    m_selectionRect->setY(rect.y());
    m_selectionRect->setSize(rect.width() - 1, rect.height());

    m_selectionRect->show();
    m_updateRect = false;
    return;
}

void NotationSelector::handleMouseMove(QMouseEvent* e)
{
    if (!m_updateRect) return;

    int w = int(e->x() - m_selectionRect->x());
    int h = int(e->y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    m_selectionRect->setSize(w,h);

    m_parentView->canvas()->update();
}

void NotationSelector::handleMouseRelease(QMouseEvent*)
{
    kdDebug(KDEBUG_AREA) << "NotationSelector::handleMouseRelease" << endl;
    m_updateRect = false;
    setViewCurrentSelection();

    // If we didn't drag out a meaningful area, but _did_ click on
    // an individual event, then select just that event
    
    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) {

	m_selectionRect->hide();

	if (m_clickedElement != 0 &&
	    m_clickedStaff   >= 0) {

	    m_parentView->setSingleSelectedEvent
		(m_clickedStaff, m_clickedElement->event());
	}
    }
}

void NotationSelector::ready()
{
    m_selectionRect = new QCanvasRectangle(m_parentView->canvas());
    
    m_selectionRect->hide();
    m_selectionRect->setPen(RosegardenGUIColours::SelectionRectangle);

    m_parentView->setCanvasCursor(Qt::arrowCursor);
    m_parentView->setPositionTracking(false);
}

void NotationSelector::stow()
{
    delete m_selectionRect;
    m_selectionRect = 0;
    m_parentView->canvas()->update();
}


void NotationSelector::hideSelection()
{
    if (!m_selectionRect) return;
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_parentView->canvas()->update();
}


EventSelection* NotationSelector::getSelection()
{
    // If selection rect is not visible or too small,
    // return 0
    //
    if (!m_selectionRect->visible()) return 0;

    //    kdDebug(KDEBUG_AREA) << "Selection x,y: " << m_selectionRect->x() << ","
    //                         << m_selectionRect->y() << "; w,h: " << m_selectionRect->width() << "," << m_selectionRect->height() << endl;

    if (m_selectionRect->width()  > -3 &&
        m_selectionRect->width()  <  3 &&
        m_selectionRect->height() > -3 &&
        m_selectionRect->height() <  3) return 0;

    double middleY = m_selectionRect->y() + m_selectionRect->height()/2;
    NotationStaff *staff = m_parentView->getStaffAtY(middleY);
    if (!staff) return 0;
    Segment& originalSegment = staff->getSegment();
    
    EventSelection* selection = new EventSelection(originalSegment);

    QCanvasItemList itemList = m_selectionRect->collisions(true);
    QCanvasItemList::Iterator it;

    QRect rect = m_selectionRect->rect().normalize();

    for (it = itemList.begin(); it != itemList.end(); ++it) {

        QCanvasItem *item = *it;
        QCanvasNotationSprite *sprite = 0;
        
        if ((sprite = dynamic_cast<QCanvasNotationSprite*>(item))) {

            if (!rect.contains(int(item->x()), int(item->y()), true)) {

                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Skipping item not really in selection rect\n";
                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
                continue;
            } else {
                
                kdDebug(KDEBUG_AREA) << "NotationSelector::getSelection Item in rect : Rect: x,y: " << rect.x() << ","
                                     << rect.y() << "; w,h: " << rect.width()
                                     << "," << rect.height() << " / Item: x,y: "
                                     << item->x() << "," << item->y() << endl;
            }
            
            NotationElement &el = sprite->getNotationElement();

            selection->addEvent(el.event());

            //             kdDebug(KDEBUG_AREA) << "Selected event : \n";
            //             el.event()->dump(std::cerr);
        }
        
    }

    return (selection->getAddedEvents() > 0) ? selection : 0;
}

void NotationSelector::setViewCurrentSelection()
{
    EventSelection* selection = getSelection();
    m_parentView->setCurrentSelection(selection);
}

//------------------------------

const QString NoteInserter::ToolName     = "noteinserter";
const QString RestInserter::ToolName     = "restinserter";
const QString ClefInserter::ToolName     = "clefinserter";
const QString NotationEraser::ToolName   = "notationeraser";
const QString NotationSelector::ToolName = "notationselector";


//----------------------------------------------------------------------
//               Unused
//----------------------------------------------------------------------

NotationSelectionPaster::NotationSelectionPaster(EventSelection& es,
                                                 NotationView* view)
    : NotationTool("NotationPaster", view),
      m_selection(es)
{
    m_parentView->setCanvasCursor(Qt::crossCursor);
}

NotationSelectionPaster::~NotationSelectionPaster()
{
}

void NotationSelectionPaster::handleLeftButtonPress(int, int staffNo,
                                                   QMouseEvent* e,
                                                   NotationElement*)
{
    QPoint eventPos = e->pos();
    
    kdDebug(KDEBUG_AREA) << "NotationSelectionPaster::handleLeftButtonPress : staffNo = "
                         << staffNo
                         << "event pos : "
                         << eventPos.x() << "," << eventPos.y() << endl;

    if (staffNo < 0) return;

    Event *tsig = 0, *clef = 0, *key = 0;

    NotationElementList::iterator closestNote =
        m_parentView->findClosestNote(eventPos.x(),
				      eventPos.y(),
				      tsig, clef, key, staffNo);

    if (closestNote ==
        m_parentView->getStaff(staffNo)->getViewElementList()->end()) {
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();

    Segment& segment = m_parentView->getStaff(staffNo)->getSegment();

    if (m_selection.pasteToSegment(segment, time)) {

        m_parentView->redoLayout(&segment,
				 0,
				 time + m_selection.getTotalDuration() + 1);

    } else {
        
        m_parentView->slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
    
    //m_parentView->slotStatusHelpMsg(i18n("Ready."));

}

