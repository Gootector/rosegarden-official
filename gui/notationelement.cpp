
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <qcanvas.h>
#include <algorithm>

#include "viewelementsmanager.h"
#include "notationelement.h"
#include "notationproperties.h"

#ifndef NDEBUG
#include "rosedebug.h"
#endif

#include "NotationTypes.h"
#include "Event.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;

NotationElement::NotationElement(Event *event)
    : ViewElement(event),
      m_x(0),
      m_y(0),
      m_canvasItem(0)
{
}

NotationElement::~NotationElement()
{
//     kdDebug(KDEBUG_AREA) << "~NotationElement()\n";

    delete m_canvasItem;
}

double
NotationElement::getEffectiveX() throw (NoCanvasItem)
{
    if (m_canvasItem)
        return m_canvasItem->x();
    else
        throw NoCanvasItem();
}

double
NotationElement::getEffectiveY() throw (NoCanvasItem)
{
    if (m_canvasItem)
        return m_canvasItem->y();
    else
        throw NoCanvasItem();
}

void
NotationElement::setNote(Note note)
{
    event()->setDuration(note.getDuration());
    event()->setMaybe<Int>(P_NOTE_TYPE, note.getNoteType());
    event()->setMaybe<Bool>(P_NOTE_DOTTED, note.isDotted());
}

Note
NotationElement::getNote() const
{
    Note::Type ntype = event()->get<Int>(P_NOTE_TYPE);
    bool isDotted = false;
    event()->get<Bool>(P_NOTE_DOTTED, isDotted);
    
    return Note(ntype, isDotted);
}


bool
NotationElement::isRest() const
{
    return event()->isa("rest");
}

bool
NotationElement::isNote() const
{
    return event()->isa(Note::EventType);
}

void
NotationElement::setCanvasItem(QCanvasItem *e, double dxoffset, double dyoffset)
{
    delete m_canvasItem;
    m_canvasItem = e;
    e->move(m_x + dxoffset, m_y + dyoffset);
}

//////////////////////////////////////////////////////////////////////

NotationElementList::~NotationElementList()
{
    for(iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
}

void
NotationElementList::erase(NotationElementList::iterator pos)
{
    delete *pos;
    multiset<NotationElement*, NotationElementCmp>::erase(pos);
}

NotationElementList::iterator
NotationElementList::findPrevious(const std::string &type, iterator i)

{
    //!!! what to return on failure? I think probably
    // end(), as begin() could be a success case
    if (i == begin()) return end();
    --i;
    for (;;) {
        if ((*i)->event()->isa(type)) return i;
        if (i == begin()) return end();
        --i;
    }
}

NotationElementList::iterator
NotationElementList::findNext(const std::string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(type); ++i);
    return i;
}

bool NotationElementList::hasSucceedingChordElements(iterator i)
{
    // find out whether there are any following chord elements
    int c = 0;
    Event::timeT myTime = (*i)->getAbsoluteTime();
    while (i != end() && (*i)->isNote() && (*i)->getAbsoluteTime() == myTime) {
        if (++c > 1) return true;
        ++i;
    }
    return false;
}

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream &dbg, NotationElement &e)
{
    dbg << "NotationElement - x : " << e.getLayoutX() << " - y : " << e.getLayoutY()
        << endl << *e.event() << endl;

//     e.event()->dump(cout);

    return dbg;
}

kdbgstream& operator<<(kdbgstream &dbg, NotationElementList &l)
{
    for(NotationElementList::const_iterator i = l.begin();
        i != l.end(); ++i) {
        dbg << *(*i) << endl;
    }

    return dbg;
}

#endif

#ifdef NOT_DEFINED

class PitchGreater {
public:
    bool operator()(const NotationElementList::iterator &a,
                    const NotationElementList::iterator &b) {
        try {
            return ((*a)->event()->get<Int>("pitch") <
                    (*b)->event()->get<Int>("pitch"));
        } catch (Event::NoData) {
            kdDebug(KDEBUG_AREA) << "Bad karma: PitchGreater failed to find one or both pitches" << endl;
            return false;
        }
    }
};
    

Chord::Chord(const NotationElementList &nel, NELIterator i,
             bool quantized) :
    m_nel(nel),
    m_initial(nel.end()),
    m_final(nel.end()),
    m_shortest(nel.end()),
    m_longest(nel.end()),
    m_quantized(quantized)
{
    if (i == nel.end()) return;

    NotationElementList::IteratorPair ipair
        (nel.findContainingSet<ChordMembershipTest>
         (i, ChordMembershipTest(*this, i)));

    m_initial = ipair.first;
    m_final = ipair.second;

    if (size() > 1) {
        stable_sort(begin(), end(), PitchGreater());
    }

    kdDebug(KDEBUG_AREA) << "Chord::Chord: pitches are:" << endl;
    for (unsigned int i = 0; i < size(); ++i) {
        kdDebug(KDEBUG_AREA) << ((*(*this)[i])->event()->get<Int>("pitch")) <<endl;
    }
}


Event::timeT Chord::duration(const NELIterator &i, bool quantized)
{
    if (quantized) {
        long d;
        bool done = (*i)->event()->get<Int>(P_QUANTIZED_DURATION, d);
        if (done) {
            return d;
        } else {
            Quantizer().quantize((*i)->event());
            return (*i)->event()->get<Int>(P_QUANTIZED_DURATION);
        }
    } else {
        return (*i)->event()->getDuration();
    }
}


void Chord::sample(const NELIterator &i)
{
    if (m_longest == m_nel.end() ||
        duration(i, m_quantized) > duration(m_longest, m_quantized)) {
        m_longest = i;
    }
    if (m_shortest == m_nel.end() ||
        duration(i, m_quantized) < duration(m_shortest, m_quantized)) {
        m_shortest = i;
    }

    push_back(i);
}    

Chord::~Chord() { }

#endif
