/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "AudioSegmentInsertCommand.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/GUIPalette.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"


namespace Rosegarden
{

AudioSegmentInsertCommand::AudioSegmentInsertCommand(
    RosegardenGUIDoc *doc,
    TrackId track,
    timeT startTime,
    AudioFileId audioFileId,
    const RealTime &audioStartTime,
    const RealTime &audioEndTime):
        KNamedCommand(i18n("Create Segment")),
        m_composition(&(doc->getComposition())),
        m_studio(&(doc->getStudio())),
        m_audioFileManager(&(doc->getAudioFileManager())),
        m_segment(0),
        m_track(track),
        m_startTime(startTime),
        m_audioFileId(audioFileId),
        m_audioStartTime(audioStartTime),
        m_audioEndTime(audioEndTime),
        m_detached(false)
{}

AudioSegmentInsertCommand::~AudioSegmentInsertCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

void
AudioSegmentInsertCommand::execute()
{
    if (!m_segment) {
        // Create and insert Segment
        //
        m_segment = new Segment(Segment::Audio);
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
        m_segment->setAudioStartTime(m_audioStartTime);
        m_segment->setAudioEndTime(m_audioEndTime);
        m_segment->setAudioFileId(m_audioFileId);

        // Set color for audio segment (DMM)
        //
        m_segment->setColourIndex(GUIPalette::AudioDefaultIndex);

        // Calculate end time
        //
        RealTime startTime =
            m_composition->getElapsedRealTime(m_startTime);

        RealTime endTime =
            startTime + m_audioEndTime - m_audioStartTime;

        timeT endTimeT = m_composition->getElapsedTimeForRealTime(endTime);

        RG_DEBUG << "AudioSegmentInsertCommand::execute : start timeT "
        << m_startTime << ", startTime " << startTime << ", audioStartTime " << m_audioStartTime << ", audioEndTime " << m_audioEndTime << ", endTime " << endTime << ", end timeT " << endTimeT << endl;

        m_segment->setEndTime(endTimeT);

        // Label by audio file name
        //
        std::string label = "";

        AudioFile *aF =
            m_audioFileManager->getAudioFile(m_audioFileId);

        if (aF)
            label = qstrtostr(i18n("%1 (inserted)").arg
                              (strtoqstr(aF->getName())));
        else
            label = qstrtostr(i18n("unknown audio file"));

        m_segment->setLabel(label);

        m_composition->addSegment(m_segment);
    } else {
        m_composition->addSegment(m_segment);
    }

    m_detached = false;
}

void
AudioSegmentInsertCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

}
