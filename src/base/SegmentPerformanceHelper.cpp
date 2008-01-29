// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#include "SegmentPerformanceHelper.h"
#include "BaseProperties.h"
#include <iostream>

namespace Rosegarden 
{
using std::endl;
using std::string;

using namespace BaseProperties;

SegmentPerformanceHelper::~SegmentPerformanceHelper() { }


SegmentPerformanceHelper::iteratorcontainer
SegmentPerformanceHelper::getTiedNotes(iterator i)
{
    iteratorcontainer c;
    c.push_back(i);

    Event *e = *i;
    if (!e->isa(Note::EventType)) return c;
    Segment::iterator j(i);

    bool tiedBack = false, tiedForward = false;
    e->get<Bool>(TIED_BACKWARD, tiedBack);
    e->get<Bool>(TIED_FORWARD, tiedForward);

    timeT d = e->getNotationDuration();
    timeT t = e->getNotationAbsoluteTime();

    if (!e->has(PITCH)) return c;
    int pitch = e->get<Int>(PITCH);

    bool valid = false;

    if (tiedBack) {
	// #1171463: If we can find no preceding TIED_FORWARD event,
	// then we remove this property
	
	while (j != begin()) {

	    --j;
	    if (!(*j)->isa(Note::EventType)) continue;
	    e = *j; // can reuse e because this branch always returns

	    timeT t2 = e->getNotationAbsoluteTime() + e->getNotationDuration();
	    if (t2 < t) break;

	    if (t2 > t || !e->has(PITCH) ||
		e->get<Int>(PITCH) != pitch) continue;

	    bool prevTiedForward = false;
	    if (!e->get<Bool>(TIED_FORWARD, prevTiedForward) ||
		!prevTiedForward) break;

	    valid = true;
	    break;
	}

	if (valid) {
	    return iteratorcontainer();
	} else {
	    (*i)->unset(TIED_BACKWARD);
	    return c;
	}
    }
    else if (!tiedForward) return c;

    for (;;) {
	while (++j != end() && !(*j)->isa(Note::EventType));
        if (j == end()) return c;

        e = *j;

        timeT t2 = e->getNotationAbsoluteTime();
        
        if (t2 > t + d) break;
        else if (t2 < t + d || !e->has(PITCH) ||
                 e->get<Int>(PITCH) != pitch) continue;

        if (!e->get<Bool>(TIED_BACKWARD, tiedBack) ||
            !tiedBack) break;

        d += e->getNotationDuration();
	c.push_back(j);
	valid = true;

        if (!e->get<Bool>(TIED_FORWARD, tiedForward) ||
            !tiedForward) return c;
    }

    if (!valid) {
	// Related to #1171463: If we can find no following
	// TIED_BACKWARD event, then we remove this property
	(*i)->unset(TIED_FORWARD);
    }

    return c;
}


timeT
SegmentPerformanceHelper::getSoundingAbsoluteTime(iterator i)
{
    return (*i)->getAbsoluteTime();
}

timeT
SegmentPerformanceHelper::getSoundingDuration(iterator i)
{
    timeT d = 0;

    if ((*i)->has(TIED_BACKWARD)) {
	
	// Formerly we just returned d in this case, but now we check
	// with getTiedNotes so as to remove any bogus backward ties
	// that have no corresponding forward tie.  Unfortunately this
	// is quite a bit slower.

	//!!! optimize. at least we should add a marker property to
	//anything we've already processed from this helper this time
	//around.

	iteratorcontainer c(getTiedNotes(i));
	
	if (c.empty()) { // the tie back is valid
	    return 0;
	}
    }

    if (!(*i)->has(TIED_FORWARD) || !(*i)->isa(Note::EventType)) {

	d = (*i)->getDuration();

    } else {

	// tied forward but not back

	iteratorcontainer c(getTiedNotes(i));
	    
	for (iteratorcontainer::iterator ci = c.begin();
	     ci != c.end(); ++ci) {
	    d += (**ci)->getDuration();
	}
    }

    return d;
}


// In theory we can do better with tuplets, because real time has
// finer precision than timeT time.  With a timeT resolution of 960ppq
// however the difference is probably not audible

RealTime
SegmentPerformanceHelper::getRealAbsoluteTime(iterator i) 
{
    return segment().getComposition()->getElapsedRealTime
	(getSoundingAbsoluteTime(i));
}


// In theory we can do better with tuplets, because real time has
// finer precision than timeT time.  With a timeT resolution of 960ppq
// however the difference is probably not audible
// 
// (If we did want to do this, it'd help to have abstime->realtime
// conversion methods that accept double args in Composition)

RealTime
SegmentPerformanceHelper::getRealSoundingDuration(iterator i)
{
    timeT t0 = getSoundingAbsoluteTime(i);
    timeT t1 = t0 + getSoundingDuration(i);

    if (t1 > segment().getEndMarkerTime()) {
	t1 = segment().getEndMarkerTime();
    }

    return segment().getComposition()->getRealTimeDifference(t0, t1);
}


}
