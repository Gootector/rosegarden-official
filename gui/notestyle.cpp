// -*- c-basic-offset: 4 -*-

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

#include "notestyle.h"
#include "notefont.h"

#include <qfileinfo.h>
#include <qdir.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include "rosestrings.h"
#include "rosedebug.h"


using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Mark;
using Rosegarden::Marks;
using Rosegarden::Clef;
using Rosegarden::Note;


namespace StandardNoteStyleNames
{
    const NoteStyleName Classical = "Classical";
    const NoteStyleName Cross = "Cross";
    const NoteStyleName Triangle = "Triangle";
    const NoteStyleName Mensural = "Mensural";

/*!!!
    const NoteStyleName Cross     = "_Cross";
    const NoteStyleName Triangle  = "_Triangle";
    const NoteStyleName Mensural  = "_Mensural";
*/

    std::vector<NoteStyleName> getStandardStyles() {

	static NoteStyleName a[] = {
	    Classical, Cross, Triangle, Mensural
	};

	static std::vector<NoteStyleName> v;
	if (v.size() == 0) {
	    for (unsigned int i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
		v.push_back(a[i]);
	}
	return v;
    }

}


NoteStyle *
NoteStyleFactory::getStyle(NoteStyleName name)
{
    StyleMap::iterator i = m_styles.find(name);

    if (i == m_styles.end()) {

	try {

	    NoteStyle *newStyle = 0;

//	    if (name == StandardNoteStyleNames::Classical) {
//		newStyle = new ClassicalNoteStyle();

/*!!!
	    } else if (name == StandardNoteStyleNames::Cross) {
		newStyle = new CrossNoteStyle(); 

	    } else if (name == StandardNoteStyleNames::Triangle) {
		newStyle = new TriangleNoteStyle();

	    } else if (name == StandardNoteStyleNames::Mensural) {
		newStyle = new MensuralNoteStyle();
*/
//	    } else {
		newStyle = new CustomNoteStyle(name);
//	    }
	
	    m_styles[name] = newStyle;
	    return newStyle;

	} catch (CustomNoteStyle::StyleFileReadFailed f) {
	    kdDebug(KDEBUG_AREA)
		<< "NoteStyleFactory::getStyle: Style file read failed: "
		<< f.reason << endl;
	    throw StyleUnavailable("Style file read failed: " + f.reason);
	}

    } else {
	return i->second;
    }
}

NoteStyleFactory::StyleMap NoteStyleFactory::m_styles;



NoteStyle::~NoteStyle()
{
    // nothing
}

const NoteStyle::NoteHeadShape NoteStyle::AngledOval = "angled oval";
const NoteStyle::NoteHeadShape NoteStyle::LevelOval = "level oval";
const NoteStyle::NoteHeadShape NoteStyle::Breve = "breve";
const NoteStyle::NoteHeadShape NoteStyle::Cross = "cross";
const NoteStyle::NoteHeadShape NoteStyle::TriangleUp = "triangle up";
const NoteStyle::NoteHeadShape NoteStyle::TriangleDown = "triangle down";
const NoteStyle::NoteHeadShape NoteStyle::Diamond = "diamond";
const NoteStyle::NoteHeadShape NoteStyle::Rectangle = "rectangle";
const NoteStyle::NoteHeadShape NoteStyle::Number = "number";


//!!! Need to amend all uses of the Note presentation accessors
// to use these instead -- so we can get rid of the Note methods
// that these are intended to replace


NoteStyle::NoteHeadShape
NoteStyle::getShape(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getShape(type);
	kdDebug(KDEBUG_AREA)
	    << "WARNING: NoteStyle::getShape: No shape defined for note type "
	    << type << ", defaulting to AngledOval" << endl;
	return AngledOval;
    }

    return i->second.shape;
}


bool
NoteStyle::isFilled(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->isFilled(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::isFilled: No definition for note type "
	    << type << ", defaulting to true" << endl;
	return true;
    }

    return i->second.filled;
}


bool
NoteStyle::hasStem(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->hasStem(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::hasStem: No definition for note type "
	    << type << ", defaulting to true" << endl;
	return true;
    }

    return i->second.stem;
}


int
NoteStyle::getFlagCount(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getFlagCount(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::getFlagCount: No definition for note type "
	    << type << ", defaulting to 0" << endl;
	return 0;
    }

    return i->second.flags;
}


CharName
NoteStyle::getNoteHeadCharName(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getNoteHeadCharName(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::getNoteHeadCharName: No definition for note type "
	    << type << ", defaulting to NOTEHEAD_BLACK" << endl;
	return NoteCharacterNames::NOTEHEAD_BLACK;
    }

    const NoteDescription &desc(i->second);

    if (desc.shape == AngledOval) {

	return desc.filled ? NoteCharacterNames::NOTEHEAD_BLACK
	                   : NoteCharacterNames::VOID_NOTEHEAD;

    } else if (desc.shape == LevelOval) {

	return desc.filled ? NoteCharacterNames::UNKNOWN //!!!
	                   : NoteCharacterNames::WHOLE_NOTE;
	
    } else if (desc.shape == Breve) {

	return desc.filled ? NoteCharacterNames::UNKNOWN //!!!
	                   : NoteCharacterNames::BREVE;
	
    } else if (desc.shape == Cross) {

	return desc.filled ? NoteCharacterNames::X_NOTEHEAD
	                   : NoteCharacterNames::CIRCLE_X_NOTEHEAD;

    } else if (desc.shape == TriangleUp) {

	return desc.filled ? NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_BLACK
                           : NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_WHITE;

    } else if (desc.shape == TriangleDown) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: TriangleDown not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!

    } else if (desc.shape == Diamond) {

	return desc.filled ? NoteCharacterNames::SEMIBREVIS_BLACK
 	                   : NoteCharacterNames::SEMIBREVIS_WHITE;

    } else if (desc.shape == Rectangle) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: Rectangle not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!
	
    } else if (desc.shape == Number) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: Number not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!

    } else {

	return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getAccidentalCharName(const Accidental &a)
{
    if      (a == Accidentals::Sharp)        return NoteCharacterNames::SHARP;
    else if (a == Accidentals::Flat)         return NoteCharacterNames::FLAT;
    else if (a == Accidentals::Natural)      return NoteCharacterNames::NATURAL;
    else if (a == Accidentals::DoubleSharp)  return NoteCharacterNames::DOUBLE_SHARP;
    else if (a == Accidentals::DoubleFlat)   return NoteCharacterNames::DOUBLE_FLAT;
    return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getMarkCharName(const Mark &mark)
{
    if      (mark == Marks::Accent)    return NoteCharacterNames::ACCENT;
    else if (mark == Marks::Tenuto)    return NoteCharacterNames::TENUTO;
    else if (mark == Marks::Staccato)  return NoteCharacterNames::STACCATO;
    else if (mark == Marks::Staccatissimo) return NoteCharacterNames::STACCATISSIMO;
    else if (mark == Marks::Marcato)   return NoteCharacterNames::MARCATO;
    else if (mark == Marks::Trill)     return NoteCharacterNames::TRILL;
    else if (mark == Marks::Turn)      return NoteCharacterNames::TURN;
    else if (mark == Marks::Pause)     return NoteCharacterNames::FERMATA;
    else if (mark == Marks::UpBow)     return NoteCharacterNames::UP_BOW;
    else if (mark == Marks::DownBow)   return NoteCharacterNames::DOWN_BOW;
    // Things like "sf" and "rf" are generated from text fonts
    return NoteCharacterNames::UNKNOWN;
}
 
CharName
NoteStyle::getClefCharName(const Clef &clef)
{
    std::string clefType(clef.getClefType());

    if (clefType == Clef::Bass) {
        return NoteCharacterNames::F_CLEF;
    } else if (clefType == Clef::Treble) {
        return NoteCharacterNames::G_CLEF;
    } else {
        return NoteCharacterNames::C_CLEF;
    }
}

CharName
NoteStyle::getRestCharName(Note::Type type)
{
    switch (type) {
    case Note::Hemidemisemiquaver:  return NoteCharacterNames::SIXTY_FOURTH_REST;
    case Note::Demisemiquaver:      return NoteCharacterNames::THIRTY_SECOND_REST;
    case Note::Semiquaver:          return NoteCharacterNames::SIXTEENTH_REST;
    case Note::Quaver:              return NoteCharacterNames::EIGHTH_REST;
    case Note::Crotchet:            return NoteCharacterNames::QUARTER_REST;
    case Note::Minim:               return NoteCharacterNames::HALF_REST;
    case Note::Semibreve:           return NoteCharacterNames::WHOLE_REST;
    case Note::Breve:               return NoteCharacterNames::MULTI_REST;
    default:
        return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getFlagCharName(int flagCount)
{
    switch (flagCount) {
    case 1:  return NoteCharacterNames::FLAG_1;
    case 2:  return NoteCharacterNames::FLAG_2;
    case 3:  return NoteCharacterNames::FLAG_3;
    case 4:  return NoteCharacterNames::FLAG_4;
    default: return NoteCharacterNames::UNKNOWN;
    }
}


void
ClassicalNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (AngledOval, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(AngledOval, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(LevelOval,  false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Breve,      false, false, 0);
}

/*!!!
void
CrossNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (Cross, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(Cross, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(Cross, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Cross, false, false, 0);
}


void
TriangleNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (TriangleUp, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(TriangleUp, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(TriangleUp, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(TriangleUp, false, false, 0);
}


void
MensuralNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (Diamond, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(Diamond, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(Diamond, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Diamond, false, false, 0);
}
*/

CustomNoteStyle::CustomNoteStyle(std::string name)
{
/*!!!
    if (name[0] == '_') {
	throw StyleFileReadFailed
	    (qstrtostr(i18n("Leading underscore (in ") +
		       strtoqstr(name) +
		       i18n(") reserved for built-in styles")));
    }
*/
    QString styleDirectory =
	KGlobal::dirs()->findResource("appdata", "styles/");

    QString styleFileName =
	QString("%1/%2.xml").arg(styleDirectory).arg(strtoqstr(name));

    QFileInfo fileInfo(styleFileName);

    if (!fileInfo.isReadable()) {
        throw StyleFileReadFailed
	    (qstrtostr(i18n("Can't open style file ") +
		       styleFileName));
    }

    QFile styleFile(styleFileName);

    QXmlInputSource source(styleFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    styleFile.close();

    if (!ok) {
	throw StyleFileReadFailed(qstrtostr(m_errorString));
    }
}

void
CustomNoteStyle::initialiseNotes()
{
    // already done in constructor, no need to do anything here
}

void
CustomNoteStyle::setBaseStyle(NoteStyleName name)
{
    try {
	m_baseStyle = NoteStyleFactory::getStyle(name);
	if (m_baseStyle == this) m_baseStyle = 0;
    } catch (NoteStyleFactory::StyleUnavailable u) {
	if (name != StandardNoteStyleNames::Classical) {
	    kdDebug(KDEBUG_AREA)
		<< "CustomNoteStyle::setBaseStyle: Base style "
		<< name << " not available, defaulting to "
		<< StandardNoteStyleNames::Classical << endl;
	    setBaseStyle(StandardNoteStyleNames::Classical);
	} else {
	    kdDebug(KDEBUG_AREA)
		<< "CustomNoteStyle::setBaseStyle: Base style "
		<< name << " not available" << endl;
	    m_baseStyle = 0;
	}
    }	    
}

void
CustomNoteStyle::setShape(Note::Type note, NoteHeadShape shape)
{
    m_notes[note].shape = shape;
}

void
CustomNoteStyle::setFilled(Note::Type note, bool filled)
{
    m_notes[note].filled = filled;
}

void
CustomNoteStyle::setStem(Note::Type note, bool stem)
{
    m_notes[note].stem = stem;
}

void
CustomNoteStyle::setFlagCount(Note::Type note, int flags)
{
    m_notes[note].flags = flags;
}

bool
CustomNoteStyle::startElement(const QString &, const QString &,
			      const QString &qName,
			      const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-note-style") {

	QString s;
	
	s = attributes.value("base-style");
	if (s) setBaseStyle(qstrtostr(s));
	else {
//	    m_errorString = i18n("base-style is a required attribute of rosegarden-note-style");
//	    return false;
	    m_baseStyle = 0;
	}

    } else if (lcName == "note") {

	QString s;
	NoteDescription desc;
	
	s = attributes.value("type");
	if (!s) {
	    m_errorString = i18n("type is a required attribute of note");
	    return false;
	}
	
	try {
	    Note::Type type = Note(qstrtostr(s)).getNoteType();
	    if (m_notes.find(type) != m_notes.end()) desc = m_notes[type];

	    s = attributes.value("shape");
	    if (s) desc.shape = s.lower();
	
	    s = attributes.value("filled");
	    if (s) desc.filled = (s.lower() == "true");
	
	    s = attributes.value("stem");
	    if (s) desc.stem = (s.lower() == "true");
	
	    s = attributes.value("flags");
	    if (s) desc.flags = (s.lower() == "true");

	    m_notes[type] = desc;

	} catch (Note::MalformedNoteName n) {
	    m_errorString = i18n("Unrecognised note name " + s);
	    return false;
	}

    } else if (lcName == "global") {
	    
	for (Note::Type type = Note::Shortest;
	     type <= Note::Longest; ++type) {

	    if (m_notes.find(type) != m_notes.end()) continue;
		
	    QString s;
	    NoteDescription desc;

	    s = attributes.value("shape");
	    if (s) desc.shape = s.lower();
	    else if (m_baseStyle) desc.shape = m_baseStyle->getShape(type);
	
	    s = attributes.value("filled");
	    if (s) desc.filled = (s.lower() == "true");
	    else if (m_baseStyle) desc.filled = m_baseStyle->isFilled(type);
	
	    s = attributes.value("stem");
	    if (s) desc.stem = (s.lower() == "true");
	    else if (m_baseStyle) desc.stem = m_baseStyle->hasStem(type);
	
	    s = attributes.value("flags");
	    if (s) desc.flags = (s.lower() == "true");
	    else if (m_baseStyle) desc.flags = m_baseStyle->getFlagCount(type);

	    m_notes[type] = desc;
	}
    }

    return true;
}

