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

#ifndef _SETS_H_
#define _SETS_H_

#include <vector>

#include "Event.h"
#include "Segment.h"
#include "BaseProperties.h"
#include "NotationTypes.h"

namespace Rosegarden
{

class Quantizer;

/**
 * A "set" in Rosegarden terminology is a collection of elements found
 * in a container (indeed, a subset of that container) all of which
 * share a particular property and are located near to one another:
 * generally either contiguous or within the same bar.  The elements
 * are most usually Events and the container most usually a Segment,
 * and although this does not have to be the case (for other examples
 * see gui/notationsets.h), the elements do have to be convertible to
 * Events somehow.
 *
 * To construct a set requires (at least) a container reference plus
 * an iterator into that container.  The constructor (or more
 * precisely the initialise() method called by the constructor) then
 * scans the surrounding area of the list for the maximal set of
 * contiguous or within-the-same-bar elements before and after the
 * passed-in iterator such that all elements are in the same set
 * (i.e. Chord, BeamedGroup etc) as the one that the passed-in
 * iterator pointed to.
 *
 * The extents of the set within the list can then be discovered via
 * getInitialElement() and getFinalElement().  If the iterator passed
 * in to the constructor was at end() or did not point to an element
 * that could be a member of this kind of set, getInitialElement()
 * will return end(); if the passed-in iterator pointed to the only
 * member of this set, getInitialElement() and getFinalElement() will
 * be equal.
 *
 * These classes are not intended to be stored anywhere; all they
 * contain is iterators into the main container, and those might not
 * persist.  Instead you should create these on-the-fly when you want,
 * for example, to consider a note as part of a chord; and then you
 * should let them expire when you've finished with them.
 */

template <class Element, class Container>
class GenericSet // abstract base
{
public:
    typedef typename Container::iterator Iterator;

    virtual ~GenericSet() { }

    /**
     * getInitialElement() returns end() if there are no elements in
     * the set.  getInitialElement() == getFinalElement() if there is
     * only one element in the set
     */
    Iterator getInitialElement() const  { return m_initial;  }
    Iterator getFinalElement() const    { return m_final;    }

    /// only return note elements; will return end() if there are none
    Iterator getInitialNote() const;
    Iterator getFinalNote() const;

    /**
     * only elements with duration > 0 are candidates for shortest and
     * longest; these will return end() if there are no such elements
     */
    Iterator getLongestElement() const  { return m_longest;  }
    Iterator getShortestElement() const { return m_shortest; }

    /// these will return end() if there are no note elements in the set
    Iterator getHighestNote() const     { return m_highest;  }
    Iterator getLowestNote() const      { return m_lowest;   }

    virtual bool contains(const Iterator &) const = 0;

protected:
    GenericSet(const Container &c, Iterator elementInSet, const Quantizer *);
    void initialise();

    /// Return true if this element is not definitely beyond bounds of set
    virtual bool test(const Iterator &i) = 0;

    /// Return true if this element, known to test() true, is a set member
    virtual bool sample(const Iterator &i);

    /// Return the pointed-to element, in Event form
    static Event *getAsEvent(const Iterator &i);

    const Container &getContainer() const { return m_container; }
    const Quantizer &getQuantizer() const { return *m_quantizer; }

    // Data members:

    const Container &m_container;
    Iterator m_initial, m_final, m_shortest, m_longest, m_highest, m_lowest;
    Iterator m_baseIterator;
    const Quantizer *m_quantizer;
};


/**
 * Chord is subclassed from a vector of iterators; this vector
 * contains iterators pointing at all the notes in the chord, in
 * ascending order of pitch.  (You can also track through all the
 * notes in the chord by iterating from getInitialElement() to
 * getFinalElement(), but this will only get them in the order in
 * which they appear in the original container.)
 *
 * This class can tell you various things about the chord it
 * describes, but not everything.  It can't tell you whether the
 * chord has a stem, for example, because that depends on the
 * notation style and system in use.  See gui/notationsets.h
 * for a NotationChord class (subclassed from this) that can.
 */

template <class Element, class Container>
class GenericChord : public GenericSet<Element, Container>,
		     public std::vector<typename Container::iterator>
{
public:
    typedef typename Container::iterator Iterator;

    /* You only need to provide the clef and key if the notes
       making up your chord lack HEIGHT_ON_STAFF properties, in
       which case this constructor will write those properties
       in to the chord for you */
    GenericChord(const Container &c,
		 Iterator elementInChord,
		 const Quantizer *quantizer,
		 const Clef &clef = Clef::DefaultClef,
		 const Key &key = Key::DefaultKey);

    virtual ~GenericChord();

    virtual std::vector<Rosegarden::Mark> getMarksForChord() const;
    virtual bool contains(const Iterator &) const;

protected:
    virtual bool test(const Iterator&);
    virtual bool sample(const Iterator&);

    class PitchGreater {
    public:
	bool operator()(const Iterator &a, const Iterator &b);
    };

    //--------------- Data members ---------------------------------

    const Clef &m_clef;
    const Key &m_key;
    timeT m_time;
    int m_subordering;
};


// forward declare hack functions -- see Sets.C for an explanation

extern long
get__Int(Event *e, const PropertyName &name);

extern bool
get__Bool(Event *e, const PropertyName &name);

extern std::string
get__String(Event *e, const PropertyName &name);

extern bool
get__Int(Event *e, const PropertyName &name, long &ref);

extern bool
get__Bool(Event *e, const PropertyName &name, bool &ref);

extern bool
get__String(Event *e, const PropertyName &name, std::string &ref);

extern bool
isPersistent__Bool(Event *e, const PropertyName &name);


template <class Element, class Container>
GenericSet<Element, Container>::GenericSet(const Container &c,
					   Iterator i, const Quantizer *q):
    m_container(c),
    m_initial(c.end()),
    m_final(c.end()),
    m_shortest(c.end()),
    m_longest(c.end()),
    m_highest(c.end()),
    m_lowest(c.end()),
    m_baseIterator(i),
    m_quantizer(q)
{
    // ...
}

template <class Element, class Container>
void
GenericSet<Element, Container>::initialise()
{
    if (m_baseIterator == m_container.end() || !test(m_baseIterator)) return;
    m_initial = m_baseIterator;
    m_final = m_baseIterator;
    sample(m_baseIterator);

    Iterator i, j;

    // first scan back to find an element not in the desired set,
    // sampling everything as far back as the one after it

    for (i = j = m_baseIterator; i != m_container.begin() && test(--j); i = j){
        if (sample(j)) m_initial = j;
    }

    j = m_baseIterator;

    // then scan forwards to find an element not in the desired set,
    // sampling everything as far forward as the one before it

    for (i = j = m_baseIterator; ++j != m_container.end() && test(j); i = j) {
        if (sample(j)) m_final = j;
    }
}

template <class Element, class Container>
bool
GenericSet<Element, Container>::sample(const Iterator &i)
{
    const Quantizer &q(getQuantizer());
    timeT d(q.getQuantizedDuration(getAsEvent(i)));
    
    if (d > 0) {
        if (m_longest == m_container.end() ||
            d > q.getQuantizedDuration(getAsEvent(m_longest))) {
            m_longest = i;
        }
        if (m_shortest == m_container.end() ||
            d < q.getQuantizedDuration(getAsEvent(m_shortest))) {
            m_shortest = i;
        }
    }

    Event *e = getAsEvent(i);
    
    if (e->isa(Note::EventType)) {
//        long p = e->get<Int>(BaseProperties::PITCH);
        long p = get__Int(e, BaseProperties::PITCH);

        if (m_highest == m_container.end() ||
            p > get__Int(getAsEvent(m_highest), BaseProperties::PITCH)) {
            m_highest = i;
        }
        if (m_lowest == m_container.end() ||
            p < get__Int(getAsEvent(m_lowest), BaseProperties::PITCH)) {
            m_lowest = i;
        }
    }

    return true;
}

template <class Element, class Container>
typename GenericSet<Element, Container>::Iterator
GenericSet<Element, Container>::getInitialNote() const
{
    Iterator i(getInitialElement());
    if (getAsEvent(i)->isa(Note::EventType)) return i;

    Iterator j(getFinalElement());
    ++j;
    while (i != j) {
        if (getAsEvent(i)->isa(Note::EventType)) return i;
        ++i;
    }

    return getContainer().end();
}

template <class Element, class Container>
typename GenericSet<Element, Container>::Iterator
GenericSet<Element, Container>::getFinalNote() const
{
    Iterator i(getFinalElement());
    if (getAsEvent(i)->isa(Note::EventType)) return i;

    Iterator j(getInitialElement());
    --j;
    while (i != j) {
        if (getAsEvent(i)->isa(Note::EventType)) return i;
        --i;
    }

    return getContainer().end();
}

//////////////////////////////////////////////////////////////////////
 
template <class Element, class Container>
GenericChord<Element, Container>::GenericChord(const Container &c, Iterator i,
					       const Quantizer *q,
					       const Clef &clef,
					       const Key &key) :
    GenericSet<Element, Container>(c, i, q),
    m_clef(clef),
    m_key(key),
    m_time(q->getQuantizedAbsoluteTime(getAsEvent(i))),
    m_subordering(getAsEvent(i)->getSubOrdering())
{
    initialise();

    if (size() > 1) {
        std::stable_sort(begin(), end(), PitchGreater());
    }

/*!!! this should all be removed ultimately
//    std::cerr << "GenericChord::GenericChord: pitches are:" << std::endl;
    int prevPitch = -999;
    for (unsigned int i = 0; i < size(); ++i) {
        try {
	    int pitch = getAsEvent((*this)[i])->get<Int>(BaseProperties::PITCH);
//            std::cerr << i << ": " << pitch << std::endl;
	    if (pitch < prevPitch) {
		cerr << "ERROR: Pitch less than previous pitch (" << pitch
		     << " < " << prevPitch << ")" << std::endl;
		throw(1);
	    }
	} catch (Event::NoData) {
            std::cerr << i << ": no pitch property" << std::endl;
        }
    }
*/
}

template <class Element, class Container>
GenericChord<Element, Container>::~GenericChord()
{
}

template <class Element, class Container>
bool
GenericChord<Element, Container>::test(const Iterator &i)
{
    return (getAsEvent(i)->isa(Note::EventType) &&
	    getQuantizer().getQuantizedAbsoluteTime(getAsEvent(i)) == m_time &&
	    getAsEvent(i)->getSubOrdering() == m_subordering);
}

template <class Element, class Container>
bool
GenericChord<Element, Container>::sample(const Iterator &i)
{
    // two notes that would otherwise be in a chord but are not in
    // the same group, or have stems pointing in different directions
    // by design, count as separate chords

    if (m_baseIterator != getContainer().end()) {

	Event *e0 = getAsEvent(m_baseIterator);
	Event *e1 = getAsEvent(i);

	if (e0->has(BaseProperties::BEAMED_GROUP_ID)) {
	    if (e1->has(BaseProperties::BEAMED_GROUP_ID)) {
		if (get__Int(e1, BaseProperties::BEAMED_GROUP_ID) !=
		    get__Int(e0, BaseProperties::BEAMED_GROUP_ID)) {
		    return false;
		}
	    } else {
		return false;
	    }
	} else {
	    if (e1->has(BaseProperties::BEAMED_GROUP_ID)) {
		return false;
	    }
	}

	if (e0->has(BaseProperties::STEM_UP) &&
	    isPersistent__Bool(e0, BaseProperties::STEM_UP) &&

	    e1->has(BaseProperties::STEM_UP) &&
	    isPersistent__Bool(e1, BaseProperties::STEM_UP) &&

	    get__Bool(e0, BaseProperties::STEM_UP) !=
	    get__Bool(e1, BaseProperties::STEM_UP)) {

	    return false;
	}
    }

    GenericSet<Element, Container>::sample(i);
    push_back(i);
    return true;
}


template <class Element, class Container>
std::vector<Mark>
GenericChord<Element, Container>::getMarksForChord() const
{
    std::vector<Mark> marks;

    for (unsigned int i = 0; i < size(); ++i) {

	long markCount = 0;
	const Iterator &itr((*this)[i]);
	get__Int(getAsEvent(itr), MARK_COUNT, markCount);

	if (markCount == 0) continue;

	for (long j = 0; j < markCount; ++j) {

	    Mark mark(Marks::NoMark);
	    (void)get__String(getAsEvent(itr), getMarkPropertyName(j), mark);

	    unsigned int k;
	    for (k = 0; k < marks.size(); ++i) {
		if (marks[k] == mark) break;
	    }
	    if (k == marks.size()) {
		marks.push_back(mark);
	    }
	}
    }

    return marks;
}


template <class Element, class Container>
bool
GenericChord<Element, Container>::contains(const Iterator &itr) const
{
    for (typename std::vector<typename Container::iterator>::const_iterator
	     i = begin();
	 i != end(); ++i) {
        if (*i == itr) return true;
    }
    return false;
}
	
template <class Element, class Container>	
bool
GenericChord<Element, Container>::PitchGreater::operator()(const Iterator &a,
							   const Iterator &b)
{
    try {
	long ap = get__Int(getAsEvent(a), BaseProperties::PITCH);
	long bp = get__Int(getAsEvent(b), BaseProperties::PITCH);
	return (ap < bp);
    } catch (Event::NoData) {
	std::cerr << "Bad karma: PitchGreater failed to find one or both pitches" << std::endl;
	return false;
    }
}


typedef GenericChord<Event, Segment> Chord;


}


#endif

