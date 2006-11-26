
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_BARBUTTONS_H_
#define _RG_BARBUTTONS_H_

#include <qvbox.h>
#include "base/Event.h"


class QWidget;
class QPaintEvent;


namespace Rosegarden
{

class RulerScale;
class RosegardenGUIDoc;
class LoopRuler;
class BarButtonsWidget;


class BarButtons : public QVBox
{
    Q_OBJECT

public:
    BarButtons(RosegardenGUIDoc *doc,
               RulerScale *rulerScale,
               double xorigin,
               int buttonHeight,
               bool invert = false, // draw upside-down
               QWidget* parent = 0,
               const char* name = 0,
               WFlags f=0);

    LoopRuler* getLoopRuler() { return m_loopRuler; }

    /**
     * Make connections from the LoopRuler to the document's
     * position pointer -- the standard use for a LoopRuler.
     * If you don't call this, you'll have to connect the
     * LoopRuler's signals up to something yourself.
     */
    void connectRulerToDocPointer(RosegardenGUIDoc *doc);
    
    void setMinimumWidth(int width);

    void setHScaleFactor(double dy);

public slots:
    void slotScrollHoriz(int x);

signals:
    /// reflected from the loop ruler
    void dragPointerToPosition(timeT);

    /// reflected from the loop ruler
    void dragLoopToPosition(timeT);


protected:
    virtual void paintEvent(QPaintEvent *);

private:
    //--------------- Data members ---------------------------------
    bool m_invert;
    int m_loopRulerHeight;
    int m_currentXOffset;

    RosegardenGUIDoc       *m_doc;
    RulerScale *m_rulerScale;

    BarButtonsWidget *m_hButtonBar;
    LoopRuler *m_loopRuler;
};



}

#endif
