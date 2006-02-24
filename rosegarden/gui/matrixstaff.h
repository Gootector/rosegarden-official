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

#ifndef MATRIXSTAFF_H
#define MATRIXSTAFF_H

#include "linedstaff.h"
#include "matrixelement.h"

class MatrixView;

class MatrixStaff : public LinedStaff
{
public:
    MatrixStaff(QCanvas *canvas,
                Rosegarden::Segment *segment, 
                Rosegarden::SnapGrid *snapGrid,
                int id, 
                int vResolution,
                MatrixView *view);
    virtual ~MatrixStaff();

protected:
    virtual int getLineCount() const;
    virtual int getLegerLineCount() const;
    virtual int getBottomLineHeight() const;
    virtual int getHeightPerLine() const;
    virtual bool elementsInSpaces() const;
    virtual bool showBeatLines() const;

    const Rosegarden::MidiKeyMapping *getKeyMapping() const;

    /**
     * Override from Rosegarden::Staff<T>
     * Wrap only notes 
     */
    virtual bool wrapEvent(Rosegarden::Event*);

    /**
     * Override from Rosegarden::Staff<T>
     * Let tools know if their current element has gone
     */
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);

    virtual Rosegarden::ViewElement* makeViewElement(Rosegarden::Event*);

public:
    LinedStaff::setResolution;

//     double getTimeScaleFactor() const { return m_scaleFactor * 2; } // TODO: GROSS HACK to enhance matrix resolution (see also in matrixview.cpp) - BREAKS MATRIX VIEW, see bug 1000595
    double getTimeScaleFactor() const { return m_scaleFactor; }
    void setTimeScaleFactor(double f) { m_scaleFactor = f; }

    int getElementHeight() { return m_resolution; }

    virtual void positionElements(Rosegarden::timeT from,
				  Rosegarden::timeT to);

    virtual void positionElement(Rosegarden::ViewElement*);

    // Get an element for an Event
    //
    MatrixElement* getElement(Rosegarden::Event *event);

private:
    double m_scaleFactor;

    MatrixView     *m_view;
};

#endif
