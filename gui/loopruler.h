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


#ifndef _LOOPRULER_H_
#define _LOOPRULER_H_

#include <qwidget.h>
#include "Event.h"
#include "rosegardenguidoc.h"

namespace Rosegarden {
    class RulerScale;
}


// Creates a canvas widget that reacts to mouse clicks and
// sends relevant signals to modify position pointer and
// playback/looping states.
//
// Hopefully use this same class in TrackEditor and editing
// clients - they will have to be synced-up or managed at
// some point.
//
//

class LoopRuler : public QWidget
{
    Q_OBJECT

public:
    LoopRuler(RosegardenGUIDoc *doc,
	      Rosegarden::RulerScale *rulerScale,
              int height = 0,
	      bool invert = false,
              QWidget* parent = 0,
              const char *name = 0);

    ~LoopRuler();

    virtual void paintEvent(QPaintEvent*);

public slots:
    void setLoopingMode(bool value) { m_loop = value; }
    void setLoopMarker(Rosegarden::timeT startLoop, Rosegarden::timeT endLoop);

protected:
    // ActiveItem interface
    virtual void mousePressEvent       (QMouseEvent *mE);
    virtual void mouseReleaseEvent     (QMouseEvent *mE);
    virtual void mouseDoubleClickEvent (QMouseEvent *mE);
    virtual void mouseMoveEvent        (QMouseEvent *mE);

signals:
    // The three main functions that this class performs
    //

    // Set the pointer position on mouse single click
    //
    void setPointerPosition(Rosegarden::timeT);

    // Set pointer position and start playing on double click
    //
    void setPlayPosition(Rosegarden::timeT);

    // Set a playing loop
    //
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

private:
    void drawBarSections(QPainter*);
    void drawLoopMarker(QPainter*);  // between loop positions

    int m_height;
    int m_snap;            // snap the loop to the nearest
    bool m_invert;

    RosegardenGUIDoc *m_doc;
    Rosegarden::RulerScale *m_rulerScale;
    
    bool m_loop;
    Rosegarden::timeT m_startLoop;
    Rosegarden::timeT m_endLoop;
};

#endif // _LOOPRULER_H_

