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


#include "Sequencer.h"
#include <iostream>
#include "MidiArts.h"
#include "MidiFile.h"
#include "Composition.h"
#include "Track.h"
#include "Event.h"
#include "NotationTypes.h"
#include <MappedComposition.h>

namespace Rosegarden
{

using std::cerr;
using std::cout;
using std::endl;

Sequencer::Sequencer():
                       _songPosition(0),
                       _songPlayPosition(0),
                       _songRecordPosition(0),
                       _lastFetchPosition(0),
                       _playStartTime(0, 0),
                       _recordStartTime(0, 0),
                       _recordStatus(ASYNCHRONOUS_MIDI),
                       _playing(false),
                       _ppq(960),
                       _recordTrack(0)
{
  initializeMidi();
}


Sequencer::~Sequencer()
{
}

void
Sequencer::initializeMidi()
{
  _midiManager = Arts::Reference("global:Arts_MidiManager");
  if (_midiManager.isNull())
  {
    cerr << "Can't get MidiManager" << endl;
    exit(1);
  }

  _soundServer = Arts::Reference("global:Arts_SoundServer");
  if (_soundServer.isNull())
  {
    cerr << "Can't get SoundServer" << endl;
    exit(1);
  }

  _midiRecordPort = Arts::DynamicCast(_soundServer.createObject("RosegardenMidiRecord"));


  if (_midiRecordPort.isNull())
  {
    cerr << "Can't create MidiRecorder" << endl;
    exit(1);
  }

  _midiPlayClient = _midiManager.addClient(Arts::mcdPlay,
                                           Arts::mctApplication,
                                          "Rosegarden (play)","rosegarden");
  if (_midiPlayClient.isNull())
  {
    cerr << "Can't create MidiClient" << endl;
    exit(1);
  }

  _midiPlayPort = _midiPlayClient.addOutputPort();
  if (_midiPlayPort.isNull())
  {
    cerr << "Can't create Midi Output Port" << endl;
    exit(1);
  }

  _midiRecordClient = _midiManager.addClient(Arts::mcdRecord,
                                             Arts::mctApplication,
                                             "Rosegarden (record)",
                                             "rosegarden");
  if (_midiRecordClient.isNull())
  {
    cerr << "Can't create MidiRecordClient" << endl;
    exit(1);
  }

  // Create our recording midi port
  //
  _midiRecordClient.addInputPort(_midiRecordPort);

  // set MIDI thru
  //
  _midiRecordPort.setMidiThru(_midiPlayPort);

}

/*
// Increment the song position in microseconds
// and increment the seconds value as required.
//
void
Sequencer::incrementSongPosition(long inc)
{
  _songTime.usec += inc;
  _songTime.sec += _songTime.usec / 1000000;
  _songTime.usec %= 1000000;
}
*/

void
Sequencer::record(const RecordStatus& recordStatus)
{

  if ( recordStatus == RECORD_MIDI )
  {
    cout << "Recording MIDI" << endl;

    // turn MIDI event recording on 
    _midiRecordPort.record(true);

    // if we're already playing then just toggle recording
    // at this point, if not we jump back by the count in
    if ( !_playing )
      play();

    // set status and the record start position
    _recordStatus = RECORD_MIDI;
    _recordStartTime = _midiRecordPort.time();

    cout << "Recording Started at : " << _recordStartTime.sec << " : "
                                      << _recordStartTime.usec << endl;
                                     
    _recordTrack = new Track;
    _recordTrack->setInstrument(1);
    _recordTrack->setStartIndex(0);

  }
  else
  {
    cout << "Currently unsupported recording mode." << endl;
  }
  
}

void
Sequencer::play()
{
   if ( !_playing)
   {
     _playing = true;
     _playStartTime = _midiPlayPort.time();

     // store where we started playing from and initialise the
     // playback slice marker (_lastFetchPosition)
     //
     _lastFetchPosition = _songPlayPosition = _songPosition;
   }
   else
   {
     // jump back to last start position in MIDI clocks
     _lastFetchPosition = _songPosition = _songPlayPosition;

     // reset the playStartTime to now  like this
     //_playStartTime = deltaTime(_midiPlayPort.time(), _playStartTime);
     _playStartTime = _midiPlayPort.time();
   }
}

void
Sequencer::stop()
{
  _recordStatus = ASYNCHRONOUS_MIDI;

  if ( _playing )
  {
    // just stop
    _playing = false;
  }
  else
  {
    // if already stopped then return to zero
    _songPosition = 0;
  }
}

Arts::TimeStamp
Sequencer::deltaTime(const Arts::TimeStamp &ts1, const Arts::TimeStamp &ts2)
{
  int usec = ts1.usec - ts2.usec;
  int sec = ts1.sec - ts2.sec;

  if ( usec < 0 )
  {
    sec--;
    usec += 1000000;
  }

  assert( sec >= 0 );

  return (Arts::TimeStamp(sec, usec));
}

Arts::TimeStamp
Sequencer::aggregateTime(const Arts::TimeStamp &ts1, const Arts::TimeStamp &ts2)
{
  int usec = ts1.usec + ts2.usec;
  int sec = ( usec / 1000000 ) + ts1.sec + ts2.sec;
  usec %= 1000000;
  return(Arts::TimeStamp(sec, usec));
}


void
Sequencer::processMidiIn(const Arts::MidiCommand &midiCommand,
                         const Arts::TimeStamp &timeStamp)
{
  Rosegarden::MidiByte channel;
  Rosegarden::MidiByte message;
  //Rosegarden::Event *event;

  if (_recordTrack == 0)
  {
    cerr << "no Track created to processMidi on to - is recording enabled?" << endl;
    exit(1);
  }

  channel = midiCommand.status & MIDI_CHANNEL_NUM_MASK;
  message = midiCommand.status & MIDI_MESSAGE_TYPE_MASK;

  // Check for a hidden NOTE OFF (NOTE ON with zero velocity)
  if ( message == MIDI_NOTE_ON && midiCommand.data2 == 0 )
  {
    message = MIDI_NOTE_OFF;
  }

  // we use a map of Notes and this is the key
  unsigned int chanNoteKey = ( channel << 8 ) + midiCommand.data1;
  //double absoluteSecs;
  int duration;

  // scan for our event
  switch(message)
  {
    case MIDI_NOTE_ON:
      if ( _noteOnMap[chanNoteKey] == 0 )
      {
        _noteOnMap[chanNoteKey] = new Event;

        // set time since recording started in Absolute internal time
        _noteOnMap[chanNoteKey]->
            setAbsoluteTime(convertToMidiTime(timeStamp));

        // set note type and pitch
        _noteOnMap[chanNoteKey]->setType(Note::EventType);
        _noteOnMap[chanNoteKey]->set<Int>("pitch", midiCommand.data1);
      }
      break;

    case MIDI_NOTE_OFF:
      // if we match an open NOTE_ON
      //
      if ( _noteOnMap[chanNoteKey] != 0 )
      {
        duration = convertToMidiTime(timeStamp) -
                   _noteOnMap[chanNoteKey]->getAbsoluteTime();

        // for the moment, ensure we're positive like this
        //
        assert(duration >= 0);

        // set the duration
        _noteOnMap[chanNoteKey]->setDuration(duration);

        // insert the record
        //
        _recordTrack->insert(_noteOnMap[chanNoteKey]);

        // tell us about it
        cout << "INSERTED NOTE at time " 
             << _noteOnMap[chanNoteKey]->getAbsoluteTime()
             << " of duration "
             << _noteOnMap[chanNoteKey]->getDuration() << endl;

        // reset the reference
        _noteOnMap[chanNoteKey] = 0;

      }
      else
        cout << "MIDI_NOTE_OFF with no matching MIDI_NOTE_ON" << endl;
      break;

    default:
      cout << "OTHER EVENT" << endl;
      break;
  }
}


void
Sequencer::updateSongPosition()
{
  int deltaPosition = (int) convertToMidiTime(
                                    deltaTime(_midiPlayPort.time(),
                                              _playStartTime));
  assert(deltaPosition >= 0);
  _songPosition = _songPlayPosition + deltaPosition;
}

void
Sequencer::processMidiOut(Rosegarden::Composition *composition)
{
  //updateSongPosition();

  // set the position of events we're fetching to into the
  // future to give us a small buffer of MIDI events sent
  // to the output queue
  unsigned int fetchToPosition = _songPosition + 100;
  Arts::TimeStamp playPortTime = _midiPlayPort.time();

  MappedComposition *mappedComp = new MappedComposition(*composition,
         _lastFetchPosition, fetchToPosition);

  Arts::MidiEvent event;

  // MidiCommandStatus
  //   mcsCommandMask
  //   mcsChannelMask
  //   mcsNoteOff
  //   mcsNoteOn
  //   mcsKeyPressure
  //   mcsParameter
  //   mcsProgram 
  //   mcsChannelPressure
  //   mcsPitchWheel

  // for the moment hardcode the channel
  MidiByte channel = 0;

  unsigned int midiPlayPosition;

  for ( MappedComposition::iterator i = mappedComp->begin();
                                    i != mappedComp->end(); ++i )
  {
    //(*i)->getDuration()

   cout << "ABS = " << (*i)->getAbsoluteTime() << " : POS = " << _songPosition << endl;
    // sort out the correct TimeStamp for playback
    assert((unsigned int)(*i)->getAbsoluteTime() >= _songPosition);

    midiPlayPosition = (*i)->getAbsoluteTime() - _songPosition;
    
    event.time = aggregateTime(playPortTime, convertToTimeStamp(midiPlayPosition));

    // load the command structure
    event.command.status = Arts::mcsNoteOn | channel;
    event.command.data1 = (*i)->getPitch();   // pitch
    event.command.data2 = 127;  // hardcode velocity

    // if a NOTE ON
    // send the event out
    _midiPlayPort.processEvent(event);

    // and log it on the Note OFF stack
/*
    NoteOffEvent *noteOffEvent =
           new NoteOffEvent((*i)->getAbsoluteTime() + (*i)->getDuration(),
                            (Rosegarden::MidiByte)event.command.data1,
                            (Rosegarden::MidiByte)event.command.status);

    _noteOffQueue.insert(noteOffEvent);
*/
    event.command.status = Arts::mcsNoteOff | channel;
    event.time = aggregateTime(playPortTime,
                  convertToTimeStamp(midiPlayPosition + (*i)->getDuration()));

    _midiPlayPort.processEvent(event);

  }

  _lastFetchPosition = fetchToPosition;
  
  //cout << "GOT " << mappedComp->size() << " EVENTS " << endl;
  
}


}
