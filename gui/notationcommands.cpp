/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "SegmentNotationHelper.h"
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
using Rosegarden::String;
using Rosegarden::Accidental;
using Rosegarden::Accidentals::NoAccidental;
using Rosegarden::Indication;
using Rosegarden::NotationDisplayPitch;

using std::string;
using std::cerr;
using std::endl;


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
NoteInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i =
        helper.insertNote(getBeginTime(), m_note, m_pitch, m_accidental);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


RestInsertionCommand::RestInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note) :
    NoteInsertionCommand(segment, time, endTime, note, 0, NoAccidental)
{
    KCommand::setName("Insert Rest");
}

RestInsertionCommand::~RestInsertionCommand()
{
    // nothing
}

void
RestInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

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
    return getSegment().getEndTime();
}

void
ClefInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Segment::iterator i = helper.insertClef(getBeginTime(), m_clef);
    if (i != helper.segment().end()) m_lastInsertedEvent = *i;
}


KeyInsertionCommand::KeyInsertionCommand(Segment &segment, timeT time,
					 Rosegarden::Key key,
					 bool convert,
					 bool transpose) :
    BasicCommand(name(&key), segment, time,
		 ((convert || transpose) ? segment.getEndTime() : time + 1)),
    m_key(key),
    m_lastInsertedEvent(0),
    m_convert(convert),
    m_transpose(transpose)
{
    // nothing
}

KeyInsertionCommand::~KeyInsertionCommand()
{
    // nothing
}

void
KeyInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    Rosegarden::Clef clef;
    Rosegarden::Key oldKey;

    if (m_convert || m_transpose) {
	helper.getClefAndKeyAt(getBeginTime(), clef, oldKey);
    }

    Segment::iterator i = helper.insertKey(getBeginTime(), m_key);

    if (i != helper.segment().end()) {

	m_lastInsertedEvent = *i;
	if (!m_convert && !m_transpose) return;

	while (++i != helper.segment().end()) {

	    //!!! what if we get two keys at the same time...?
	    if ((*i)->isa(Rosegarden::Key::EventType)) break;

	    if ((*i)->isa(Rosegarden::Clef::EventType)) {
		clef = Rosegarden::Clef(**i);
		continue;
	    }

	    if ((*i)->isa(Rosegarden::Note::EventType) &&
		(*i)->has(Rosegarden::BaseProperties::PITCH)) {

		long pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
		
		if (m_convert) {
		    (*i)->set<Int>(Rosegarden::BaseProperties::PITCH,
				   m_key.convertFrom(pitch, oldKey));
		} else {
		    (*i)->set<Int>(Rosegarden::BaseProperties::PITCH,
				   m_key.transposeFrom(pitch, oldKey));
		}

		(*i)->unset(Rosegarden::BaseProperties::ACCIDENTAL);
	    }
	}
    }
}


EraseCommand::EraseCommand(Segment &segment,
			   Event *event,
			   bool collapseRest) :
    BasicCommand(makeName(event->getType()).c_str(), segment,
		 event->getAbsoluteTime(),
		 event->getAbsoluteTime() + event->getDuration(),
		 true),
    m_collapseRest(collapseRest),
    m_event(event),
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
EraseCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    string eventType = m_event->getType();

    if (eventType == Note::EventType) {

	helper.deleteNote(m_event, m_collapseRest);
	return;

    } else if (eventType == Note::EventRestType) {

	helper.deleteRest(m_event);
	return;
	
    } else if (eventType == Rosegarden::Clef::EventType ||
	       eventType == Rosegarden::Key::EventType) {

	helper.segment().eraseSingle(m_event);
	m_relayoutEndTime = helper.segment().getEndTime();
	return;
	
    } else {
	    
	helper.segment().eraseSingle(m_event);
	return;
    }
}

void GroupMenuBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.makeBeamedGroup(getBeginTime(), getEndTime(),
                           Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
}

void GroupMenuAutoBeamCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.autoBeam(getBeginTime(), getEndTime(),
                    Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
}

void GroupMenuBreakCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    helper.unbeam(getBeginTime(), getEndTime());
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
GroupMenuAddIndicationCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    Indication indication(m_indicationType, m_indicationDuration);
    Event *e = indication.getAsEvent(getBeginTime());
    helper.segment().insert(e);
    m_lastInsertedEvent = e;
}

QString
GroupMenuAddIndicationCommand::name(std::string indicationType)
{
    std::string n = "Add &";
    n += (char)toupper(indicationType[0]);
    n += indicationType.substr(1);
    return QString(n.c_str());
}


void TransformsMenuNormalizeRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.normalizeRests(getBeginTime(), getEndTime());
}

void TransformsMenuCollapseRestsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    helper.collapseRestsAggressively(getBeginTime(), getEndTime());
}

void
TransformsMenuCollapseNotesCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());

    EventSelection::eventcontainer::iterator i;
    timeT endTime = getEndTime();
    
    // We go in reverse order, because collapseNoteAggressively
    // may delete the event following the one it's passed, but
    // never deletes anything before it

    i = m_selection->getSegmentEvents().end();

    while (i-- != m_selection->getSegmentEvents().begin()) {

	helper.collapseNoteAggressively((*i), endTime);
	helper.makeNoteViable(helper.segment().findSingle(*i));
    }
}


void
TransformsMenuChangeStemsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->set<Rosegarden::Bool>(NotationProperties::STEM_UP, m_up);
	}
    }
}


void
TransformsMenuRestoreStemsCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    (*i)->unset(NotationProperties::STEM_UP);
	}
    }
}

//!!! bleah -- merge these three classes 

void
TransformsMenuTransposeCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
	    pitch += m_semitones;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(Rosegarden::BaseProperties::PITCH, pitch); 
	    (*i)->unset(Rosegarden::BaseProperties::ACCIDENTAL);
	}
    }
}

void
TransformsMenuTransposeOneStepCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    int offset = m_up ? 1 : -1;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
	    pitch += offset;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(Rosegarden::BaseProperties::PITCH, pitch); 
	    (*i)->unset(Rosegarden::BaseProperties::ACCIDENTAL);
	}
    }
}

void
TransformsMenuTransposeOctaveCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    int offset = m_up ? 12 : -12;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {

	if ((*i)->isa(Note::EventType)) {
	    long pitch = (*i)->get<Int>(Rosegarden::BaseProperties::PITCH);
	    pitch += offset;
	    if (pitch < 0) pitch = 0;
	    if (pitch > 127) pitch = 127;
	    (*i)->set<Int>(Rosegarden::BaseProperties::PITCH, pitch); 
	    (*i)->unset(Rosegarden::BaseProperties::ACCIDENTAL);
	}
    }
}


QString
TransformsMenuAddMarkCommand::name(Rosegarden::Mark markType)
{
    std::string m = markType;

    // Gosh, lots of collisions
    if (markType == Rosegarden::Marks::Sforzando) m = "S&forzando";
    else if (markType == Rosegarden::Marks::Rinforzando) m = "R&inforzando";
    else if (markType == Rosegarden::Marks::Tenuto) m = "T&enuto";
    else if (markType == Rosegarden::Marks::Trill) m = "Tri&ll";
    else m = std::string("&") + (char)toupper(m[0]) + m.substr(1);

    m = std::string("Add ") + m;
    return QString(m.c_str());
}

void
TransformsMenuAddMarkCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(Rosegarden::BaseProperties::MARK_COUNT, n);
	(*i)->set<Int>(Rosegarden::BaseProperties::MARK_COUNT, n + 1);
	(*i)->set<String>(Rosegarden::BaseProperties::getMarkPropertyName(n),
			  m_mark);
    }
}

void
TransformsMenuAddTextMarkCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(Rosegarden::BaseProperties::MARK_COUNT, n);
	(*i)->set<Int>(Rosegarden::BaseProperties::MARK_COUNT, n + 1);
	(*i)->set<String>(Rosegarden::BaseProperties::getMarkPropertyName(n),
			  Rosegarden::Marks::getTextMark(m_text));
    }
}

void
TransformsMenuRemoveMarksCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    EventSelection::eventcontainer::iterator i;

    for (i  = m_selection->getSegmentEvents().begin();
	 i != m_selection->getSegmentEvents().end(); ++i) {
	
	long n = 0;
	(*i)->get<Int>(Rosegarden::BaseProperties::MARK_COUNT, n);
	(*i)->unset(Rosegarden::BaseProperties::MARK_COUNT);
	
	for (int j = 0; j < n; ++j) {
	    (*i)->unset(Rosegarden::BaseProperties::getMarkPropertyName(j));
	}
    }
}

