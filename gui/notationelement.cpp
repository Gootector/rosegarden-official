
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

#include "viewelementsmanager.h"
#include "notationelement.h"

#ifndef NDEBUG

#include "rosedebug.h"
#endif

#include "NotationTypes.h"
#include "Event.h"

NotationElement::NotationElement(Event *event)
    : ViewElement(event),
      m_x(0),
      m_y(0),
      m_canvasItem(0)
{
}

NotationElement::~NotationElement()
{
    // de-register from "observer"
    delete m_canvasItem;
}

bool
NotationElement::isRest() const
{
    //???
    return event()->isa("core", "rest");
//    return event()->type() == "rest";
}

bool
NotationElement::isNote() const
{
    return event()->isa(Note::EventPackage, Note::EventType);
//    return event()->type() == "note";
}

void
NotationElement::setCanvasItem(QCanvasItem *e)
{
    delete m_canvasItem;
    m_canvasItem = e;
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
NotationElementList::findPrevious(const string &package,
                                  const string &type, iterator i)

{
    //!!! what to return on failure? I think probably
    // end(), as begin() could be a success case
    if (i == begin()) return end();
    --i;
    for (;;) {
        if ((*i)->event()->isa(package, type)) return i;
        if (i == begin()) return end();
        --i;
    }
}

NotationElementList::iterator
NotationElementList::findNext(const string &package,
                              const string &type, iterator i)
{
    if (i == end()) return i;
    for (++i; i != end() && !(*i)->event()->isa(package, type); ++i);
    return i;
}

#ifndef NDEBUG
kdbgstream& operator<<(kdbgstream &dbg, NotationElement &e)
{
    dbg << "NotationElement - x : " << e.x() << " - y : " << e.y()
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
