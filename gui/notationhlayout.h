
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

#ifndef NOTATIONHLAYOUT_H
#define NOTATIONHLAYOUT_H

#include "layoutengine.h"
#include "notationelement.h"
#include "staff.h"
#include "Track.h"
#include "FastVector.h"

/**
 * Horizontal notation layout
 *
 * computes the X coordinates of notation elements
 */
class NotationHLayout : public LayoutEngine
{
public:
    NotationHLayout();
    ~NotationHLayout();

    /**
     * Precomputes layout data for a single staff.  The resulting data
     * is stored in the BarDataMap, keyed from the staff reference;
     * the entire map is then used by reconcileBars() and layout().
     * The map should be cleared (by calling reset()) before a full
     * set of staffs is preparsed.
     */
    void preparse(Staff &staff);

    /**
     * Resets internal data stores, notably the BarDataMap that is
     * used to retain the data computed by preparse().
     */
    void reset(Staff *staff = 0);

    /// Tries to harmonize the bar positions for all the staves
    void reconcileBars();

    /// Effectively perform layout
    void layout();

    /**
     * Inner class for bar data, used by preparse()
     */
    struct BarData
    {
        int barNo;              // of corresponding BarPosition in Track
        NotationElementList::iterator start; // i.e. event following barline
        int x;                // coordinate for display of barline
        int idealWidth;       // theoretical width of bar following barline
        int fixedWidth;       // minimum possible width of bar following barline
	bool correct;         // bar preceding barline has correct duration
        bool widthChanged;
        
        BarData(int ibarno, NotationElementList::iterator istart,
                int ix, int iwidth, int fwidth, bool icorrect) :
            barNo(ibarno), start(istart), x(ix), idealWidth(iwidth),
            fixedWidth(fwidth), correct(icorrect), widthChanged(true) { }
    };

    typedef FastVector<BarData> BarDataList;

    /**
     * Returns the bar positions for a given staff, provided that
     * staff has been preparsed since the last reset
     */
    BarDataList& getBarData(Staff &staff);
    const BarDataList& getBarData(Staff &staff) const;

    /**
     * Returns the total length of all elements once layout is done
     * This is the x-coord of the end of the last element on the longest staff
     */
    double getTotalWidth() { return m_totalWidth; }

protected:
    typedef std::map<Staff *, BarDataList> BarDataMap;

    class AccidentalTable : public std::vector<Rosegarden::Accidental>
    {
    public:
        AccidentalTable(Rosegarden::Key, Rosegarden::Clef);
        Rosegarden::Accidental getDisplayAccidental(Rosegarden::Accidental,
                                                    int height) const;
        void update(Rosegarden::Accidental, int height);
    private:
        Rosegarden::Key m_key;
        Rosegarden::Clef m_clef;
    };

    void layout(BarDataMap::iterator);

    void addNewBar(Staff &staff, int barNo, NotationElementList::iterator start,
                   int width, int fwidth, bool correct);

    int getMinWidth(const NotePixmapFactory &, const NotationElement &) const;

    int getComfortableGap(const NotePixmapFactory &npf,
                          Rosegarden::Note::Type type) const;

    int getIdealBarWidth(Staff &staff, int fixedWidth,
                         NotationElementList::iterator shortest,
                         const NotePixmapFactory &npf, int shortCount,
                         int totalCount,
                         const Rosegarden::TimeSignature &timeSignature) const;

    BarDataMap m_barData;

    double m_totalWidth;
};

#endif
