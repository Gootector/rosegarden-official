
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

#ifndef NOTEPIXMAPFACTORY_H
#define NOTEPIXMAPFACTORY_H

#include <vector>
#include <qcanvas.h>

#include "staff.h"
#include "NotationTypes.h"

typedef vector<int> ChordPitches;
typedef vector<Accidental> Accidentals;


/**
 * Helper class to compute various offsets
 * Used by the NotePixmapFactory
 * @author Guillaume Laurent
 */
class NotePixmapOffsets
{
public:

    typedef pair<QPoint, QPoint> stalkpoints;

    NotePixmapOffsets();

    void offsetsFor(Note::Type,
                    bool dotted,
                    Accidental,
                    bool drawTail,
                    bool stalkGoesUp);
    
    const QPoint&      getBodyOffset()     { return m_bodyOffset; }
    const QSize&       getPixmapSize()     { return m_pixmapSize; }
    const QPoint&      getHotSpot()        { return m_hotSpot; }
    const stalkpoints& getStalkPoints()    { return m_stalkPoints; }
    const QPoint&      getAccidentalOffset() { return m_accidentalOffset; }

    void setNoteBodySizes(QSize empty, QSize filled);
    void setTailWidth(unsigned int);
    void setAccidentalsWidth(unsigned int sharp,
                             unsigned int flat,
                             unsigned int natural);
    void setDotSize(QSize size);

protected:

    void computePixmapSize();
    void computeAccidentalAndStalkSize();
    void computeBodyOffset();

    QSize m_noteBodyEmptySize;
    QSize m_noteBodyFilledSize;

    unsigned int m_tailWidth;
    unsigned int m_sharpWidth;
    unsigned int m_flatWidth;
    unsigned int m_naturalWidth;

    Note::Type m_note;
    Accidental m_accidental;
    bool m_drawTail;
    bool m_stalkGoesUp;
    bool m_noteHasStalk;
    bool m_dotted;

    QPoint m_bodyOffset;
    QPoint m_hotSpot;
    QPoint m_accidentalOffset;
    QSize m_bodySize;
    QSize m_pixmapSize;
    QSize m_accidentalStalkSize;
    QSize m_dotSize;
    
    stalkpoints m_stalkPoints;
};


/**
 * Generates QCanvasPixmaps for single notes and chords
 * (chords unused)
 *
 *@author Guillaume Laurent
 */
class NotePixmapFactory
{
public:

    NotePixmapFactory();
    ~NotePixmapFactory();

    /**
     * Generate a pixmap for a single note
     *
     * @param note : note type
     * @param drawTail : if the pixmap should have a tail or not
     *   (useful when the tail should be collapsed with the one of a neighboring note)
     * @param stalkGoesUp : if the note's stalk should go up or down
     */
    QCanvasPixmap makeNotePixmap(Note::Type note,
                                 bool dotted,
                                 Accidental accidental = NoAccidental,
                                 bool drawTail = false,
                                 bool stalkGoesUp = true);

    QCanvasPixmap makeRestPixmap(Note::Type note, bool dotted);

    QCanvasPixmap makeClefPixmap(string type);

    QCanvasPixmap makeKeyPixmap(string type, string cleftype);

protected:
    const QPixmap* tailUp(Note::Type note) const;
    const QPixmap* tailDown(Note::Type note) const;

    void drawStalk(Note::Type note, bool drawTail, bool stalkGoesUp);
    void drawAccidental(Accidental, bool stalkGoesUp);
    void drawDot();

    void createPixmapAndMask(int width = -1, int height = -1);

    NotePixmapOffsets m_offsets;

    unsigned int m_generatedPixmapHeight;
    unsigned int m_noteBodyHeight;
    unsigned int m_noteBodyWidth;
    unsigned int m_tailWidth;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;

    QPainter m_p, m_pm;

    QPixmap m_noteBodyFilled;
    QPixmap m_noteBodyEmpty;

    QPixmap m_accidentalSharp;
    QPixmap m_accidentalFlat;
    QPixmap m_accidentalNatural;

    QPixmap m_dot;

    vector<QPixmap*> m_tailsUp;
    vector<QPixmap*> m_tailsDown;
    vector<QPixmap*> m_rests;

    static QPoint m_pointZero;

};

/**
 * Generates QCanvasPixmaps for chords - currently broken
 *
 *@author Guillaume Laurent
 */
class ChordPixmapFactory : public NotePixmapFactory
{
public:

    ChordPixmapFactory(const Staff&);

    /**
     * Generate a pixmap for a chord
     *
     * @param pitches : a \b sorted vector of relative pitches
     * @param duration : chord duration
     * @param drawTail : if the pixmap should have a tail or not
     *   (useful when the tail should be collapsed with the one of a neighboring chord)
     * @param stalkGoesUp : if the note's stalk should go up or down
     */
    QCanvasPixmap makeChordPixmap(const ChordPitches &pitches,
                                  const Accidentals &accidentals,
                                  Note::Type note,
                                  bool dotted,
                                  bool drawTail = false,
                                  bool stalkGoesUp = true);

protected:
    const Staff &m_referenceStaff;
};
    

#endif
