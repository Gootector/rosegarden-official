// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Alterations and additions by Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "lilypondio.h"
#include "config.h"

#include "Composition.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"

#include <iostream>
#include <fstream>
#include <string>

using Rosegarden::BaseProperties;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Composition;
using Rosegarden::Indication;
using Rosegarden::Int;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::String;
using Rosegarden::timeT;
using Rosegarden::TimeSignature;


LilypondExporter::LilypondExporter(Composition *composition,
				   std::string fileName) :
    m_composition(composition),
    m_fileName(fileName)
{
    // nothing else
}

LilypondExporter::~LilypondExporter()
{
    // nothing
}

void
LilypondExporter::handleStartingEvents(eventstartlist &eventsToStart, bool &addTie, std::ofstream &str) {
    for (eventstartlist::iterator m = eventsToStart.begin();
	 m != eventsToStart.end();
	 ++m) {
	if ((*m)->isa(Indication::EventType)) {
	    std::string which((*m)->get<String>(Indication::IndicationTypePropertyName));
	    if (which == Indication::Slur) {
		str << "( ";
	    } else if (which == Indication::Crescendo) {
		str << "\\< ";
	    } else if (which == Indication::Decrescendo) {
		str << "\\> ";
	    }
	} else {
	    // Not an indication
	}
        // Incomplete: erase during iteration not guaranteed
        // This is bad, but can't find docs on return value at end
        // i.e. I want to increment m with m = events...erase(m), but
        // what is returned when erase(eventsToStart.end())?
	eventsToStart.erase(m);
    }
    if (addTie) {
	str << "~ ";
	addTie = false;
    }
}

void
LilypondExporter::handleEndingEvents(eventendlist &eventsInProgress, Segment::iterator &j, std::ofstream &str) {
    for (eventendlist::iterator k = eventsInProgress.begin();
	 k != eventsInProgress.end();
	 ++k) {
	// Handle and remove all the relevant events in progress
	// This assumes all deferred events are indications
	long indicationDuration = 0;
	(*k)->get<Int>(Indication::IndicationDurationPropertyName, indicationDuration);
	if ((*k)->getAbsoluteTime() + indicationDuration <=
	    (*j)->getAbsoluteTime() + (*j)->getDuration()) {
	    if ((*k)->isa(Indication::EventType)) {
		std::string whichIndication((*k)->get<String>(Indication::IndicationTypePropertyName));
        
		if (whichIndication == Indication::Slur) {
		    str << ") ";
		} else if (whichIndication == Indication::Crescendo ||
			   whichIndication == Indication::Decrescendo) {
		    str << "\\! ";
		}
		eventsInProgress.erase(k);
	    } else {
		std::cerr << "\n[Lilypond] Unhandled Deferred Ending Event type: " << (*k)->getType();
	    }
	}
    }
}

//deeply rooted here and in musicxmlio.cpp, avoid touching
//needlessly
char
convertPitchToName(int pitch, bool isFlatKeySignature)
{
    // shift to a->g, rather than c->b
    pitch += 3;
    pitch %= 12;

    // For flat key signatures, accidentals use next note name
    if (isFlatKeySignature &&
	(pitch == 1 || pitch == 4 ||
	 pitch == 6 || pitch == 9 || pitch == 11)) {
	pitch++;
    }
    pitch = pitch % 12;

    // compensate for no c-flat or f-flat
    //
    // DMM - FIXME
    // there's just no way to resolve B/C E/F without key sig so you have to
    // figure that out after all.  get number of accidentals for the key out
    // somehow and if that number is > or < a threshold you can assume the Cb
    // B# without actually having to calculate everything for the key sig
    // probably - do the math
    if (pitch > 2) {
	pitch++;
    }
    if (pitch > 8) {
	pitch++;
    }
    char pitchLetter = (char)((int)(pitch/2) + 97);
    return pitchLetter;
}

//DMM - A note needs an accidental for Lilypond purposes if it is one of the
//"black key" notes (C# D# F# G# A#) because these must be written out
//with a manual spelling to suit the key signature/accidental
bool
needsAccidental(int pitch) {
    if (pitch > 4) {
	pitch++;
    }
    return (pitch % 2 == 1);
}

bool
LilypondExporter::write()
{
    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        std::cerr << "LilypondExporter::write() - can't write file " << m_fileName << std::endl;
        return false;
    }

    // Lilypond header information
    // DMM - FIXME  we need to check for illegal characters inside user strings
    // here (and elsewhere) and protect them...
    str << "\\version \"1.4.10\"\n";
    str << "\\header {\n";
    str << "\ttitle = \"" << "Title" << "\"\n";  // needs new variable
    str << "\tsubtitle = \"" << "subtitle" << "\"\n"; // needs new variable
    // DMM - TODO hook up new variables here when the GUI guys provide them:
    // meta events like:
    // <metadata>
    //<copyright></copyright>
    //</metadata>
    // go look at how to get these then probably
    // str them out with a while loop or something of the sort
    //
    // probably need help figuring out just what to do here

    str << "\tfooter = \"Rosegarden " << VERSION << "\"\n";
    //DMM - FIXME - read about string class stuff to figure out how to pretty
    //up the filename  (basename)
    str << "\ttagline = \"Exported from" << m_fileName << " by Rosegarden " << VERSION "\"\n";   
/*    str << "\tcopyright = \"";
    try {
        if (m_composition->getCopyrightNote() != "") {
            str << m_composition->getCopyrightNote();  //incomplete??  check illegal chars
        }
    }
    catch (...) {
        //an exception occurred
    }            
             
    str << "\"\n"; */
    
    str << "}\n";

    // Lilypond music data!   Mapping:
    // Lilypond Voice = Rosegarden Segment
    // Lilypond Staff = Rosegarden Track
    // (not the cleanest output but maybe the most reliable)
    // Incomplete: add an option to cram it all into one grand staff
    //
    // DMM - possibly hook into dialog to allow export of selected tracks with
    // toggle to render as grand staff
    str << "\\score {\n";
    str << "\t\\notes <\n";

    // Make chords offset colliding notes by default
    str << "\t\t\\property Score.NoteColumn \\override #\'force-hshift = #1.0\n";

    // Incomplete: Handle time signature changes
    // i.e. getTimeSignatureCount, getTimeSignatureChange
    TimeSignature timeSignature = m_composition->getTimeSignatureAt(m_composition->getStartMarker());
    str << "\t\t\\time "
        << timeSignature.getNumerator() << "/"
        << timeSignature.getDenominator() << "\n";

    bool isFlatKeySignature = false;
    int lastTrackIndex = -1;
    int voiceCounter = 0;

    // Lilypond remembers the duration of the last note or
    // rest and reuses it unless explicitly changed.
    Note::Type lastType = Note::QuarterNote;
    int lastNumDots = 0;

    // Write out all segments for each Track
    for (Composition::iterator i = m_composition->begin();
	 i != m_composition->end(); ++i) {

	timeT lastChordTime = m_composition->getStartMarker() - 1;
	bool currentlyWritingChord = false;
      
	// We may need to wait before adding a tie or slur
	// if we are currently in a chord
	bool addTie = false;

	if ((*i)->getTrack() != lastTrackIndex) {  //cast to shut up warning? look at which is which
	    if (lastTrackIndex != -1) {
		// Close the old track (staff)
		str << "\n\t\t>\n";
	    }
	    lastTrackIndex = (*i)->getTrack();
	    // Will there be problems with quotes, spaces, etc. in staff/track labels?
	    str << "\t\t\\context Staff = \"" << m_composition->getTrackByIndex(lastTrackIndex)->getLabel() << "\" <\n";
	    str << "\t\t\t\\property Staff.instrument = \"" << m_composition->getTrackByIndex(lastTrackIndex)->getLabel() << "\"\n";
	
	}
	SegmentNotationHelper tmpHelper(**i);

	// Temporary storage for non-atomic events (!BOOM)
	// ex. Lilypond expects signals when a decrescendo starts 
	// as well as when it ends
	eventendlist eventsInProgress;
	eventstartlist eventsToStart;
      
        // If the segment doesn't start at 0, add a "skip" to the start
        // No worries about overlapping segments, because Voices can overlap
        // voiceCounter is a hack because Lilypond does not by default make 
        // them unique
        str << "\t\t\t\\context Voice = \"voice" << voiceCounter++ << "\" {\n";
        timeT segmentStart = (*i)->getStartTime(); // getFirstEventTime
        if (segmentStart > 0) {
            long curNote = long(Note(Note::WholeNote).getDuration());
            long wholeNoteDuration = curNote;
            // Incomplete: Make this a constant!
            // This is the smallest unit on which a Segment may begin
            long MIN_NOTE_SKIP_DURATION = long(Note(Note::ThirtySecondNote).getDuration());
            while (curNote >= MIN_NOTE_SKIP_DURATION) {
                int numCurNotes = ((int)(segmentStart / curNote));
                if (numCurNotes > 0) {
                    str << "\\skip " << (wholeNoteDuration / curNote) << "*" << numCurNotes << "\n";
                    segmentStart = segmentStart - numCurNotes*curNote;
                }
                curNote /= 2;
            }
        }

	timeT prevTime = 0;
        int curTupletNotesRemaining = 0;
        // Write out all events for this Segment
        for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {

	    // We need to deal with absolute time if we don't have 
	    // "complete" lines of music and/or nonoverlapping events
	    // i.e. chords etc.
	    timeT absoluteTime = (*j)->getAbsoluteTime();

	    // cc: new bar?
	    if (j == (*i)->begin() ||
		(prevTime < m_composition->getBarStartForTime(absoluteTime))) {
		str << "\n\t\t\t";
	    }
	    prevTime = absoluteTime;

        //DMM - TODO - text events probably go in here...
        //->isa(Text::EventType) ????
        //
        //probably bind to the following note, so just grab the event and ++
        //the iterator to get a new note...  or something, then figure out
        //what kind of text we've got and format it accordingly
        //
            timeT duration = (*j)->getDuration();
            if ((*j)->isa(Note::EventType) ||
                (*j)->isa(Note::EventRestType)) {
                // Tuplet code from notationhlayout.cpp
                int tcount = 0;
                int ucount = 0;
                if ((*j)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
                    tcount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_TUPLED_COUNT);
                    ucount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT);
                    assert(tcount != 0);

                    duration = ((*j)->getDuration() / tcount) * ucount;
                    if (curTupletNotesRemaining == 0) {
                        // +1 is a hack so we can close the tuplet bracket
                        // at the right time
                        curTupletNotesRemaining = ucount + 1;
                    }
                }
                Note tmpNote = Note::getNearestNote(duration, MAX_DOTS);

                if ((*j)->isa(Note::EventType)) {
		    // Algorithm for writing chords:
		    // 1) Close the old chord
		    //   - if there is one
		    //   - if the next note is not part of the old chord
		    // 2) Open the new chord
		    //   - if the next note is in a chord
		    //   - and we're not writing one now
		    bool nextNoteIsInChord = tmpHelper.noteIsInChord(*j);
		    if (currentlyWritingChord &&
			(!nextNoteIsInChord ||
			 (nextNoteIsInChord && lastChordTime != absoluteTime))) {
			str << "> ";
			currentlyWritingChord = false;
		    }
                    if (curTupletNotesRemaining > 0) {
                        curTupletNotesRemaining--;
                        if (curTupletNotesRemaining == 0) {
                            str << "} ";
                        }
                    }
                    if (ucount == curTupletNotesRemaining &&
                        ucount != 0) {
                        str << "\\times " << tcount << "/" << ucount << " { ";
                    }
		    if (nextNoteIsInChord && !currentlyWritingChord) {
			currentlyWritingChord = true;
			str << "< ";
			lastChordTime = absoluteTime;
		    }

		    // Incomplete: velocity?
		    long velocity = 127;
		    (*j)->get<Int>(BaseProperties::VELOCITY, velocity);

		    handleEndingEvents(eventsInProgress, j, str);

		    // Note pitch (need name as well as octave)
		    // It is also possible to have "relative" pitches,
		    // but for simplicity we always use absolute pitch
		    // 60 is middle C, one unit is a half-step
		    long pitch = 0;
            Rosegarden::Accidental accidental; //DMM
            std::string lilyAccidental;        //DMM
            bool flatAccidental = false;       //DMM
            bool doubleAccidental = false;     //DMM
            char notename, lilynote;           //DMM


            //DMM - allow intentional enharmonic variations to override the
            //key...  BaseProperties::ACCIDENTAL will be set (by user) for an
            //out-of-key variation, so we grab it and deal with it first
            if ((*j)->get<String>(BaseProperties::ACCIDENTAL, accidental)) {
                if (accidental == Rosegarden::Accidentals::Sharp) {
                    lilyAccidental = "is";
                    flatAccidental = false;
                } else if (accidental == Rosegarden::Accidentals::DoubleSharp) {
                    lilyAccidental = "isis";
                    flatAccidental = false;
                    doubleAccidental = true;
                } else if (accidental == Rosegarden::Accidentals::Flat) {
                    lilyAccidental = "es";
                    flatAccidental = true;
                } else if (accidental == Rosegarden::Accidentals::DoubleFlat) {
                    lilyAccidental = "eses";
                    flatAccidental = true;
                    doubleAccidental = true;
                }                
            } else {               
            //then we revert to the key (doubles will always be out of key
            //because RG doesn't allow extreme key signatures)
                if (isFlatKeySignature) {
                    lilyAccidental = "es";
                    flatAccidental = true;
                } else {
                    lilyAccidental = "is";
                } 
            }

            //broken case:  E# comes out F-nat in G#m test scale  Will
            //probably break for B# too...  bug in cPTN logic?
            //
            // no, see note in cPTN, need keg sig + num accidents
            //
            //broken case:  Bbb comes out 8va  - deal with octave break
                
            //get pitch letter and add necessary accidental
		    (*j)->get<Int>(BaseProperties::PITCH, pitch);
            notename = convertPitchToName(pitch % 12, flatAccidental);
            lilynote = notename;
            if (doubleAccidental) {  //adjust note letter for x/bb w/o hacking on
                                     //convertPitchToName and breaking tons of
                                     //other code
                if (flatAccidental) {
                    if (++lilynote > 'g') lilynote = 'a';
                } else {
                    if (--lilynote < 'a') lilynote = 'g';
                }
            }
		    str << lilynote;
		    if (needsAccidental(pitch % 12)||doubleAccidental) {
                str << lilyAccidental;
            } 

            //ugly, temporary debug stuff, ignore
            std::cout << "lilypondio: pitch = " << pitch
                    << " pitch%12 = " << (pitch % 12) 
                    << " lilynote = " << lilynote
                    << " notename = " << notename
                    << " lilyAccidental = " << lilyAccidental
                    << std::endl;
                                
/*			if (isFlatKeySignature) {  (Boy this sure was an easier way out. :)
			    str << "es";
			} else {
			    str << "is";
			}
*/
            

            int octave = (int)(pitch / 12);
            if (octave < 4) {
                for (; octave < 4; octave++) {
                    str << ",";
                }
            } else {
                for (; octave > 4; octave--) {
                    str << "\'";
                }
            }
                } else { // it's a rest
            if (currentlyWritingChord) {
            currentlyWritingChord = false;
			handleStartingEvents(eventsToStart, addTie, str);
			str << "> ";
		    }
                    if (curTupletNotesRemaining > 0) {
                        curTupletNotesRemaining--;
                        if (curTupletNotesRemaining == 0) {
                            str << "} ";
                        }
                    }
                    if (ucount == curTupletNotesRemaining &&
                        ucount != 0) {
                        str << "\\times " << tcount << "/" << ucount << " { ";
                    }
		    handleEndingEvents(eventsInProgress, j, str);
		    str << "r";
                }
                
                // Note/rest length (type), including dots
                if (tmpNote.getNoteType() != lastType ||
                    tmpNote.getDots() != lastNumDots) {
		    switch (tmpNote.getNoteType()) {
                    case Note::SixtyFourthNote:
			str << "64"; break;
                    case Note::ThirtySecondNote:
			str << "32"; break;
                    case Note::SixteenthNote:
			str << "16"; break;
                    case Note::EighthNote:
			str << "8"; break;
                    case Note::QuarterNote:
			str << "4"; break;
                    case Note::HalfNote:
			str << "2"; break;
                    case Note::WholeNote:
			str << "1"; break;
                   case Note::DoubleWholeNote:
                       str << "\\breve"; break;
		    }
		    lastType = tmpNote.getNoteType();
		    for (int numDots = 0; numDots < tmpNote.getDots(); numDots++) {
			str << ".";
		    }
		    lastNumDots = tmpNote.getDots();
                }

                // Add a tie if necessary (or postpone it if we're in a chord)
                bool tiedForward = false;
                (*j)->get<Bool>(BaseProperties::TIED_FORWARD, tiedForward);
                if (tiedForward) {
		    // Lilypond doesn't like ties inside chords, so defer writing
		    // This should go in eventsToStart, but ties aren't Indications
		    addTie = true;
                }
                
                // Add a space before the next note/event
                str << " ";

                if (!currentlyWritingChord) {
		    // And write any deferred start-of-events
		    handleStartingEvents(eventsToStart, addTie, str);
                }
	    } else if ((*j)->isa(Clef::EventType)) {
		if (currentlyWritingChord) {
		    // Incomplete: Consolidate
		    currentlyWritingChord = false;
		    handleStartingEvents(eventsToStart, addTie, str);
		    str << "> ";
		}

		// Incomplete: Set which note the clef should center on
//cc		str << "\t\t\t\\clef ";
		str << "\\clef ";
		std::string whichClef((*j)->get<String>(Clef::ClefPropertyName));
		if (whichClef == Clef::Treble) { str << "treble\n"; }
		else if (whichClef == Clef::Tenor) { str << "tenor\n"; }
		else if (whichClef == Clef::Alto) { str << "alto\n"; }
		else if (whichClef == Clef::Bass) { str << "bass\n"; }
		str << "\t\t\t"; //cc

	    } else if ((*j)->isa(Key::EventType)) {
		if (currentlyWritingChord) {
		    // Incomplete: Consolidate
		    currentlyWritingChord = false;
		    handleStartingEvents(eventsToStart, addTie, str);
		    str << "> ";
		}

//cc		str << "\n\t\t\t\\key ";
		str << "\\key ";
		Key whichKey(**j);
		isFlatKeySignature = !whichKey.isSharp();
		str << convertPitchToName(whichKey.getTonicPitch(), isFlatKeySignature);

		if (needsAccidental(whichKey.getTonicPitch())) {
		    if (isFlatKeySignature) {
			str << "es";
		    } else {
			str << "is";
		    }
		}

		if (whichKey.isMinor()) {
		    str << " \\minor";
		} else {
		    str << " \\major";
		}
		str << "\n\t\t\t";
	    } else if ((*j)->isa(Indication::EventType)) {
		// Handle the end of these events when it's time
		eventsToStart.insert(*j);
		eventsInProgress.insert(*j);
	    } else {
		std::cerr << "\n[Lilypond] Unhandled Event type: " << (*j)->getType();
	    }
	}
	if (currentlyWritingChord) {
	    // Incomplete: Consolidate
	    currentlyWritingChord = false;
	    handleStartingEvents(eventsToStart, addTie, str);
	    str << "> ";
	}
	// Close the voice
	str << "\n\t\t\t}\n";
    }
    // Close the last track (staff)
    str << "\n\t\t>\n";
    
//     int tempoCount = m_composition->getTempoChangeCount();

//     if (tempoCount > 0) {

// 	str << "\nt ";

// 	for (int i = 0; i < tempoCount - 1; ++i) {

// 	    std::pair<Rosegarden::timeT, long> tempoChange = 
// 		m_composition->getRawTempoChange(i);

// 	    timeT myTime = tempoChange.first;
// 	    timeT nextTime = myTime;
// 	    if (i < m_composition->getTempoChangeCount()-1) {
// 		nextTime = m_composition->getRawTempoChange(i+1).first;
// 	    }
	    
// 	    int tempo = tempoChange.second / 60;

// 	    str << convertTime(  myTime) << " " << tempo << " "
// 		<< convertTime(nextTime) << " " << tempo << " ";
// 	}

// 	str << convertTime(m_composition->getRawTempoChange(tempoCount-1).first)
// 	    << " "
// 	    << m_composition->getRawTempoChange(tempoCount-1).second/60
// 	    << std::endl;
//     }

    str << "\n\t>\n"; // close notes section

    // Incomplete: Add paper info?
    //
    // DMM - yes, should be in Lilypond Export Properties list or whatever,
    // to set, eg. A4 vs. US letter and possibly other parameters
    str << "\\paper {}\n";

    str << "}\n" << std::endl; // close score section
    str.close();
    return true;
}
