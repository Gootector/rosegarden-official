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

#include "barbuttons.h"
#include "loopruler.h"
#include "colours.h"
#include <qvbox.h>
#include <qlabel.h>
#include <qcanvas.h>


BarButtons::BarButtons(RosegardenGUIDoc* doc,
                       QWidget* parent,
                       QHeader *vHeader,
                       QHeader *hHeader,
                       const char* name,
                       WFlags f):
    QHBox(parent, name), m_doc(doc)
{
   m_barHeight = vHeader->sectionSize(0);
   m_barWidth = hHeader->sectionSize(0);
   m_bars = hHeader->count();

   m_offset = 4;

   setMinimumHeight(m_barHeight);
   setMaximumHeight(m_barHeight);

   drawButtons();
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons()
{

    // Create a horizontal spacing label to jog everything
    // up with the main SegmentCanvas
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumWidth(m_offset);
    label->setMaximumWidth(m_offset);

    int loopBarHeight = 8; // the height of the loop bar
    QVBox *bar;

    // Create a vertical box for the loopBar and the bar buttons
    //
    QVBox *buttonBar = new QVBox(this);
    buttonBar->setMinimumSize(m_barWidth * m_bars, m_barHeight);
    buttonBar->setMaximumSize(m_barWidth * m_bars, m_barHeight);

    // The loop bar is where we're going to be defining our loops
    //
    QCanvas *canvas = new QCanvas(buttonBar);
    canvas->resize(m_barWidth * m_bars, loopBarHeight);
    canvas->setBackgroundColor(RosegardenGUIColours::LoopRulerBackground);


    // Loop ruler works its bar spacing out from the m_doc just
    // like we do
    //
    LoopRuler *loopRuler = new LoopRuler(m_doc,
                                         canvas,
                                         buttonBar,
                                         m_bars,
                                         m_barWidth,
                                         loopBarHeight);

    // Another horizontal layout box..
    //
    QHBox *hButtonBar = new QHBox(buttonBar);


    // First bar width by which others are judged
    //
    std::pair<Rosegarden::timeT, Rosegarden::timeT> fTimes =
        m_doc->getComposition().getBarRange(1, false);

    int firstBarWidth = fTimes.second - fTimes.first;
 
    for (int i = 0; i < m_bars; i++)
    {
        bar = new QVBox(hButtonBar);
        bar->setSpacing(0);

        std::pair<Rosegarden::timeT, Rosegarden::timeT> times =
            m_doc->getComposition().getBarRange(i, false);

        // Normalise this bar width to a ratio of the first
        //
        int barWidth =  m_barWidth * (times.second - times.first) /
                                     firstBarWidth;

        bar->setMinimumSize(barWidth, m_barHeight - loopBarHeight);
        bar->setMaximumSize(barWidth, m_barHeight - loopBarHeight);

        // attempt a style
        //
        bar->setFrameStyle(StyledPanel);
        bar->setFrameShape(StyledPanel);
        bar->setFrameShadow(Raised);

        label = new QLabel(bar);
        label->setText(QString("%1").arg(i));
        label->setAlignment(AlignLeft|AlignVCenter);
        label->setIndent(4);
        label->setMinimumHeight(m_barHeight - loopBarHeight - 2);
        label->setMaximumHeight(m_barHeight - loopBarHeight - 2);

    }

}




