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

#include "segmentcommands.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "notationcommands.h"
#include "NotationTypes.h"
#include "Property.h"
#include "Composition.h"

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::timeT;
using Rosegarden::TrackId;


SegmentCommand::SegmentCommand(QString name, const std::vector<Rosegarden::Segment*>& segments)
    : XKCommand(name)
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
//     kdDebug(KDEBUG_AREA) << "SegmentCommandChangeTransposeValue : nb segments : " << m_segments.size()
//                          << endl;

// }


// void SegmentCommandChangeTransposeValue::execute()
// {
//     segmentlist::iterator it;

//     for (it = m_segments.begin(); it != m_segments.end(); ++it) {
//         kdDebug(KDEBUG_AREA) << "SegmentCommandChangeTransposeValue::execute : saving " << (*it)->getTranspose()
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
//         kdDebug(KDEBUG_AREA) << "SegmentCommandChangeTransposeValue::unexecute : restoring " << (*itV)
//                              << endl;
        
//         (*it)->setTranspose((*itV));
//     }
// }


// --------- Erase Segment --------
//
SegmentEraseCommand::SegmentEraseCommand(Segment *segment) :
    XKCommand("Erase Segment"),
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
    XKCommand("Quick-Copy Segment"),
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


// --------- Insert Segment --------
//
SegmentInsertCommand::SegmentInsertCommand(RosegardenGUIDoc *doc,
                                           TrackId track,
                                           timeT startTime,
                                           timeT endTime):
    XKCommand("Create Segment"),
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
        Rosegarden::Track *track = m_composition->getTrackByIndex(m_track);
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
    XKCommand("Record"),
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
    XKCommand(name)
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
    XKCommand("Split Audio Segment"),
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
        m_segment->setAudioEndTime(m_segment->getAudioStartTime() + splitDiff);

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
    XKCommand("Split Segment"),
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

	// Copy the last occurence of clef and key
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
/*!!!
    if (m_segment->getEndTime() < m_newSegment->getStartTime()) {
	m_segment->fillWithRests(m_newSegment->getStartTime());
    }

    for (Segment::iterator it = m_newSegment->begin();
	 it != m_newSegment->end(); ++it) {
	m_segment->insert(new Event(**it));
    }

    m_segment->clearEndMarker();
*/
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


// ----------- Audio Segment Auto-Split ------
//
AudioSegmentAutoSplitCommand::AudioSegmentAutoSplitCommand(
        Segment *segment) :
    XKCommand(getGlobalName()),
    m_segment(segment),
    m_composition(segment->getComposition()),
    m_detached(false)
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
    /*
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
    */
}

void
AudioSegmentAutoSplitCommand::unexecute()
{
    /*
    for (unsigned int i = 0; i < m_newSegments.size(); ++i) {
	m_composition->detachSegment(m_newSegments[i]);
    }
    m_composition->addSegment(m_segment);
    m_detached = false;
    */
}

// ----------- Segment Auto-Split ------------
//

SegmentAutoSplitCommand::SegmentAutoSplitCommand(Segment *segment) :
    XKCommand(getGlobalName()),
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

struct AutoSplitPoint
{
    timeT time;
    timeT lastSoundTime;
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    AutoSplitPoint(timeT t, timeT lst, Rosegarden::Clef c, Rosegarden::Key k) :
	time(t), lastSoundTime(lst), clef(c), key(k) { }
};

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


SegmentRescaleCommand::SegmentRescaleCommand(Segment *s,
					     int multiplier,
					     int divisor) :
    XKCommand(getGlobalName()),
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


SegmentChangeQuantizationCommand::SegmentChangeQuantizationCommand(Rosegarden::StandardQuantization *sq) :
    XKCommand(getGlobalName(sq)),
    m_quantization(sq)
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

	if (m_quantization) {

	    rec.oldQuantizer =
		new Rosegarden::Quantizer(rec.segment->getQuantizer());
	    rec.segment->setQuantizeLevel(*m_quantization);

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

	if (m_quantization) {

	    if (!rec.wasQuantized) rec.segment->setQuantization(false);

	    rec.segment->setQuantizeLevel(*rec.oldQuantizer);
	    delete rec.oldQuantizer;
	    rec.oldQuantizer = 0;

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
    rec.oldQuantizer = 0;
    rec.wasQuantized = false; // shouldn't matter what we initialise this to
    m_records.push_back(rec);
}
    
QString
SegmentChangeQuantizationCommand::getGlobalName(Rosegarden::StandardQuantization *sq)
{
    if (!sq) {
	return "Unquantize";
    } else {
	return QString("Quantize to ") + strtoqstr(sq->name);
    }
}



// --------- Add Time Signature --------
// 

AddTimeSignatureCommand::AddTimeSignatureCommand(Composition *composition,
						 timeT time,
						 Rosegarden::TimeSignature timeSig) :
    XKCommand(getGlobalName()),
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
    timeT endTime(composition->getDuration());

    int index = composition->getTimeSignatureNumberAt(time);
    if (composition->getTimeSignatureCount() > index + 1) {
	endTime = composition->getTimeSignatureChange(index + 1).first;
    }
	
    for (Composition::iterator i = composition->begin();
	 i != composition->end(); ++i) {

	int startTime = (*i)->getStartTime();
	if (startTime >= endTime) continue;
	if (startTime < time) startTime = time;

	addCommand(new TransformsMenuNormalizeRestsCommand
		   (**i, time, std::min((*i)->getEndTime(), endTime)));
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
