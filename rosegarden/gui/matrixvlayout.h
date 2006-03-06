// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef MATRIXVLAYOUT_H
#define MATRIXVLAYOUT_H

#include "LayoutEngine.h"
#include "matrixelement.h"

class MatrixVLayout : public Rosegarden::VerticalLayoutEngine
{
public:
    MatrixVLayout();

    virtual ~MatrixVLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(Rosegarden::Staff &staff,
			    Rosegarden::timeT = 0,
			    Rosegarden::timeT = 0);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(Rosegarden::Staff &staff,
			   Rosegarden::timeT = 0,
			   Rosegarden::timeT = 0);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout(Rosegarden::timeT = 0,
			      Rosegarden::timeT = 0);

    static const int minMIDIPitch;
    static const int maxMIDIPitch;

protected:
    //--------------- Data members ---------------------------------


};

#endif
