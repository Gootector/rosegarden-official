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


#ifndef _LOOPRULER_H_
#define _LOOPRULER_H_

#include "Event.h"
#include "SnapGrid.h"

#include "rosegardenguidoc.h"
#include "hzoomable.h"

namespace Rosegarden { class RulerScale; }

/**
 * LoopRuler is a widget that shows bar and beat durations on a
 * ruler-like scale, and reacts to mouse clicks by sending relevant
 * signals to modify position pointer and playback/looping states.
*/
class LoopRuler : public QWidget, public HZoomable
{
    Q_OBJECT

public:
    LoopRuler(Rosegarden::RulerScale *rulerScale,
              int height = 0,
	      double xorigin = 0.0,
	      bool invert = false,
              QWidget* parent = 0,
              const char *name = 0);

    ~LoopRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void scrollHoriz(int x);

    void setMinimumWidth(int width) { m_width = width; }

    void setHorizScaleFactor(double dy) { m_hScaleFactor = dy; }

    bool hasActiveMousePress() { return m_activeMousePress; }

    bool getLoopingMode() { return m_loopingMode; }
    
public slots:
    void slotSetLoopMarker(Rosegarden::timeT startLoop,
			   Rosegarden::timeT endLoop);

protected:
    // ActiveItem interface
    virtual void mousePressEvent       (QMouseEvent*);
    virtual void mouseReleaseEvent     (QMouseEvent*);
    virtual void mouseDoubleClickEvent (QMouseEvent*);
    virtual void mouseMoveEvent        (QMouseEvent*);

    virtual void paintEvent(QPaintEvent*);

    void setLoopingMode(bool value) { m_loopingMode = value; }
    void drawBarSections(QPainter*);
    void drawLoopMarker(QPainter*);  // between loop positions

signals:
    // The three main functions that this class performs
    //
    /// Set the pointer position on mouse single click
    void setPointerPosition(Rosegarden::timeT);

    /// Set the pointer position on mouse drag
    void dragPointerToPosition(Rosegarden::timeT);

    /// Set pointer position and start playing on double click
    void setPlayPosition(Rosegarden::timeT);

    /// Set a playing loop
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

    /// Set the loop end position on mouse drag
    void dragLoopToPosition(Rosegarden::timeT);

    void startMouseMove(int directionConstraint);
    void stopMouseMove();
    void mouseMove();

protected:

    //--------------- Data members ---------------------------------
    int  m_height;
    double m_xorigin;
    bool m_invert;
    int  m_currentXOffset;
    int  m_width;
    bool m_activeMousePress;

    Rosegarden::RulerScale *m_rulerScale;
    Rosegarden::SnapGrid    m_grid;
    
    bool m_loopingMode;
    Rosegarden::timeT m_startLoop;
    Rosegarden::timeT m_endLoop;
};

#endif // _LOOPRULER_H_

