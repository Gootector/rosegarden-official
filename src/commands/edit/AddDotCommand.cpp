/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AddDotCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include <qstring.h>


namespace Rosegarden
{

void
AddDotCommand::modifySegment()
{
    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;

    EventSelection::eventcontainer::iterator i;
    timeT endTime = getEndTime();

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {

            Note note = Note::getNearestNote
                        ((*i)->getNotationDuration());
            int dots = note.getDots();
            if (++dots > 2)
                dots = 0;

            toErase.push_back(*i);

            Event *e;

            if (m_notationOnly) {
                e = new Event(**i,
                              (*i)->getAbsoluteTime(),
                              (*i)->getDuration(),
                              (*i)->getSubOrdering(),
                              (*i)->getNotationAbsoluteTime(),
                              Note(note.getNoteType(),
                                   dots).getDuration());

            } else {
                e = new Event(**i,
                              (*i)->getNotationAbsoluteTime(),
                              Note(note.getNoteType(),
                                   dots).getDuration());
            }

            if (e->getNotationAbsoluteTime() + e->getNotationDuration() > endTime) {
                endTime = e->getNotationAbsoluteTime() + e->getNotationDuration();
            }

            toInsert.push_back(e);
        }
    }

    for (std::vector<Event *>::iterator i = toErase.begin(); i != toErase.end(); ++i) {
        m_selection->getSegment().eraseSingle(*i);
    }

    for (std::vector<Event *>::iterator i = toInsert.begin(); i != toInsert.end(); ++i) {
        m_selection->getSegment().insert(*i);
        m_selection->addEvent(*i);
    }

    m_selection->getSegment().normalizeRests(getStartTime(), endTime);
}

}
