// -*- c-basic-offset: 4 -*-
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


#ifndef _ROSEGARDEN_MIDI_FILE_H_
#define _ROSEGARDEN_MIDI_FILE_H_

#include "Midi.h"
#include "MidiEvent.h"
#include "Composition.h"

#include <fstream>
#include <string>
#include <list>
#include <map>

// Conversion class for Rosegarden::Composition to and
// from MIDI Files.  Despite the fact you can reuse this
// object it's probably safer just to create it for a
// single way conversion and then throw it away (MIDI
// to Composition conversion invalidates the internal
// MIDI model).
//
//
//

namespace Rosegarden
{

// Our internal MIDI structure is just a list of MidiEvents.
// We use a list and not a set because we want the order of
// the events to be arbitrary until we explicitly sort them
// (necessary when converting Composition absolute times to
// MIDI delta times).
//
typedef std::map<unsigned int, std::list<MidiEvent> > MidiComposition;
typedef std::list<MidiEvent>::iterator MidiTrackIterator;

class MidiFile
{
public:

    typedef enum
        {
            MIDI_SINGLE_TRACK_FILE          = 0x00,
            MIDI_SIMULTANEOUS_TRACK_FILE    = 0x01,
            MIDI_SEQUENTIAL_TRACK_FILE      = 0x02,
            MIDI_CONVERTED_TO_APPLICATION   = 0xFE,
            MIDI_FILE_NOT_LOADED            = 0xFF
        } MIDIFileFormatType;

    MidiFile();
    MidiFile (const char *fn);
    ~MidiFile();

    MidiFile& operator=(const MidiFile& mF)
    {
        m_filename = mF.m_filename;
        m_timingDivision = mF.m_timingDivision;
        m_numberOfTracks = mF.m_numberOfTracks;
        m_format = mF.m_format;
        return *this;
    }

    // open a file of the given filename
    bool open();

    // write a file of the internal filename
    bool write();

    const std::string& filename() { return m_filename; }
    const int& timingDivision() { return m_timingDivision; }
    const MIDIFileFormatType& format() { return m_format; }
    const unsigned int& numberOfTracks() { return m_numberOfTracks; }

    // Conversion in and out of Rosegarden
    //
    Rosegarden::Composition* convertToRosegarden();
    void convertToMidi(const Rosegarden::Composition &comp);

private:
    std::string            m_filename;
    int                    m_timingDivision;   // pulses per quarter note
    MIDIFileFormatType     m_format;
    unsigned int           m_numberOfTracks;

    // Internal counters
    //
    unsigned long         m_trackByteCount;
    bool                  m_decrementCount;

    // Internal representation
    //
    MidiComposition       m_midiComposition;

    // Split the tasks up with these top level private methods
    //
    bool parseHeader(const std::string& midiHeader);
    bool parseTrack(std::ifstream* midiFile, const unsigned int &trackNum);
    bool writeHeader(std::ofstream* midiFile);
    bool writeTrack(std::ofstream* midiFile, const unsigned int &trackNumber);

    // Internal convenience functions
    //
    const unsigned int midiBytesToInt(const std::string& bytes);
    const unsigned long midiBytesToLong(const std::string& bytes);
    const unsigned int getNumberFromMidiBytes(std::ifstream* midiFile);
    const std::string getMidiBytes(std::ifstream* midiFile, const unsigned int &bytes);
    bool skipToNextTrack(std::ifstream *midiFile);
    void intToMidiBytes(std::ofstream* midiFile, int number);
    void longToMidiBytes(std::ofstream* midiFile, const unsigned long &number);
    void longToVarBuffer(std::string &buffer, const unsigned long &number);

};

}

#endif // _ROSEGARDEN_MIDI_FILE_H_
