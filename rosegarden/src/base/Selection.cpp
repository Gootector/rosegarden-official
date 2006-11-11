// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include "Selection.h"
#include "Segment.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"

namespace Rosegarden {

EventSelection::EventSelection(Segment& t) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);
}

EventSelection::EventSelection(Segment& t, timeT beginTime, timeT endTime, bool overlap) :
    m_originalSegment(t),
    m_beginTime(0),
    m_endTime(0),
    m_haveRealStartTime(false)
{
    t.addObserver(this);

    Segment::iterator i = t.findTime(beginTime);
    Segment::iterator j = t.findTime(endTime);

    if (i != t.end()) {
	m_beginTime = (*i)->getAbsoluteTime();
	while (i != j) {
	    m_endTime = (*i)->getAbsoluteTime() + (*i)->getDuration();
	    m_segmentEvents.insert(*i);
	    ++i;
	}
	m_haveRealStartTime = true;
    }

    // Find events overlapping the beginning
    //
    if (overlap) {
        i = t.findTime(beginTime);

        while (i != t.begin() && i != t.end() && i != j) {

            if ((*i)->getAbsoluteTime() + (*i)->getDuration() > beginTime)
            {
                m_segmentEvents.insert(*i); // duplicates are filtered automatically
                m_beginTime = (*i)->getAbsoluteTime();
            }
            else
                break;

            --i;
        }

    }

}

EventSelection::EventSelection(const EventSelection &sel) :
    SegmentObserver(),
    m_originalSegment(sel.m_originalSegment),
    m_segmentEvents(sel.m_segmentEvents),
    m_beginTime(sel.m_beginTime),
    m_endTime(sel.m_endTime),
    m_haveRealStartTime(sel.m_haveRealStartTime)
{
    m_originalSegment.addObserver(this);
}

EventSelection::~EventSelection()
{
    m_originalSegment.removeObserver(this);
}

void EventSelection::addEvent(Event *e)
{ 
    timeT eventDuration = e->getDuration();
    if (eventDuration == 0) eventDuration = 1;

    if (contains(e)) return;

    if (e->getAbsoluteTime() < m_beginTime || !m_haveRealStartTime) {
	m_beginTime = e->getAbsoluteTime();
	m_haveRealStartTime = true;
    }
    if (e->getAbsoluteTime() + eventDuration > m_endTime) {
	m_endTime = e->getAbsoluteTime() + eventDuration;
    }
    m_segmentEvents.insert(e);
}

void EventSelection::addFromSelection(EventSelection *sel)
{
    for (eventcontainer::iterator i = sel->getSegmentEvents().begin();
	 i != sel->getSegmentEvents().end(); ++i) {
	if (!contains(*i)) addEvent(*i);
    }
}

void EventSelection::removeEvent(Event *e) 
{
    std::pair<eventcontainer::iterator, eventcontainer::iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::iterator it = interval.first;
         it != interval.second; it++)
    {
        if (*it == e) {
	    m_segmentEvents.erase(it); 
	    return;
	}
    }
}

bool EventSelection::contains(Event *e) const
{
    std::pair<eventcontainer::const_iterator, eventcontainer::const_iterator> 
	interval = m_segmentEvents.equal_range(e);

    for (eventcontainer::const_iterator it = interval.first;
         it != interval.second; ++it)
    {
        if (*it == e) return true;
    }

    return false;
}

bool EventSelection::contains(const std::string &type) const
{
    for (eventcontainer::const_iterator i = m_segmentEvents.begin();
	 i != m_segmentEvents.end(); ++i) {
	if ((*i)->isa(type)) return true;
    }
    return false;
}

timeT EventSelection::getTotalDuration() const
{
    return getEndTime() - getStartTime();
}

EventSelection::RangeList
EventSelection::getRanges() const
{
    RangeList ranges;

    Segment::iterator i = m_originalSegment.findTime(getStartTime());
    Segment::iterator j = i;
    Segment::iterator k = m_originalSegment.findTime(getEndTime());

    while (j != k) {

        for (j = i; j != k && contains(*j); ++j);

        if (j != i) {
            ranges.push_back(RangeList::value_type(i, j));
	}

	for (i = j; i != k && !contains(*i); ++i);
	j = i;
    }

    return ranges;
}

EventSelection::RangeTimeList
EventSelection::getRangeTimes() const
{
    RangeList ranges(getRanges());
    RangeTimeList rangeTimes;

    for (RangeList::iterator i = ranges.begin(); i != ranges.end(); ++i) {
	timeT startTime = m_originalSegment.getEndTime();
	timeT   endTime = m_originalSegment.getEndTime();
	if (i->first != m_originalSegment.end()) {
	    startTime = (*i->first)->getAbsoluteTime();
	}
	if (i->second != m_originalSegment.end()) {
	    endTime = (*i->second)->getAbsoluteTime();
	}
	rangeTimes.push_back(RangeTimeList::value_type(startTime, endTime));
    }

    return rangeTimes;
}

void
EventSelection::eventRemoved(const Segment *s, Event *e)
{
    if (s == &m_originalSegment /*&& contains(e)*/) {
        removeEvent(e);
    }
}

void
EventSelection::segmentDeleted(const Segment *)
{
    /*
    std::cerr << "WARNING: EventSelection notified of segment deletion: this is probably a bug "
	      << "(selection should have been deleted before segment)" << std::endl;
              */
}

bool SegmentSelection::hasNonAudioSegment() const
{
    for (const_iterator i = begin(); i != end(); ++i) {
        if ((*i)->getType() == Segment::Internal)
            return true;
    }
    return false;
}


TimeSignatureSelection::TimeSignatureSelection() { }

TimeSignatureSelection::TimeSignatureSelection(Composition &composition,
					       timeT beginTime,
					       timeT endTime,
					       bool includeOpeningTimeSig)
{
    int n = composition.getTimeSignatureNumberAt(endTime);

    for (int i = composition.getTimeSignatureNumberAt(beginTime);
	 i <= n;
	 ++i) {

	if (i < 0) continue;

	std::pair<timeT, TimeSignature> sig =
	    composition.getTimeSignatureChange(i);

	if (sig.first < endTime) {
	    if (sig.first < beginTime) {
		if (includeOpeningTimeSig) {
		    sig.first = beginTime;
		} else {
		    continue;
		}
	    }
	    addTimeSignature(sig.first, sig.second);
	}
    }
}

TimeSignatureSelection::~TimeSignatureSelection() { }

void
TimeSignatureSelection::addTimeSignature(timeT t, TimeSignature timeSig)
{
    m_timeSignatures.insert(timesigcontainer::value_type(t, timeSig));
}

TempoSelection::TempoSelection() { }

TempoSelection::TempoSelection(Composition &composition,
			       timeT beginTime,
			       timeT endTime,
			       bool includeOpeningTempo)
{
    int n = composition.getTempoChangeNumberAt(endTime);

    for (int i = composition.getTempoChangeNumberAt(beginTime);
	 i <= n;
	 ++i) {

	if (i < 0) continue;

	std::pair<timeT, tempoT> change = composition.getTempoChange(i);

	if (change.first < endTime) {
	    if (change.first < beginTime) {
		if (includeOpeningTempo) {
		    change.first = beginTime;
		} else {
		    continue;
		}
	    }
	    std::pair<bool, tempoT> ramping =
		composition.getTempoRamping(i, false);
	    addTempo(change.first, change.second,
		     ramping.first ? ramping.second : -1);
	}
    }
}

TempoSelection::~TempoSelection() { }

void
TempoSelection::addTempo(timeT t, tempoT tempo, tempoT targetTempo)
{
    m_tempos.insert(tempocontainer::value_type
		    (t, tempochange(tempo, targetTempo)));
}

}
