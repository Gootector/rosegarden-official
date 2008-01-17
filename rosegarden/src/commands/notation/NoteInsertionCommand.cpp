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


#include "NoteInsertionCommand.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentMatrixHelper.h"
#include "base/SegmentNotationHelper.h"
#include "document/BasicCommand.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NoteStyleFactory.h"
#include "base/BaseProperties.h"


namespace Rosegarden
{

using namespace BaseProperties;

NoteInsertionCommand::NoteInsertionCommand(Segment &segment, timeT time,
                                           timeT endTime, Note note, int pitch,
                                           Accidental accidental,
                                           AutoBeamMode autoBeam,
                                           MatrixMode matrixType,
                                           GraceMode grace,
                                           float targetSubordering,
                                           NoteStyleName noteStyle) :
        BasicCommand(i18n("Insert Note"), segment,
                     getModificationStartTime(segment, time),
                     (autoBeam ? segment.getBarEndForTime(endTime) : endTime)),
        m_insertionTime(time),
        m_note(note),
        m_pitch(pitch),
        m_accidental(accidental),
        m_autoBeam(autoBeam == AutoBeamOn),
        m_matrixType(matrixType == MatrixModeOn),
        m_grace(grace == GraceModeOn),
        m_targetSubordering(targetSubordering),
        m_noteStyle(noteStyle),
        m_lastInsertedEvent(0)
{
    // nothing
}

NoteInsertionCommand::~NoteInsertionCommand()
{
    // nothing
}

timeT
NoteInsertionCommand::getModificationStartTime(Segment &segment,
        timeT time)
{
    // We may be splitting a rest to insert this note, so we'll have
    // to record the change from the start of that rest rather than
    // just the start of the note

    timeT barTime = segment.getBarStartForTime(time);
    Segment::iterator i = segment.findNearestTime(time);

    if (i != segment.end() &&
            (*i)->getAbsoluteTime() < time &&
            (*i)->getAbsoluteTime() + (*i)->getDuration() > time &&
            (*i)->isa(Note::EventRestType)) {
        return std::min(barTime, (*i)->getAbsoluteTime());
    }

    return barTime;
}

void
NoteInsertionCommand::modifySegment()
{
    Segment &segment(getSegment());
    SegmentNotationHelper helper(segment);
    Segment::iterator i, j;

    // insert via a model event, so as to apply the note style

    // subordering is always negative for these insertions; round it down
    int actualSubordering = lrintf(floorf(m_targetSubordering + 0.01));
    if (m_grace && actualSubordering >= 0) actualSubordering = -1;

    // this is true if the subordering is "more or less" an integer,
    // as opposed to something like -0.5
    bool suborderingExact = (actualSubordering != 
                             (lrintf(floorf(m_targetSubordering - 0.01))));

    std::cerr << "actualSubordering = " << actualSubordering
              << " suborderingExact = " << suborderingExact << std::endl;

    Event *e = new Event
               (Note::EventType,
                m_insertionTime,
                m_grace ? 0 : m_note.getDuration(),
                m_grace ? (actualSubordering == 0 ? -1 : actualSubordering) : 0,
                m_insertionTime,
                m_note.getDuration());

    e->set<Int>(PITCH, m_pitch);
    e->set<Int>(VELOCITY, 100);

    if (m_accidental != Accidentals::NoAccidental) {
        e->set<String>(ACCIDENTAL, m_accidental);
    }

    if (m_noteStyle != NoteStyleFactory::DefaultStyle) {
        e->set<String>(NotationProperties::NOTE_STYLE, m_noteStyle);
    }

    if (m_grace) {

        if (!suborderingExact) {

            // Adjust suborderings of any existing grace notes, if there
            // is at least one with the same subordering and
            // suborderingExact is not set

            segment.getTimeSlice(m_insertionTime, i, j);
            bool collision = false;
            for (Segment::iterator k = i; k != j; ++k) {
                if ((*k)->getSubOrdering() == actualSubordering) {
                    collision = true;
                    break;
                }
            }
            
            if (collision) {
                std::vector<Event *> toInsert, toErase;
                for (Segment::iterator k = i; k != j; ++k) {
                    if ((*k)->isa(Note::EventType) &&
                        (*k)->getSubOrdering() <= actualSubordering) {
                        toErase.push_back(*k);
                        toInsert.push_back
                            (new Event(**k,
                                       (*k)->getAbsoluteTime(),
                                       (*k)->getDuration(),
                                       (*k)->getSubOrdering() - 1,
                                       (*k)->getNotationAbsoluteTime(),
                                       (*k)->getNotationDuration()));
                    }
                }
                for (std::vector<Event *>::iterator k = toErase.begin();
                     k != toErase.end(); ++k) segment.eraseSingle(*k);
                for (std::vector<Event *>::iterator k = toInsert.begin();
                     k != toInsert.end(); ++k) segment.insert(*k);
            }
        }

        e->set<Bool>(IS_GRACE_NOTE, true);
        i = segment.insert(e);

        Segment::iterator k;
        segment.getTimeSlice(m_insertionTime, j, k);
        Segment::iterator bg0 = segment.end(), bg1 = segment.end();
        while (j != k) {
            std::cerr << "testing for truthiness: time " << (*j)->getAbsoluteTime() << ", subordering " << (*j)->getSubOrdering() << std::endl;
            if ((*j)->isa(Note::EventType) &&
                (*j)->getSubOrdering() < 0 &&
                (*j)->has(IS_GRACE_NOTE) &&
                (*j)->get<Bool>(IS_GRACE_NOTE)) {
                std::cerr << "truthiful" << std::endl;
                if (bg0 == segment.end()) bg0 = j;
                bg1 = j;
            }
            ++j;
        }
        if (bg0 != segment.end() && bg1 != bg0) {
            helper.makeBeamedGroupExact(bg0, bg1, GROUP_TYPE_BEAMED);
        }
            
    } else {

        // If we're attempting to insert at the same time and pitch as
        // an existing note, then we remove the existing note first
        // (so as to change its duration, if the durations differ)
        segment.getTimeSlice(m_insertionTime, i, j);
        while (i != j) {
            if ((*i)->isa(Note::EventType)) {
                long pitch;
                if ((*i)->get<Int>(PITCH, pitch) && pitch == m_pitch) {
                    helper.deleteNote(*i);
                    break;
                }
            }
            ++i;
        }
        
        if (m_matrixType) {
            i = SegmentMatrixHelper(segment).insertNote(e);
        } else {
            i = helper.insertNote(e);
            // e is just a model for SegmentNotationHelper::insertNote
            delete e;
        }
    }

    if (i != segment.end()) m_lastInsertedEvent = *i;

    if (m_autoBeam) {

        // We auto-beam the bar if it contains no beamed groups
        // after the insertion point and if it contains no tupled
        // groups at all.

        timeT barStartTime = segment.getBarStartForTime(m_insertionTime);
        timeT barEndTime = segment.getBarEndForTime(m_insertionTime);

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() < barEndTime;
                ++j) {
            if ((*j)->has(BEAMED_GROUP_ID))
                return ;
        }

        for (Segment::iterator j = i;
                j != segment.end() && (*j)->getAbsoluteTime() >= barStartTime;
                --j) {
            if ((*j)->has(BEAMED_GROUP_TUPLET_BASE))
                return ;
            if (j == segment.begin())
                break;
        }

        helper.autoBeam(m_insertionTime, m_insertionTime, GROUP_TYPE_BEAMED);
    }
}

}
