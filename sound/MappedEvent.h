// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#ifndef _MAPPEDEVENT_H_
#define _MAPPEDEVENT_H_

// Used as a transformation stage between Composition Events and output
// at the Sequencer this class and MidiComposition eliminates the notion
// of Segment.  The MappedEvents are ripe for playing.
//
// NOTE: for the moment until the Composition handles it we're hard
// coding the velocity components of all MappedEvents to maximum (127)
//
//
// MappedEvents also code playback of audio samples - if the
// m_isAudio flag is set by the GUI then the sequencer will
// attempt to map the Pitch (m_pitch) to the audio id.
//

#include "Composition.h" // for Rosegarden::RealTime
#include "Event.h"
#include <multiset.h>
#include <qdatastream.h>

namespace Rosegarden
{

typedef unsigned int trackT;
typedef unsigned int velocityT;

class MappedEvent
{
public:
    MappedEvent(): m_pitch(0),
                   m_absoluteTime(0, 0),
                   m_duration(0, 0),
                   m_velocity(0),
                   m_track(0),
                   m_isAudio(false) {;}

    // Our main constructors used to convert from Events -
    // note that we put in place default velocities at this
    // point in case our Composition is missing them.
    //
    //
    MappedEvent(const Event &e);

    MappedEvent(const Event &e,
                const Rosegarden::RealTime &absoluteTime,
                const Rosegarden::RealTime &duration);

    MappedEvent(const int &pitch, const Rosegarden::RealTime &absTime,
                const Rosegarden::RealTime &duration,
                const velocityT &velocity, const trackT &track):
        m_pitch(pitch),
        m_absoluteTime(absTime),
        m_duration(duration),
        m_velocity(velocity),
        m_track(track),
        m_isAudio(false) {;}
    ~MappedEvent() {;}

    void setPitch(const int &p) { m_pitch = p; }
    void setAbsoluteTime(const Rosegarden::RealTime &a) { m_absoluteTime = a; }
    void setDuration(const Rosegarden::RealTime &d) { m_duration = d; }
    void setTrack(const trackT &i) { m_track = i; }
    void setVelocity(const velocityT &v) { m_velocity = v; }

    int   getPitch() const { return m_pitch; }
    Rosegarden::RealTime getAbsoluteTime() const { return m_absoluteTime; }
    Rosegarden::RealTime getDuration() const { return m_duration; }
    velocityT getVelocity() const { return m_velocity; }
    trackT getTrack() const { return m_track; }

    // Audio MappedEvent methods
    //
    void setStartIndex(const Rosegarden::RealTime sI)
        { m_absoluteTime = sI; }
    Rosegarden::RealTime getStartIndex() const { return m_absoluteTime; }

    void setAudioID(const int &id) { m_pitch = id; }
    int getAudioID() const { return m_pitch; }

    bool isAudio() const { return m_isAudio; }
    void isAudio(const bool &value) { m_isAudio = value; }

    struct MappedEventCmp
    {
        bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
        {
            return *mE1 < *mE2;
        }
    };

    friend bool operator<(const MappedEvent &a, const MappedEvent &b);

private:

    int                      m_pitch;
    Rosegarden::RealTime     m_absoluteTime;
    Rosegarden::RealTime     m_duration;
    velocityT                m_velocity;
    trackT                   m_track;
    bool                     m_isAudio;

};

}

#endif
