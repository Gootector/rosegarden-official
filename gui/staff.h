
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

#include "qcanvasitemgroup.h"
#include "notepixmapfactory.h"
#include "viewelementsmanager.h"
#include "notationelement.h"

class QCanvasLineGroupable;

/**
 * The Staff is a repository for information about the notation
 * representation of a single Track.  This includes all of the
 * NotationElements representing the Events on that Track, as well as
 * basic positional and size data.
 *
 * Staff owns the ViewElementsManager and NotationElementList objects
 * it contains, but not the Track object it's constructed from.
 */

class Staff : public QCanvasItemGroup
{
public:
    typedef std::vector<QCanvasLineGroupable*> barlines;
    
    Staff(QCanvas *, Rosegarden::Track *, int resolution);
    ~Staff();

    // bit dubious, really -- I'd rather have a NotePixmapFactory that
    // worked even through a const reference, but most of its drawing
    // methods are necessarily non-const because it stores so much
    // state internally
    NotePixmapFactory &getNotePixmapFactory() { return m_npf; }

    const ViewElementsManager *getViewElementsManager() const {
	return &m_manager;
    }
    ViewElementsManager *getViewElementsManager() {
	return &m_manager;
    }

    const NotationElementList *getNotationElementList() const {
	return m_notes;
    }
    NotationElementList *getNotationElementList() {
	return m_notes;
    }
    
    int yCoordOfHeight(int height) const;
    unsigned int getBarLineHeight() const { return m_barLineHeight; }
    unsigned int getBarMargin() const { return m_resolution * 2; }
    unsigned int getStaffHeight() const {
	return m_resolution * nbLines + linesOffset * 2 + 1;
    }

    void insertBar(unsigned int barPos, bool correct);
    void deleteBars(unsigned int fromPos);
    void deleteBars();

    void setLinesLength(unsigned int);

    static const int nbLines;        // number of main lines on the staff
    static const int linesOffset;    // from top of canvas to top line (bad!)

protected:
    int m_barLineHeight;
    int m_horizLineLength;
    int m_resolution;

    barlines m_barLines;
    barlines m_staffLines;

    NotePixmapFactory m_npf;
    ViewElementsManager m_manager;
    NotationElementList *m_notes;
};

#endif
