
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

#include <qdatastream.h>
#include "MappedComposition.h"
#include "MappedEvent.h"

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

// We use some globals here for speed - we're
// making a lot of these conversions when sending
// this class over DCOP
//
MappedCompositionIterator it;
MappedEvent               *insertEvent;
int                       sliceSize;
int                       pitch;
timeT                     absTime;
timeT                     duration;
instrumentT               instrument;
velocityT                 velocity;



MappedComposition::MappedComposition(Rosegarden::Composition &comp,
                                     const unsigned int &sT,
                                     const unsigned int &eT):
  _startTime(sT),
  _endTime(eT)
{
  unsigned int eventTime;

  assert(_endTime >= _startTime);
    
  for (Composition::iterator i = comp.begin(); i != comp.end(); i++ )
  {
    if ( (*i)->getStartIndex() >= int(_endTime) )
      continue;

    for ( Track::iterator j = (*i)->begin(); j != (*i)->end(); j++ )
    {
      // for the moment ensure we're all positive
      //assert((*j)->getAbsoluteTime() >= 0 );

      // Skip this event if it doesn't have pitch
      // (check that the event has "velocity" soon as well)
      //
      if (!(*j)->has("pitch"))
        continue;

      // get the eventTime
      eventTime = (unsigned int) (*j)->getAbsoluteTime();

      // eventually filter only for the events we're interested in
      if ( eventTime >= _startTime && eventTime <= _endTime )
      {
        // insert event
        MappedEvent *me = new MappedEvent(**j);
        me->setInstrument((*i)->getInstrument());
        this->insert(me);
      }
    }
  }
}



// turn a MappedComposition into a data stream
//
QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
  
  dS << mC.size();

  for ( it = mC.begin(); it != mC.end(); ++it )
  {
    dS << (*it)->getPitch();
    dS << (*it)->getAbsoluteTime();
    dS << (*it)->getDuration();
    dS << (*it)->getVelocity();
    dS << (*it)->getInstrument();
  }

  return dS;
}


// turn a data stream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
  dS >> sliceSize;

  while (!dS.atEnd() && sliceSize)
  {
    dS >> pitch;
    dS >> absTime;
    dS >> duration;
    dS >> velocity;
    dS >> instrument;

    insertEvent = new MappedEvent(pitch, absTime, duration,
                                  velocity, instrument);
    mC.insert(insertEvent);

    sliceSize--;

  }

  if (sliceSize)
  {
    cerr << "operator>> - wrong number of events received" << endl;
  }
    

  return dS;
}



}


