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
                       int baseBarWidth,
                       int barHeight,
                       QWidget* parent,
                       const char* name,
                       WFlags /*f*/):
    QHBox(parent, name),
    m_barHeight(barHeight),
    m_baseBarWidth(baseBarWidth),
    m_doc(doc)
{
    m_offset = 4;

    setMinimumHeight(m_barHeight);
    setMaximumHeight(m_barHeight);

    Rosegarden::Composition &comp = doc->getComposition();
    m_bars = comp.getNbBars();

    if (m_bars == 0)
    {
        m_bars = comp.getBarNumber(comp.getEndMarker() -
                                   comp.getStartMarker(), false);
    }

    drawButtons();
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons()
{
    if (!m_doc) return;

    Rosegarden::Composition &comp = m_doc->getComposition();

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

    // First bar width by which others are judged
    //
    std::pair<Rosegarden::timeT, Rosegarden::timeT> fTimes =
        comp.getBarRange(0, false); 

    int firstBarWidth = fTimes.second - fTimes.first;
 
    int buttonBarWidth = m_baseBarWidth *
                         comp.getBarRange(m_bars, false).second
                         / firstBarWidth;

    buttonBar->setMinimumSize(buttonBarWidth, m_barHeight);
    buttonBar->setMaximumSize(buttonBarWidth, m_barHeight);

    // Loop ruler works its bar spacing out from the m_doc just
    // like we do in this class.  Then connect up the LoopRuler
    // signals passing back through the outside world.
    //
    //
    LoopRuler *loopRuler = new LoopRuler(m_doc,
                                         m_baseBarWidth,
                                         loopBarHeight,
                                         buttonBar);

    connect(loopRuler, SIGNAL(setPointerPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(loopRuler, SIGNAL(setPlayPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPlayPosition(Rosegarden::timeT)));

    connect(loopRuler, SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
            this,      SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    connect(this,      SIGNAL(signalSetLoopingMode(bool)),
            loopRuler, SLOT(setLoopingMode(bool)));

    connect(this,      SIGNAL(signalSetLoopMarker(Rosegarden::timeT, Rosegarden::timeT)),
            loopRuler, SLOT(setLoopMarker(Rosegarden::timeT, Rosegarden::timeT)));

    // Another horizontal layout box..
    //
    QHBox *hButtonBar = new QHBox(buttonBar);

    for (int i = 0; i < m_bars; i++)
    {
        bar = new QVBox(hButtonBar);
        bar->setSpacing(0);

        std::pair<Rosegarden::timeT, Rosegarden::timeT> times =
            comp.getBarRange(i, false);

        // Normalise this bar width to a ratio of the first
        //
        int barWidth =  m_baseBarWidth * (times.second - times.first) /
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


void
BarButtons::setLoopingMode(bool value)
{
    emit signalSetLoopingMode(value);
}


