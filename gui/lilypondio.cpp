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

    Enharmonics fix and trivial additions by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h> // i18n
#include <kapp.h>

#include <qstring.h>
#include <qregexp.h> // QT3.0 replace()

#include "lilypondio.h"
#include "config.h"
#include "rosestrings.h" // strtoqstr

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
using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Text;
using Rosegarden::PropertyName;
using Rosegarden::Marks;

LilypondExporter::LilypondExporter(QObject *parent,
                                   Composition *composition,
                                   std::string fileName) :
    ProgressReporter(parent, "lilypondExporter"),
    m_composition(composition),
    m_fileName(fileName) {
    // nothing else
}

LilypondExporter::~LilypondExporter() {
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
                std::cerr << "\nLilypondExporter::handleEndingEvents - unhandled deferred ending event, type: " << (*k)->getType();
            }
        }
    }
}

// processes input to produce a Lilypond-format note written correctly for all
// keys and out-of-key accidental combinations.
std::string
LilypondExporter::convertPitchToLilyNote (long pitch, bool isFlatKeySignature,
                        int accidentalCount, Accidental accidental) {
    std::string lilyNote = "";
    int pitchNote, c;
    
    // get raw semitone number
    pitchNote = (pitch % 12);

    // no accidental, or notes with Natural property
    switch (pitchNote) {
        case 0:  lilyNote = "c";
                 break;
        case 2:  lilyNote = "d";
                 break;
        case 4:  lilyNote = "e";
                 break;
        case 5:  lilyNote = "f";
                 break;
        case 7:  lilyNote = "g";
                 break;
        case 9:  lilyNote = "a";
                 break;
        case 11: lilyNote = "b";
                 break;
        // failsafe to deal with annoying fact that imported/recorded notes don't have
        // persistent accidental properties
        case 1:  lilyNote = (isFlatKeySignature) ? "des" : "cis";
                 break;
        case 3:  lilyNote = (isFlatKeySignature) ? "ees" : "dis";
                 break;
        case 6:  lilyNote = (isFlatKeySignature) ? "ges" : "fis";
                 break;
        case 8:  lilyNote = (isFlatKeySignature) ? "aes" : "gis";
                 break;
        case 10: lilyNote = (isFlatKeySignature) ? "bes" : "ais";
                 break;
    }
        
    // assign out-of-key accidentals first, by BaseProperty::ACCIDENTAL
    if (accidental != "") {
        if (accidental == Accidentals::Sharp) {
            switch (pitchNote) {
                case  5: lilyNote = "eis"; // 5 + Sharp = E#
                         break;
                case  0: lilyNote = "bis"; // 0 + Sharp = B#
                         break;
                case  1: lilyNote = "cis";
                         break;
                case  3: lilyNote = "dis";
                         break;
                case  6: lilyNote = "fis";
                         break;
                case  8: lilyNote = "gis";
                         break;
                case 10: lilyNote = "ais";
            }
        } else if (accidental == Accidentals::Flat) {
            switch (pitchNote) {
                case 11: lilyNote = "ces"; // 11 + Flat = Cb
                         break;
                case  4: lilyNote = "fes"; //  4 + Flat = Fb
                         break;
                case  1: lilyNote = "des";
                         break;
                case  3: lilyNote = "ees";
                         break;
                case  6: lilyNote = "ges";
                         break;
                case  8: lilyNote = "aes";
                         break;
                case 10: lilyNote = "bes";
            }
        } else if (accidental == Accidentals::DoubleSharp) {
            switch (pitchNote) {
                case  1: lilyNote = "bisis"; // 1 + ## = B##
                         break;
                case  2: lilyNote = "cisis"; // 2 + ## = C##
                         break;
                case  4: lilyNote = "disis"; // 4 + ## = D##
                         break;
                case  6: lilyNote = "eisis"; // 6 + ## = E##
                         break;
                case  7: lilyNote = "fisis"; // 7 + ## = F##
                         break;
                case  9: lilyNote = "gisis"; // 9 + ## = G##
                         break;
                case 11: lilyNote = "aisis"; //11 + ## = A##
                         break;
            }
        } else if (accidental == Accidentals::DoubleFlat) {
            switch (pitchNote) {
                case 10: lilyNote = "ceses"; //10 + bb = Cbb
                         break;
                case  0: lilyNote = "deses"; // 0 + bb = Dbb
                         break;
                case  2: lilyNote = "eeses"; // 2 + bb = Ebb
                         break;
                case  3: lilyNote = "feses"; // 3 + bb = Fbb
                         break;
                case  5: lilyNote = "geses"; // 5 + bb = Gbb
                         break;
                case  7: lilyNote = "aeses"; // 7 + bb = Abb
                         break;
                case  9: lilyNote = "beses"; // 9 + bb = Bbb
                         break;
            }
        } else if (accidental == Accidentals::Natural) {
            // do we have anything explicit left to do in this
            // case?  probably not, but I'll leave this placeholder for now
            //
            // eg. note is B + Natural in key Cb, but since it has Natural
            // it winds up here, instead of getting the Cb from the key below.
            // since it will be called "b" from the first switch statement in
            // the entry to this complex logic block, and nothing here changes it,
            // the implicit handling to this point should resolve the case
            // without further effort.
        }
    } else {  // no explicit accidental; note must be in-key
        for (c = 0; c <= accidentalCount; c++) {
            if (isFlatKeySignature) {                              // Flat Keys:
                switch (c) {
                    case 7: if (pitchNote == 11) lilyNote = "ces"; // Cb 
                    case 6: if (pitchNote ==  4) lilyNote = "fes"; // Fb 
                    case 5: if (pitchNote ==  6) lilyNote = "ges"; // Gb 
                    case 4: if (pitchNote ==  1) lilyNote = "des"; // Db 
                    case 3: if (pitchNote ==  3) lilyNote = "ees"; // Eb 
                    case 2: if (pitchNote ==  8) lilyNote = "aes"; // Ab 
                    case 1: if (pitchNote == 10) lilyNote = "bes"; // Bb 
                }
            } else {                                               // Sharp Keys:
                switch (c) {                                       
                    case 7: if (pitchNote ==  0) lilyNote = "bis"; // C# 
                    case 6: if (pitchNote ==  5) lilyNote = "eis"; // F# 
                    case 5: if (pitchNote == 10) lilyNote = "ais"; // B  
                    case 3: if (pitchNote ==  8) lilyNote = "gis"; // A  
                    case 4: if (pitchNote ==  3) lilyNote = "dis"; // D  
                    case 2: if (pitchNote ==  1) lilyNote = "cis"; // D  
                    case 1: if (pitchNote ==  6) lilyNote = "fis"; // G  
                }
           }
       } 
    }

    // leave this in to see if there are any _other_ problems that are going
    // to break this method...
    if (lilyNote == "") {
        std::cerr << "LilypondExporter::convertPitchToLilyNote() -  WARNING: cannot resolve note"
                  << std::endl << "pitch = " << pitchNote << "\tkey sig. = "
                  << ((isFlatKeySignature) ? "flat" : "sharp") << "\tno. of accidentals = "
                  << accidentalCount << "\textra accidental = \"" << accidental << "\""
                  << std::endl;
        
    }

    return lilyNote;
}

// composes and returns a string to be appended to a note in order to
// represent various Marks (tenuto, accents, etc.)
//
// warning:  text markup features are slated to be re-written
// and the syntax/semantics completely altered, so this will probably
// need extensive reworking eventually, possibly with version awareness
// 
std::string
LilypondExporter::composeLilyMark(std::string eventMark, bool stemUp) {

    std::string inStr = "", outStr = "";
    std::string prefix = (stemUp) ? "_" : "^";
    
    // shoot text mark straight through unless it's sf or rf
    if (Marks::isTextMark(eventMark)) {
        inStr = Marks::getTextFromMark(eventMark);
        
        if (inStr == "sf") {
            inStr = "\\sf";  // try this
        } else if (inStr == "rf") {
            inStr = "\\rfz";
        } else {        
            inStr = "\"" + inStr + "\"";
        }

        outStr = prefix + inStr;
        
    } else {
        
        // property calculated by the notation editor at view time.  property
        // can be made persistent for all notes by changing a single stem
        // manually, but that leaves much to be desired.  Consequently, we'll
        // let Lilypond make as many decisions as possible, and will only use
        // stemUp to make direction decisions when it's unavoidable
        outStr = "-";

        // use full \accent format for everything, even though some shortcuts
        // exist, for the sake of consistency
        if (eventMark == Marks::Accent) {
            outStr += "\\accent";
        } else if (eventMark == Marks::Tenuto) {
            outStr += "\\tenuto";
        } else if (eventMark == Marks::Staccato) {
            outStr += "\\staccato";
        } else if (eventMark == Marks::Staccatissimo) {
            outStr += "\\staccatissimo";
        } else if (eventMark == Marks::Marcato) {
            outStr += "\\marcato";
        } else if (eventMark == Marks::Trill) {
            outStr += "\\trill";
        } else if (eventMark == Marks::Turn) {
            outStr += "\\turn";
        } else if (eventMark == Marks::Pause) {
            outStr += "\\fermata";
        } else if (eventMark == Marks::UpBow) {
            outStr += "\\upbow";
        } else if (eventMark == Marks::DownBow) {
            outStr += "\\downbow";
        } else {
            outStr = "";
            std::cerr << "LilypondExporter::composeLilyMark() - unhandled mark:  "
                      << eventMark << std::endl;
        }
    }

    return outStr;
}

bool
LilypondExporter::write() {
    QString tmp_fileName = strtoqstr(m_fileName);
    // The correct behaviour is to show an error box warning the user -
    // but for now, just remove the illegal chars automatically.
//     if (tmp_fileName.contains(' ') || tmp_fileName.contains('\\')) {
//         // Error: "Lilypond does not allow spaces or backslashes in
//         // the name of the .ly file.  Would you like RG to remove them
//         // automatically or go back and change the filename yourself?"
//         // [Remove] [Cancel Save]
//         return false;
//     }
    tmp_fileName.replace(QRegExp(" "), "");
    tmp_fileName.replace(QRegExp("\\\\"), "");
    tmp_fileName.replace(QRegExp("'"), "");
    tmp_fileName.replace(QRegExp("\""), "");

    std::ofstream str(qstrtostr(tmp_fileName).c_str(), std::ios::out);
    if (!str) {
        std::cerr << i18n("LilypondExporter::write() - can't write file %1").arg(tmp_fileName) << std::endl;
        return false;
    }

    // Lilypond header information
    str << "\\version \"1.4.10\"\n";
    // user-specified headers coming in from new metadata eventually ???
    tmp_fileName.replace(QRegExp("&"), "\\&");
    tmp_fileName.replace(QRegExp("_"), "\\_");
    str << "\\header {\n";
    str << "\ttitle = \"" << tmp_fileName << "\"\n";
    str << "\tsubtitle = \"subtitle\"\n";
    str << "\tfooter = \"Rosegarden " << VERSION << "\"\n";
    str << "\ttagline = \"Exported from " << tmp_fileName << " by Rosegarden " << VERSION << "\"\n";

    try {
       if (m_composition->getCopyrightNote() != "") {
           str << "\tcopyright = \""
               //??? Incomplete: need to remove newlines from copyright note?
                << m_composition->getCopyrightNote() << "\"\n";
       }
    }

    catch (...) {
        // nothing to do?
        ;
    }
    
    str << "}\n";

    // Lilypond music data!   Mapping:
    // Lilypond Voice = Rosegarden Segment
    // Lilypond Staff = Rosegarden Track
    // (not the cleanest output but maybe the most reliable)
    // Incomplete: add an option to cram it all into one grand staff
    str << "\\score {\n";
    str << "\t\\notes <\n";

    // Make chords offset colliding notes by default
    str << "\t\t\\property Score.NoteColumn \\override #\'force-hshift = #1.0\n";
    
    // set initial time signature
    TimeSignature timeSignature = m_composition->
            getTimeSignatureAt(m_composition->getStartMarker());

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

    int trackNo = 0;

    // Write out all segments for each Track
    for (Composition::iterator i = m_composition->begin();
         i != m_composition->end(); ++i) {

        emit setProgress(int(double(trackNo++)/
                             double(m_composition->getNbTracks()) * 100.0));
        kapp->processEvents(50);

        timeT lastChordTime = m_composition->getStartMarker() - 1;
        bool currentlyWritingChord = false;
      
        // We may need to wait before adding a tie or slur
        // if we are currently in a chord
        bool addTie = false;

        if ((int) (*i)->getTrack() != lastTrackIndex) {
            if (lastTrackIndex != -1) {
                // Close the old track (staff)
                str << "\n\t\t>\n";
            }
            lastTrackIndex = (*i)->getTrack();
            // Will there be problems with quotes, spaces, etc. in staff/track labels?
            str << "\t\t\\context Staff = \"" << m_composition->
                    getTrackByIndex(lastTrackIndex)->getLabel() << "\" <\n";
            
            str << "\t\t\t\\property Staff.instrument = \"" << m_composition->
                    getTrackByIndex(lastTrackIndex)->getLabel() << "\"\n";
        
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
                    str << "\\skip " << (wholeNoteDuration / curNote)
                        << "*" << numCurNotes << "\n";
                    segmentStart = segmentStart - numCurNotes*curNote;
                }
                curNote /= 2;
            }
        }

        timeT prevTime = 0;
        int curTupletNotesRemaining = 0;
        int accidentalCount = 0; 
// WIP       int timeSignatureIterator = 0;

        std::string lilyText = "";  // must be declared outside the scope of the next for loop
        
        // Write out all events for this Segment
        for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {

            // We need to deal with absolute time if we don't have 
            // "complete" lines of music and/or nonoverlapping events
            // i.e. chords etc.
            timeT absoluteTime = (*j)->getAbsoluteTime();

            // new bar
            bool multipleTimeSignatures = ((m_composition->getTimeSignatureCount ()) > 1);
            
/*  WIP       long timeSignatureCount = m_composition->getTimeSignatureCount();
            bool multipleTimeSignatures = (timeSignatureCount > 1);

            if (multipleTimeSignatures) {
                timeSignatureIterator++;
                timeSignature = m_composition->getTimeSignatureChange(timeSignatureIterator); */
            
            if (j == (*i)->begin() ||
                (prevTime < m_composition->getBarStartForTime(absoluteTime))) {
                str << "\n\t\t\t";
                
                // DMM - this is really pretty much an ugly hack, but it works...
                // check time signature against previous one; if different,
                // write new one here at bar line...
                // FIXME - rewrite this to getTimeSignatureChange for time
                // sig. 1, look at "now" to see if it's time to write it, and
                // when we get to that point, ++ the time sig iterator to grab
                // the next change and be looking for that, etc. until out of
                // events.  Probably much less ugly and worth doing, but the
                // existing implementation works well enough to put off doing
                // that until after text events and marks are working....
                // just basically that the current method wastes a lot of CPU
                // cycles checking stuff that doesn't really have to be
                // checked because the times are known in advance if you look.
                //
                // (WIP)
                TimeSignature prevTimeSignature = timeSignature;
                if (multipleTimeSignatures) {
                    timeSignature = m_composition->getTimeSignatureAt(absoluteTime);
                    if (
                        (timeSignature.getNumerator() != prevTimeSignature.getNumerator())
                        || 
                        (timeSignature.getDenominator() != prevTimeSignature.getDenominator())
                        &&
                        !(timeSignature.isHidden())
                       ) {
                        str << "\\time "
                            << timeSignature.getNumerator() << "/"
                            << timeSignature.getDenominator() << "\n\t\t\t";
                    }
                }
            }
            
            prevTime = absoluteTime;

            timeT duration = (*j)->getNotationDuration();

            // handle text events...  unhandled text types:
            // UnspecifiedType, StaffName, ChordName, KeyName, Annotation
            //
            // text must be bound to a note, and it needs to be bound to the
            // note immediately after receiving the text event, so we'll
            // process it, then continue out of the loop, and then
            // write it out the next time we have a Note event.
            //
            // Incomplete?  There may be cases where binding it to the next
            // note arbitrarily is the wrong thing to do...
            if ((*j)->isa(Text::EventType)) {
                
                std::string text = "";
                (*j)->get<String>(Text::TextPropertyName, text);

                // Incomplete - these interpretations probably aren't very
                // good...  I need to do more research to see how these
                // type things are typically done in Lilypond.  The manual is
                // sparse in this area, and I don't have any examples.  This
                // will do for the time being anyway.

                if (Text::isTextOfType(*j, Text::Tempo)) {
                    // print above staff
                    lilyText = "^#'((bold Large)\"" + text + "\")";
                } else if (Text::isTextOfType(*j, Text::LocalTempo)) {
                    // print above staff
                    lilyText = "^#'(bold \"" + text + "\")";
                } else if (Text::isTextOfType(*j, Text::Lyric)) {
                    // print below staff
                    //
                    // Lilypond has a mechanism dedicated to handling lyrics,
                    // but making use of it seems like it would be very ugly.
                    // It would be necessary to assemble all the Text::Lyric
                    // events and then write them in a separate section, so
                    // let's see how _this_ approach is received...
                    lilyText = "_#\"" + text + "\"";
                } else if (Text::isTextOfType(*j, Text::Dynamic)) {
                    // pass through only supported types
                    if (text == "ppp" || text == "pp"  || text == "p"  ||
                        text == "mp"  || text == "mf"  || text == "f"  ||
                        text == "ff"  || text == "fff" || text == "rfz" ||
                        text == "sf") {
                        
                        lilyText = "-\\" + text;
                    } else {
                        std::cerr << "LilypondExporter::write() - illegal Lilypond dynamic: "
                                  << text << std::endl;
                    }                         
                } else if (Text::isTextOfType(*j, Text::Direction)) {
                    // print above staff
                    lilyText = "^#'((bold Large)\"" + text + "\")";
                } else if (Text::isTextOfType(*j, Text::LocalDirection)) {
                    // print above staff
                    lilyText = "^#'(bold \"" + text + "\")";
                } else {
                    (*j)->get<String>(Text::TextTypePropertyName, text);
                    std::cerr << "LilypondExporter::write() - unhandled text type: "
                              << text << std::endl;
                }
                
                // jump out of this for loop so that lilyText can be attached
                // to the next note we come to...  this might actually be
                // redundant, but it shouldn't hurt anything just in case
                continue;

            } else if ((*j)->isa(Note::EventType) ||
                        (*j)->isa(Note::EventRestType)) {
                // Tuplet code from notationhlayout.cpp
                int tcount = 0;
                int ucount = 0;
                if ((*j)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
                    tcount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_TUPLED_COUNT);
                    ucount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT);
                    assert(tcount != 0);

                    duration = ((*j)->getNotationDuration() / tcount) * ucount;
                    if (curTupletNotesRemaining == 0) {
                        // +1 is a hack so we can close the tuplet bracket
                        // at the right time
                        curTupletNotesRemaining = ucount + 1;
                    }
                }
                
                Note tmpNote = Note::getNearestNote(duration, MAX_DOTS);
                std::string lilyMark = "";

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

                    // format of Lilypond note is:
                    // name + (duration) + octave + text markup

                    // calculate note name and write note
                    Accidental accidental;
                    std::string lilyNote;

                    (*j)->get<Int>(BaseProperties::PITCH, pitch);
                    
                    (*j)->get<String>(BaseProperties::ACCIDENTAL, accidental);

                    lilyNote = convertPitchToLilyNote(pitch, isFlatKeySignature,
                                                  accidentalCount, accidental);

                    str << lilyNote;

                    // generate and write octave marks
                    std::string octaveMarks = "";
                    int octave = (int)(pitch / 12);

                    // tweak the octave break for B# / Cb
                    if ((lilyNote == "bisis")||(lilyNote == "bis")) {
                        octave--;
                    } else if ((lilyNote == "ceses")||(lilyNote == "ces")) {
                        octave++;
                    }

                    if (octave < 4) {
                        for (; octave < 4; octave++) octaveMarks += ",";
                    } else {
                        for (; octave > 4; octave--) octaveMarks += "\'";
                    }

                    str << octaveMarks;

                    // string together Marks, to be written outside this if
                    // block we're in, after the duration is written
                    long markCount = 0;
                    bool stemUp;
                    std::string eventMark = "",
                                stem = "";
                    
                    (*j)->get<Int>(BaseProperties::MARK_COUNT, markCount);
                    (*j)->get<Bool>(BaseProperties::STEM_UP, stemUp);

                    if (markCount > 0) {
                        for (int c = 0; c < markCount; c++) {
                            (*j)->get<String>(BaseProperties::getMarkPropertyName(c), eventMark);
                            std::string mark = composeLilyMark(eventMark, stemUp);
                            lilyMark += mark;
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
                } // end isa Note

                // write string of marks after duration
                if (lilyMark != "") {
                    str << lilyMark;
                }

                // write text carried forward from previous iteration of the
                // for loop we're in
                if (lilyText != "") {
                    str << lilyText;
                    lilyText = "";  // purge now that text has been bound to note
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
                str << "\\clef ";
                std::string whichClef((*j)->get<String>(Clef::ClefPropertyName));
                if (whichClef == Clef::Treble) { str << "treble\n"; }
                else if (whichClef == Clef::Tenor) { str << "tenor\n"; }
                else if (whichClef == Clef::Alto) { str << "alto\n"; }
                else if (whichClef == Clef::Bass) { str << "bass\n"; }
                str << "\t\t\t"; //cc

            } else if ((*j)->isa(Rosegarden::Key::EventType)) {
                if (currentlyWritingChord) {
                    // Incomplete: Consolidate
                    currentlyWritingChord = false;
                    handleStartingEvents(eventsToStart, addTie, str);
                    str << "> ";
                }

                str << "\\key ";
                Rosegarden::Key whichKey(**j);
                isFlatKeySignature = !whichKey.isSharp();
                accidentalCount = whichKey.getAccidentalCount();
                
                str << convertPitchToLilyNote(whichKey.getTonicPitch(), isFlatKeySignature,
                                              accidentalCount, "");
                
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
                std::cerr << "\nLilypondExporter::write() - unhandled event type: "
                          << (*j)->getType();
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

//      str << "\nt ";

//      for (int i = 0; i < tempoCount - 1; ++i) {

//          std::pair<Rosegarden::timeT, long> tempoChange = 
//              m_composition->getRawTempoChange(i);

//          timeT myTime = tempoChange.first;
//          timeT nextTime = myTime;
//          if (i < m_composition->getTempoChangeCount()-1) {
//              nextTime = m_composition->getRawTempoChange(i+1).first;
//          }
            
//          int tempo = tempoChange.second / 60;

//          str << convertTime(  myTime) << " " << tempo << " "
//              << convertTime(nextTime) << " " << tempo << " ";
//      }

//      str << convertTime(m_composition->getRawTempoChange(tempoCount-1).first)
//          << " "
//          << m_composition->getRawTempoChange(tempoCount-1).second/60
//          << std::endl;
//     }

    str << "\n\t>\n"; // close notes section

    // Incomplete: Add paper info?
    str << "\\paper {}\n";

    str << "}\n" << std::endl; // close score section
    str.close();
    return true;
}
