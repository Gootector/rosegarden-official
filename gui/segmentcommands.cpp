// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include "segmentcommands.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "notationcommands.h"
#include "notationstrings.h"
#include "NotationTypes.h"
#include "SegmentNotationHelper.h"
#include "Property.h"
#include "BaseProperties.h"
#include "Composition.h"
#include "PeakFile.h"
#include "Clipboard.h"
#include "Sets.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::TrackId;
using Rosegarden::AudioFileManager;


/**
 *
 *  *** IMPORTANT!
 *
 *  For a command that modifies segments to be able to undo and redo
 *  successfully with the simplest possible implementation, it is often
 *  necessary for it to be able to "remember" segment pointers and know
 *  for certain that they're still valid later.  For example, it's much
 *  easier to undo a segment copy command if it can just remember a
 *  pointer to the segment it created, and remove that from the segment
 *  in its unexecute method, rather than having to locate the correct
 *  segment to remove given some other information about it.
 *
 *  Almost all of the existing commands already rely on this, and it
 *  only works if _all_ segment commands do the same thing.
 *
 *  For this to work correctly, segment commands must:
 *
 *   1. Only create new segments the _first_ time execute() is run.
 *      Thereafter they should just reuse the same pointer they created
 *      the first time.
 *
 *   2. Never delete segments in unexecute(), just detach them and
 *      save them for later.  You can delete them in the destructor.
 *      (Never create segments in unexecute() either, though that's
 *      a less likely mistake.)
 *
 */


SegmentCommand::SegmentCommand(QString name, const std::vector<Rosegarden::Segment*>& segments)
    : KNamedCommand(name)
{
    m_segments.resize(segments.size());
    std::copy(segments.begin(), segments.end(), m_segments.begin());
}

// --------- Set Repeat on Segments --------
//
SegmentCommandRepeat::SegmentCommandRepeat(const std::vector<Rosegarden::Segment*>& segments,
                                           bool repeat)
    : SegmentCommand(i18n("Repeat Segments"), segments),
      m_repeatState(repeat)
{
}


void SegmentCommandRepeat::execute()
{
    segmentlist::iterator it;

    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setRepeating(m_repeatState);
}

void SegmentCommandRepeat::unexecute()
{
    segmentlist::iterator it;

    for (it = m_segments.begin(); it != m_segments.end(); it++)
        (*it)->setRepeating(!m_repeatState);
}

// SegmentCommandChangeTransposeValue::SegmentCommandChangeTransposeValue(const std::vector<Rosegarden::Segment*>& segments,
//                                                                      int transposeValue)
//     : SegmentCommand(i18n("Transpose Segments"), segments),
//       m_transposeValue(transposeValue)
// {
//     RG_DEBUG << "SegmentCommandChangeTransposeValue : nb segments : " << m_segments.size()
//                          << endl;

// }


// void SegmentCommandChangeTransposeValue::execute()
// {
//     segmentlist::iterator it;

//     for (it = m_segments.begin(); it != m_segments.end(); ++it) {
//         RG_DEBUG << "SegmentCommandChangeTransposeValue::execute : saving " << (*it)->getTranspose()
//                              << endl;

//         m_savedValues.push_back((*it)->getTranspose());
//         (*it)->setTranspose(m_transposeValue);
//     }
// }

// void SegmentCommandChangeTransposeValue::unexecute()
// {
//     segmentlist::iterator it = m_segments.begin();
//     std::vector<int>::iterator itV = m_savedValues.begin();
    
//     for (; it != m_segments.end() && itV != m_savedValues.end();
//          ++it, ++itV) {
//         RG_DEBUG << "SegmentCommandChangeTransposeValue::unexecute : restoring " << (*itV)
//                              << endl;
        
//         (*it)->setTranspose((*itV));
//     }
// }


// --------- Erase Segment --------
//
SegmentEraseCommand::SegmentEraseCommand(Segment *segment) :
    KNamedCommand(i18n("Erase Segment")),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_mgr(0),
    m_audioFileName(""),
    m_detached(false)
{
    // nothing else
}

SegmentEraseCommand::SegmentEraseCommand(Segment *segment,
					 AudioFileManager *mgr) :
    KNamedCommand(i18n("Erase Segment")),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_mgr(mgr),
    m_detached(false)
{
    // If this is an audio segment, we want to make a note of
    // its associated file name in case we need to undo and restore
    // the file.
    if (m_segment->getType() == Rosegarden::Segment::Audio) {
	unsigned int id = m_segment->getAudioFileId();
	Rosegarden::AudioFile *file = mgr->getAudioFile(id);
	if (file) m_audioFileName = file->getFilename();
    }
}

SegmentEraseCommand::~SegmentEraseCommand()
{
    // This is the only place in this command that the Segment can
    // safely be deleted, and then only if it is not in the
    // Composition (i.e. if we executed more recently than we
    // unexecuted).  Can't safely call through the m_segment pointer
    // here; someone else might have got to it first

    if (m_detached) {
	delete m_segment;
    }
}

void
SegmentEraseCommand::execute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

void
SegmentEraseCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_detached = false;

    if (m_segment->getType() == Rosegarden::Segment::Audio &&
	m_audioFileName != "" &&
	m_mgr) {
	int id = m_mgr->fileExists(m_audioFileName);

	RG_DEBUG << "SegmentEraseCommand::unexecute: file is " << m_audioFileName << endl;

	if (id == -1) id = (int)m_mgr->addFile(m_audioFileName);
	m_segment->setAudioFileId(id);
    }
}

// --------- Copy Segment ---------
//
SegmentQuickCopyCommand::SegmentQuickCopyCommand(Segment *segment):
    KNamedCommand(getGlobalName()),
    m_composition(segment->getComposition()),
    m_segmentToCopy(segment),
    m_segment(0),
    m_detached(false)
{
}

SegmentQuickCopyCommand::~SegmentQuickCopyCommand()
{
    if (m_detached) {
        delete m_segment;
    }
}

void
SegmentQuickCopyCommand::execute()
{
    if (!m_segment) {
	m_segment = new Segment(*m_segmentToCopy);
	m_segment->setLabel(qstrtostr(i18n("%1 (copied)").arg
				      (strtoqstr(m_segment->getLabel()))));
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
}

void
SegmentQuickCopyCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

//  ----------- SegmentRepeatToCopyCommand -------------
//
//
SegmentRepeatToCopyCommand::SegmentRepeatToCopyCommand(
        Rosegarden::Segment *segment):
    KNamedCommand(i18n("Turn Repeats into Copies")),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_detached(false)
{
}

SegmentRepeatToCopyCommand::~SegmentRepeatToCopyCommand()
{
    if (m_detached)
    {
        std::vector<Rosegarden::Segment*>::iterator it =
            m_newSegments.begin();

        for (; it != m_newSegments.end(); it++)
            delete (*it);
    }
}


void
SegmentRepeatToCopyCommand::execute()
{
    if (m_newSegments.size() == 0)
    {
        Rosegarden::timeT newStartTime = m_segment->getEndMarkerTime();
        Rosegarden::timeT newDuration =
            m_segment->getEndMarkerTime() - m_segment->getStartTime();
        Rosegarden::Segment *newSegment;
        Rosegarden::timeT repeatEndTime = m_segment->getRepeatEndTime();

        while(newStartTime + newDuration < repeatEndTime)
        {
            // Create new segment, transpose and turn off repeat
            //
            newSegment = new Segment(*m_segment);
            newSegment->setStartTime(newStartTime);
            newSegment->setRepeating(false);

            // Insert and store
            m_composition->addSegment(newSegment);
            m_newSegments.push_back(newSegment);

            // Move onto next
            newStartTime += newDuration;
        }

        // fill remaining partial segment
    }
    else
    {
        std::vector<Rosegarden::Segment*>::iterator it =
            m_newSegments.begin();

        for (; it != m_newSegments.end(); it++)
            m_composition->addSegment(*it);
    }
    m_segment->setRepeating(false);
    m_detached = false;
}


void
SegmentRepeatToCopyCommand::unexecute()
{
    std::vector<Rosegarden::Segment*>::iterator it =
        m_newSegments.begin();

    for (; it != m_newSegments.end(); it++)
        m_composition->detachSegment(*it);

    m_detached = true;
    m_segment->setRepeating(true);
}



SegmentSingleRepeatToCopyCommand::SegmentSingleRepeatToCopyCommand(
        Rosegarden::Segment *segment,
	Rosegarden::timeT time):
    KNamedCommand(i18n("Turn Single Repeat into Copy")),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_newSegment(0),
    m_time(time),
    m_detached(false)
{
}

SegmentSingleRepeatToCopyCommand::~SegmentSingleRepeatToCopyCommand()
{
    if (m_detached) delete m_newSegment;
}

void
SegmentSingleRepeatToCopyCommand::execute()
{
    if (!m_newSegment) {
	m_newSegment = new Segment(*m_segment);
	m_newSegment->setStartTime(m_time);
	m_newSegment->setRepeating(true);
    }

    m_composition->addSegment(m_newSegment);
    m_detached = false;
}

void
SegmentSingleRepeatToCopyCommand::unexecute()
{
    m_composition->detachSegment(m_newSegment);
    m_detached = true;
}


// -------- Audio Insert Segment --------
//

AudioSegmentInsertCommand::AudioSegmentInsertCommand(
        RosegardenGUIDoc *doc,
        TrackId track,
        timeT startTime,
        Rosegarden::AudioFileId audioFileId,
        const Rosegarden::RealTime &audioStartTime,
        const Rosegarden::RealTime &audioEndTime):
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
{
}

AudioSegmentInsertCommand::~AudioSegmentInsertCommand()
{
    if (m_detached) {
	delete m_segment;
    }
}

void
AudioSegmentInsertCommand::execute()
{
    if (!m_segment)
    {
        // Create and insert Segment
        //
        m_segment = new Segment(Rosegarden::Segment::Audio);
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
        m_segment->setAudioStartTime(m_audioStartTime);
        m_segment->setAudioEndTime(m_audioEndTime);
        m_segment->setAudioFileId(m_audioFileId);

        // Calculate end time
        //
        Rosegarden::RealTime startTime =
            m_composition->getElapsedRealTime(m_startTime);

        Rosegarden::RealTime endTime =
            startTime + m_audioEndTime - m_audioStartTime;

        timeT endTimeT = m_composition->getElapsedTimeForRealTime(endTime);

	RG_DEBUG << "AudioSegmentInsertCommand::execute : start timeT "
		 << m_startTime << ", startTime " << startTime << ", audioStartTime " << m_audioStartTime << ", audioEndTime " << m_audioEndTime << ", endTime " << endTime << ", end timeT " << endTimeT << endl;

	m_segment->setEndTime(endTimeT);

        // Label by audio file name
        //
        std::string label = "";

        Rosegarden::AudioFile *aF =
                m_audioFileManager->getAudioFile(m_audioFileId);

        if (aF)
            label = qstrtostr(i18n("%1 (inserted)").arg
			      (strtoqstr(aF->getName())));
        else
            label = qstrtostr(i18n("unknown audio file"));

        m_segment->setLabel(label);

	m_composition->addSegment(m_segment);
    }
    else
    {
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

// --------- Insert Segment --------
//
SegmentInsertCommand::SegmentInsertCommand(RosegardenGUIDoc *doc,
                                           TrackId track,
                                           timeT startTime,
                                           timeT endTime):
    KNamedCommand(i18n("Create Segment")),
    m_composition(&(doc->getComposition())),
    m_studio(&(doc->getStudio())),
    m_segment(0),
    m_track(track),
    m_startTime(startTime),
    m_endTime(endTime),
    m_detached(false)
{
}

SegmentInsertCommand::SegmentInsertCommand(Rosegarden::Composition *composition,
					   Rosegarden::Segment *segment,
                                           TrackId track):
    KNamedCommand(i18n("Create Segment")),
    m_composition(composition),
    m_studio(0),
    m_segment(segment),
    m_track(track),
    m_startTime(0),
    m_endTime(0),
    m_detached(false)
{
}

SegmentInsertCommand::~SegmentInsertCommand()
{
    if (m_detached) {
	delete m_segment;
    }
}

Rosegarden::Segment *
SegmentInsertCommand::getSegment() const
{
    return m_segment;
}

void
SegmentInsertCommand::execute()
{
    if (!m_segment)
    {
        // Create and insert Segment
        //
        m_segment = new Segment();
        m_segment->setTrack(m_track);
        m_segment->setStartTime(m_startTime);
	m_composition->addSegment(m_segment);
	m_segment->setEndTime(m_endTime);

        // Do our best to label the Segment with whatever is currently
        // showing against it.
        //
        Rosegarden::Track *track = m_composition->getTrackById(m_track);
        std::string label;

        if (track)
        {
            // try to get a reasonable Segment name by Instrument
            //
            label = m_studio->getSegmentName(track->getInstrument());

            // if not use the track label
            //
            if (label == "")
                label = track->getLabel();

            m_segment->setLabel(label);
        }
    }
    else
    {
	m_segment->setTrack(m_track);
        m_composition->addSegment(m_segment);
    }

    m_detached = false;
}

void
SegmentInsertCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}

// --------- Record Segment --------
//

SegmentRecordCommand::SegmentRecordCommand(Segment *s) :
    KNamedCommand(i18n("Record")),
    m_composition(s->getComposition()),
    m_segment(s),
    m_detached(false)
{
}

SegmentRecordCommand::~SegmentRecordCommand()
{
    if (m_detached) {
	delete m_segment;
    }
}

void
SegmentRecordCommand::execute()
{
    if (!m_segment->getComposition()) {
	m_composition->addSegment(m_segment);
    }

    m_detached = false;
}

void
SegmentRecordCommand::unexecute()
{
    m_composition->detachSegment(m_segment);
    m_detached = true;
}


// --------- Reconfigure Segments --------
//

SegmentReconfigureCommand::SegmentReconfigureCommand(QString name) :
    KNamedCommand(name)
{
}

SegmentReconfigureCommand::~SegmentReconfigureCommand()
{
}

void
SegmentReconfigureCommand::addSegment(Segment *segment,
				      timeT startTime,
				      timeT endMarkerTime,
				      TrackId track)
{
    SegmentRec record;
    record.segment = segment;
    record.startTime = startTime;
    record.endMarkerTime = endMarkerTime;
    record.track = track;
    m_records.push_back(record);
}

void
SegmentReconfigureCommand::addSegments(const SegmentRecSet &records)
{
    for (SegmentRecSet::const_iterator i = records.begin(); i != records.end(); ++i) {
	m_records.push_back(*i);
    }
}

void
SegmentReconfigureCommand::execute()
{
    swap();
}

void
SegmentReconfigureCommand::unexecute()
{
    swap();
}

void
SegmentReconfigureCommand::swap()
{
    for (SegmentRecSet::iterator i = m_records.begin();
	 i != m_records.end(); ++i) {

	// set the segment's values from the record, but set the
	// previous values back in to the record for use in the
	// next iteration of the execute/unexecute cycle.

	// #1083496: look up both of the "old" values before we set
	// anything, as setting the start time is likely to change the
	// end marker time.

	timeT prevStartTime = i->segment->getStartTime();
	timeT prevEndMarkerTime = i->segment->getEndMarkerTime();

	if (i->segment->getStartTime() != i->startTime) {
	    i->segment->setStartTime(i->startTime);
	}

	if (i->segment->getEndMarkerTime() != i->endMarkerTime) {
	    i->segment->setEndMarkerTime(i->endMarkerTime);
	}

	i->startTime = prevStartTime;
	i->endMarkerTime = prevEndMarkerTime;

	TrackId currentTrack = i->segment->getTrack();

	if (currentTrack != i->track) {
	    i->segment->setTrack(i->track);
	    i->track = currentTrack;
	}
    }
}


// SegmentResizeFromStartCommand
//
//

SegmentResizeFromStartCommand::SegmentResizeFromStartCommand(Segment *s,
							     timeT time) :
    BasicCommand(i18n("Resize Segment"), *s,
		 std::min(time, s->getStartTime()),
		 std::max(time, s->getStartTime())),
    m_segment(s),
    m_oldStartTime(s->getStartTime()),
    m_newStartTime(time)
{
    // nothing else 
}

SegmentResizeFromStartCommand::~SegmentResizeFromStartCommand()
{
    // nothing
}

void
SegmentResizeFromStartCommand::modifySegment()
{
    if (m_newStartTime < m_oldStartTime) {
	m_segment->fillWithRests(m_newStartTime, m_oldStartTime);
    } else {

	for (Segment::iterator i = m_segment->begin();
	     m_segment->isBeforeEndMarker(i); ) {

	    Segment::iterator j = i;
	    ++j;

	    if ((*i)->getAbsoluteTime() >= m_newStartTime) break;

	    if ((*i)->getAbsoluteTime() + (*i)->getDuration() <= m_newStartTime) {
		m_segment->erase(i);
	    } else {
		Event *e = new Event
		    (**i, m_newStartTime,
		     (*i)->getAbsoluteTime() + (*i)->getDuration() - m_newStartTime);
		m_segment->erase(i);
		m_segment->insert(e);
	    }

	    i = j;
	}
    }
}


// --------- Audio Split Segment -----------
//
//

AudioSegmentSplitCommand::AudioSegmentSplitCommand(Segment *segment,
					           timeT splitTime) :
    KNamedCommand(i18n("Split Audio Segment")),
    m_segment(segment),
    m_newSegment(0),
    m_splitTime(splitTime),
    m_previousEndMarkerTime(0),
    m_detached(false)
{
}

AudioSegmentSplitCommand::~AudioSegmentSplitCommand()
{
    if (m_detached) {
	delete m_newSegment;
    }
    delete m_previousEndMarkerTime;
}

void
AudioSegmentSplitCommand::execute()
{
    if (!m_newSegment) {

        m_newSegment = new Segment(Rosegarden::Segment::Audio);

        // Basics
        //
        m_newSegment->setAudioFileId(m_segment->getAudioFileId());
        m_newSegment->setTrack(m_segment->getTrack());

        // Get the RealTime split time
        //
        Rosegarden::RealTime splitDiff =
            m_segment->getComposition()->getRealTimeDifference(
                        m_segment->getStartTime(), m_splitTime);

        // Set audio start and end
        //
        m_newSegment->setAudioStartTime
                (m_segment->getAudioStartTime() + splitDiff);
        m_newSegment->setAudioEndTime(m_segment->getAudioEndTime());

        // Insert into composition before setting end time
        //
        m_segment->getComposition()->addSegment(m_newSegment);

        // Set start and end times
        //
        m_newSegment->setStartTime(m_splitTime);
        m_newSegment->setEndTime(m_segment->getEndTime());

        // Set original end time
        //
        m_previousEndAudioTime = m_segment->getAudioEndTime();
        m_segment->setAudioEndTime(m_newSegment->getAudioStartTime());

        // Set labels
        //
        m_segmentLabel = m_segment->getLabel();
	m_segment->setLabel(qstrtostr(i18n("%1 (split)").arg
				      (strtoqstr(m_segmentLabel))));
        m_newSegment->setLabel(m_segment->getLabel());
    }

    // Resize left hand Segment
    //
    const timeT *emt = m_segment->getRawEndMarkerTime();
    if (emt) {
	m_previousEndMarkerTime = new timeT(*emt);
    } else {
	m_previousEndMarkerTime = 0;
    }

    m_segment->setEndMarkerTime(m_splitTime);

    if (!m_newSegment->getComposition()) {
	m_segment->getComposition()->addSegment(m_newSegment);
    }

    m_detached = false;

}

void
AudioSegmentSplitCommand::unexecute()
{
    if (m_previousEndMarkerTime) {
	m_segment->setEndMarkerTime(*m_previousEndMarkerTime);
	delete m_previousEndMarkerTime;
	m_previousEndMarkerTime = 0;
    } else {
	m_segment->clearEndMarker();
    }

    m_segment->setLabel(m_segmentLabel);
    m_segment->setAudioEndTime(m_previousEndAudioTime);
    m_segment->getComposition()->detachSegment(m_newSegment);
    m_detached = true;
}


// --------- Split Segment --------
//

SegmentSplitCommand::SegmentSplitCommand(Segment *segment,
					 timeT splitTime) :
    KNamedCommand(i18n("Split Segment")),
    m_segment(segment),
    m_newSegment(0),
    m_splitTime(splitTime),
    m_previousEndMarkerTime(0),
    m_detached(false)
{
}

SegmentSplitCommand::~SegmentSplitCommand()
{
    if (m_detached) {
	delete m_newSegment;
    }
    delete m_previousEndMarkerTime;
}

void
SegmentSplitCommand::execute()
{
    if (!m_newSegment) {

	m_newSegment = new Segment;

	m_newSegment->setTrack(m_segment->getTrack());
	m_newSegment->setStartTime(m_splitTime);
	m_segment->getComposition()->addSegment(m_newSegment);

	Event *clefEvent = 0;
	Event *keyEvent = 0;

	// Copy the last occurrence of clef and key
	// from the left hand side of the split (nb. timesig events
	// don't appear in segments, only in composition)
	//
	Segment::iterator it = m_segment->findTime(m_splitTime);

	while (it != m_segment->begin()) {

	    --it;

	    if (!clefEvent && (*it)->isa(Rosegarden::Clef::EventType)) {
		clefEvent = new Event(**it, m_splitTime);
	    }

	    if (!keyEvent && (*it)->isa(Rosegarden::Key::EventType)) {
		keyEvent = new Event(**it, m_splitTime);
	    }

	    if (clefEvent && keyEvent) break;
	}

	// Insert relevant meta info if we've found some
	//
	if (clefEvent)
	    m_newSegment->insert(clefEvent);

	if (keyEvent)
	    m_newSegment->insert(keyEvent);

	// Copy through the Events
	//
	it = m_segment->findTime(m_splitTime);

	if (it != m_segment->end() && (*it)->getAbsoluteTime() > m_splitTime) {
	    m_newSegment->fillWithRests((*it)->getAbsoluteTime());
	}

	while (it != m_segment->end()) {
	    m_newSegment->insert(new Event(**it));
	    ++it;
	}
	m_newSegment->setEndTime(m_segment->getEndTime());
        m_newSegment->setEndMarkerTime(m_segment->getEndMarkerTime());

        // Set labels
        //
        m_segmentLabel = m_segment->getLabel();
	m_segment->setLabel(qstrtostr(i18n("%1 (split)").arg
				      (strtoqstr(m_segmentLabel))));
        m_newSegment->setLabel(m_segment->getLabel());
	m_newSegment->setColourIndex(m_segment->getColourIndex());
        m_newSegment->setTranspose(m_segment->getTranspose());
        m_newSegment->setDelay(m_segment->getDelay());
    }

    // Resize left hand Segment
    //
    const timeT *emt = m_segment->getRawEndMarkerTime();
    if (emt) {
	m_previousEndMarkerTime = new timeT(*emt);
    } else {
	m_previousEndMarkerTime = 0;
    }

    m_segment->setEndMarkerTime(m_splitTime);

    if (!m_newSegment->getComposition()) {
	m_segment->getComposition()->addSegment(m_newSegment);
    }

    m_detached = false;

}

void
SegmentSplitCommand::unexecute()
{
    if (m_previousEndMarkerTime) {
	m_segment->setEndMarkerTime(*m_previousEndMarkerTime);
	delete m_previousEndMarkerTime;
	m_previousEndMarkerTime = 0;
    } else {
	m_segment->clearEndMarker();
    }

    m_segment->setLabel(m_segmentLabel);
    m_segment->getComposition()->detachSegment(m_newSegment);
    m_detached = true;
}

struct AutoSplitPoint
{
    timeT time;
    timeT lastSoundTime;
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    AutoSplitPoint(timeT t, timeT lst, Rosegarden::Clef c, Rosegarden::Key k) :
	time(t), lastSoundTime(lst), clef(c), key(k) { }
};


// ----------- Audio Segment Auto-Split ------
//
AudioSegmentAutoSplitCommand::AudioSegmentAutoSplitCommand(
        RosegardenGUIDoc *doc,
        Segment *segment,
        int threshold) :
    KNamedCommand(getGlobalName()),
    m_segment(segment),
    m_composition(segment->getComposition()),
    m_audioFileManager(&(doc->getAudioFileManager())),
    m_detached(false),
    m_threshold(threshold)
{
}

AudioSegmentAutoSplitCommand::~AudioSegmentAutoSplitCommand()
{
    if (m_detached) {
	delete m_segment;
    } else {
	for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	    delete m_newSegments[i];
	}
    }
}

void
AudioSegmentAutoSplitCommand::execute()
{
    if (m_newSegments.size() == 0) {

	std::vector<AutoSplitPoint> splitPoints;

	if (m_segment->getType() != Rosegarden::Segment::Audio)
	    return;
	
        // Auto split the audio file - we ask for a minimum
        // result file size of 0.2secs - that's probably fair
        // enough.
        //
	std::vector<Rosegarden::SplitPointPair> rtSplitPoints =
	    m_audioFileManager->
                getSplitPoints(m_segment->getAudioFileId(),
                               m_segment->getAudioStartTime(),
                               m_segment->getAudioEndTime(),
                               m_threshold,
                               Rosegarden::RealTime(0, 200000000));
	
	std::vector<Rosegarden::SplitPointPair>::iterator it;
	Rosegarden::timeT absStartTime, absEndTime;
	
	char splitNumber[10];
	int splitCount = 0;
	
	for (it = rtSplitPoints.begin(); it != rtSplitPoints.end(); it++)
	{
	    absStartTime = m_segment->getStartTime() +
		m_composition->getElapsedTimeForRealTime(it->first);
	    
	    absEndTime = m_segment->getStartTime() +
		m_composition->getElapsedTimeForRealTime(it->second);
	    
	    Segment *newSegment = new Segment(*m_segment);
	    newSegment->setAudioStartTime(it->first);
	    newSegment->setAudioEndTime(it->second);
            newSegment->setAudioFileId(m_segment->getAudioFileId());
	    
	    // label
	    sprintf(splitNumber, "%d", splitCount++);
	    newSegment->
                setLabel(qstrtostr(i18n("%1 (autosplit %2)").arg
                        (strtoqstr(m_segment->getLabel())).arg
                        (splitNumber)));

	    newSegment->setColourIndex(m_segment->getColourIndex());
	    
	    newSegment->setStartTime(absStartTime);
	    newSegment->setEndTime(absEndTime);

	    RG_DEBUG << "AudioSegmentAutoSplitCommand::execute "
                     << "seg start = " << newSegment->getStartTime()
                     << ", seg end = "<< newSegment->getEndTime()
                     << ", audio start = " << newSegment->getAudioStartTime() 
                     << ", audio end = " << newSegment->getAudioEndTime()
                     << endl;
	    
	    m_newSegments.push_back(newSegment);
	}
    }
    
    for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	m_composition->addSegment(m_newSegments[i]);
    }
	    
    m_composition->detachSegment(m_segment);

    m_detached = true;
}

void
AudioSegmentAutoSplitCommand::unexecute()
{
    for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	m_composition->detachSegment(m_newSegments[i]);
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
}

// ----------- Segment Auto-Split ------------
//

SegmentAutoSplitCommand::SegmentAutoSplitCommand(Segment *segment) :
    KNamedCommand(getGlobalName()),
    m_segment(segment),
    m_composition(segment->getComposition()),
    m_detached(false)
{
}

SegmentAutoSplitCommand::~SegmentAutoSplitCommand()
{
    if (m_detached) {
	delete m_segment;
    } else {
	for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	    delete m_newSegments[i];
	}
    }
}

void
SegmentAutoSplitCommand::execute()
{
    if (m_newSegments.size() == 0) {

	std::vector<AutoSplitPoint> splitPoints;

	Rosegarden::Clef clef;
	Rosegarden::Key key;
	timeT segmentStart = m_segment->getStartTime();
	timeT lastSoundTime = segmentStart;
	timeT lastSplitTime = segmentStart - 1;
	
	for (Segment::iterator i = m_segment->begin();
	     m_segment->isBeforeEndMarker(i); ++i) {
	    
	    timeT myTime = (*i)->getAbsoluteTime();
	    int barNo = m_composition->getBarNumber(myTime);
	    
	    if ((*i)->isa(Rosegarden::Clef::EventType)) {
		clef = Rosegarden::Clef(**i);
	    } else if ((*i)->isa(Rosegarden::Key::EventType)) {
		key = Rosegarden::Key(**i);
	    }
	    
	    if (myTime <= lastSplitTime) continue;
	    
	    bool newTimeSig = false;
	    Rosegarden::TimeSignature tsig =
		m_composition->getTimeSignatureInBar(barNo, newTimeSig);
	    
	    if (newTimeSig) {
		
		// If there's a new time sig in this bar and we haven't
		// already made a split in this bar, make one
		
		if (splitPoints.size() == 0 ||
		    m_composition->getBarNumber
		    (splitPoints[splitPoints.size()-1].time) < barNo) {
		    
		    splitPoints.push_back(AutoSplitPoint(myTime, lastSoundTime,
							 clef, key));
		    lastSoundTime = lastSplitTime = myTime;
		}
		
	    } else if ((*i)->isa(Rosegarden::Note::EventRestType)) {
		
		// Otherwise never start a subsegment on a rest
		
		continue;
		
	    } else {
		
		// When we meet a non-rest event, start a new split
		// if an entire bar has passed since the last one
		
		int lastSoundBarNo = m_composition->getBarNumber(lastSoundTime);
		
		if (lastSoundBarNo < barNo - 1 ||
		    (lastSoundBarNo == barNo - 1 &&
		     m_composition->getBarStartForTime(lastSoundTime) ==
		     lastSoundTime &&
		     lastSoundTime > segmentStart)) {
		    
		    splitPoints.push_back
			(AutoSplitPoint
			 (m_composition->getBarStartForTime(myTime), lastSoundTime,
			  clef, key));
		    lastSplitTime = myTime;
		}
	    }
	    
	    lastSoundTime = std::max(lastSoundTime, myTime + (*i)->getDuration());
	}
	
	for (unsigned int split = 0; split <= splitPoints.size(); ++split) {
	    
	    Segment *newSegment = new Segment();
	    newSegment->setTrack(m_segment->getTrack());
	    newSegment->setLabel(qstrtostr(i18n("%1 (part)").arg
					   (strtoqstr(m_segment->getLabel()))));
	    newSegment->setColourIndex(m_segment->getColourIndex());
	    
	    timeT startTime = segmentStart;
	    if (split > 0) {
		
		RG_DEBUG << "Auto-split point " << split-1 << ": time "
			 << splitPoints[split-1].time << ", lastSoundTime "
			 << splitPoints[split-1].lastSoundTime << endl;
		
		startTime = splitPoints[split-1].time;
		newSegment->insert(splitPoints[split-1].clef.getAsEvent(startTime));
		newSegment->insert(splitPoints[split-1].key.getAsEvent(startTime));
	    }
	    
	    Segment::iterator i = m_segment->findTime(startTime);

	    // A segment has to contain at least one note to be a worthy
	    // candidate for adding back into the composition
	    bool haveNotes = false;
	    
	    while (m_segment->isBeforeEndMarker(i)) {
		timeT t = (*i)->getAbsoluteTime();
		if (split < splitPoints.size() &&
		    t >= splitPoints[split].lastSoundTime) break;
		if ((*i)->isa(Rosegarden::Note::EventType)) haveNotes = true;
		newSegment->insert(new Event(**i));
		++i;
	    }
	    
	    if (haveNotes) m_newSegments.push_back(newSegment);
	    else delete newSegment;
	}
    }
	    
    m_composition->detachSegment(m_segment);
    for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	m_composition->addSegment(m_newSegments[i]);
    }
    m_detached = true;
}

void
SegmentAutoSplitCommand::unexecute()
{
    for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	m_composition->detachSegment(m_newSegments[i]);
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
}


SegmentMergeCommand::SegmentMergeCommand(Rosegarden::SegmentSelection &
					 segments) :
    KNamedCommand(getGlobalName()),
    m_newSegment(0),
    m_detached(false) // true if the old segments are detached, not the new
{
    for (Rosegarden::SegmentSelection::iterator i = segments.begin();
	 i != segments.end(); ++i) {
	m_oldSegments.push_back(*i);
    }
    assert(m_oldSegments.size() > 0);
}

SegmentMergeCommand::~SegmentMergeCommand()
{
    if (m_detached) {
	for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
	    delete m_oldSegments[i];
	}
    } else {
	delete m_newSegment;
    }
}

void
SegmentMergeCommand::execute()
{
    Composition *composition = m_oldSegments[0]->getComposition();
    if (!composition) {
	std::cerr
	    << "SegmentMergeCommand::execute: ERROR: old segments are not in composition!"
	    << std::endl;
	return;
    }

    if (!m_newSegment) {

	m_newSegment = new Segment(*m_oldSegments[0]);

	// that duplicated segment 0; now do the rest

	for (unsigned int i = 1; i < m_oldSegments.size(); ++i) {

	    //!!! we really should normalize rests in any overlapping areas

	    if (m_oldSegments[i]->getStartTime() >
		m_newSegment->getEndMarkerTime()) {
		m_newSegment->setEndMarkerTime
		    (m_oldSegments[i]->getStartTime());
	    }

	    for (Segment::iterator si = m_oldSegments[i]->begin();
		 m_oldSegments[i]->isBeforeEndMarker(si); ++si) {

		// weed out duplicate clefs and keys

		if ((*si)->isa(Rosegarden::Clef::EventType)) {
		    try {
			Rosegarden::Clef newClef(**si);
			if (m_newSegment->getClefAtTime
			    ((*si)->getAbsoluteTime() + 1) == newClef) {
			    continue;
			}
		    } catch (...) { }
		}

		if ((*si)->isa(Rosegarden::Key::EventType)) {
		    try {
			Rosegarden::Key newKey(**si);
			if (m_newSegment->getKeyAtTime
			    ((*si)->getAbsoluteTime() + 1) == newKey) {
			    continue;
			}
		    } catch (...) { }
		}

		m_newSegment->insert(new Event(**si));
	    }

	    if (m_oldSegments[i]->getEndMarkerTime() >
		m_newSegment->getEndMarkerTime()) {
		m_newSegment->setEndMarkerTime
		    (m_oldSegments[i]->getEndMarkerTime());
	    }
	}
    }

    composition->addSegment(m_newSegment);

    for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
	composition->detachSegment(m_oldSegments[i]);
    }
    
    m_detached = true;
}

void
SegmentMergeCommand::unexecute()
{
    for (unsigned int i = 0; i < m_oldSegments.size(); ++i) {
	m_newSegment->getComposition()->addSegment(m_oldSegments[i]);
    }

    m_newSegment->getComposition()->detachSegment(m_newSegment);
    m_detached = false;
}


SegmentRescaleCommand::SegmentRescaleCommand(Segment *s,
					     int multiplier,
					     int divisor) :
    KNamedCommand(getGlobalName()),
    m_segment(s),
    m_newSegment(0),
    m_multiplier(multiplier),
    m_divisor(divisor),
    m_detached(false)
{
    // nothing
}

SegmentRescaleCommand::~SegmentRescaleCommand()
{
    if (m_detached) {
	delete m_segment;
    } else {
	delete m_newSegment;
    }
}

Rosegarden::timeT
SegmentRescaleCommand::rescale(Rosegarden::timeT t)
{
    // avoid overflows by using doubles
    double d = t;
    d *= m_multiplier;
    d /= m_divisor;
    d += 0.5;
    return (Rosegarden::timeT)d;
}

void
SegmentRescaleCommand::execute()
{
    timeT startTime = m_segment->getStartTime();

    if (!m_newSegment) {

	m_newSegment = new Segment();
	m_newSegment->setTrack(m_segment->getTrack());
	m_newSegment->setLabel(qstrtostr(i18n("%1 (rescaled)").arg
					 (strtoqstr(m_segment->getLabel()))));
	m_newSegment->setColourIndex(m_segment->getColourIndex());
	
	for (Segment::iterator i = m_segment->begin();
	     m_segment->isBeforeEndMarker(i); ++i) {
	    
	    if ((*i)->isa(Rosegarden::Note::EventRestType)) continue;
	    
	    timeT dt = (*i)->getAbsoluteTime() - startTime;
	    timeT duration = (*i)->getDuration();
	    
	    //!!! use doubles for this calculation where necessary

	    m_newSegment->insert
		(new Event(**i,
			   startTime + rescale(dt),
			   rescale(duration)));
	}
    }
    
    m_segment->getComposition()->addSegment(m_newSegment);
    m_segment->getComposition()->detachSegment(m_segment);
    m_newSegment->normalizeRests(m_newSegment->getStartTime(),
				 m_newSegment->getEndTime());
    
    m_newSegment->setEndMarkerTime
	(startTime + rescale(m_segment->getEndMarkerTime() - startTime));

    m_detached = true;
}

void
SegmentRescaleCommand::unexecute()
{
    m_newSegment->getComposition()->addSegment(m_segment);
    m_newSegment->getComposition()->detachSegment(m_newSegment);
    m_detached = false;
}


SegmentChangeQuantizationCommand::SegmentChangeQuantizationCommand(Rosegarden::timeT unit) :
    KNamedCommand(getGlobalName(unit)),
    m_unit(unit)
{
    // nothing
}

SegmentChangeQuantizationCommand::~SegmentChangeQuantizationCommand()
{
    // nothing
}

void
SegmentChangeQuantizationCommand::execute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

	SegmentRec &rec = m_records[i];

	if (m_unit) {

	    rec.oldUnit = rec.segment->getQuantizer()->getUnit();
	    rec.segment->setQuantizeLevel(m_unit);

	    rec.wasQuantized = rec.segment->hasQuantization();
	    rec.segment->setQuantization(true);

	} else {

	    rec.wasQuantized = rec.segment->hasQuantization();
	    rec.segment->setQuantization(false);
	}
    }
}

void
SegmentChangeQuantizationCommand::unexecute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

	SegmentRec &rec = m_records[i];

	if (m_unit) {

	    if (!rec.wasQuantized) rec.segment->setQuantization(false);
	    rec.segment->setQuantizeLevel(rec.oldUnit);

	} else {

	    if (rec.wasQuantized) rec.segment->setQuantization(true);
	}
    }
}

void
SegmentChangeQuantizationCommand::addSegment(Rosegarden::Segment *s)
{
    SegmentRec rec;
    rec.segment = s;
    rec.oldUnit = 0; // shouldn't matter what we initialise this to
    rec.wasQuantized = false; // shouldn't matter what we initialise this to
    m_records.push_back(rec);
}
    
QString
SegmentChangeQuantizationCommand::getGlobalName(Rosegarden::timeT unit)
{
    if (!unit) {
	return "Unquantize";
    } else {
	Rosegarden::timeT error = 0;
	QString label = NotationStrings::makeNoteMenuLabel(unit, true, error);
	return QString("Quantize to %1").arg(label);
    }
}



// --------- Add Time Signature --------
// 

AddTimeSignatureCommand::AddTimeSignatureCommand(Composition *composition,
						 timeT time,
						 Rosegarden::TimeSignature timeSig) :
    KNamedCommand(getGlobalName()),
    m_composition(composition),
    m_time(time),
    m_timeSignature(timeSig),
    m_oldTimeSignature(0)
{
    // nothing else
}

AddTimeSignatureCommand::~AddTimeSignatureCommand()
{
    if (m_oldTimeSignature) delete m_oldTimeSignature;
}

void
AddTimeSignatureCommand::execute()
{
    int oldIndex = m_composition->getTimeSignatureNumberAt(m_time);
    if (oldIndex >= 0) {
	std::pair<timeT, Rosegarden::TimeSignature> data =
	    m_composition->getTimeSignatureChange(oldIndex);
	if (data.first == m_time) {
	    m_oldTimeSignature = new Rosegarden::TimeSignature(data.second);
	}
    }

    m_timeSigIndex = m_composition->addTimeSignature(m_time, m_timeSignature);
}

void
AddTimeSignatureCommand::unexecute()
{
    m_composition->removeTimeSignature(m_timeSigIndex);
    if (m_oldTimeSignature) {
	m_composition->addTimeSignature(m_time, *m_oldTimeSignature);
    }
}


AddTimeSignatureAndNormalizeCommand::AddTimeSignatureAndNormalizeCommand
(Composition *composition, timeT time, Rosegarden::TimeSignature timeSig) :
    KMacroCommand(AddTimeSignatureCommand::getGlobalName())
{
    addCommand(new AddTimeSignatureCommand(composition, time, timeSig));

    // only up to the next time signature
    timeT nextTimeSigTime(composition->getDuration());

    int index = composition->getTimeSignatureNumberAt(time);
    if (composition->getTimeSignatureCount() > index + 1) {
	nextTimeSigTime = composition->getTimeSignatureChange(index + 1).first;
    }
	
    for (Composition::iterator i = composition->begin();
	 i != composition->end(); ++i) {

	timeT startTime = (*i)->getStartTime();
	timeT   endTime = (*i)->getEndTime();

	if (startTime >= nextTimeSigTime || endTime <= time) continue;

	// "Make Notes Viable" splits and ties notes at barlines, and
	// also does a rest normalize.  It's what we normally want
	// when adding a time signature.

	addCommand(new AdjustMenuMakeRegionViableCommand
		   (**i,
		    std::max(startTime, time),
		    std::min(endTime, nextTimeSigTime)));
    }
}

AddTimeSignatureAndNormalizeCommand::~AddTimeSignatureAndNormalizeCommand()
{
    // well, nothing really
}

void
RemoveTimeSignatureCommand::execute()
{
    if (m_timeSigIndex >= 0)
    {
        std::pair<timeT, Rosegarden::TimeSignature> data = 
            m_composition->getTimeSignatureChange(m_timeSigIndex);

        // store
        m_oldTime = data.first;
        m_oldTimeSignature = data.second;
    }

    // do we need to (re)store the index number?
    //
    m_composition->removeTimeSignature(m_timeSigIndex);

}

void
RemoveTimeSignatureCommand::unexecute()
{
    m_composition->addTimeSignature(m_oldTime, m_oldTimeSignature);
}

AddTempoChangeCommand::~AddTempoChangeCommand()
{
    // nothing here either
}

void
AddTempoChangeCommand::execute()
{
    int oldIndex = m_composition->getTempoChangeNumberAt(m_time);

    if (oldIndex >= 0)
    {
        std::pair<timeT, long> data = 
            m_composition->getRawTempoChange(oldIndex);

        if (data.first == m_time) m_oldTempo = data.second;
    }

    m_tempoChangeIndex = m_composition->addTempo(m_time, m_tempo);
}

void
AddTempoChangeCommand::unexecute()
{
    m_composition->removeTempoChange(m_tempoChangeIndex);

    if (m_oldTempo != 0) {
        m_composition->addRawTempo(m_time, m_oldTempo);
    }
}

void
RemoveTempoChangeCommand::execute()
{
    if (m_tempoChangeIndex >= 0)
    {
        std::pair<timeT, long> data = 
            m_composition->getRawTempoChange(m_tempoChangeIndex);

        // store
        m_oldTime = data.first;
        m_oldTempo = data.second;
    }

    // do we need to (re)store the index number?
    //
    m_composition->removeTempoChange(m_tempoChangeIndex);

}

void
RemoveTempoChangeCommand::unexecute()
{
    m_composition->addRawTempo(m_oldTime, m_oldTempo);
}

void
ModifyDefaultTempoCommand::execute()
{
    m_oldTempo = m_composition->getDefaultTempo();
    m_composition->setDefaultTempo(m_tempo);
}

void
ModifyDefaultTempoCommand::unexecute()
{
    m_composition->setDefaultTempo(m_oldTempo);
}



// --------- Add Tracks --------
//
AddTracksCommand::AddTracksCommand(Rosegarden::Composition *composition,
                                   unsigned int nbTracks,
                                   Rosegarden::InstrumentId id):
        KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_nbNewTracks(nbTracks),
        m_instrumentId(id),
        m_detached(false)

{

}

AddTracksCommand::~AddTracksCommand()
{
    if (m_detached)
    {
        for (unsigned int i = 0; i < m_newTracks.size(); ++i)
            delete m_newTracks[i];

        m_newTracks.clear();
    }
}


void AddTracksCommand::execute()
{
    // Re-attach tracks
    //
    if (m_detached)
    {
        for (unsigned int i = 0; i < m_newTracks.size(); i++)
            m_composition->addTrack(m_newTracks[i]);

        return;
    }

    int highPosition = 0;
    Rosegarden::Composition::trackiterator it =
        m_composition->getTracks().begin();

    for (; it != m_composition->getTracks().end(); ++it)
    {
        if ((*it).second->getPosition() > highPosition)
            highPosition = (*it).second->getPosition();
    }

    for (unsigned int i = 0; i < m_nbNewTracks; ++i)
    {
        Rosegarden::TrackId trackId = m_composition->getNewTrackId();
        Rosegarden::Track *track = new Rosegarden::Track(trackId);

        track->setPosition(highPosition + 1 + i);
        track->setInstrument(m_instrumentId);

        m_composition->addTrack(track);
    }
}

void AddTracksCommand::unexecute()
{
    unsigned int startTrack = m_composition->getNbTracks();
    unsigned int endTrack = startTrack - m_nbNewTracks;

    for (unsigned int i = startTrack; i > endTrack; --i) 
    {
        Rosegarden::Track *track = m_composition->getTrackByPosition(i - 1);

        if (track)
        {
            if (m_detached == false) m_newTracks.push_back(track);
            m_composition->detachTrack(track);
        }
        else
            RG_DEBUG<< "AddTracksCommand::unexecute - "
                     << "can't detach track at position " << i << endl;
    }

    m_detached = true;
}

// ------------ Delete Tracks -------------
//

DeleteTracksCommand::DeleteTracksCommand(Rosegarden::Composition *composition,
                    std::vector<Rosegarden::TrackId> tracks):
    KNamedCommand(getGlobalName()),
    m_composition(composition),
    m_tracks(tracks),
    m_detached(false)
{
}

DeleteTracksCommand::~DeleteTracksCommand()
{
    if (m_detached)
    {
        for (unsigned int i = 0; i < m_oldTracks.size(); ++i)
            delete m_oldTracks[i];

        for (unsigned int i = 0; i < m_oldSegments.size(); ++i)
            delete m_oldSegments[i];

        m_oldTracks.clear();
        m_oldSegments.clear();
    }
}


void DeleteTracksCommand::execute()
{
    Rosegarden::Track *track = 0;
    const Composition::segmentcontainer &segments =
        m_composition->getSegments();

    //cout << "DeleteTracksCommand::execute()" << endl;

    m_oldSegments.clear();
    m_oldTracks.clear();
    
    // Remap positions and track numbers
    //

    Rosegarden::Composition::trackiterator tit;
    Rosegarden::Composition::trackcontainer
                &tracks = m_composition->getTracks();

    for (unsigned int i = 0; i < m_tracks.size(); ++i)
    {
        // detach segments and store tracks somewhere
        track = m_composition->getTrackById(m_tracks[i]);

        if (track)
        {
            // detach all segments for that track
            //
            for (Composition::segmentcontainer::const_iterator
                    it = segments.begin();
                    it != segments.end(); ++it)
            {
                if ((*it)->getTrack() == m_tracks[i])
                {
                    m_oldSegments.push_back(*it);
                    m_composition->detachSegment(*it);
                }
            }

            // store old tracks
            m_oldTracks.push_back(track);
            if (m_composition->detachTrack(track) == false)
            {
                RG_DEBUG << "DeleteTracksCommand::execute - can't detach track" << endl;
            }
        }
    }

    std::vector<Rosegarden::Track*>::iterator otIt;
    for (otIt = m_oldTracks.begin(); otIt != m_oldTracks.end(); ++otIt)
    {
        for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        {
            if ((*tit).second->getPosition() > (*otIt)->getPosition())
            {
                // If the track we've removed was after the current
                // track then decrement the track position.
                //
                int newPosition = (*tit).second->getPosition() - 1;

                (*tit).second->setPosition(newPosition);
                
            }
        }
    }

    m_detached = true;
}

void DeleteTracksCommand::unexecute()
{
    // Add the tracks and the segments back in
    //

    // Remap positions and track numbers
    //
    Rosegarden::Composition::trackcontainer
                &tracks = m_composition->getTracks();
    Rosegarden::Composition::trackiterator tit;

    std::vector<Rosegarden::Track*>::iterator otIt;
    for (otIt = m_oldTracks.begin(); otIt != m_oldTracks.end(); ++otIt)
    {
        // From the back we shuffle out the tracks to allow the new
        // (old) track some space to come back in.
        //
        tit = tracks.end();
        while(true)
        {
            --tit;

            if ((*tit).second->getPosition() >= (*otIt)->getPosition())
            {
                // If the track we're adding has position after the
                // current track then increment position.
                //
                int newPosition = (*tit).second->getPosition() + 1;

                (*tit).second->setPosition(newPosition);
            }

            if (tit == tracks.begin()) break;
        }

        m_composition->addTrack(*otIt);
    }

    for (unsigned int i = 0; i < m_oldSegments.size(); ++i)
        m_composition->addSegment(m_oldSegments[i]);

    m_detached = false;
}

// ------------------ MoveTracksCommand ---------------------
//
MoveTracksCommand::MoveTracksCommand(Rosegarden::Composition *composition,
                                     Rosegarden::TrackId srcTrack,
                                     Rosegarden::TrackId destTrack):
    KNamedCommand(getGlobalName()),
    m_composition(composition),
    m_srcTrack(srcTrack),
    m_destTrack(destTrack)
{
}

MoveTracksCommand::~MoveTracksCommand()
{
}

void
MoveTracksCommand::execute()
{
    Rosegarden::Track *srcTrack = m_composition->getTrackById(m_srcTrack);
    Rosegarden::Track *destTrack = m_composition->getTrackById(m_destTrack);

    int srcPosition = srcTrack->getPosition();

    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
}

void
MoveTracksCommand::unexecute()
{
    Rosegarden::Track *srcTrack = m_composition->getTrackById(m_srcTrack);
    Rosegarden::Track *destTrack = m_composition->getTrackById(m_destTrack);

    int srcPosition = srcTrack->getPosition();

    srcTrack->setPosition(destTrack->getPosition());
    destTrack->setPosition(srcPosition);

    m_composition->updateRefreshStatuses();
}

// ------------------ RenameTrackCommand ---------------------
//
RenameTrackCommand::RenameTrackCommand(Rosegarden::Composition *composition,
				       Rosegarden::TrackId trackId,
				       std::string name) :
    KNamedCommand(getGlobalName()),
    m_composition(composition),
    m_track(trackId),
    m_newName(name)
{
    Rosegarden::Track *track = composition->getTrackById(m_track);
    if (!track) {
	RG_DEBUG << "Hey! No Track in RenameTrackCommand (track id " << track
		 << ")!" << endl;
	return;
    }
    m_oldName = track->getLabel();
}

RenameTrackCommand::~RenameTrackCommand()
{
}

void
RenameTrackCommand::execute()
{
    Rosegarden::Track *track = m_composition->getTrackById(m_track);
    if (!track) return;
    track->setLabel(m_newName);
}

void
RenameTrackCommand::unexecute()
{
    Rosegarden::Track *track = m_composition->getTrackById(m_track);
    if (!track) return;
    track->setLabel(m_oldName);
}
    
    

// ------------------ ChangeCompositionLengthCommand ------------------
//
ChangeCompositionLengthCommand::ChangeCompositionLengthCommand(
        Rosegarden::Composition *composition,
        Rosegarden::timeT startTime,
        Rosegarden::timeT endTime):
            KNamedCommand(getGlobalName()),
            m_composition(composition),
            m_startTime(startTime),
            m_endTime(endTime),
            m_oldStartTime(m_composition->getStartMarker()),
            m_oldEndTime(m_composition->getEndMarker())
{
}

ChangeCompositionLengthCommand::~ChangeCompositionLengthCommand()
{
}

void
ChangeCompositionLengthCommand::execute()
{
    m_composition->setStartMarker(m_startTime);
    m_composition->setEndMarker(m_endTime);
}

void
ChangeCompositionLengthCommand::unexecute()
{
    m_composition->setStartMarker(m_oldStartTime);
    m_composition->setEndMarker(m_oldEndTime);
}


SegmentSplitByPitchCommand::SegmentSplitByPitchCommand(Segment *segment,
						       int p, bool r, bool d,
						       ClefHandling c) :
    KNamedCommand(i18n("Split by Pitch")),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_newSegmentA(0),
    m_newSegmentB(0),
    m_splitPitch(p),
    m_ranging(r),
    m_dupNonNoteEvents(d),
    m_clefHandling(c),
    m_executed(false)
{
}

SegmentSplitByPitchCommand::~SegmentSplitByPitchCommand()
{
    if (m_executed) {
	delete m_segment;
    } else { 
	delete m_newSegmentA;
	delete m_newSegmentB;
    }
}

void
SegmentSplitByPitchCommand::execute()
{
    if (!m_newSegmentA) {

	m_newSegmentA = new Segment;
	m_newSegmentB = new Segment;

	m_newSegmentA->setTrack(m_segment->getTrack());
	m_newSegmentA->setStartTime(m_segment->getStartTime());
	
	m_newSegmentB->setTrack(m_segment->getTrack());
	m_newSegmentB->setStartTime(m_segment->getStartTime());
	
	int splitPitch(m_splitPitch);
	
	for (Segment::iterator i = m_segment->begin();
	     m_segment->isBeforeEndMarker(i); ++i) {
	    
	    if ((*i)->isa(Rosegarden::Note::EventRestType)) continue;
	    if ((*i)->isa(Rosegarden::Clef::EventType) &&
		m_clefHandling != LeaveClefs) continue;
	    
	    if ((*i)->isa(Rosegarden::Note::EventType)) {
		
		if (m_ranging) {
		    splitPitch = getSplitPitchAt(i, splitPitch);
		}
		
		if ((*i)->has(Rosegarden::BaseProperties::PITCH) &&
		    (*i)->get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH) <
		    splitPitch) {
		    if (m_newSegmentB->empty()) {
			m_newSegmentB->fillWithRests((*i)->getAbsoluteTime());
		    }
		    m_newSegmentB->insert(new Event(**i));
		} else {
		    if (m_newSegmentA->empty()) {
			m_newSegmentA->fillWithRests((*i)->getAbsoluteTime());
		    }
		    m_newSegmentA->insert(new Event(**i));
		}
		
	    } else {
		
		m_newSegmentA->insert(new Event(**i));
		
		if (m_dupNonNoteEvents) {
		    m_newSegmentB->insert(new Event(**i));
		}
	    }
	}

//!!!	m_newSegmentA->fillWithRests(m_segment->getEndMarkerTime());
//	m_newSegmentB->fillWithRests(m_segment->getEndMarkerTime());
	m_newSegmentA->normalizeRests(m_segment->getStartTime(),
				      m_segment->getEndMarkerTime());
	m_newSegmentB->normalizeRests(m_segment->getStartTime(),
				      m_segment->getEndMarkerTime());
    }

    m_composition->addSegment(m_newSegmentA);
    m_composition->addSegment(m_newSegmentB);
    
    Rosegarden::SegmentNotationHelper helperA(*m_newSegmentA);
    Rosegarden::SegmentNotationHelper helperB(*m_newSegmentB);
    
    if (m_clefHandling == RecalculateClefs) {
	
	m_newSegmentA->insert
	    (helperA.guessClef(m_newSegmentA->begin(),
			       m_newSegmentA->end()).getAsEvent
	     (m_newSegmentA->getStartTime()));
	
	m_newSegmentB->insert
	    (helperB.guessClef(m_newSegmentB->begin(),
			       m_newSegmentB->end()).getAsEvent
	     (m_newSegmentB->getStartTime()));
	
    } else if (m_clefHandling == UseTrebleAndBassClefs) {
	    
	m_newSegmentA->insert
	    (Rosegarden::Clef(Rosegarden::Clef::Treble).getAsEvent
	     (m_newSegmentA->getStartTime()));
	
	m_newSegmentB->insert
	    (Rosegarden::Clef(Rosegarden::Clef::Bass).getAsEvent
	     (m_newSegmentB->getStartTime()));
    }
    
//!!!    m_composition->getNotationQuantizer()->quantize(m_newSegmentA);
//    m_composition->getNotationQuantizer()->quantize(m_newSegmentB);
    helperA.autoBeam(m_newSegmentA->begin(), m_newSegmentA->end(),
		     Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    helperB.autoBeam(m_newSegmentB->begin(), m_newSegmentB->end(),
		     Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    
    std::string label = m_segment->getLabel();
    m_newSegmentA->setLabel(qstrtostr(i18n("%1 (upper)").arg
				      (strtoqstr(label))));
    m_newSegmentB->setLabel(qstrtostr(i18n("%1 (lower)").arg
				      (strtoqstr(label))));
    m_newSegmentA->setColourIndex(m_segment->getColourIndex());
    m_newSegmentB->setColourIndex(m_segment->getColourIndex());

    m_composition->detachSegment(m_segment);
    m_executed = true;
}
    
void
SegmentSplitByPitchCommand::unexecute()
{
    m_composition->addSegment(m_segment);
    m_composition->detachSegment(m_newSegmentA);
    m_composition->detachSegment(m_newSegmentB);
    m_executed = false;
}

int
SegmentSplitByPitchCommand::getSplitPitchAt(Segment::iterator i,
					    int lastSplitPitch)
{
    typedef std::set<int>::iterator PitchItr;
    std::set<int> pitches;

    // when this algorithm appears to be working ok, we should be
    // able to make it much quicker

    const Rosegarden::Quantizer *quantizer
	(m_segment->getComposition()->getNotationQuantizer());

    int myHighest, myLowest;
    int prevHighest = 0, prevLowest = 0;
    bool havePrev = false;

    Rosegarden::Chord c0(*m_segment, i, quantizer);
    std::vector<int> c0p(c0.getPitches());
    pitches.insert<std::vector<int>::iterator>(c0p.begin(), c0p.end());

    myLowest = c0p[0];
    myHighest = c0p[c0p.size()-1];
    
    Segment::iterator j(c0.getPreviousNote());
    if (j != m_segment->end()) {

	havePrev = true;

	Rosegarden::Chord c1(*m_segment, j, quantizer);
	std::vector<int> c1p(c1.getPitches());
	pitches.insert<std::vector<int>::iterator>(c1p.begin(), c1p.end());

	prevLowest = c1p[0];
	prevHighest = c1p[c1p.size()-1];
    }

    if (pitches.size() < 2) return lastSplitPitch;

    PitchItr pi = pitches.begin();
    int lowest(*pi);

    pi = pitches.end(); --pi;
    int highest(*pi);

    if ((pitches.size() == 2 || highest - lowest <= 18) &&
	  myHighest > lastSplitPitch &&
 	   myLowest < lastSplitPitch &&
	prevHighest > lastSplitPitch &&
	 prevLowest < lastSplitPitch) {

	if (havePrev) {
	    if ((myLowest > prevLowest && myHighest > prevHighest) ||
		(myLowest < prevLowest && myHighest < prevHighest)) {
		int avgDiff = ((myLowest - prevLowest) +
			       (myHighest - prevHighest)) / 2;
		if (avgDiff < -5) avgDiff = -5;
		if (avgDiff >  5) avgDiff =  5;
		return lastSplitPitch + avgDiff;
	    }
	}

	return lastSplitPitch;
    }

    int middle = (highest - lowest) / 2 + lowest;

    while (lastSplitPitch > middle && lastSplitPitch > m_splitPitch - 12) {
	if (lastSplitPitch - lowest < 12) return lastSplitPitch;
	if (lastSplitPitch <= m_splitPitch - 12) return lastSplitPitch;
	--lastSplitPitch;
    }

    while (lastSplitPitch < middle && lastSplitPitch < m_splitPitch + 12) {
	if (highest - lastSplitPitch < 12) return lastSplitPitch;
	if (lastSplitPitch >= m_splitPitch + 12) return lastSplitPitch;
	++lastSplitPitch;
    }

    return lastSplitPitch;
}


SegmentLabelCommand::SegmentLabelCommand(
        Rosegarden::SegmentSelection &segments,
        const QString &label):
    KNamedCommand(i18n("Label Segments")),
    m_newLabel(label)
{
    for (Rosegarden::SegmentSelection::iterator i = segments.begin();
	 i != segments.end(); ++i) m_segments.push_back(*i);
}

SegmentLabelCommand::~SegmentLabelCommand()
{
}

void
SegmentLabelCommand::execute()
{
    bool addLabels = false;
    if (m_labels.size() == 0) addLabels = true;

    for (unsigned int i = 0; i < m_segments.size(); ++i)
    {
        if (addLabels) m_labels.push_back(strtoqstr(m_segments[i]->getLabel()));

        m_segments[i]->setLabel(qstrtostr(m_newLabel));
    }
}

void
SegmentLabelCommand::unexecute()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i)
        m_segments[i]->setLabel(qstrtostr(m_labels[i]));
}


// Alter colour

SegmentColourCommand::SegmentColourCommand(
        Rosegarden::SegmentSelection &segments,
        const unsigned int index):
    KNamedCommand(i18n("Change Segment Color")),
    m_newColourIndex(index)
{
    for (Rosegarden::SegmentSelection::iterator i = segments.begin(); i != segments.end(); ++i) 
        m_segments.push_back(*i);
}

SegmentColourCommand::~SegmentColourCommand()
{
}

void
SegmentColourCommand::execute()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i)
    {
        m_oldColourIndexes.push_back(m_segments[i]->getColourIndex());
        m_segments[i]->setColourIndex(m_newColourIndex);
    }
}

void
SegmentColourCommand::unexecute()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i)
        m_segments[i]->setColourIndex(m_oldColourIndexes[i]);
}

// Alter ColourMap
SegmentColourMapCommand::SegmentColourMapCommand(
              RosegardenGUIDoc      *doc,
        const Rosegarden::ColourMap &map):
    KNamedCommand(i18n("Change Segment Color Map")),
    m_doc(doc),
    m_oldMap(m_doc->getComposition().getSegmentColourMap()),
    m_newMap(map)
{

}

SegmentColourMapCommand::~SegmentColourMapCommand()
{
}

void
SegmentColourMapCommand::execute()
{
    m_doc->getComposition().setSegmentColourMap(m_newMap);
    m_doc->slotDocColoursChanged();
}

void
SegmentColourMapCommand::unexecute()
{
    m_doc->getComposition().setSegmentColourMap(m_oldMap);
    m_doc->slotDocColoursChanged();
}


AddTriggerSegmentCommand::AddTriggerSegmentCommand(RosegardenGUIDoc *doc,
						   Rosegarden::timeT duration,
						   int basePitch,
						   int baseVelocity) :
    KNamedCommand(i18n("Add Triggered Segment")),
    m_composition(&doc->getComposition()),
    m_duration(duration),
    m_basePitch(basePitch),
    m_baseVelocity(baseVelocity),
    m_id(0),
    m_segment(0),
    m_detached(false)
{
    // nothing else
}

AddTriggerSegmentCommand::~AddTriggerSegmentCommand()
{
    if (m_detached) delete m_segment;
}

Rosegarden::TriggerSegmentId
AddTriggerSegmentCommand::getId() const
{
    return m_id;
}

void
AddTriggerSegmentCommand::execute()
{
    if (m_segment) {
	m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);
    } else {
	m_segment = new Rosegarden::Segment();
	m_segment->setEndMarkerTime(m_duration);
	Rosegarden::TriggerSegmentRec *rec = m_composition->addTriggerSegment
	    (m_segment, m_basePitch, m_baseVelocity);
	if (rec) m_id = rec->getId();
    }
    m_detached = false;
}

void
AddTriggerSegmentCommand::unexecute()
{
    if (m_segment) m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}


DeleteTriggerSegmentCommand::DeleteTriggerSegmentCommand(RosegardenGUIDoc *doc,
							 Rosegarden::TriggerSegmentId id) :
    KNamedCommand(i18n("Delete Triggered Segment")),
    m_composition(&doc->getComposition()),
    m_id(id),
    m_segment(0),
    m_basePitch(-1),
    m_baseVelocity(-1),
    m_detached(true)
{
    // nothing else
}

DeleteTriggerSegmentCommand::~DeleteTriggerSegmentCommand()
{
    if (m_detached) delete m_segment;
}

void
DeleteTriggerSegmentCommand::execute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    m_segment = rec->getSegment();
    m_basePitch = rec->getBasePitch();
    m_baseVelocity = rec->getBaseVelocity();
    m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}

void
DeleteTriggerSegmentCommand::unexecute()
{
    if (m_segment)
	m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);
    m_detached = false;
}


PasteToTriggerSegmentCommand::PasteToTriggerSegmentCommand(Rosegarden::Composition *composition,
							   Rosegarden::Clipboard *clipboard,
							   QString label,
							   int basePitch,
							   int baseVelocity) :
    KNamedCommand(i18n("Paste as New Triggered Segment")),
    m_composition(composition),
    m_clipboard(clipboard),
    m_label(label),
    m_basePitch(basePitch),
    m_baseVelocity(baseVelocity),
    m_segment(0),
    m_detached(false)
{
    // nothing else
}

PasteToTriggerSegmentCommand::~PasteToTriggerSegmentCommand()
{
    if (m_detached) delete m_segment;
}

void
PasteToTriggerSegmentCommand::execute()
{
    if (m_segment) {

	m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);

    } else {
	
	if (m_clipboard->isEmpty()) return;
	
	m_segment = new Rosegarden::Segment();
	
	timeT earliestStartTime = 0;
	timeT latestEndTime = 0;
	
	for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	     i != m_clipboard->end(); ++i) {

	    if (i == m_clipboard->begin() ||
		(*i)->getStartTime() < earliestStartTime) {
		earliestStartTime = (*i)->getStartTime();
	    }

	    if ((*i)->getEndMarkerTime() > latestEndTime)
		latestEndTime = (*i)->getEndMarkerTime();
	}

	for (Rosegarden::Clipboard::iterator i = m_clipboard->begin();
	     i != m_clipboard->end(); ++i) {
	    
	    for (Rosegarden::Segment::iterator si = (*i)->begin();
		 (*i)->isBeforeEndMarker(si); ++si) {
		if (!(*si)->isa(Rosegarden::Note::EventRestType)) {
		    m_segment->insert
			(new Event(**si, 
				   (*si)->getAbsoluteTime() - earliestStartTime));
		}
	    }
	}
	
	m_segment->setLabel(qstrtostr(m_label));

	Rosegarden::TriggerSegmentRec *rec = 
	    m_composition->addTriggerSegment(m_segment, m_basePitch, m_baseVelocity);
	if (rec) m_id = rec->getId();
    }

    m_composition->getTriggerSegmentRec(m_id)->updateReferences();
    
    m_detached = false;
}

void
PasteToTriggerSegmentCommand::unexecute()
{
    if (m_segment) m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}
    

SetTriggerSegmentBasePitchCommand::SetTriggerSegmentBasePitchCommand(Rosegarden::Composition *composition,
								     Rosegarden::TriggerSegmentId id,
								     int newPitch) :
    KNamedCommand(i18n("Set Base Pitch")),
    m_composition(composition),
    m_id(id),
    m_newPitch(newPitch),
    m_oldPitch(-1)
{
    // nothing
}

SetTriggerSegmentBasePitchCommand::~SetTriggerSegmentBasePitchCommand()
{
    // nothing
}

void
SetTriggerSegmentBasePitchCommand::execute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    if (m_oldPitch == -1) {
	m_oldPitch = rec->getBasePitch();
    }
    rec->setBasePitch(m_newPitch);
}

void
SetTriggerSegmentBasePitchCommand::unexecute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    rec->setBasePitch(m_oldPitch);
}


SetTriggerSegmentBaseVelocityCommand::SetTriggerSegmentBaseVelocityCommand(Rosegarden::Composition *composition,
								     Rosegarden::TriggerSegmentId id,
								     int newVelocity) :
    KNamedCommand(i18n("Set Base Velocity")),
    m_composition(composition),
    m_id(id),
    m_newVelocity(newVelocity),
    m_oldVelocity(-1)
{
    // nothing
}

SetTriggerSegmentBaseVelocityCommand::~SetTriggerSegmentBaseVelocityCommand()
{
    // nothing
}

void
SetTriggerSegmentBaseVelocityCommand::execute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    if (m_oldVelocity == -1) {
	m_oldVelocity = rec->getBaseVelocity();
    }
    rec->setBaseVelocity(m_newVelocity);
}

void
SetTriggerSegmentBaseVelocityCommand::unexecute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    rec->setBaseVelocity(m_oldVelocity);
}


SetTriggerSegmentDefaultTimeAdjustCommand::SetTriggerSegmentDefaultTimeAdjustCommand(Rosegarden::Composition *composition,
										     Rosegarden::TriggerSegmentId id,
										     std::string newDefaultTimeAdjust) :
    KNamedCommand(i18n("Set Default Time Adjust")),
    m_composition(composition),
    m_id(id),
    m_newDefaultTimeAdjust(newDefaultTimeAdjust),
    m_oldDefaultTimeAdjust("")
{
    // nothing
}

SetTriggerSegmentDefaultTimeAdjustCommand::~SetTriggerSegmentDefaultTimeAdjustCommand()
{
    // nothing
}

void
SetTriggerSegmentDefaultTimeAdjustCommand::execute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    if (m_oldDefaultTimeAdjust == "") {
	m_oldDefaultTimeAdjust = rec->getDefaultTimeAdjust();
    }
    rec->setDefaultTimeAdjust(m_newDefaultTimeAdjust);
}

void
SetTriggerSegmentDefaultTimeAdjustCommand::unexecute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    rec->setDefaultTimeAdjust(m_oldDefaultTimeAdjust);
}




SetTriggerSegmentDefaultRetuneCommand::SetTriggerSegmentDefaultRetuneCommand(Rosegarden::Composition *composition,
									     Rosegarden::TriggerSegmentId id,
									     bool newDefaultRetune) :
    KNamedCommand(i18n("Set Default Retune")),
    m_composition(composition),
    m_id(id),
    m_newDefaultRetune(newDefaultRetune),
    m_oldDefaultRetune(false),
    m_haveOldDefaultRetune(false)
{
    // nothing
}

SetTriggerSegmentDefaultRetuneCommand::~SetTriggerSegmentDefaultRetuneCommand()
{
    // nothing
}

void
SetTriggerSegmentDefaultRetuneCommand::execute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    if (!m_haveOldDefaultRetune) {
	m_oldDefaultRetune = rec->getDefaultRetune();
    }
    rec->setDefaultRetune(m_newDefaultRetune);
}

void
SetTriggerSegmentDefaultRetuneCommand::unexecute()
{
    Rosegarden::TriggerSegmentRec *rec = m_composition->getTriggerSegmentRec(m_id);
    if (!rec) return;
    rec->setDefaultRetune(m_oldDefaultRetune);
}


CreateTempoMapFromSegmentCommand::CreateTempoMapFromSegmentCommand(Segment *groove) :
    KNamedCommand(i18n("Set Tempos from Beat Segment")),
    m_composition(groove->getComposition())
{
    initialise(groove);
}

CreateTempoMapFromSegmentCommand::~CreateTempoMapFromSegmentCommand()
{
    // nothing
}

void
CreateTempoMapFromSegmentCommand::execute()
{
    for (TempoMap::iterator i = m_oldTempi.begin(); i != m_oldTempi.end(); ++i) {
	int n = m_composition->getTempoChangeNumberAt(i->first);
	if (n < m_composition->getTempoChangeCount()) {
	    m_composition->removeTempoChange(n);
	}
    }

    for (TempoMap::iterator i = m_newTempi.begin(); i != m_newTempi.end(); ++i) {
	m_composition->addRawTempo(i->first, i->second);
    }
}

void
CreateTempoMapFromSegmentCommand::unexecute()
{
    for (TempoMap::iterator i = m_newTempi.begin(); i != m_newTempi.end(); ++i) {
	int n = m_composition->getTempoChangeNumberAt(i->first);
	if (n < m_composition->getTempoChangeCount()) {
	    m_composition->removeTempoChange(n);
	}
    }

    for (TempoMap::iterator i = m_oldTempi.begin(); i != m_oldTempi.end(); ++i) {
	m_composition->addRawTempo(i->first, i->second);
    }
}

void
CreateTempoMapFromSegmentCommand::initialise(Rosegarden::Segment *s)
{
    m_oldTempi.clear();
    m_newTempi.clear();

    //!!! need an additional option: per-chord, per-beat, per-bar.
    // Let's work per-beat for the moment.  Even for this, we should
    // probably use TimeSignature.getDivisions()

    std::vector<Rosegarden::timeT> beatTimeTs;
    std::vector<Rosegarden::RealTime> beatRealTimes;

    int startBar = m_composition->getBarNumber(s->getStartTime());
    int barNo = startBar;
    int beat = 0;

    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {
	if ((*i)->isa(Rosegarden::Note::EventType)) {

	    bool isNew;
	    Rosegarden::TimeSignature sig =
		m_composition->getTimeSignatureInBar(barNo, isNew);

	    beatTimeTs.push_back(m_composition->getBarStart(barNo) +
				 beat * sig.getBeatDuration());

	    if (++beat >= sig.getBeatsPerBar()) {
		++barNo;
		beat = 0;
	    }

	    beatRealTimes.push_back(s->getComposition()->getElapsedRealTime
				    ((*i)->getAbsoluteTime()));
	}
    }

    if (beatTimeTs.size() < 2) return;

    long prevRawTempo = -1;

    // set up m_oldTempi and prevRawTempo

    for (int i = m_composition->getTempoChangeNumberAt(*beatTimeTs.begin()-1) + 1;
	 i <= m_composition->getTempoChangeNumberAt(*beatTimeTs.end()-1); ++i) {

	std::pair<Rosegarden::timeT, long> tempoChange =
	    m_composition->getRawTempoChange(i);
	m_oldTempi[tempoChange.first] = tempoChange.second;
	if (prevRawTempo == -1) prevRawTempo = tempoChange.second;
    }

    RG_DEBUG << "starting raw tempo: " << prevRawTempo << endl;

    Rosegarden::timeT quarter = Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration();

    for (int beat = 1; beat < beatTimeTs.size(); ++beat) {

	Rosegarden::timeT beatTime = beatTimeTs[beat] - beatTimeTs[beat-1];
	Rosegarden::RealTime beatRealTime = beatRealTimes[beat] - beatRealTimes[beat-1];

	// Calculate tempo in quarter notes per hour.
	// This is 3600 / {quarter note duration in seconds}
	// = 3600 / ( {beat in seconds} * {quarter in ticks} / { beat in ticks} )
	// = ( 3600 * {beat in ticks} ) / ( {beat in seconds} * {quarter in ticks} )

	double beatSec = double(beatRealTime.sec) +
	    double(beatRealTime.usec() / 1000000.0);
	double qph = (3600.0 * beatTime) / (beatSec * quarter);
	long rawTempo = long(qph + 0.000001);

	RG_DEBUG << "prev beat: " << beatTimeTs[beat] << ", prev beat real time " << beatRealTimes[beat] << endl;
	RG_DEBUG << "time " << beatTime << ", rt " << beatRealTime << ", beatSec " << beatSec << ", rawTempo " << rawTempo << endl;

	if (rawTempo != prevRawTempo) {
	    m_newTempi[beatTimeTs[beat-1]] = rawTempo;
	    prevRawTempo = rawTempo;
	}
    }
	
}
