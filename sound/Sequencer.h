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

// From this class we control our sound subsystems - audio
// and MIDI are initialised, playback and recording handles
// are available to the higher levels for sending and 
// retreiving MIDI and audio.  When the Rosegarden sequencer
// (sequencer/) initialises it creates a Rosegarden::Sequencer
// object which prepares itself for playback and recording.
//
// At this level we accept MappedCompositions (single point
// representation - NOTE ONs with durations) and turn them
// into MIDI events (generate and segment NOTE OFFs).
//
// Recording wise we take aRTS events and turn them into
// a MappedComposition before sending it up to the gui.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include <map>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MappedComposition.h"
#include "Midi.h"
#include "MidiRecord.h"
#include "MidiEvent.h"


namespace Rosegarden
{

// NOTE OFF queue. These hold a time ordered set of
// NOTE OFF events.
//
class NoteOffEvent
{
public:
    NoteOffEvent() {;}
    NoteOffEvent(const unsigned int &midiTime,
                 const unsigned int &pitch,
                 const Rosegarden::MidiByte &status):
        m_midiTime(midiTime),
        m_pitch(pitch),
        m_status(status) {;}
    ~NoteOffEvent() {;}

    struct NoteOffEventCmp
    {
        bool operator()(NoteOffEvent *nO1, NoteOffEvent *nO2)
        { 
            return nO1->getMidiTime() < nO2->getMidiTime();
        }
    };
    
    unsigned int getMidiTime() { return m_midiTime; }
    Rosegarden::MidiByte getPitch() { return m_pitch; }

private: 
    unsigned int         m_midiTime;
    Rosegarden::MidiByte m_pitch;
    Rosegarden::MidiByte m_status;

};

class NoteOffQueue : public std::multiset<NoteOffEvent *,
                     NoteOffEvent::NoteOffEventCmp>
{
public:
    NoteOffQueue() {;}
    ~NoteOffQueue() {;}
private:
};


class Sequencer
{
public:

    typedef enum
    {
	ASYNCHRONOUS_MIDI,
	ASYNCHRONOUS_AUDIO,
	RECORD_MIDI,
	RECORD_AUDIO
    } RecordStatus;

    Sequencer();
    ~Sequencer();

    // Activate a recording state
    //
    void record(const RecordStatus& recordStatus);

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity display purposes.
    //
    RecordStatus recordStatus() { return m_recordStatus; }
    
    // set and get tempo
    const double getTempo() const { return m_tempo; }
    void setTempo(const double &tempo) { m_tempo = tempo; }

    // resolution - required?
    const unsigned int resolution() const { return m_ppq; }

    // get the difference in TimeStamps
    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    Arts::TimeStamp aggregateTime(const Arts::TimeStamp &ts1,
                                  const Arts::TimeStamp &ts2);

    // get the TimeStamp from the beginning of playing
    inline Arts::TimeStamp playTime(Arts::TimeStamp const &ts)
    { return (deltaTime(ts, m_playStartTime)); }

    // get the TimeStamp from the beginning of recording
    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
    { return (deltaTime(ts, m_recordStartTime)); }

    // See docs/discussion/sequencer_timing.txt for explanation of
    // the maths here.
    //
    //
    inline Rosegarden::timeT convertToInternalTime(const Arts::TimeStamp &timeStamp)
    {
        return (Rosegarden::timeT)
            ((((double)(timeStamp.sec * 1000000 + timeStamp.usec)) *
              m_ppq * (double) m_tempo )  /
             60000000.0);
    }

    // See docs/discussion/sequencer_timing.txt for explanation of
    // the maths here.
    //
    //
    inline Arts::TimeStamp convertToArtsTimeStamp(const unsigned int &midiTime)
    {
        // We ignore the Time Sigs for the moment
        //
        //
        unsigned int usec = (unsigned int)(((double)60000000.0 *(double)midiTime)/
                                           ((double)m_ppq * (double)m_tempo));
        unsigned int sec = usec / 1000000;
        usec %= 1000000;

        return (Arts::TimeStamp(sec, usec));
    }


    // this method turns any recorded MIDI into a MappedComposition
    // and returns it to the gui
    //
    MappedComposition getMappedComposition();

    // Process MappedComposition into MIDI events and send to aRTS.
    void processMidiOut(Rosegarden::MappedComposition mappedComp,
                        const Rosegarden::timeT &playLatency);

    // Reset internal states ready for new playback to commence
    //
    void initializePlayback(const timeT &position);

    Rosegarden::timeT getSequencerTime();

    bool isPlaying() { return m_playing; }

    void stopPlayback();

    // NOTE OFF control
    void processNotesOff(unsigned int midiTime);
    void allNotesOff();

    Rosegarden::timeT getStartPosition() { return m_playStartPosition; }

    // Temporary cheat to enable quick sending of midi events from a 
    // thin aRTS client - not to be used in anger!
    //
    Arts::MidiPort* playMidiPort() { return &m_midiPlayPort; }

private:

    // set-up
    void initializeMidi();

    // get a vector of recorded events from aRTS
    // (only for internal use)
    //
    inline std::vector<Arts::MidiEvent>* getMidiQueue()
        { return m_midiRecordPort.getQueue(); }

    // process a raw aRTS MIDI event into internal representation
    // (only for internal use)
    //
    void processMidiIn(const Arts::MidiCommand &midiCommand,
                       const Arts::TimeStamp &timeStamp);

    // aRTS devices
    //
    Arts::Dispatcher       m_dispatcher;
    Arts::SoundServer      m_soundServer;
    Arts::MidiManager      m_midiManager;
    Arts::MidiClient       m_midiPlayClient;
    Arts::MidiClient       m_midiRecordClient;
    RosegardenMidiRecord   m_midiRecordPort;
    Arts::MidiPort         m_midiPlayPort;

    // TimeStamps mark the real world (aRts) times of the start
    // of play or record.  These are for internal use when
    // calculating how to queue up playback.
    //
    Arts::TimeStamp        m_playStartTime;
    Arts::TimeStamp        m_recordStartTime;
    NoteOffQueue           m_noteOffQueue;

    // Absolute time playback start position
    Rosegarden::timeT      m_playStartPosition;

    // Current recording status
    RecordStatus           m_recordStatus;

    // Playback flags
    bool                   m_startPlayback;
    bool                   m_playing;

    // Internal measures
    unsigned int           m_ppq;   // sequencer resolution
    double                 m_tempo; // Beats Per Minute

    MappedComposition      m_recordComposition;

    std::map<unsigned int, MappedEvent*> m_noteOnMap;

};


}

 

#endif // _ROSEGARDEN_SEQUENCER_H_
