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


#include <iostream>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MidiArts.h"

#include "MidiFile.h"
#include "Composition.h"
#include "Track.h"
#include "Event.h"
#include "MidiRecord.h"
#include "Sequencer.h"



int
main(int argc, char **argv)
{
  // Create a Rosegarden MIDI file
  //
  Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("glazunov.mid");
  //Rosegarden::MidiFile *midiFile = new Rosegarden::MidiFile("Kathzy.mid");

  // open the MIDI file
  midiFile->open();

  // Create a Rosegarden composition
  Rosegarden::Composition comp = midiFile->convertToRosegarden();

  // initialize MIDI and audio subsystems
  //
  Rosegarden::Sequencer sequencer;

  // set the tempo
  sequencer.tempo(120);

  unsigned long i;
  vector<Arts::MidiEvent>::iterator midiQueueIt;
  vector<Arts::MidiEvent> *midiQueue;
  Arts::TimeStamp midiTime;
  Arts::MidiEvent event;
  
  int noteVal = 50;

  // turn MIDI recording on
  //
  //sequencer.record(Rosegarden::Sequencer::RECORD_MIDI);

  // turn on playing
  sequencer.play();

  while(true)
  {
    
    // pause - to keep things in check
    for (i = 0; i < 1000000; i++);

    // set the song position to the current time
    sequencer.updateSongPosition();

    if (sequencer.isPlaying())
    {
      cout << "CLOCK @ " << sequencer.songPosition() << endl;
      sequencer.processMidiOut(comp);
    }
 
    // the recording section
    switch(sequencer.recordStatus())
    {
      case Rosegarden::Sequencer::RECORD_MIDI:
        midiQueue = sequencer.getMidiQueue();

        if (midiQueue->size() > 0)
        {
          for (midiQueueIt = midiQueue->begin();
               midiQueueIt != midiQueue->end();
               midiQueueIt++)
          {
            // want to send the event both ways - to the track and to the GUI
            // so we process the midi commands individually from the small clump
            // we've received back in the queue.  Dependent on performance
            // this may allow us to update our GUI and keep reading in the
            // events from a single thread.
            //
            sequencer.processMidiIn(midiQueueIt->command,
                                    sequencer.recordTime(midiQueueIt->time));
          }
        }
        break;

      case Rosegarden::Sequencer::ASYNCHRONOUS_MIDI:
        // send asynchronous MIDI events in up to the GUI
        break;

      default:
        break;
    }

   //sequencer.incrementSongPosition(60000);
   //midiQueueIt = midiQueue->begin();

  }

}
