// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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
#include "Sets.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::TrackId;
using Rosegarden::AudioFileManager;


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
    KNamedCommand("Erase Segment"),
    m_composition(segment->getComposition()),
    m_segment(segment),
    m_detached(false)
{
    // nothing else
}

SegmentEraseCommand::~SegmentEraseCommand()
{
    // This is the only place the Segment can safely be deleted, and
    // then only if it is not in the Composition (i.e. if we executed
    // more recently than we unexecuted).  Can't safely call through
    // the m_segment pointer here; someone else might have got to it
    // first

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
}

// --------- Copy Segment ---------
//
SegmentQuickCopyCommand::SegmentQuickCopyCommand(Segment *segment):
    KNamedCommand("Quick-Copy Segment"),
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
    m_segment = new Segment(*m_segmentToCopy);
    m_segment->setLabel
	(m_segment->getLabel() + " " + qstrtostr(i18n("(copied)")));
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
    KNamedCommand("Repeating Segment to Copies"),
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


// -------- Audio Insert Segment --------
//

AudioSegmentInsertCommand::AudioSegmentInsertCommand(
        RosegardenGUIDoc *doc,
        TrackId track,
        timeT startTime,
        Rosegarden::AudioFileId audioFileId,
        const Rosegarden::RealTime &audioStartTime,
        const Rosegarden::RealTime &audioEndTime):
    KNamedCommand("Create Segment"),
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
	m_composition->addSegment(m_segment);
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

	m_segment->setEndTime(endTimeT);

        // Label by audio file name
        //
        std::string label = "";

        Rosegarden::AudioFile *aF =
                m_audioFileManager->getAudioFile(m_audioFileId);

        if (aF)
            label = aF->getName() + std::string(" (inserted)");
        else
            label = std::string("unknown audio file");

        m_segment->setLabel(label);

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
    KNamedCommand("Create Segment"),
    m_composition(&(doc->getComposition())),
    m_studio(&(doc->getStudio())),
    m_segment(0),
    m_track(track),
    m_startTime(startTime),
    m_endTime(endTime),
    m_detached(false)
{
}

SegmentInsertCommand::~SegmentInsertCommand()
{
    if (m_detached) {
	delete m_segment;
    }
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
    KNamedCommand("Record"),
    m_composition(s->getComposition()),
    m_segment(s),
    m_detached(false),
    m_firstTime(true)
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

	if (m_firstTime) {
	    // Apply a quantization for notation only (does not affect
	    // any other editing or performance facility).  This
	    // normalizes the rests for us too.
	    m_composition->getNotationQuantizer()->quantize(m_segment);
	    Rosegarden::SegmentNotationHelper helper(*m_segment);
	    m_segment->insert
		(helper.guessClef(m_segment->begin(),
				  m_segment->end()).getAsEvent(0));
	    helper.autoBeam(m_segment->begin(),
			    m_segment->end(),
			    Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
	}

        m_segment->setLabel(std::string("recorded audio"));
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
				      timeT endTime,
				      TrackId track)
{
    SegmentRec record;
    record.segment = segment;
    record.startTime = startTime;
    record.endTime = endTime;
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
	// next iteration of the execute/unexecute cycle

	timeT currentEndTime = i->segment->getEndMarkerTime();
	timeT currentStartTime = i->segment->getStartTime();
	TrackId currentTrack = i->segment->getTrack();

	if (currentStartTime != i->startTime) {
	    i->segment->setStartTime(i->startTime);
	    i->startTime = currentStartTime;
	}

	if (currentEndTime != i->endTime) {
	    i->segment->setEndMarkerTime(i->endTime);
	    i->endTime = currentEndTime;
	}

	if (currentTrack != i->track) {
	    i->segment->setTrack(i->track);
	    i->track = currentTrack;
	}
    }
}


// --------- Audio Split Segment -----------
//
//

AudioSegmentSplitCommand::AudioSegmentSplitCommand(Segment *segment,
					           timeT splitTime) :
    KNamedCommand("Split Audio Segment"),
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
        m_segment->setLabel(m_segmentLabel + std::string(" (split)"));
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
    KNamedCommand("Split Segment"),
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
        m_segment->setLabel(m_segmentLabel + std::string(" (split)"));
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
    std::vector<AutoSplitPoint> splitPoints;

    if (m_segment->getType() != Rosegarden::Segment::Audio)
        return;

    std::vector<Rosegarden::SplitPointPair> rtSplitPoints =
        m_audioFileManager->getSplitPoints(m_segment->getAudioFileId(),
                                           m_segment->getAudioStartTime(),
                                           m_segment->getAudioEndTime(),
                                           m_threshold);

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

        // label
        sprintf(splitNumber, "%d", splitCount++);
        newSegment->setLabel(
                m_segment->getLabel() + std::string(" (autosplit ") +
                splitNumber + std::string(" )"));

	m_composition->addSegment(newSegment);
        newSegment->setStartTime(absStartTime);
        newSegment->setEndTime(absEndTime);
        cout << "CREATING FROM " << newSegment->getAudioStartTime() << " TO "
                                 << newSegment->getAudioEndTime() << endl;

	m_newSegments.push_back(newSegment);
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
	newSegment->setLabel(m_segment->getLabel() + " " +
			     qstrtostr(i18n("(part)")));

	timeT startTime = segmentStart;
	if (split > 0) {
	    startTime = splitPoints[split-1].time;
	    newSegment->insert(splitPoints[split-1].clef.getAsEvent(startTime));
	    newSegment->insert(splitPoints[split-1].key.getAsEvent(startTime));
	}

	Segment::iterator i = m_segment->findTime(startTime);

	while (m_segment->isBeforeEndMarker(i)) {
	    timeT t = (*i)->getAbsoluteTime();
	    if (split < splitPoints.size() &&
		t >= splitPoints[split].lastSoundTime) break;
	    newSegment->insert(new Event(**i));
	    ++i;
	}

	m_newSegments.push_back(newSegment);
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


SegmentMergeCommand::SegmentMergeCommand(const Rosegarden::SegmentSelection &
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
    Composition *composition = 0;

    if (!m_newSegment) {

	m_newSegment = new Segment(*m_oldSegments[0]);
	composition = m_oldSegments[0]->getComposition();

	// that duplicated segment 0; now do the rest

	for (unsigned int i = 1; i < m_oldSegments.size(); ++i) {

	    if (m_oldSegments[i]->getStartTime() >
		m_newSegment->getEndMarkerTime()) {
		m_newSegment->setEndMarkerTime
		    (m_oldSegments[i]->getStartTime());
	    }

	    for (Segment::iterator si = m_oldSegments[i]->begin();
		 m_oldSegments[i]->isBeforeEndMarker(si); ++si) {
		m_newSegment->insert(new Event(**si));
	    }

	    if (m_oldSegments[i]->getEndMarkerTime() >
		m_newSegment->getEndMarkerTime()) {
		m_newSegment->setEndMarkerTime
		    (m_oldSegments[i]->getEndMarkerTime());
	    }
	}
	
    } else {
	composition = m_newSegment->getComposition();
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

void
SegmentRescaleCommand::execute()
{
    timeT startTime = m_segment->getStartTime();
    m_newSegment = new Segment();
    m_newSegment->setTrack(m_segment->getTrack());
    m_newSegment->setLabel(m_segment->getLabel() + " " +
			   qstrtostr(i18n("(rescaled)")));

    for (Segment::iterator i = m_segment->begin();
	 m_segment->isBeforeEndMarker(i); ++i) {

	if ((*i)->isa(Rosegarden::Note::EventRestType)) continue;

	timeT dt = (*i)->getAbsoluteTime() - startTime;
	timeT duration = (*i)->getDuration();

	m_newSegment->insert
	    (new Event(**i,
		       startTime + (dt * m_multiplier / m_divisor),
		       duration * m_multiplier / m_divisor));
    }

    m_segment->getComposition()->addSegment(m_newSegment);
    m_segment->getComposition()->detachSegment(m_segment);
    m_newSegment->normalizeRests(m_newSegment->getStartTime(),
				 m_newSegment->getEndTime());

    m_newSegment->setEndMarkerTime
	(startTime + (m_segment->getEndMarkerTime() - startTime) *
	 m_multiplier / m_divisor);

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

	addCommand(new TransformsMenuNormalizeRestsCommand
		   (**i,
		    std::max(startTime, time),
		    std::min(endTime, nextTimeSigTime)));
    }
}

AddTimeSignatureAndNormalizeCommand::~AddTimeSignatureAndNormalizeCommand()
{
    // well, nothing really
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
void AddTracksCommand::execute()
{
    using Rosegarden::Track;

    unsigned int currentNbTracks = m_composition->getNbTracks();

    for (unsigned int i = 0; i < m_nbNewTracks; ++i) {
        Track* track = new Rosegarden::Track;
        track->setId(i + currentNbTracks);
        track->setPosition(i + currentNbTracks);
        track->setInstrument(m_instrumentId);

        m_composition->addTrack(track);
    }
}

void AddTracksCommand::unexecute()
{
    for (unsigned int i = 0; i < m_nbNewTracks; ++i) {

        m_composition->deleteTrack(m_composition->getNbTracks() - 1);
    }
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

    m_oldSegments.clear();
    m_oldTracks.clear();
    
    // Remap positions and track numbers
    //

    Rosegarden::Composition::trackiterator tit;
    Rosegarden::Composition::trackcontainer
                &tracks = m_composition->getTracks();

    /*
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
    {
        cout << "TRACK " << (*tit).first << " - "
             << (*tit).second->getId()
             << " - POSITION = " << (*tit).second->getPosition()
             << endl;
    }
    */

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
                std::cerr << "DeleteTracksCommand::execute - "
                          << "can't detach track" << std::endl;
            }
        }
    }


    std::vector<Rosegarden::Track*>::iterator otIt;
    for (otIt = m_oldTracks.begin(); otIt != m_oldTracks.end(); ++otIt)
    {
        for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        {
            /*
            if ((*tit).second->getId() > (*otIt)->getId())
            {
                (*tit).second->setId((*tit).second->getId() - 1);

                // Don't forget we're using a map for the tracks too
                // and the map value needs to be updated.  Urgh.
                //
            }
            */

            if ((*tit).second->getPosition() > (*otIt)->getPosition())
            {
                (*tit).second->setPosition((*tit).second->getPosition() - 1);
            }
        }
    }

    /*
    for (tit = tracks->begin(); tit != tracks->end(); ++tit)
    {
        cout << "TRACK " << (*tit).first << " - "
             << (*tit).second->getId()
             << " - POSITION = " << (*tit).second->getPosition()
             << endl;
    }
    */

    // Sort out the record track - make sure it's valid
    //
    /*
    Rosegarden::TrackId recordTrack = m_composition.getRecordTrack();
    int recordPosition = m_composition.getTrackById(recordTrack)->getPosition();

    if (recordPosition == 0) recordPosition = 
    if (m_composition.getTrackByPosition(recordPosition - 1)
    */
    m_composition->setRecordTrack(m_composition->getTrackByPosition(0)->getId());

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
        for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        {
            /*
            if ((*tit).second->getId() >= (*otIt)->getId())
                (*tit).second->setId((*tit).second->getId() + 1);
                */

            if ((*tit).second->getPosition() >= (*otIt)->getPosition())
                (*tit).second->setPosition((*tit).second->getPosition() + 1);
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
    KNamedCommand("Split by Pitch"),
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
    m_newSegmentA = new Segment;
    m_newSegmentB = new Segment;

    m_newSegmentA->setTrack(m_segment->getTrack());
    m_newSegmentA->setStartTime(m_segment->getStartTime());
    m_segment->getComposition()->addSegment(m_newSegmentA);

    m_newSegmentB->setTrack(m_segment->getTrack());
    m_newSegmentB->setStartTime(m_segment->getStartTime());
    m_segment->getComposition()->addSegment(m_newSegmentB);
    
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

		m_newSegmentB->insert(new Event(**i));
	    } else {
		m_newSegmentA->insert(new Event(**i));
	    }
	    
	} else {

	    m_newSegmentA->insert(new Event(**i));

	    if (m_dupNonNoteEvents) {
		m_newSegmentB->insert(new Event(**i));
	    }
	}
    }
    
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
    
    m_segment->getComposition()->getNotationQuantizer()->quantize(m_newSegmentA);
    m_segment->getComposition()->getNotationQuantizer()->quantize(m_newSegmentB);
    helperA.autoBeam(m_newSegmentA->begin(), m_newSegmentA->end(),
		     Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    helperB.autoBeam(m_newSegmentB->begin(), m_newSegmentB->end(),
		     Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);

    std::string label = m_segment->getLabel();
    m_newSegmentA->setLabel(label + std::string(" (upper)"));
    m_newSegmentB->setLabel(label + std::string(" (lower)"));

    m_segment->getComposition()->detachSegment(m_segment);
    m_executed = true;
}
    
void
SegmentSplitByPitchCommand::unexecute()
{
    m_newSegmentA->getComposition()->addSegment(m_segment);
    delete m_newSegmentA;
    delete m_newSegmentB;
    m_newSegmentA = 0;
    m_newSegmentB = 0;
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
        const Rosegarden::SegmentSelection &segments,
        const QString &label):
    KNamedCommand("Label Segments"),
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


