
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

#include <string>

#include "Event.h"
#include "Track.h"
#include "Composition.h"
#include "NotationTypes.h"

#include "rg21io.h"
#include "rosedebug.h"

using Rosegarden::Event;
using Rosegarden::Track;
using Rosegarden::Int;
using Rosegarden::String;

RG21Loader::RG21Loader(const QString& fileName)
    : m_file(fileName),
      m_stream(0),
      m_composition(0),
      m_currentTrack(0),
      m_currentTrackTime(0),
      m_currentTrackNb(0),
      m_nbStaves(0)
{

    if (m_file.open(IO_ReadOnly)) {

        m_stream = new QTextStream(&m_file);

        parse();
    }

}

RG21Loader::~RG21Loader()
{
    delete m_stream;
}

bool RG21Loader::parseClef()
{
    if (m_tokens.count() != 3 || !m_currentTrack) return false;
    
    std::string clefName = m_tokens[2].lower().data();

    Event *clefEvent = new Event(Rosegarden::Clef::EventType);
    clefEvent->setAbsoluteTime(0);
    clefEvent->set<String>(Rosegarden::Clef::ClefPropertyName,
                           clefName);
    
    m_currentTrack->insert(clefEvent);

    return true;
}

bool RG21Loader::parseChordItem()
{
    using Rosegarden::Note;
    using Rosegarden::timeT;

    if (m_tokens.count() < 4) return false;

    QString durationString = m_tokens[0].lower();
    QStringList::Iterator i = m_tokens.begin(); 
    ++i;
    
    if (durationString == "dotted") {
        durationString += ' ';
        durationString += m_tokens[1].lower();
        ++i;
    }
    
    Rosegarden::timeT duration = 0;

    try {
        Note n(durationString.latin1());
        duration = n.getDuration();
    } catch (Note::BadType b) {
        kdDebug(KDEBUG_AREA) << "RG21Loader::parseChordItem: Bad duration: "
                             << durationString << endl;
        return false;
    }


    // get chord mod flags and nb of notes
    int chordMods = (*i).toInt(); ++i;
    int nbNotes   = (*i).toInt(); ++i;

    // now get notes
    for(;i != m_tokens.end(); ++i) {

        long pitch = (*i).toInt();
        ++i;
        int noteMods = (*i).toInt();
        pitch = convertRG21Pitch(pitch, noteMods);

        Event *noteEvent = new Event(Note::EventType);
        noteEvent->setDuration(duration);
        noteEvent->setAbsoluteTime(m_currentTrackTime);
        noteEvent->set<Int>("pitch", pitch);

        kdDebug(KDEBUG_AREA) << "RG21Loader::parseChordItem() : insert note pitch " << pitch
                             << " at time " << m_currentTrackTime << endl;

        m_currentTrack->insert(noteEvent);
    }

    m_currentTrackTime += duration;

    return true;
}

void RG21Loader::closeTrackOrComposition()
{
    if (m_currentTrack) {
        m_currentTrack->setInstrument(m_currentTrackNb);
        m_composition->addTrack(m_currentTrack);
        m_currentTrack = 0;
        m_currentTrackTime = 0;
    } else {
        // ??
    }
}

long RG21Loader::convertRG21Pitch(long pitch, int noteModifier)
{
  long rtn = 0;
  int octave = 5;

  while (pitch < 0) { octave -= 1; pitch += 7; }
  while (pitch > 7) { octave += 1; pitch -= 7; }

  if (pitch > 4) ++octave;

  switch(pitch) {

  case 0: rtn =  4; break;	/* bottom line, treble clef: E */
  case 1: rtn =  5; break;	/* F */
  case 2: rtn =  7; break;	/* G */
  case 3: rtn =  9; break;	/* A, in next octave */
  case 4: rtn = 11; break;	/* B, likewise*/
  case 5: rtn =  0; break;	/* C, moved up an octave (see above) */
  case 6: rtn =  2; break;	/* D, likewise */
  case 7: rtn =  4; break;	/* E, likewise */
  }

  if (noteModifier & ModSharp) ++ rtn;
  if (noteModifier & ModFlat)  -- rtn;

  switch(m_currentClef) {

  case  TrebleClef: break;
  case   TenorClef: octave -= 1; break;
  case    AltoClef: octave -= 1; break;
  case    BassClef: octave -= 2; break;
  case InvalidClef: break;
  }

  rtn += 12 * octave;

  return rtn;
}


bool RG21Loader::parse()
{
    while (!m_stream->eof()) {
        m_currentLine = m_stream->readLine();
        m_currentLine = m_currentLine.simplifyWhiteSpace();

        if (m_currentLine[0] == '#' ||
            m_currentLine.length() == 0) continue; // skip comments

        m_tokens = QStringList::split(' ', m_currentLine);

        QString firstToken = m_tokens.first();
        
        if (firstToken == "Staves" || firstToken == "Staffs") { // nb staves

            m_nbStaves = m_tokens[1].toUInt();
            m_composition = new Rosegarden::Composition;

        } else if (firstToken == "Name") { // Staff name

            m_currentStaffName = m_tokens[1]; // we don't do anything with it yet
            m_currentTrack = new Rosegarden::Track;
            ++m_currentTrackNb;
            
        } else if (firstToken == "Clef") {

            parseClef();

        } else if (firstToken == ":") { // chord

            m_tokens.remove(m_tokens.begin()); // get rid of 1st token ':'
            parseChordItem();

        } else if (firstToken == "End") {

            closeTrackOrComposition();
            
        }
        
    }
    
    return true;
}
