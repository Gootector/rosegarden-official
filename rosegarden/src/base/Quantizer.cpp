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

#include "Quantizer.h"
#include "BaseProperties.h"
#include "NotationTypes.h"
#include "Selection.h"
#include "Composition.h"
#include "Sets.h"
#include "Profiler.h"

#include <iostream>
#include <cmath>
#include <cstdio> // for sprintf
#include <ctime>

using std::cout;
using std::cerr;
using std::endl;

//#define DEBUG_NOTATION_QUANTIZER 1

namespace Rosegarden {

using namespace BaseProperties;

const std::string Quantizer::RawEventData = "";
const std::string Quantizer::DefaultTarget = "DefaultQ";
const std::string Quantizer::GlobalSource = "GlobalQ";
const std::string Quantizer::NotationPrefix = "Notation";

Quantizer::Quantizer(std::string source,
		     std::string target) :
    m_source(source), m_target(target)
{
    makePropertyNames();
}


Quantizer::Quantizer(std::string target) :
    m_target(target)
{
    if (target == RawEventData) {
	m_source = GlobalSource;
    } else {
	m_source = RawEventData;
    }

    makePropertyNames();
}


Quantizer::~Quantizer()
{
    // nothing
}

void
Quantizer::quantize(Segment *s) const
{
    quantize(s, s->begin(), s->getEndMarker());
}

void
Quantizer::quantize(Segment *s,
		    Segment::iterator from,
		    Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    quantizeRange(s, from, to);

    insertNewEvents(s);
}

void
Quantizer::quantize(EventSelection *selection)
{
    assert(m_toInsert.size() == 0);

    Segment &segment = selection->getSegment();

    // Attempt to handle non-contiguous selections.

    // We have to be a bit careful here, because the rest-
    // normalisation that's carried out as part of a quantize
    // process is liable to replace the event that follows
    // the quantized range.  (moved here from editcommands.cpp)

    EventSelection::RangeList ranges(selection->getRanges());

    // So that we can retrieve a list of new events we cheat and stop
    // the m_toInsert vector from being cleared automatically.  Remember
    // to turn it back on.
    //

    EventSelection::RangeList::iterator r = ranges.end();
    while (r-- != ranges.begin()) {

/*
	cerr << "Quantizer: quantizing range ";
	if (r->first == segment.end()) {
	    cerr << "end";
	} else {
	    cerr << (*r->first)->getAbsoluteTime();
	}
	cerr << " to ";
	if (r->second == segment.end()) {
	    cerr << "end";
	} else {
	    cerr << (*r->second)->getAbsoluteTime();
	}
	cerr << endl;
*/

        quantizeRange(&segment, r->first, r->second);
    }

    // Push the new events to the selection
    for (int i = 0; i < m_toInsert.size(); ++i) {
	selection->addEvent(m_toInsert[i]);
    }

    // and then to the segment
    insertNewEvents(&segment);
}


void
Quantizer::fixQuantizedValues(Segment *s,
			      Segment::iterator from,
			      Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    quantize(s, from, to);

    if (m_target == RawEventData) return;

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {

	++nextFrom;
	
	timeT t = getFromTarget(*from, AbsoluteTimeValue);
	timeT d = getFromTarget(*from, DurationValue);
	Event *e = new Event(**from, t, d);
	s->erase(from);
	m_toInsert.push_back(e);
    }
    
    insertNewEvents(s);
}


timeT
Quantizer::getQuantizedDuration(const Event *e) const
{
    if (m_target == RawEventData) {
	return e->getDuration();
    } else if (m_target == NotationPrefix) {
	return e->getNotationDuration();
    } else {
	timeT d = e->getDuration();
	e->get<Int>(m_targetProperties[DurationValue], d);
	return d;
    }
}

timeT
Quantizer::getQuantizedAbsoluteTime(const Event *e) const
{
    if (m_target == RawEventData) {
	return e->getAbsoluteTime();
    } else if (m_target == NotationPrefix) {
	return e->getNotationAbsoluteTime();
    } else {
	timeT t = e->getAbsoluteTime();
	e->get<Int>(m_targetProperties[AbsoluteTimeValue], t);
	return t;
    }
}

timeT
Quantizer::getUnquantizedAbsoluteTime(Event *e) const
{
    return getFromSource(e, AbsoluteTimeValue);
}

timeT
Quantizer::getUnquantizedDuration(Event *e) const
{
    return getFromSource(e, DurationValue);
}

void
Quantizer::quantizeRange(Segment *s,
			 Segment::iterator from,
			 Segment::iterator to) const
{
    //!!! It is vital that ordering is maintained after quantization.
    // That is, an event whose absolute time quantizes to a time t must
    // appear in the original segment before all events whose times
    // quantize to greater than t.  This means we must quantize the
    // absolute times of non-note events as well as notes.

    // We don't need to worry about quantizing rests, however; they're
    // only used for notation and will be explicitly recalculated when
    // the notation quantization values change.

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {

	++nextFrom;
	quantizeSingle(s, from);
    }
}
    
void
Quantizer::unquantize(Segment *s,
		      Segment::iterator from,
		      Segment::iterator to) const
{
    assert(m_toInsert.size() == 0);

    for (Segment::iterator nextFrom = from; from != to; from = nextFrom) {
	++nextFrom;

	if (m_target == RawEventData || m_target == NotationPrefix) {
	    setToTarget(s, from,
			getFromSource(*from, AbsoluteTimeValue),
			getFromSource(*from, DurationValue));
	    
	} else {
	    removeTargetProperties(*from);
	}
    }
    
    insertNewEvents(s);
}

void
Quantizer::unquantize(EventSelection *selection) const
{
    assert(m_toInsert.size() == 0);

    Segment *s = &selection->getSegment();

    Rosegarden::EventSelection::eventcontainer::iterator it
        = selection->getSegmentEvents().begin();

    for (; it != selection->getSegmentEvents().end(); it++) {

	if (m_target == RawEventData || m_target == NotationPrefix) {

            Segment::iterator from = s->findSingle(*it);
            Segment::iterator to = s->findSingle(*it);
	    setToTarget(s, from,
			getFromSource(*from, AbsoluteTimeValue),
			getFromSource(*to, DurationValue));

	} else {
	    removeTargetProperties(*it);
	}
    }
    
    insertNewEvents(&selection->getSegment());
}



timeT
Quantizer::getFromSource(Event *e, ValueType v) const
{
    Profiler profiler("Quantizer::getFromSource");

//    cerr << "Quantizer::getFromSource: source is \"" << m_source << "\"" << endl;

    if (m_source == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else if (m_source == NotationPrefix) {

	if (v == AbsoluteTimeValue) return e->getNotationAbsoluteTime();
	else return e->getNotationDuration();

    } else {

	// We need to write the source from the target if the
	// source doesn't exist (and the target does)

	bool haveSource = e->has(m_sourceProperties[v]);
	bool haveTarget = ((m_target == RawEventData) ||
			   (e->has(m_targetProperties[v])));
	timeT t = 0;

	if (!haveSource && haveTarget) {
	    t = getFromTarget(e, v);
	    e->setMaybe<Int>(m_sourceProperties[v], t);
	    return t;
	}

	e->get<Int>(m_sourceProperties[v], t);
	return t;
    }
}

timeT
Quantizer::getFromTarget(Event *e, ValueType v) const
{
    Profiler profiler("Quantizer::getFromTarget");

    if (m_target == RawEventData) {

	if (v == AbsoluteTimeValue) return e->getAbsoluteTime();
	else return e->getDuration();

    } else if (m_target == NotationPrefix) {

	if (v == AbsoluteTimeValue) return e->getNotationAbsoluteTime();
	else return e->getNotationDuration();

    } else {
	timeT value;
	if (v == AbsoluteTimeValue) value = e->getAbsoluteTime();
	else value = e->getDuration();
	e->get<Int>(m_targetProperties[v], value);
	return value;
    }
}

void
Quantizer::setToTarget(Segment *s, Segment::iterator i,
		       timeT absTime, timeT duration) const
{
    Profiler profiler("Quantizer::setToTarget");

    //cerr << "Quantizer::setToTarget: target is \"" << m_target << "\", absTime is " << absTime << ", duration is " << duration << " (unit is " << m_unit << ", original values are absTime " << (*i)->getAbsoluteTime() << ", duration " << (*i)->getDuration() << ")" << endl;

    timeT st = 0, sd = 0;
    bool haveSt = false, haveSd = false;
    if (m_source != RawEventData && m_target == RawEventData) {
	haveSt = (*i)->get<Int>(m_sourceProperties[AbsoluteTimeValue], st);
	haveSd = (*i)->get<Int>(m_sourceProperties[DurationValue],     sd);
    }
    
    Event *e;
    if (m_target == RawEventData) {
	e = new Event(**i, absTime, duration);
    } else if (m_target == NotationPrefix) {
	// Setting the notation absolute time on an event without
	// recreating it would be dodgy, just as setting the absolute
	// time would, because it could change the ordering of events
	// that are already being referred to in ViewElementLists,
	// preventing us from locating them in the ViewElementLists
	// because their ordering would have silently changed
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer: setting " << absTime << " to notation absolute time and "
	     << duration << " to notation duration"
	     << endl;
#endif
	e = new Event(**i, (*i)->getAbsoluteTime(), (*i)->getDuration(),
		      (*i)->getSubOrdering(), absTime, duration);
    } else {
	e = *i;
	e->clearNonPersistentProperties();
    }

    if (m_target == NotationPrefix) {
	timeT normalizeStart = std::min(absTime, (*i)->getAbsoluteTime());
	timeT normalizeEnd = std::max(absTime + duration,
				      (*i)->getAbsoluteTime() +
				      (*i)->getDuration()) + 1;

	if (m_normalizeRegion.first != m_normalizeRegion.second) {
	    normalizeStart = std::min(normalizeStart, m_normalizeRegion.first);
	    normalizeEnd = std::max(normalizeEnd, m_normalizeRegion.second);
	}
	
	m_normalizeRegion = std::pair<timeT, timeT>
	    (normalizeStart, normalizeEnd);
    }
    
    if (haveSt) e->setMaybe<Int>(m_sourceProperties[AbsoluteTimeValue],st);
    if (haveSd) e->setMaybe<Int>(m_sourceProperties[DurationValue],    sd);
    
    if (m_target != RawEventData && m_target != NotationPrefix) {
	e->setMaybe<Int>(m_targetProperties[AbsoluteTimeValue], absTime);
	e->setMaybe<Int>(m_targetProperties[DurationValue], duration);
    } else {
	s->erase(i);
	m_toInsert.push_back(e);
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "m_toInsert.size() is now " << m_toInsert.size() << endl;
#endif
}

void
Quantizer::removeProperties(Event *e) const
{
    if (m_source != RawEventData) {
	e->unset(m_sourceProperties[AbsoluteTimeValue]);
	e->unset(m_sourceProperties[DurationValue]);
    }

    if (m_target != RawEventData && m_target != NotationPrefix) {
	e->unset(m_targetProperties[AbsoluteTimeValue]);
	e->unset(m_targetProperties[DurationValue]);
    }	
}

void
Quantizer::removeTargetProperties(Event *e) const
{
    if (m_target != RawEventData) {
	e->unset(m_targetProperties[AbsoluteTimeValue]);
	e->unset(m_targetProperties[DurationValue]);
    }	
}

void
Quantizer::makePropertyNames()
{
    if (m_source != RawEventData && m_source != NotationPrefix) {
	m_sourceProperties[AbsoluteTimeValue] = m_source + "AbsoluteTimeSource";
	m_sourceProperties[DurationValue]     = m_source + "DurationSource";
    }

    if (m_target != RawEventData && m_target != NotationPrefix) {
	m_targetProperties[AbsoluteTimeValue] = m_target + "AbsoluteTimeTarget";
	m_targetProperties[DurationValue]     = m_target + "DurationTarget";
    }
}	

void
Quantizer::insertNewEvents(Segment *s) const
{
    unsigned int sz = m_toInsert.size();

    timeT minTime = m_normalizeRegion.first,
	  maxTime = m_normalizeRegion.second;

    for (unsigned int i = 0; i < sz; ++i) {

	timeT myTime = m_toInsert[i]->getAbsoluteTime();
	timeT myDur  = m_toInsert[i]->getDuration();
	if (i == 0 || myTime < minTime) minTime = myTime;
	if (i == 0 || myTime + myDur > maxTime) maxTime = myTime + myDur;

	s->insert(m_toInsert[i]);
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "Quantizer::insertNewEvents: sz is " << sz
	      << ", minTime " << minTime << ", maxTime " << maxTime
	      << endl;
#endif

    if (m_target == NotationPrefix || m_target == RawEventData) {

	if (m_normalizeRegion.first == m_normalizeRegion.second) {
	    if (sz > 0) {
		s->normalizeRests(minTime, maxTime);
	    }
	} else {
	    s->normalizeRests(minTime, maxTime);
	    m_normalizeRegion = std::pair<timeT, timeT>(0, 0);
	}
    }
		
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer: calling normalizeRests("
		  << minTime << ", " << maxTime << ")" << endl;
#endif

    m_toInsert.clear();
}


BasicQuantizer::BasicQuantizer(timeT unit, bool doDurations,
			       int swing, int iterate) :
    Quantizer(RawEventData),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(std::string source, std::string target,
			       timeT unit, bool doDurations,
			       int swing, int iterate) :
    Quantizer(source, target),
    m_unit(unit),
    m_durations(doDurations),
    m_swing(swing),
    m_iterate(iterate)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

BasicQuantizer::BasicQuantizer(const BasicQuantizer &q) :
    Quantizer(q.m_target),
    m_unit(q.m_unit),
    m_durations(q.m_durations),
    m_swing(q.m_swing),
    m_iterate(q.m_iterate)
{
    // nothing else
}

BasicQuantizer::~BasicQuantizer()
{
    // nothing
}

void
BasicQuantizer::quantizeSingle(Segment *s, Segment::iterator i) const
{
    timeT d = getFromSource(*i, DurationValue);

    if (d == 0 && (*i)->isa(Note::EventType)) {
	s->erase(i);
	return;
    }

    if (m_unit == 0) return;

    timeT t = getFromSource(*i, AbsoluteTimeValue);
    timeT d0(d), t0(t);

    timeT barStart = s->getBarStartForTime(t);

    t -= barStart;

    int n = t / m_unit;
    timeT low = n * m_unit;
    timeT high = low + m_unit;
    timeT swingOffset = (m_unit * m_swing) / 300;

    if (high - t > t - low) {
	t = low;
    } else {
	t = high;
	++n;
    }

    if (n % 2 == 1) {
	t += swingOffset;
    }
    
    if (m_durations && d != 0) {

	low = (d / m_unit) * m_unit;
	high = low + m_unit;

	if (low > 0 && (high - d > d - low)) {
	    d = low;
	} else {
	    d = high;
	}

	int n1 = n + d / m_unit;

	if (n % 2 == 0) { // start not swung
	    if (n1 % 2 == 0) { // end not swung
		// do nothing
	    } else { // end swung
		d += swingOffset;
	    }
	} else { // start swung
	    if (n1 % 2 == 0) { // end not swung
		d -= swingOffset;
	    } else {
		// do nothing
	    }
	}
    }
	
    t += barStart;

    timeT t1(t), d1(d);
    t = (t - t0) * m_iterate / 100 + t0;
    d = (d - d0) * m_iterate / 100 + d0;

    // if an iterative quantize results in something much closer than
    // the shortest actual note resolution we have, just snap it
    if (m_iterate != 100) {
	timeT close = Note(Note::Shortest).getDuration()/2;
	if (t >= t1 - close && t <= t1 + close) t = t1;
	if (d >= d1 - close && d <= d1 + close) d = d1;
    }

    if (t0 != t || d0 != d) setToTarget(s, i, t, d);
}


std::vector<timeT>
BasicQuantizer::getStandardQuantizations()
{
    checkStandardQuantizations();
    return m_standardQuantizations;
}

void
BasicQuantizer::checkStandardQuantizations()
{
    if (m_standardQuantizations.size() > 0) return;

    for (Note::Type nt = Note::Semibreve; nt >= Note::Shortest; --nt) {

	int i1 = (nt < Note::Quaver ? 1 : 0);
	for (int i = 0; i <= i1; ++i) {
	    
	    int divisor = (1 << (Note::Semibreve - nt));
	    if (i) divisor = divisor * 3 / 2;

	    timeT unit = Note(Note::Semibreve).getDuration() / divisor;
	    m_standardQuantizations.push_back(unit);
	}
    }
}    

timeT
BasicQuantizer::getStandardQuantization(Segment *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    for (Segment::iterator i = s->begin(); s->isBeforeEndMarker(i); ++i) {
	
	if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
	timeT myUnit = getUnitFor(*i);
	if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return unit;
}

timeT
BasicQuantizer::getStandardQuantization(EventSelection *s)
{
    checkStandardQuantizations();
    timeT unit = -1;

    if (!s) return 0;

    for (EventSelection::eventcontainer::iterator i =
	     s->getSegmentEvents().begin();
	 i != s->getSegmentEvents().end(); ++i) {
	
	if (!(*i)->isa(Rosegarden::Note::EventType)) continue;
	timeT myUnit = getUnitFor(*i);
	if (unit < 0 || myUnit < unit) unit = myUnit;
    }

    return unit;
}

timeT
BasicQuantizer::getUnitFor(Event *e)
{	    
    timeT absTime = e->getAbsoluteTime();
    timeT myQuantizeUnit = 0;
    
    // m_quantizations is in descending order of duration;
    // stop when we reach one that divides into the note's time
    
    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
	if (absTime % m_standardQuantizations[i] == 0) {
	    myQuantizeUnit = m_standardQuantizations[i];
	    break;
	}
    }

    return myQuantizeUnit;
}

std::vector<timeT>
BasicQuantizer::m_standardQuantizations;


LegatoQuantizer::LegatoQuantizer(timeT unit) :
    Quantizer(RawEventData),
    m_unit(unit)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

LegatoQuantizer::LegatoQuantizer(std::string source, std::string target, timeT unit) :
    Quantizer(source, target),
    m_unit(unit)
{
    if (m_unit < 0) m_unit = Note(Note::Shortest).getDuration();
}

LegatoQuantizer::LegatoQuantizer(const LegatoQuantizer &q) :
    Quantizer(q.m_target),
    m_unit(q.m_unit)
{
    // nothing else
}

LegatoQuantizer::~LegatoQuantizer()
{
    // nothing
}

void
LegatoQuantizer::quantizeRange(Segment *s,
			       Segment::iterator from,
			       Segment::iterator to) const
{
    Segment::iterator tmp;
    while (from != to) {
	quantizeSingle(s, from, tmp);
	from = tmp;
	if (!s->isBeforeEndMarker(from) ||
	    (s->isBeforeEndMarker(to) &&
	     ((*from)->getAbsoluteTime() >= (*to)->getAbsoluteTime()))) break;
    }
}

void
LegatoQuantizer::quantizeSingle(Segment *s, Segment::iterator i,
				Segment::iterator &nexti) const
{
    // Stretch each note out to reach the quantized start time of the
    // next note whose quantized start time is greater than or equal
    // to the end time of this note after quantization

    timeT t = getFromSource(*i, AbsoluteTimeValue);
    timeT d = getFromSource(*i, DurationValue);

    timeT d0(d), t0(t);

    timeT barStart = s->getBarStartForTime(t);

    t -= barStart;
    t = quantizeTime(t);
    t += barStart;

    nexti = i;
    ++nexti;

    for (Segment::iterator j = i; s->isBeforeEndMarker(j); ++j) {
	if (!(*j)->isa(Note::EventType)) continue;
	
	timeT qt = (*j)->getAbsoluteTime();
	qt -= barStart;
	qt = quantizeTime(qt);
	qt += barStart;

	if (qt >= t + d) {
	    d = qt - t;
	}
	if (qt > t) {
	    break;
	}
    }
    
    if (t0 != t || d0 != d) {
	setToTarget(s, i, t, d);
	nexti = s->findTime(t + d);
    }
}

timeT
LegatoQuantizer::quantizeTime(timeT t) const
{
    if (m_unit != 0) {
	timeT low = (t / m_unit) * m_unit;
	timeT high = low + m_unit;
	t = ((high - t > t - low) ? low : high);
    }
    return t;
}


class NotationQuantizer::Impl
{
public:
    Impl(NotationQuantizer *const q) :
	m_unit(Note(Note::Demisemiquaver).getDuration()),
	m_simplicityFactor(13),
	m_maxTuplet(3),
	m_articulate(true),
	m_q(q),
	m_provisionalBase("notationquantizer-provisionalBase"),
	m_provisionalAbsTime("notationquantizer-provisionalAbsTime"),
	m_provisionalDuration("notationquantizer-provisionalDuration"),
	m_provisionalNoteType("notationquantizer-provisionalNoteType"),
	m_provisionalScore("notationquantizer-provisionalScore")
    { }

    Impl(const Impl &i) :
	m_unit(i.m_unit),
	m_simplicityFactor(i.m_simplicityFactor),
	m_maxTuplet(i.m_maxTuplet),
	m_articulate(i.m_articulate),
	m_q(i.m_q),
	m_provisionalBase(i.m_provisionalBase),
	m_provisionalAbsTime(i.m_provisionalAbsTime),
	m_provisionalDuration(i.m_provisionalDuration),
	m_provisionalNoteType(i.m_provisionalNoteType),
	m_provisionalScore(i.m_provisionalScore)
    { }

    class ProvisionalQuantizer : public Quantizer {
	// This class exists only to pick out the provisional abstime
	// and duration values from half-quantized events, so that we
	// can treat them using the normal Chord class
    public:
	ProvisionalQuantizer(Impl *i) : Quantizer("blah", "blahblah"), m_impl(i) { }
	virtual timeT getQuantizedDuration(const Event *e) const {
	    return m_impl->getProvisional((Event *)e, DurationValue);
	}
	virtual timeT getQuantizedAbsoluteTime(const Event *e) const {
	    timeT t = m_impl->getProvisional((Event *)e, AbsoluteTimeValue);
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "ProvisionalQuantizer::getQuantizedAbsoluteTime: returning " << t << endl;
#endif
	    return t;
	}

    private:
	Impl *m_impl;
    };

    void quantizeRange(Segment *,
		       Segment::iterator,
		       Segment::iterator) const;

    void quantizeAbsoluteTime(Segment *, Segment::iterator) const;
    long scoreAbsoluteTimeForBase(Segment *, const Segment::iterator &,
				  int depth, timeT base, timeT sigTime,
				  timeT t, timeT d, int noteType,
				  const Segment::iterator &,
				  const Segment::iterator &,
				  bool &right) const;
    void quantizeDurationProvisional(Segment *, Segment::iterator) const;
    void quantizeDuration(Segment *, Chord &) const;

    void scanTupletsInBar(Segment *,
			  timeT barStart, timeT barDuration,
			  timeT wholeStart, timeT wholeDuration,
			  const std::vector<int> &divisions) const;
    void scanTupletsAt(Segment *, Segment::iterator, int depth,
		       timeT base, timeT barStart,
		       timeT tupletStart, timeT tupletBase) const;
    bool isValidTupletAt(Segment *, const Segment::iterator &,
			 int depth, timeT base, timeT sigTime,
			 timeT tupletBase) const;
    
    void setProvisional(Event *, ValueType value, timeT t) const;
    timeT getProvisional(Event *, ValueType value) const;
    void unsetProvisionalProperties(Event *) const;

    timeT m_unit;
    int m_simplicityFactor;
    int m_maxTuplet;
    bool m_articulate;
    bool m_contrapuntal;

private:
    NotationQuantizer *const m_q;

    PropertyName m_provisionalBase;
    PropertyName m_provisionalAbsTime;
    PropertyName m_provisionalDuration;
    PropertyName m_provisionalNoteType;
    PropertyName m_provisionalScore;
};

NotationQuantizer::NotationQuantizer() :
    Quantizer(NotationPrefix),
    m_impl(new Impl(this))
{
    // nothing else 
}

NotationQuantizer::NotationQuantizer(std::string source, std::string target) :
    Quantizer(source, target),
    m_impl(new Impl(this))
{
    // nothing else 
}

NotationQuantizer::NotationQuantizer(const NotationQuantizer &q) :
    Quantizer(q.m_target),
    m_impl(new Impl(*q.m_impl))
{
    // nothing else
}

NotationQuantizer::~NotationQuantizer()
{
    delete m_impl;
}

void
NotationQuantizer::setUnit(timeT unit) 
{
    m_impl->m_unit = unit;
}

timeT
NotationQuantizer::getUnit() const 
{
    return m_impl->m_unit;
}

void
NotationQuantizer::setMaxTuplet(int m) 
{
    m_impl->m_maxTuplet = m;
}

int
NotationQuantizer::getMaxTuplet() const 
{
    return m_impl->m_maxTuplet;
}

void
NotationQuantizer::setSimplicityFactor(int s) 
{
    m_impl->m_simplicityFactor = s;
}

int
NotationQuantizer::getSimplicityFactor() const 
{
    return m_impl->m_simplicityFactor;
}

void
NotationQuantizer::setContrapuntal(bool c) 
{
    m_impl->m_contrapuntal = c;
}

bool
NotationQuantizer::getContrapuntal() const 
{
    return m_impl->m_contrapuntal;
}

void
NotationQuantizer::setArticulate(bool a) 
{
    m_impl->m_articulate = a;
}

bool
NotationQuantizer::getArticulate() const 
{
    return m_impl->m_articulate;
}

void
NotationQuantizer::Impl::setProvisional(Event *e, ValueType v, timeT t) const
{
    if (v == AbsoluteTimeValue) {
	e->setMaybe<Int>(m_provisionalAbsTime, t);
    } else {
	e->setMaybe<Int>(m_provisionalDuration, t);
    }
}

timeT
NotationQuantizer::Impl::getProvisional(Event *e, ValueType v) const
{
    timeT t;
    if (v == AbsoluteTimeValue) {
	t = e->getAbsoluteTime();
	e->get<Int>(m_provisionalAbsTime, t);
    } else {
	t = e->getDuration();
	e->get<Int>(m_provisionalDuration, t);
    }
    return t;
}

void
NotationQuantizer::Impl::unsetProvisionalProperties(Event *e) const
{
    e->unset(m_provisionalBase);
    e->unset(m_provisionalAbsTime);
    e->unset(m_provisionalDuration);
    e->unset(m_provisionalNoteType);
    e->unset(m_provisionalScore);
}

void
NotationQuantizer::Impl::quantizeAbsoluteTime(Segment *s, Segment::iterator i) const
{
    Profiler profiler("NotationQuantizer::Impl::quantizeAbsoluteTime");

    Composition *comp = s->getComposition();
    
    TimeSignature timeSig;
    timeT t = m_q->getFromSource(*i, AbsoluteTimeValue);
    timeT sigTime = comp->getTimeSignatureAt(t, timeSig);

    timeT d = getProvisional(*i, DurationValue);
    int noteType = Note::getNearestNote(d).getNoteType();
    (*i)->setMaybe<Int>(m_provisionalNoteType, noteType);

    int maxDepth = 8 - noteType;
    if (maxDepth < 4) maxDepth = 4;
    std::vector<int> divisions;
    timeSig.getDivisions(maxDepth, divisions);
    if (timeSig == TimeSignature()) // special case for 4/4
	divisions[0] = 2;

    // At each depth of beat subdivision, we find the closest match
    // and assign it a score according to distance and depth.  The
    // calculation for the score should accord "better" scores to
    // shorter distance and lower depth, but it should avoid giving
    // a "perfect" score to any combination of distance and depth
    // except where both are 0.  Also, the effective depth is
    // 2 more than the value of our depth counter, which counts
    // from 0 at a point where the effective depth is already 1.
    
    timeT base = timeSig.getBarDuration();

    timeT bestBase = -2;
    long bestScore = 0;
    bool bestRight = false;

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "quantizeAbsoluteTime: t is " << t << ", d is " << d << endl;
#endif

    // scoreAbsoluteTimeForBase wants to know the previous starting
    // note (N) and the previous starting note that ends (roughly)
    // before this one starts (N').  Much more efficient to calculate
    // them once now before the loop.

    static timeT shortTime = Note(Note::Shortest).getDuration();
    
    Segment::iterator j(i);
    Segment::iterator n(s->end()), nprime(s->end());
    for (;;) {
	if (j == s->begin()) break;
	--j;
	if ((*j)->isa(Note::EventType)) {
	    if (n == s->end()) n = j;
	    if ((*j)->getAbsoluteTime() + (*j)->getDuration() + shortTime/2
		<= (*i)->getAbsoluteTime()) {
		nprime = j;
		break;
	    }
	}
    }
    
#ifdef DEBUG_NOTATION_QUANTIZER
    if (n != s->end() && n != nprime) {
	cout << "found n (distinct from nprime) at " << (*n)->getAbsoluteTime() << endl;
    }
    if (nprime != s->end()) {
	cout << "found nprime at " << (*nprime)->getAbsoluteTime()
	     << ", duration " << (*nprime)->getDuration() << endl;
    }
#endif

    for (int depth = 0; depth < maxDepth; ++depth) {

	base /= divisions[depth];
	if (base < m_unit) break;
	bool right = false;
	long score = scoreAbsoluteTimeForBase(s, i, depth, base, sigTime,
					      t, d, noteType, n, nprime, right);

	if (depth == 0 || score < bestScore) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << " [*]";
#endif
	    bestBase = base;
	    bestScore = score;
	    bestRight = right;
	}

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << endl;
#endif
    }

    if (bestBase == -2) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Quantizer::quantizeAbsoluteTime: weirdness: no snap found" << endl;
#endif
    } else {
	// we need to snap relative to the time sig, not relative
	// to the start of the whole composition
	t -= sigTime;

	t = (t / bestBase) * bestBase;
	if (bestRight) t += bestBase;

/*
	timeT low = (t / bestBase) * bestBase;
	timeT high = low + bestBase;
	t = ((high - t > t - low) ? low : high);
*/

	t += sigTime;
	
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "snap base is " << bestBase << ", snapped to " << t << endl;
#endif
    }

    setProvisional(*i, AbsoluteTimeValue, t);
    (*i)->setMaybe<Int>(m_provisionalBase, bestBase);
    (*i)->setMaybe<Int>(m_provisionalScore, bestScore);
}

long
NotationQuantizer::Impl::scoreAbsoluteTimeForBase(Segment *s,
						  const Segment::iterator & /* i */,
						  int depth,
						  timeT base,
						  timeT sigTime,
						  timeT t,
						  timeT d,
						  int noteType,
						  const Segment::iterator &n,
						  const Segment::iterator &nprime,
						  bool &wantRight)
    const
{
    Profiler profiler("NotationQuantizer::Impl::scoreAbsoluteTimeForBase");

    // Lower score is better.
    
    static timeT shortTime = Note(Note::Shortest).getDuration();

    double simplicityFactor(m_simplicityFactor);
    simplicityFactor -= Note::Crotchet - noteType;
    if (simplicityFactor < 10) simplicityFactor = 10;

    double effectiveDepth = pow(depth + 2, simplicityFactor / 10);

    //!!! use velocity to adjust the effective depth as well? -- louder
    // notes are more likely to be on big boundaries.  Actually, perhaps
    // introduce a generally-useful "salience" score a la Dixon et al

    long leftScore = 0;

    for (int ri = 0; ri < 2; ++ri) {

	bool right = (ri == 1);

	long distance = (t - sigTime) % base;
	if (right) distance = base - distance;
	long score = long((distance + shortTime / 2) * effectiveDepth);
    
	double penalty1 = 1.0;
    
	// seriously penalise moving a note beyond its own end time
	if (d > 0 && right && distance >= d * 0.9) {
	    penalty1 = double(distance) / d + 0.5;
	}
    
	double penalty2 = 1.0;

	// Examine the previous starting note (N), and the previous
	// starting note that ends before this one starts (N').
    
	// We should penalise moving this note to before the performed end
	// of N' and seriously penalise moving it to the same quantized
	// start time as N' -- but we should encourage moving it to the
	// same time as the provisional end of N', or to the same start
	// time as N if N != N'.
	
	if (!right) {
	    if (n != s->end()) {
		if (n != nprime) {
		    timeT nt = getProvisional(*n, AbsoluteTimeValue);
		    if (t - distance == nt) penalty2 = penalty2 * 2 / 3;
		}
		if (nprime != s->end()) {
		    timeT npt = getProvisional(*nprime, AbsoluteTimeValue);
		    timeT npd = getProvisional(*nprime, DurationValue);
		    if (t - distance <= npt) penalty2 *= 4;
		    else if (t - distance < npt + npd) penalty2 *= 2;
		    else if (t - distance == npt + npd) penalty2 = penalty2 * 2 / 3;
		}
	    }
	}
    
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "  depth/eff/dist/t/score/pen1/pen2/res: " << depth << "/" << effectiveDepth << "/" << distance << "/" << (right ? t + distance : t - distance) << "/" << score << "/" << penalty1 << "/" << penalty2 << "/" << (score * penalty1 * penalty2);
	if (right) cout << " -> ";
	else cout << " <- ";
	if (ri == 0) cout << endl;
#endif
    
	score = long(score * penalty1);
	score = long(score * penalty2);
    
	if (ri == 0) {
	    leftScore = score;
	} else {
	    if (score < leftScore) {
		wantRight = true;
		return score;
	    } else {
		wantRight = false;
		return leftScore;
	    }
	}
    }

    return leftScore;
}
    
void
NotationQuantizer::Impl::quantizeDurationProvisional(Segment *, Segment::iterator i)
    const
{
    Profiler profiler("NotationQuantizer::Impl::quantizeDurationProvisional");

    // Calculate a first guess at the likely notation duration based
    // only on its performed duration, without considering start time.

    timeT d = m_q->getFromSource(*i, DurationValue);
    if (d == 0) {
	setProvisional(*i, DurationValue, d);
	return;
    }

    Note shortNote = Note::getNearestNote(d, 2);

    timeT shortTime = shortNote.getDuration();
    timeT time = shortTime;

    if (shortTime != d) {

	Note longNote(shortNote);

	if ((shortNote.getDots() > 0 ||
	     shortNote.getNoteType() == Note::Shortest)) { // can't dot that
	    
	    if (shortNote.getNoteType() < Note::Longest) {
		longNote = Note(shortNote.getNoteType() + 1, 0);
	    }
	
	} else {
	    longNote = Note(shortNote.getNoteType(), 1);
	}
	
	timeT longTime = longNote.getDuration();
	
	// we should prefer to round up to a note with fewer dots rather
	// than down to one with more

	//!!! except in dotted time, etc -- we really want this to work on a
	// similar attraction-to-grid basis to the abstime quantization

	if ((longNote.getDots() + 1) * (longTime - d) <
	    (shortNote.getDots() + 1) * (d - shortTime)) {
	    time = longTime;
	}
    }

    setProvisional(*i, DurationValue, time);

    if ((*i)->has(BEAMED_GROUP_TUPLET_BASE)) {
	// We're going to recalculate these, and use our own results
	(*i)->unset(BEAMED_GROUP_ID);
	(*i)->unset(BEAMED_GROUP_TYPE);
	(*i)->unset(BEAMED_GROUP_TUPLET_BASE);
	(*i)->unset(BEAMED_GROUP_TUPLED_COUNT);
	(*i)->unset(BEAMED_GROUP_UNTUPLED_COUNT);
//!!!	(*i)->unset(TUPLET_NOMINAL_DURATION);
    }
}

void
NotationQuantizer::Impl::quantizeDuration(Segment *s, Chord &c) const
{
    static int totalFracCount = 0;
    static float totalFrac = 0;

    Profiler profiler("NotationQuantizer::Impl::quantizeDuration");

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "quantizeDuration: chord has " << c.size() << " notes" << endl;
#endif

    Composition *comp = s->getComposition();
    
    TimeSignature timeSig;
//    timeT t = m_q->getFromSource(*c.getInitialElement(), AbsoluteTimeValue);
//    timeT sigTime = comp->getTimeSignatureAt(t, timeSig);

    timeT d = getProvisional(*c.getInitialElement(), DurationValue);
    int noteType = Note::getNearestNote(d).getNoteType();
    int maxDepth = 8 - noteType;
    if (maxDepth < 4) maxDepth = 4;
    std::vector<int> divisions;
    timeSig.getDivisions(maxDepth, divisions);

    Segment::iterator nextNote = c.getNextNote();
    timeT nextNoteTime =
	(s->isBeforeEndMarker(nextNote) ?
	 getProvisional(*nextNote, AbsoluteTimeValue) :
	 s->getEndMarkerTime());

    timeT nonContrapuntalDuration = 0;
    
    for (Chord::iterator ci = c.begin(); ci != c.end(); ++ci) {

	if (!(**ci)->isa(Note::EventType)) continue;
	if ((**ci)->has(m_provisionalDuration) &&
	    (**ci)->has(BEAMED_GROUP_TUPLET_BASE)) {
	    // dealt with already in tuplet code, we'd only mess it up here
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "not recalculating duration for tuplet" << endl;
#endif
	    continue;
	}
	
	timeT ud = 0;

	if (!m_contrapuntal) {
	    // if not contrapuntal, give all notes in chord equal duration
	    if (nonContrapuntalDuration > 0) {
#ifdef DEBUG_NOTATION_QUANTIZER 
		cout << "setting duration trivially to " << nonContrapuntalDuration << endl;
#endif
		setProvisional(**ci, DurationValue, nonContrapuntalDuration);
		continue;
	    } else {
		// establish whose duration to use, then set it at the
		// bottom after it's been quantized
		Segment::iterator li = c.getLongestElement();
		if (li != s->end()) ud = m_q->getFromSource(*li, DurationValue);
		else ud = m_q->getFromSource(**ci, DurationValue);
	    }
	} else {
	    ud = m_q->getFromSource(**ci, DurationValue);
	}

	timeT qt = getProvisional(**ci, AbsoluteTimeValue);

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "note at time " << (**ci)->getAbsoluteTime() << " (provisional time " << qt << ")" << endl;
#endif

	timeT base = timeSig.getBarDuration();
	std::pair<timeT, timeT> bases;
	for (int depth = 0; depth < maxDepth; ++depth) {
	    if (base >= ud) {
		bases = std::pair<timeT, timeT>(base / divisions[depth], base);
	    }
	    base /= divisions[depth];
	}

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "duration is " << ud << ", probably between "
		  << bases.first << " and " << bases.second << endl;
#endif

	timeT qd = getProvisional(**ci, DurationValue);

	timeT spaceAvailable = nextNoteTime - qt;
	
	if (spaceAvailable > 0) {
	    float frac = float(ud) / float(spaceAvailable);
	    totalFrac += frac;
	    totalFracCount += 1;
	}

	if (!m_contrapuntal && qd > spaceAvailable) {

	    qd = Note::getNearestNote(spaceAvailable).getDuration();

#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "non-contrapuntal segment, rounded duration down to "
		 << qd << " (as only " << spaceAvailable << " available)"
		 << endl;
#endif

	} else {

	    //!!! Note longer than the longest note we have.  Deal with
	    //this -- how?  Quantize the end time?  Split the note?
	    //(Prefer to do that in a separate phase later if requested.)
	    //Leave it as it is?  (Yes, for now.)
	    if (bases.first == 0) return;
	    
	    timeT absTimeBase = bases.first;
	    (**ci)->get<Int>(m_provisionalBase, absTimeBase);

	    spaceAvailable = std::min(spaceAvailable, 
				      comp->getBarEndForTime(qt) - qt);

	    // We have a really good possibility of staccato if we have a
	    // note on a boundary whose base is double the note duration
	    // and there's nothing else until the next boundary and we're
	    // shorter than about a quaver (i.e. the base is a quaver or
	    // less)
	    
	    if (qd*2 <= absTimeBase && (qd*8/3) >= absTimeBase &&
		bases.second == absTimeBase) {
		
		if (nextNoteTime >= qt + bases.second) {
#ifdef DEBUG_NOTATION_QUANTIZER
		    cout << "We rounded to " << qd
			 << " but we're on " << absTimeBase << " absTimeBase"
			 << " and the next base is " << bases.second
			 << " and we have room for it, so"
			 << " rounding up again" << endl;
#endif
		    qd = bases.second;
		}
		
	    } else {
		
		// Alternatively, if we rounded down but there's space to
		// round up, consider doing so
		
		//!!! mark staccato if necessary, and take existing marks into account
		
		Note note(Note::getNearestNote(qd));
		
		if (qd < ud || (qd == ud && note.getDots() == 2)) {
		    
		    if (note.getNoteType() < Note::Longest) {
			
			if (bases.second <= spaceAvailable) {
#ifdef DEBUG_NOTATION_QUANTIZER
			    cout << "We rounded down to " << qd
				 << " but have room for " << bases.second
				 << ", rounding up again" << endl;
#endif
			    qd = bases.second;
			} else {
#ifdef DEBUG_NOTATION_QUANTIZER
			    cout << "We rounded down to " << qd
				 << "; can't fit " << bases.second << endl;
#endif
			}			
		    }
		}
	    }
	}

	setProvisional(**ci, DurationValue, qd);
	if (!m_contrapuntal) nonContrapuntalDuration = qd;
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "totalFrac " << totalFrac << ", totalFracCount " << totalFracCount << ", avg " << (totalFracCount > 0 ? (totalFrac / totalFracCount) : 0) << endl;
#endif
}


void
NotationQuantizer::Impl::scanTupletsInBar(Segment *s,
					  timeT barStart,
					  timeT barDuration,
					  timeT wholeStart,
					  timeT wholeEnd,
					  const std::vector<int> &divisions) const
{
    Profiler profiler("NotationQuantizer::Impl::scanTupletsInBar");

    //!!! need to further constrain the area scanned so as to cope with
    // partial bars

    timeT base = barDuration;

    for (int depth = -1; depth < int(divisions.size()) - 2; ++depth) {

	if (depth >= 0) base /= divisions[depth];
	if (base <= Note(Note::Semiquaver).getDuration()) break;

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nscanTupletsInBar: trying at depth " << depth << " (base " << base << ")" << endl;
#endif

	// check for triplets if our next divisor is 2 and the following
	// one is not 3

	if (divisions[depth+1] != 2 || divisions[depth+2] == 3) continue;

	timeT tupletBase = base / 3;
	timeT tupletStart = barStart;

	while (tupletStart < barStart + barDuration) {

	    timeT tupletEnd = tupletStart + base;
	    if (tupletStart < wholeStart || tupletEnd > wholeEnd) {
		tupletStart = tupletEnd;
		continue;
	    }

#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "scanTupletsInBar: testing " << tupletStart << "," << base << " at tuplet base " << tupletBase << endl;
#endif

	    // find first note within a certain distance whose start time
	    // quantized to tupletStart or greater
	    Segment::iterator j = s->findTime(tupletStart - tupletBase / 3);
	    timeT jTime = tupletEnd;

	    while (s->isBeforeEndMarker(j) &&
		   (!(*j)->isa(Note::EventType) ||
		    !(*j)->get<Int>(m_provisionalAbsTime, jTime) ||
		    jTime < tupletStart)) {
		if ((*j)->getAbsoluteTime() > tupletEnd + tupletBase / 3) {
		    break;
		}
		++j;
	    }

	    if (jTime >= tupletEnd) { // nothing to make tuplets of
#ifdef DEBUG_NOTATION_QUANTIZER
		cout << "scanTupletsInBar: nothing here" << endl;
#endif
		tupletStart = tupletEnd;
		continue;
	    }

	    scanTupletsAt(s, j, depth+1, base, barStart,
			  tupletStart, tupletBase);

	    tupletStart = tupletEnd;
	}
    }
}
	

void
NotationQuantizer::Impl::scanTupletsAt(Segment *s,
				       Segment::iterator i,
				       int depth,
				       timeT base,
				       timeT sigTime,
				       timeT tupletStart,
				       timeT tupletBase) const
{
    Profiler profiler("NotationQuantizer::Impl::scanTupletsAt");

    Segment::iterator j = i;
    timeT tupletEnd = tupletStart + base;
    timeT jTime = tupletEnd;

    std::vector<Event *> candidates;
    int count = 0;

    while (s->isBeforeEndMarker(j) &&
	   ((*j)->isa(Note::EventRestType) ||
	    ((*j)->get<Int>(m_provisionalAbsTime, jTime) &&
	     jTime < tupletEnd))) {
	
	if (!(*j)->isa(Note::EventType)) { ++j; continue; }

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "scanTupletsAt time " << jTime << " (unquantized "
	     << (*j)->getAbsoluteTime() << "), found note" << endl;
#endif

	// reject any group containing anything already a tuplet
	if ((*j)->has(BEAMED_GROUP_TUPLET_BASE)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "already made tuplet here" << endl;
#endif
	    return;
	}

	timeT originalBase;

	if (!(*j)->get<Int>(m_provisionalBase, originalBase)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "some notes not provisionally quantized, no good" << endl;
#endif
	    return;
	}

	if (originalBase == base) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "accepting note at original base" << endl;
#endif
	    candidates.push_back(*j);
	} else if (((jTime - sigTime) % base) == 0) {
#ifdef DEBUG_NOTATION_QUANTIZER
	    cout << "accepting note that happens to lie on original base" << endl;
#endif
	    candidates.push_back(*j);
	} else {

	    // This is a note that did not quantize to the original base
	    // (the first note in the tuplet would have, but we can't tell
	    // anything from that).  Reject the entire group if it fails
	    // any of the likelihood tests for tuplets.

	    if (!isValidTupletAt(s, j, depth, base, sigTime, tupletBase)) {
#ifdef DEBUG_NOTATION_QUANTIZER
		cout << "no good" << endl;
#endif
		return;
	    }

	    candidates.push_back(*j);
	    ++count;
	}

	++j;
    }

    // must have at least one note that is not already quantized to the
    // original base
    if (count < 1) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "scanTupletsAt: found no note not already quantized to " << base << endl;
#endif
	return;
    }

#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "scanTupletsAt: Tuplet group of duration " << base << " starting at " << tupletStart << endl;
#endif

    // Woo-hoo!  It looks good.

    int groupId = s->getNextId();
    std::map<int, bool> multiples;

    for (std::vector<Event *>::iterator ei = candidates.begin();
	 ei != candidates.end(); ++ei) {

	//!!! Interesting -- we can't modify rests here, but Segment's
	// normalizeRests won't insert the correct sort of rest for us...
	// what to do?
	//!!! insert a tupleted rest, and prevent Segment::normalizeRests
	// from messing about with it
	if (!(*ei)->isa(Note::EventType)) continue;
	(*ei)->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);

	//!!! This is too easy, because we rejected any notes of
	//durations not conforming to a single multiple of the
	//tupletBase in isValidTupletAt

	(*ei)->set<Int>(BEAMED_GROUP_ID, groupId);
	(*ei)->set<Int>(BEAMED_GROUP_TUPLET_BASE, base/2); //!!! wrong if tuplet count != 3
	(*ei)->set<Int>(BEAMED_GROUP_TUPLED_COUNT, 2); //!!! as above
	(*ei)->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, base/tupletBase);

	timeT t = (*ei)->getAbsoluteTime();
	t -= tupletStart;
	timeT low = (t / tupletBase) * tupletBase;
	timeT high = low + tupletBase;
	t = ((high - t > t - low) ? low : high);

	multiples[t / tupletBase] = true;

	t += tupletStart;

	setProvisional(*ei, AbsoluteTimeValue, t);
	setProvisional(*ei, DurationValue, tupletBase);
    }

    // fill in with tupleted rests

    for (int m = 0; m < base / tupletBase; ++m) {

	if (multiples[m]) continue;

	timeT absTime = tupletStart + m * tupletBase;
	timeT duration = tupletBase;
//!!!	while (multiples[++m]) duration += tupletBase;

	Event *rest = new Event(Note::EventRestType, absTime, duration);

	rest->set<String>(BEAMED_GROUP_TYPE, GROUP_TYPE_TUPLED);
	rest->set<Int>(BEAMED_GROUP_ID, groupId);
	rest->set<Int>(BEAMED_GROUP_TUPLET_BASE, base/2); //!!! wrong if tuplet count != 3
	rest->set<Int>(BEAMED_GROUP_TUPLED_COUNT, 2); //!!! as above
	rest->set<Int>(BEAMED_GROUP_UNTUPLED_COUNT, base/tupletBase);

	m_q->m_toInsert.push_back(rest);
    }
}

bool
NotationQuantizer::Impl::isValidTupletAt(Segment *s,
					 const Segment::iterator &i,
					 int depth,
					 timeT /* base */,
					 timeT sigTime,
					 timeT tupletBase) const
{
    Profiler profiler("NotationQuantizer::Impl::isValidTupletAt");

    //!!! This is basically wrong; we need to be able to deal with groups
    // that contain e.g. a crotchet and a quaver, tripleted.

    timeT ud = m_q->getFromSource(*i, DurationValue);

    if (ud > (tupletBase * 5 / 4)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nNotationQuantizer::isValidTupletAt: note too long at "
	     << (*i)->getDuration() << " (tupletBase is " << tupletBase << ")"
	     << endl;
#endif
	return false; // too long
    }

    //!!! This bit is a cop-out.  It means we reject anything that looks
    // like it's going to have rests in it.  Bah.
    if (ud <= (tupletBase * 3 / 8)) {
#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "\nNotationQuantizer::isValidTupletAt: note too short at "
	     << (*i)->getDuration() << " (tupletBase is " << tupletBase << ")"
	     << endl;
#endif
	return false;
    }

    long score = 0;
    if (!(*i)->get<Int>(m_provisionalScore, score)) return false;

    timeT t = m_q->getFromSource(*i, AbsoluteTimeValue);
    timeT d = getProvisional(*i, DurationValue);
    int noteType = (*i)->get<Int>(m_provisionalNoteType);

    //!!! not as complete as the calculation we do in the original scoring
    bool dummy;
    long tupletScore = scoreAbsoluteTimeForBase
	(s, i, depth, tupletBase, sigTime, t, d, noteType, s->end(), s->end(), dummy);
#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "\nNotationQuantizer::isValidTupletAt: score " << score
	 << " vs tupletScore " << tupletScore << endl;
#endif
    return (tupletScore < score);
}
				 

void
NotationQuantizer::quantizeRange(Segment *s,
				 Segment::iterator from,
				 Segment::iterator to) const
{
    m_impl->quantizeRange(s, from, to);
}

void
NotationQuantizer::Impl::quantizeRange(Segment *s,
				       Segment::iterator from,
				       Segment::iterator to) const
{
    Profiler *profiler = new Profiler("NotationQuantizer::Impl::quantizeRange");

    clock_t start = clock();
    int events = 0, notes = 0, passes = 0;
    int setGood = 0, setBad = 0;
    
//#ifdef DEBUG_NOTATION_QUANTIZER
    cout << "NotationQuantizer::Impl::quantizeRange: from time "
	      << (from == s->end() ? -1 : (*from)->getAbsoluteTime())
	      << " to "
	      << (to == s->end() ? -1 : (*to)->getAbsoluteTime())
	      << endl;
//#endif

    // This process does several passes over the data.  It's assumed
    // that this is not going to be invoked in any really time-critical
    // place.

    // Calculate absolute times on the first pass, so that we know
    // which things are chords.  We need to assign absolute times to
    // all events, but we only need do durations for notes.

    PropertyName provisionalBase("notationquantizer-provisionalBase");

    // We don't use setToTarget until we have our final values ready,
    // as it erases and replaces the events.  Just set the properties.

    // Set a provisional duration to each note first

    for (Segment::iterator i = from; i != to; ++i) {

	++events;
	if ((*i)->isa(Note::EventRestType)) continue;
	if ((*i)->isa(Note::EventType)) ++notes;
	quantizeDurationProvisional(s, i);
    }
    ++passes;

    // now do the absolute-time calculation

    timeT wholeStart = 0, wholeEnd = 0;

    Segment::iterator i = from;

    for (Segment::iterator nexti = i; i != to; i = nexti) {

	++nexti;

	if ((*i)->isa(Note::EventRestType)) {
	    if (i == from) ++from;
	    s->erase(i);
	    continue;
	}

	quantizeAbsoluteTime(s, i);

	timeT t0 = (*i)->get<Int>(m_provisionalAbsTime);
	timeT t1 = (*i)->get<Int>(m_provisionalDuration) + t0;
	if (wholeStart == wholeEnd) {
	    wholeStart = t0;
	    wholeEnd = t1;
	} else if (t1 > wholeEnd) {
	    wholeEnd = t1;
	}
    }
    ++passes;

    // now we've grouped into chords, look for tuplets next

    Composition *comp = s->getComposition();

    if (m_maxTuplet >= 2) {

	std::vector<int> divisions;
	comp->getTimeSignatureAt(wholeStart).getDivisions(7, divisions);

	for (int barNo = comp->getBarNumber(wholeStart);
	     barNo <= comp->getBarNumber(wholeEnd); ++barNo) {

	    bool isNew = false;
	    TimeSignature timeSig = comp->getTimeSignatureInBar(barNo, isNew);
	    if (isNew) timeSig.getDivisions(7, divisions);
	    scanTupletsInBar(s, comp->getBarStart(barNo),
			     timeSig.getBarDuration(),
			     wholeStart, wholeEnd, divisions);
	}
	++passes;
    }
    
    ProvisionalQuantizer provisionalQuantizer((Impl *)this);

    for (i = from; i != to; ++i) {

	if (!(*i)->isa(Note::EventType)) continue;

	// could potentially supply clef and key here, but at the
	// moment Chord doesn't do anything with them (unlike
	// NotationChord) and we don't have any really clever
	// ideas for how to use them here anyway
//	Chord c(*s, i, m_q);
	Chord c(*s, i, &provisionalQuantizer);

	quantizeDuration(s, c);

	bool ended = false;
	for (Segment::iterator ci = c.getInitialElement();
	     s->isBeforeEndMarker(ci); ++ci) {
	    if (ci == to) ended = true;
	    if (ci == c.getFinalElement()) break;
	}
	if (ended) break;

	i = c.getFinalElement();
    }
    ++passes;

    // staccato (we now do slurs separately, in SegmentNotationHelper::autoSlur)

    if (m_articulate) {

	for (i = from; i != to; ++i) {

	    if (!(*i)->isa(Note::EventType)) continue;

	    timeT qd = getProvisional(*i, DurationValue);
	    timeT ud = m_q->getFromSource(*i, DurationValue);

	    if (ud < (qd * 3 / 4) &&
		qd <= Note(Note::Crotchet).getDuration()) {
		Marks::addMark(**i, Marks::Staccato, true);
	    } else if (ud > qd) {
		Marks::addMark(**i, Marks::Tenuto, true);
	    }	    
	}
	++passes;
    }

    i = from;

    for (Segment::iterator nexti = i; i != to; i = nexti) {

	++nexti;

	if ((*i)->isa(Note::EventRestType)) continue;

	timeT t = getProvisional(*i, AbsoluteTimeValue);
	timeT d = getProvisional(*i, DurationValue);

	unsetProvisionalProperties(*i);

	if ((*i)->getAbsoluteTime() == t &&
	    (*i)->getDuration() == d) ++setBad;
	else ++setGood;

#ifdef DEBUG_NOTATION_QUANTIZER
	cout << "Setting to target at " << t << "," << d << endl;
#endif

	m_q->setToTarget(s, i, t, d);
    }
    ++passes;

    cerr << "NotationQuantizer: " << events << " events ("
	 << notes << " notes), " << passes << " passes, "
	 << setGood << " good sets, " << setBad << " bad sets, "
	 << ((clock() - start) * 1000 / CLOCKS_PER_SEC) << "ms elapsed"
	 << endl;

    delete profiler; // on heap so it updates before the next line:
    Profiles::getInstance()->dump();

}	
    
    

}
