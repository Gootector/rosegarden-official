
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTESTYLE_H_
#define _RG_NOTESTYLE_H_

#include "base/NotationTypes.h"
#include <map>
#include "NoteCharacterNames.h"
#include <string>
#include <utility>
#include "gui/editors/notation/NoteCharacterNames.h"


class Mark;
class Accidental;


namespace Rosegarden
{

class Clef;

typedef std::string NoteStyleName;


class NoteStyle
{
public:
    virtual ~NoteStyle();

    typedef std::string NoteHeadShape;

    static const NoteHeadShape AngledOval;
    static const NoteHeadShape LevelOval;
    static const NoteHeadShape Breve;
    static const NoteHeadShape Cross;
    static const NoteHeadShape TriangleUp;
    static const NoteHeadShape TriangleDown;
    static const NoteHeadShape Diamond;
    static const NoteHeadShape Rectangle;
    static const NoteHeadShape CustomCharName;
    static const NoteHeadShape Number;

    enum HFixPoint { Normal, Central, Reversed };
    enum VFixPoint { Near, Middle, Far };

    NoteStyleName getName() const { return m_name; }

    NoteHeadShape getShape     (Note::Type);
    bool          isFilled     (Note::Type);
    bool          hasStem      (Note::Type);
    int           getFlagCount (Note::Type);
    int           getSlashCount(Note::Type);

    typedef std::pair<CharName, bool> CharNameRec; // bool is "inverted"
    CharNameRec getNoteHeadCharName(Note::Type);

    CharName getRestCharName(Note::Type, bool restOutsideStave);
    CharName getPartialFlagCharName(bool final);
    CharName getFlagCharName(int flagCount);
    CharName getAccidentalCharName(const Accidental &);
    CharName getMarkCharName(const Mark &);
    CharName getClefCharName(const Clef &);
    CharName getTimeSignatureDigitName(int digit);

    void setBaseStyle (NoteStyleName name);
    void setShape     (Note::Type, NoteHeadShape);
    void setCharName  (Note::Type, CharName);
    void setFilled    (Note::Type, bool);
    void setStem      (Note::Type, bool);
    void setFlagCount (Note::Type, int);
    void setSlashCount(Note::Type, int);

    void getStemFixPoints(Note::Type, HFixPoint &, VFixPoint &);
    void setStemFixPoints(Note::Type, HFixPoint, VFixPoint);

protected:
    struct NoteDescription {
        NoteHeadShape shape; // if CustomCharName, use charName
        CharName charName; // only used if shape == CustomCharName
        bool filled;
        bool stem;
        int flags;
        int slashes;
        HFixPoint hfix;
        VFixPoint vfix;

        NoteDescription() :
            shape(AngledOval), charName(NoteCharacterNames::UNKNOWN),
            filled(true), stem(true), flags(0), slashes(0),
            hfix(Normal), vfix(Middle) { }

        NoteDescription(NoteHeadShape _shape, CharName _charName,
                        bool _filled, bool _stem, int _flags, int _slashes,
                        HFixPoint _hfix, VFixPoint _vfix) :
            shape(_shape), charName(_charName),
            filled(_filled), stem(_stem), flags(_flags), slashes(_slashes),
            hfix(_hfix), vfix(_vfix) { }
    };

    typedef std::map<Note::Type, NoteDescription> NoteDescriptionMap;

    NoteDescriptionMap m_notes;
    NoteStyle *m_baseStyle;
    NoteStyleName m_name;

    void checkDescription(Note::Type type);

protected: // for use by NoteStyleFileReader
    NoteStyle(NoteStyleName name) : m_baseStyle(0), m_name(name) { }
    friend class NoteStyleFileReader;
};




}

#endif
