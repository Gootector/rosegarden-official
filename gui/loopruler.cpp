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


#include "loopruler.h"
#include "colours.h"
#include <iostream>
#include "Event.h"

using std::cerr;
using std::cout;
using std::endl;

LoopRuler::LoopRuler(RosegardenGUIDoc *doc,
                     QCanvas *canvas,
                     QWidget *parent,
                     const int &bars,
                     const int &barWidth,
                     const int &height,
                     const char *name):
    QCanvasView(canvas, parent, name),
    m_bars(bars), m_barWidth(barWidth), m_height(height),
    m_barSubSections(4), m_canvas(canvas), m_doc(doc),
    m_loop(false), m_startLoop(0), m_loopMarker(0)
{
    setMinimumSize(bars * barWidth, height);
    setMaximumSize(bars * barWidth, height);
    setFrameStyle(NoFrame);

    // Load in our bar widths from the Composition
    //
    std::pair<Rosegarden::timeT, Rosegarden::timeT> barMarkers;
    for (int i = 0; i < m_bars; i++)
    {
        barMarkers = m_doc->getComposition().getBarRange(i, false);
        m_barWidthMap[i] = barMarkers.second - barMarkers.first;
    }
 
    // Draw out the sections according to the bar widths
    //
    drawBarSections();
}

LoopRuler::~LoopRuler()
{
}

void
LoopRuler::drawBarSections()
{
    QCanvasLine *line;
    int subSectionLinePos, barWidth;
    int runningWidth = 0;

    for (int i = 0; i < m_bars; i++)
    {
        // Normalise this bar width to a ratio of the first
        //
        barWidth = m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];

        line = new QCanvasLine(m_canvas);
        line->setPoints(runningWidth, 2 * m_height / 7,
                        runningWidth, m_height);
        line->setPen(RosegardenGUIColours::LoopRulerForeground);
        line->show();

        for (int j = 0; j < m_barSubSections; j++)
        {
            if (j == 0) continue;
            
            subSectionLinePos = (runningWidth) +
                                (barWidth * j/m_barSubSections);

            line = new QCanvasLine(m_canvas);
            line->setPoints(subSectionLinePos, m_height,
                            subSectionLinePos, 5 * m_height / 7);
            line->setPen(RosegardenGUIColours::LoopRulerForeground);
            line->show();

        }

        runningWidth += barWidth;

    }

}

void
LoopRuler::contentsMousePressEvent(QMouseEvent *mE)
{
    if (mE->button() == LeftButton)
    {
        if (m_loop)
            m_startLoop = getPointerPosition(mE->pos().x());
        else
            emit setPointerPosition(getPointerPosition(mE->pos().x()));
    }
}

void
LoopRuler::contentsMouseReleaseEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton)
    {
        if (m_loop)
        {
            Rosegarden::timeT endLoop = getPointerPosition(position);

            emit setLoop(m_startLoop, endLoop);
            m_startLoop = 0;
        }
    }
}

void
LoopRuler::contentsMouseDoubleClickEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (mE->button() == LeftButton && !m_loop)
        emit setPlayPosition(getPointerPosition(position));
}

void
LoopRuler::contentsMouseMoveEvent(QMouseEvent *mE)
{
    int position = mE->pos().x();
    if (position < 0) position = 0;
    
    if (m_loop)
    {
        m_endLoop = getPointerPosition(position);
        drawLoopMarker();
    }
    else
        emit setPointerPosition(getPointerPosition(position));
}


// From an x position work out the pointer position.
// Loop through all the bars removing a calculated
// x position every pass through until we're left
// in the bar we require.
//
//
Rosegarden::timeT
LoopRuler::getPointerPosition(const int &xPos)
{
    Rosegarden::timeT result = 0;
    Rosegarden::timeT runningPosition = 0;
    int barWidth, runningWidth = 0;

    for (int i = 0; i < m_bars; i++)
    {
        barWidth = m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];

        if (runningWidth + barWidth > xPos)
        {
            result = runningPosition + (m_barWidthMap[i] *
                                        (xPos - runningWidth)/barWidth);
            return result;
        }

        runningPosition += m_barWidthMap[i];
        runningWidth += barWidth;
    }

    return result;
}

// Compute an X position on the LoopRuler from a given pos
//
int
LoopRuler::getXPosition(const Rosegarden::timeT &pos)
{
    int result = 0;
    int thisBar = m_doc->getComposition().getBarNumber(pos, false);

    cout << "POS = " << pos << " : BAR = " << thisBar << endl;

    for (int i = 0; i < thisBar; i++)
    {
        result += m_barWidth * m_barWidthMap[i] / m_barWidthMap[0];
    }

    cout << "BAR START = " << m_doc->getComposition().getBarStart(pos) << endl;

    result += (pos - m_doc->getComposition().getBarStart(pos))
              * m_barWidth * m_barWidthMap[thisBar]/m_barWidthMap[0];

    return result;
}


void
LoopRuler::drawLoopMarker()
{
    if (m_loopMarker == 0)
        m_loopMarker = new QCanvasRectangle(m_canvas);

    getXPosition(m_startLoop);
    
}
