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

#ifndef STAFF_H
#define STAFF_H

#include <vector>
#include <string>

#include "qcanvasgroupableitem.h"
#include "notepixmapfactory.h"
#include "notationelement.h"

#include "Staff.h"

class QCanvasLineGroupable;
class QCanvasSimpleSprite;
class EventSelection;

/**
 * The Staff is a repository for information about the notation
 * representation of a single Segment.  This includes all of the
 * NotationElements representing the Events on that Segment, the staff
 * lines, as well as basic positional and size data.  This class
 * used to be in gui/staff.h, but it's been moved and renamed
 * following the introduction of the core Staff base class.
 */
class NotationStaff : public Rosegarden::Staff<NotationElement>,
		      public QCanvasItemGroup
{
public:
    typedef std::vector<QCanvasLineGroupable *> LineList;
    typedef std::set<QCanvasSimpleSprite *> SpriteSet;
    
    /**
     * Creates a new NotationStaff for the specified Segment
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas*, Rosegarden::Segment*, unsigned int id,
                  std::string fontName, int resolution);
    ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc; can't change resolution of the actual layout
     * or pixmaps on the staff, the notation view should do that
     */
    void changeFont(std::string fontName, int resolution);

    void setLegatoDuration(Rosegarden::timeT duration);

    /**
     * Gets a read-only reference to the pixmap factory used by the
     * staff.  (For use by NotationHLayout, principally.)  This
     * reference isn't const because the NotePixmapFactory maintains
     * too much state for its methods to be const, but you should
     * treat the returned reference as if it were const anyway.
     */
    NotePixmapFactory& getNotePixmapFactory() { return *m_npf; }

    /**
     * Returns the Y coordinate of the specified line on the staff.
     *
     * 0 is the bottom staff-line, 8 is the top one.
     */
    int yCoordOfHeight(int height) const;

    /**
     * Returns the difference between the y-coord of one visible line
     * and that of its neighbour
     */
    unsigned int getLineSpacing() const {
	return m_npf->getLineSpacing();
    }

    /**
     * Returns the height of a bar line.
     */
    unsigned int getBarLineHeight() const {
	return getLineSpacing() * (nbLines - 1);
    }

    /**
     * Returns the total height of a staff
     */
    unsigned int getStaffHeight() const {
	return getTopLineOffset() * 2 + getBarLineHeight()
	    + m_npf->getStaffLineThickness();
    }

    /**
     * Returns the space between the top of the staff object and the
     * first visible line on the staff.  (This is the same as the
     * space between the last visible line and the bottom of the staff
     * object.)
     */
    unsigned int getTopLineOffset() const {
	return getLineSpacing() * nbLegerLines;
    }
 
   /**
     * Return the id of the staff
     * This will be passed to the NotationTools
     * so they know on which staff a mouse event occurred
     *
     * @see NotationTool#handleMousePress
     * @see NotationView#itemPressed
     */
    unsigned int getId() { return m_id; }

    bool showElements();

    bool showElements(NotationElementList::iterator from,
		      NotationElementList::iterator to,
		      bool positionOnly = false);

    bool showSelection(const EventSelection *selection);
    bool clearSelection();


    /**
     * Insert a bar line at x-coordinate \a barPos.
     *
     * If \a correct is true, the bar line ends a correct (timewise)
     * bar.  If false, the bar line ends an incorrect bar (for instance,
     * two minims in 3:4 time), and will be drawn in red
     */
    void insertBar(unsigned int barPos, bool correct);

    /**
     * Return a rectangle describing the full width and height of the
     * bar containing the given x-cooordinate.  Used for setting a
     * selection to the scope of a full bar.
     */
    QRect getBarExtents(unsigned int x);

    /**
     * Insert time signature at x-coordinate \a x.
     */
    void insertTimeSignature(unsigned int x,
			     const Rosegarden::TimeSignature &timeSig);

    /**
     * Delete all bars which are after X position \a fromPos
     */
    void deleteBars(unsigned int fromPos);

    /**
     * Delete all bars
     */
    void deleteBars();

    /**
     * Delete all time signatures
     */
    void deleteTimeSignatures();

    /**
     * Set the start and end x-coords of the staff lines
     */
    void setLines(double xfrom, double xto);

    void getClefAndKeyAtX(int x, Rosegarden::Clef &clef, Rosegarden::Key &key)
	const;

    static const int nbLines;        // number of main lines on the staff
    static const int nbLegerLines;   // number of lines above or below

protected:

    enum RefreshType { FullRefresh, PositionRefresh, SelectionRefresh };
    bool showElements(NotationElementList::iterator from,
		      NotationElementList::iterator to,
		      RefreshType, const EventSelection *selection = 0);

    /**
     * Return a QCanvasSimpleSprite representing the NotationElement
     * pointed to by the given iterator
     */
    QCanvasSimpleSprite* makeNoteSprite(NotationElementList::iterator);

    int m_id;

    int m_horizLineLength;
    int m_resolution;

    LineList m_barLines;
    LineList m_staffLines;
    SpriteSet m_timeSigs;

    typedef std::pair<int, Rosegarden::Clef> ClefChange;
    std::vector<ClefChange> m_clefChanges;

    typedef std::pair<int, Rosegarden::Key> KeyChange;
    std::vector<KeyChange> m_keyChanges;

    QCanvasLineGroupable *m_initialBarA, *m_initialBarB;
    NotePixmapFactory *m_npf;

    bool m_haveSelection;
};

#endif
