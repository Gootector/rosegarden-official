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


#ifndef _ROSEGARDEN_MIDI_EVENT_H_
#define _ROSEGARDEN_MIDI_EVENT_H_

#include "Midi.h"
#include "Event.h"

// MidiEvent holds MIDI and Rosegarden::Event data during MIDI file I/O.
// We don't use this class at all for playback or recording of MIDI -
// for that look at MappedEvent and MappedComposition.
//
// Rosegarden doesn't have any internal concept of MIDI events, only
// Rosegarden::Events which are a superset of MIDI functionality.
//
// Check out Rosegarden::Event in base/ for more information.
//
//
//

namespace Rosegarden
{
class MidiEvent
{

public:
    MidiEvent();

    // single data byte case
    //
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1);

    // double data byte
    //
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &data1,
              const MidiByte &data2);

    // meta event
    //
    MidiEvent(const Rosegarden::timeT &deltaTime,
              const MidiByte &eventCode,
              const MidiByte &metaEventCode,
              const std::string &metaMessage);

    ~MidiEvent();

    // View our event as text
    //
    void print();


    void setTime(const Rosegarden::timeT &time) { m_deltaTime = time; }
    void setDuration(const Rosegarden::timeT& duration) {m_duration = duration;}
    Rosegarden::timeT addTime(const Rosegarden::timeT &time);

    MidiByte getMessageType() const
        { return ( m_eventCode & MIDI_MESSAGE_TYPE_MASK ); }

    MidiByte getChannelNumber() const
        { return ( m_eventCode & MIDI_CHANNEL_NUM_MASK ); }

    Rosegarden::timeT getTime() const { return m_deltaTime; }
    Rosegarden::timeT getDuration() const { return m_duration; }

    MidiByte getPitch() const { return m_data1; }
    MidiByte getVelocity() const { return m_data2; }
    MidiByte getData1() const { return m_data1; }
    MidiByte getData2() const { return m_data2; }
    MidiByte getEventCode() const { return m_eventCode; }

    bool isMeta() const { return(m_eventCode == MIDI_FILE_META_EVENT); }

    MidiByte getMetaEventCode() const { return m_metaEventCode; }
    std::string getMetaMessage() const { return m_metaMessage; }

    friend bool operator<(const MidiEvent &a, const MidiEvent &b);

private:

    MidiEvent& operator=(const MidiEvent);

    Rosegarden::timeT m_deltaTime;
    Rosegarden::timeT m_duration;
    MidiByte          m_eventCode;
    MidiByte          m_data1;         // or Note
    MidiByte          m_data2;         // or Velocity

    MidiByte          m_metaEventCode;
    std::string       m_metaMessage;

};

}

#endif // _ROSEGARDEN_MIDI_EVENT_H_
