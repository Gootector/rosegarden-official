// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#include <set>
#include <string>

#include <qdatastream.h>

#include "Composition.h" // for Rosegarden::RealTime
#include "Event.h"


#ifndef _MAPPEDEVENT_H_
#define _MAPPEDEVENT_H_

// Used as a transformation stage between Composition, Events and output
// at the Sequencer this class and MidiComposition eliminate the notion
// of the Segment and Track for ease of Event access.  The MappedEvents
// are ready for playing or routing through an Instrument or Effects
// boxes.
//
// MappedEvents can also represent instructions for playback of audio
// samples - if the m_type is Audio then the sequencer will attempt to
// map the Pitch (m_data1) to the audio id.  Note that this limits us
// to 256 audio files in the Composition unless we use a different
// parameter for storing these IDs.
// 
// The MappedEvent/Instrument relationship is interesting - we don't
// want to duplicate the entire Instrument at the Sequencer level as
// it'd be messy and unnecessary.  Instead we use a MappedInstrument
// which is just a very cut down Sequencer-side version of an Instrument.
//
// Some of these Events are unidirectional, some are bidirectional -
// that is they only have a meaning in one direction (they are still
// legal at either end).  They are broadcast in both directions using
// the "getSequencerSlice" and "processAsync/Recorded" interfaces on
// which the control messages can piggyback and eventually stripped out.
//

namespace Rosegarden
{

class MappedEvent
{
public:
    typedef enum
    {
        MidiNote                = 0x000001,
        MidiNoteOneShot         = 0x000002,  // doesn't need NOTE OFFs
        MidiProgramChange       = 0x000004,
        MidiKeyPressure         = 0x000008,
        MidiChannelPressure     = 0x000010,
        MidiPitchBend           = 0x000020,
        MidiController          = 0x000040,
        MidiSystemExclusive     = 0x000080,
        // Sent from the gui to play an audio file
        Audio                   = 0x000100,
        // Sent from gui to cancel playing an audio file
        AudioCancel             = 0x000200,
        // Sent to the gui with audio level on Instrument
        AudioLevel              = 0x000400,
        // Sent to the gui to inform an audio file stopped
        AudioStopped            = 0x000800,
        // The gui is clear to generate a preview for a new audio file
        AudioGeneratePreview    = 0x001000,
        // Update Instruments - new ALSA client detected
        SystemUpdateInstruments = 0x002000,
        // Set RG as JACK master
        SystemJackMaster        = 0x004000,
        // Set RG as MMC master
        SystemMMCMaster         = 0x008000,
        // Set Record device
        SystemRecordDevice      = 0x010000,
        // Set Metronome device
        SystemMetronomeDevice   = 0x020000

    } MappedEventType;

    MappedEvent(): m_instrument(0),
                   m_type(MidiNote),
                   m_data1(0),
                   m_data2(0),
                   m_eventTime(0, 0),
                   m_duration(0, 0),
                   m_audioStartMarker(0, 0),
                   m_dataBlock("") {;}

    // Construct from Events to Internal (MIDI) type MappedEvent
    //
    MappedEvent(const Event &e);

    // Another Internal constructor from Events
    MappedEvent(InstrumentId id,
                const Event &e,
                const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration);

    // A shortcut for creating MIDI/Internal MappedEvents
    // from base properties
    //
    MappedEvent(InstrumentId id,
                MidiByte pitch,
                MidiByte velocity,
                const Rosegarden::RealTime &absTime,
                const Rosegarden::RealTime &duration):
        m_instrument(id),
        m_type(MidiNote),
        m_data1(pitch),
        m_data2(velocity),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(Rosegarden::RealTime(0,0)),
        m_dataBlock("") {;}

    // A general MappedEvent constructor for any MappedEvent type
    //
    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte pitch,
                MidiByte velocity,
                const RealTime &absTime,
                const RealTime &duration,
                const RealTime &audioStartMarker):
        m_instrument(id),
        m_type(type),
        m_data1(pitch),
        m_data2(velocity),
        m_eventTime(absTime),
        m_duration(duration),
        m_audioStartMarker(audioStartMarker),
        m_dataBlock("") {;}

    // Audio MappedEvent shortcut constructor
    //
    MappedEvent(InstrumentId id,
                MidiByte audioID,
                const Rosegarden::RealTime &eventTime,
                const Rosegarden::RealTime &duration,
                const Rosegarden::RealTime &audioStartMarker):
         m_instrument(id),
         m_type(Audio),
         m_data1(audioID),
         m_data2(0),
         m_eventTime(eventTime),
         m_duration(duration),
         m_audioStartMarker(audioStartMarker),
         m_dataBlock("") {;}

    // More generalised MIDI event containers for
    // large and small events (one param, two param)
    //
    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte data1,
                MidiByte data2):
         m_instrument(id),
         m_type(type),
         m_data1(data1),
         m_data2(data2),
         m_eventTime(Rosegarden::RealTime(0, 0)),
         m_duration(Rosegarden::RealTime(0, 0)),
         m_audioStartMarker(Rosegarden::RealTime(0, 0)),
         m_dataBlock("") {;}

    MappedEvent(InstrumentId id,
                MappedEventType type,
                MidiByte data1):
        m_instrument(id),
        m_type(type),
        m_data1(data1),
        m_data2(0),
        m_eventTime(Rosegarden::RealTime(0, 0)),
        m_duration(Rosegarden::RealTime(0, 0)),
        m_audioStartMarker(Rosegarden::RealTime(0, 0)),
        m_dataBlock("") {;}
                

    // Copy constructor
    //
    MappedEvent(const MappedEvent &mE):
        m_instrument(mE.getInstrument()),
        m_type(mE.getType()),
        m_data1(mE.getData1()),
        m_data2(mE.getData2()),
        m_eventTime(mE.getEventTime()),
        m_duration(mE.getDuration()),
        m_audioStartMarker(mE.getAudioStartMarker()),
        m_dataBlock("") {;}

    // Copy from pointer
    MappedEvent(MappedEvent *mE):
        m_instrument(mE->getInstrument()),
        m_type(mE->getType()),
        m_data1(mE->getData1()),
        m_data2(mE->getData2()),
        m_eventTime(mE->getEventTime()),
        m_duration(mE->getDuration()),
        m_audioStartMarker(mE->getAudioStartMarker()),
        m_dataBlock("") {;}

    ~MappedEvent() {;}

    // Event time
    //
    void setEventTime(const Rosegarden::RealTime &a) { m_eventTime = a; }
    Rosegarden::RealTime getEventTime() const { return m_eventTime; }

    // Duration
    //
    void setDuration(const Rosegarden::RealTime &d) { m_duration = d; }
    Rosegarden::RealTime getDuration() const { return m_duration; }

    // Instrument
    void setInstrument(InstrumentId id) { m_instrument = id; }
    InstrumentId getInstrument() const { return m_instrument; }

    MidiByte getPitch() const { return m_data1; }

    // Keep pitch within MIDI limits
    //
    void setPitch(MidiByte p)
    {
        m_data1 = p;
        if (m_data1 > MidiMaxValue) m_data1 = MidiMaxValue;
    }

    void setVelocity(MidiByte v) { m_data2 = v; }
    MidiByte getVelocity() const { return m_data2; }

    // And the trendy names for them
    //
    MidiByte getData1() const { return m_data1; }
    MidiByte getData2() const { return m_data2; }
    void setData1(MidiByte d1) { m_data1 = d1; }
    void setData2(MidiByte d2) { m_data2 = d2; }

    // Also use the pitch as the Audio file ID
    //
    void setAudioID(MidiByte id) { m_data1 = id; }
    int getAudioID() const { return m_data1; }

    // A sample doesn't have to be played from the beginning.  When
    // passing an Audio event this value may be set to indicate from
    // where in the sample it should be played.  Duration is measured
    // against total sounding length (not absolute position).
    //
    //
    void setAudioStartMarker(const Rosegarden::RealTime &aS)
        { m_audioStartMarker = aS; }
    Rosegarden::RealTime getAudioStartMarker() const
        { return m_audioStartMarker; }

    MappedEventType getType() const { return m_type; }
    void setType(const MappedEventType &value) { m_type = value; }

    // Data block
    //
    std::string getDataBlock() const { return m_dataBlock; }
    void setDataBlock(const std::string &dataBlock) { m_dataBlock = dataBlock; }
    
    // How MappedEvents are ordered in the MappedComposition
    //
    struct MappedEventCmp
    {
        bool operator()(const MappedEvent *mE1, const MappedEvent *mE2) const
        {
            return *mE1 < *mE2;
        }
    };

    friend bool operator<(const MappedEvent &a, const MappedEvent &b);

    MappedEvent& operator=(const MappedEvent &mE);

    friend QDataStream& operator>>(QDataStream &dS, MappedEvent *mE);
    friend QDataStream& operator<<(QDataStream &dS, MappedEvent *mE);
    friend QDataStream& operator>>(QDataStream &dS, MappedEvent &mE);
    friend QDataStream& operator<<(QDataStream &dS, const MappedEvent &mE);


private:
    InstrumentId     m_instrument;
    MappedEventType  m_type;
    MidiByte         m_data1;
    MidiByte         m_data2;
    RealTime         m_eventTime;
    RealTime         m_duration;
    RealTime         m_audioStartMarker;

    // Use this when we want to store something in addition to the
    // other bytes in this type, e.g. System Exclusive.
    //
    std::string      m_dataBlock;

};

}

#endif
