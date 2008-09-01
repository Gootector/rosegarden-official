
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENLABEL_H_
#define _RG_ROSEGARDENLABEL_H_

#include <QLabel>


class QWidget;
class QWheelEvent;
class QMouseEvent;


namespace Rosegarden
{

class Label : public QLabel
{
    Q_OBJECT
public:
    Label(QWidget *parent = 0, const char *name=0):
        QLabel(parent, name) {;}

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent * /*e*/)
        { emit doubleClicked(); }

    virtual void wheelEvent(QWheelEvent * e) 
        { emit scrollWheel(e->delta()); }

signals:
    void doubleClicked();
    void scrollWheel(int);

};


}

#endif
