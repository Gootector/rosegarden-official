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

#include "notationcommands.h"
#include "eventselection.h"
#include "notationview.h"
#include "rosegardenguidoc.h"

#include "BaseProperties.h"
#include "notationproperties.h"

#include "rosedebug.h"
#include <iostream>
#include <cctype>

using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Int;
using Rosegarden::Accidental;
using Rosegarden::NoAccidental;
using Rosegarden::Indication;

using std::string;
using std::cerr;
using std::endl;


BasicCommand::BasicCommand(const QString &name, Segment &segment,
			   timeT begin, timeT end) :
    KCommand(name),
    m_segment(segment),
    m_savedEvents(begin),
    m_endTime(end)
{
    // nothing
}

BasicCommand::~BasicCommand()
{
    deleteSavedEvents();
}

void
BasicCommand::beginExecute()
{
    for (Segment::iterator i = m_segment.findTime
	     (m_savedEvents.getStartIndex());
	 i != m_segment.findTime(m_endTime); ++i) {
	m_savedEvents.insert(new Event(**i));
    }
}

void
BasicCommand::finishExecute()
{
    //!!! we could do this a lot more elegantly with a signal
    NotationView::redoLayout(&m_segment,
			     m_savedEvents.getStartIndex(),
			     getRelayoutEndTime());
}

void
BasicCommand::deleteSavedEvents()
{
    m_savedEvents.erase(m_savedEvents.begin(), m_savedEvents.end());
}

void
BasicCommand::execute()
{
    beginExecute();
    SegmentNotationHelper helper(m_segment);
    modifySegment(helper);
    finishExecute();
}

void
BasicCommand::unexecute()
{
    m_segment.erase(m_segment.findTime(m_savedEvents.getStartIndex()),
		    m_segment.findTime(m_endTime));

    for (Segment::iterator i = m_savedEvents.begin();
	 i != m_savedEvents.end(); ++i) {
	m_segment.insert(new Event(**i));
    }

    deleteSavedEvents();
    finishExecute();
}
    
BasicSelectionCommand::BasicSelectionCommand(const QString &name,
					     EventSelection &selection) :
    BasicCommand(name,
		 selection.getSegment(),
		 selection.getBeginTime(),
		 selection.getEndTime())
{
    // nothing else
}

BasicSelectionCommand::~BasicSelectionCommand()
{
    // nothing
}


NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental) :
    BasicCommand("Insert Note", segment, time, endTime),
    m_note(note),
    m_pitch(pitch),
    m_accidental(accidental),
    m_lastInsertedEvent(0)
{
    // nothing
}

NoteInsertionCommand::~NoteInsertionCommand()
{
    // nothing
}

void
NoteInsertionCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i =
        helper.insertNote(getBeginTime(), m_note, m_pitch, m_accidental);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


RestInsertionCommand::RestInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(segment, time, endTime, note, 0, NoAccidental)
{
    setName("Insert Rest");
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i =
        helper.insertRest(getBeginTime(), m_note);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


ClefInsertionCommand::ClefInsertionCommand(Segment &segment, timeT time,
					   Clef clef) :
    BasicCommand("Insert Clef", segment, time, time + 1),
    m_clef(clef),
    m_lastInsertedEvent(0)
{
    // nothing
}

ClefInsertionCommand::~ClefInsertionCommand()
{
    // nothing
}

timeT
ClefInsertionCommand::getRelayoutEndTime()
{
    // Inserting a clef can change the y-coord of every subsequent note
    return getSegment().getEndIndex();
}

void
ClefInsertionCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i = helper.insertClef(getBeginTime(), m_clef);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


EraseCommand::EraseCommand(Segment &segment,
			   timeT time,
			   string eventType,
			   int pitch,
			   bool collapseRest) :
    BasicCommand(makeName(eventType).c_str(), segment, time, time + 1),
    m_eventType(eventType),
    m_pitch(pitch),
    m_collapseRest(collapseRest),
    m_relayoutEndTime(getEndTime())
{
    // nothing
}

EraseCommand::~EraseCommand()
{
    // nothing
}

string
EraseCommand::makeName(string e)
{
    string n = "Erase ";
    n += (char)toupper(e[0]);
    n += e.substr(1);
    return n;
}

timeT
EraseCommand::getRelayoutEndTime()
{
    return m_relayoutEndTime;
}

void
EraseCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i = helper.segment().findTime(getBeginTime());

    for ( ; i != helper.segment().end() &&
	      (*i)->getAbsoluteTime() == getBeginTime(); ++i) {

	if (!((*i)->isa(m_eventType))) continue;

	if (m_eventType == Note::EventType) {

	    if ((*i)->get<Int>(Rosegarden::BaseProperties::PITCH) == m_pitch) {
		helper.deleteNote((*i), m_collapseRest);
		return;
	    }

	} else if (m_eventType == Note::EventRestType) {

	    helper.deleteRest((*i));
	    return;

	} else if (m_eventType == Clef::EventType) {

	    helper.segment().erase(i);
	    m_relayoutEndTime = helper.segment().getEndIndex();
	    return;

	} else {
	    
	    helper.segment().erase(i);
	    return;
	}
    }
}



GroupMenuAddIndicationCommand::GroupMenuAddIndicationCommand(std::string indicationType, 
							     EventSelection &selection) :
    BasicCommand(name(indicationType),
		 selection.getSegment(), selection.getBeginTime(),
		 selection.getBeginTime() + 1),
    m_indicationType(indicationType),
    m_indicationDuration(selection.getEndTime() - selection.getBeginTime()),
    m_lastInsertedEvent(0)
{
    // nothing else
}

GroupMenuAddIndicationCommand::~GroupMenuAddIndicationCommand()
{
    // empty
}

void
GroupMenuAddIndicationCommand::modifySegment(SegmentNotationHelper &helper)
{
    Indication indication(m_indicationType, m_indicationDuration);
    Event *e = indication.getAsEvent(getBeginTime());
    helper.segment().insert(e);
    m_lastInsertedEvent = e;
}

QString
GroupMenuAddIndicationCommand::name(std::string indicationType)
{
    std::string n = "Add ";
    n += (char)toupper(indicationType[0]);
    n += indicationType.substr(1);
    return QString(n.c_str());
}


void
TransformsMenuChangeStemsCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i = helper.segment().findTime(getBeginTime());
    Segment::iterator j = helper.segment().findTime(getEndTime());

    while (i != j) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->set<Rosegarden::Bool>(NotationProperties::STEM_UP, m_up);
	}

	++i;
    }
}


void
TransformsMenuRestoreStemsCommand::modifySegment(SegmentNotationHelper &helper)
{
    Segment::iterator i = helper.segment().findTime(getBeginTime());
    Segment::iterator j = helper.segment().findTime(getEndTime());

    while (i != j) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->unset(NotationProperties::STEM_UP);
	}

	++i;
    }
}

