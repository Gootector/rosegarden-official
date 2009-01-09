// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2009 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#include <qdatastream.h>
#include "MappedComposition.h"
#include "MappedEvent.h"
#include "SegmentPerformanceHelper.h"
#include <iostream>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

MappedComposition::~MappedComposition()
{
    clear();
}

// copy constructor
MappedComposition::MappedComposition(const MappedComposition &mC):
        std::multiset<MappedEvent *, MappedEvent::MappedEventCmp>()
{
    clear();

    // deep copy
    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it));

}

// Turn a MappedComposition into a QDataStream - MappedEvents can
// stream themselves.
//
QDataStream&
operator<<(QDataStream &dS, MappedComposition *mC)
{
    dS << int(mC->size());

    for (MappedCompositionIterator it = mC->begin(); it != mC->end(); ++it )
        dS << (*it);

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
    dS << int(mC.size());

    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); ++it )
        dS << (*it);

    return dS;
}


// Turn a QDataStream into a MappedComposition
//
QDataStream&
operator>>(QDataStream &dS, MappedComposition *mC)
{
    int sliceSize;
    MappedEvent *mE;

    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize) {
        mE = new MappedEvent();
        dS >> mE;

        try {
            mC->insert(mE);
        } catch (...) {
            ;
        }

        sliceSize--;

    }

#ifdef DEBUG_MAPPEDCOMPOSITION
    if (sliceSize) {
        cerr << "operator>> - wrong number of events received" << endl;
    }
#endif

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedComposition &mC)
{
    int sliceSize;
    MappedEvent *mE;

    dS >> sliceSize;

    while (!dS.atEnd() && sliceSize) {
        mE = new MappedEvent();

        dS >> mE;

        try {
            mC.insert(mE);
        } catch (...) {
            ;
        }

        sliceSize--;

    }

#ifdef DEBUG_MAPPEDCOMPOSITION
    if (sliceSize) {
        cerr << "operator>> - wrong number of events received" << endl;
    }
#endif


    return dS;
}

// Move the start time of this MappedComposition and all its events.
// Actually - we have a special case for audio events at the moment..
//
//
void
MappedComposition::moveStartTime(const RealTime &mT)
{
    MappedCompositionIterator it;

    for (it = this->begin(); it != this->end(); ++it) {
        // Reset start time and duration
        //
        (*it)->setEventTime((*it)->getEventTime() + mT);
        (*it)->setDuration((*it)->getDuration() - mT);

        // For audio adjust the start index
        //
        if ((*it)->getType() == MappedEvent::Audio)
            (*it)->setAudioStartMarker((*it)->getAudioStartMarker() + mT);
    }

    m_startTime = m_startTime + mT;
    m_endTime = m_endTime + mT;

}


// Concatenate MappedComposition
//
MappedComposition&
MappedComposition::operator+(const MappedComposition &mC)
{
    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it)); // deep copy

    return *this;
}

// Assign (clear and deep copy)
//
MappedComposition&
MappedComposition::operator=(const MappedComposition &mC)
{
    if (&mC == this)
        return * this;

    clear();

    // deep copy
    for (MappedComposition::const_iterator it = mC.begin(); it != mC.end(); it++)
        this->insert(new MappedEvent(**it));

    return *this;
}

void
MappedComposition::clear()
{
    // Only clear if the events aren't persistent
    //
    for (MappedCompositionIterator it = this->begin(); it != this->end(); it++)
        if (!(*it)->isPersistent())
            delete (*it);

    this->erase(this->begin(), this->end());
}



}


