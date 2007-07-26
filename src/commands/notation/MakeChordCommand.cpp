/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "MakeChordCommand.h"

#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include <qstring.h>


namespace Rosegarden
{

void
MakeChordCommand::registerCommand(CommandRegistry *r)
{
    r->registerCommand // <MakeChordCommand>
        (getGlobalName(), "group-chord", "", "make_chord",
         new SelectionCommandBuilder<MakeChordCommand>());
}

void
MakeChordCommand::modifySegment()
{
    // find all the notes in the selection, and bring them back to align
    // with the start of the selection, giving them the same duration as
    // the longest note among them

    std::vector<Event *> toErase;
    std::vector<Event *> toInsert;
    Segment &segment(m_selection->getSegment());

    for (EventSelection::eventcontainer::iterator i =
                m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        if ((*i)->isa(Note::EventType)) {
            toErase.push_back(*i);
            toInsert.push_back(new Event(**i, m_selection->getStartTime()));
        }
    }

    for (unsigned int j = 0; j < toErase.size(); ++j) {
        Segment::iterator jtr(segment.findSingle(toErase[j]));
        if (jtr != segment.end())
            segment.erase(jtr);
    }

    for (unsigned int j = 0; j < toInsert.size(); ++j) {
        segment.insert(toInsert[j]);
    }

    segment.normalizeRests(getStartTime(), getEndTime());

    //!!! should select all notes in chord now
}

}
