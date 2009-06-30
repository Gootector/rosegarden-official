/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _CONTROLITEM_H
#define _CONTROLITEM_H

//#include <Q3CanvasRectangle>
//#include <Q3Canvas>
#include <QPolygonF>
class QPainter;
class QMouseEvent;
class QWheelEvent;

namespace Rosegarden {

#define MIDI_CONTROL_MAX_VALUE 0x7F // Shouldnt be here

class ControlRuler;
//class ElementAdapter;
class Event;

class ControlItem : public QPolygonF
{
public:
    ControlItem(ControlRuler* controlRuler,
//                ElementAdapter* adapter,
                Event* event,
                QPolygonF polygon);
//                int x, int width = DefaultWidth);

    ~ControlItem();

    virtual void setValue(long);
    int getValue() const { return m_value; }

    //void setWidth(int w)  { setSize(w, height()); }
    //void setHeight(int h) {
	//setSize(width(), h);
	//setZ(50.0+(h/2.0));
    //}
    //int getHeight()       { return size().height(); }

    virtual void draw(QPainter &painter);

    virtual void handleMouseButtonPress(QMouseEvent *e);
    virtual void handleMouseButtonRelease(QMouseEvent *e);
    virtual void handleMouseMove(QMouseEvent *e, int deltaX, int deltaY);
    virtual void handleMouseWheel(QWheelEvent *e);

    virtual void setSelected(bool yes);
    bool isSelected() { return m_selected; }
    //    virtual void setHighlighted(bool yes) { m_highlighted=yes; update(); }
    /// recompute height according to represented value prior to a canvas repaint
    virtual void updateFromValue();

    /// update value according to height after a user edit
    virtual void updateValue();

    virtual void update(); ///CJ Don't know what to do here yet
    virtual void setX(int);
    virtual void setWidth(int);

//    ElementAdapter* getElementAdapter() { return m_elementAdapter; }
    Event* getEvent() { return m_event; }

protected:

    //--------------- Data members ---------------------------------

    long m_value;
    bool m_handlingMouseMove;
    bool m_selected;

    ControlRuler* m_controlRuler;
//    ElementAdapter* m_elementAdapter;
    Event* m_event;

    static const unsigned int BorderThickness;
    static const unsigned int DefaultWidth;
};

class ControlItemList : public std::list<ControlItem*> {};

//typedef std::list<ControlItem*> ControlItemList;
//typedef std::list<ControlItem*>::iterator ControlItemListIterator;

}

#endif
