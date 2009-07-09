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

#ifndef _CONTROL_MOUSE_EVENT_H_
#define _CONTROL_MOUSE_EVENT_H_

#include <QPoint>

#include "base/Event.h" // for timeT

namespace Rosegarden
{

class ControlItem;

class ControlMouseEvent
{
public:
    ControlItemList itemList; // list of items under the cursor, if any

    timeT time; // un-snapped and un-cropped
    int value;

    Qt::KeyboardModifiers modifiers;
    Qt::MouseButtons buttons;

    ControlMouseEvent() :
        itemList(),
        time(0), value(0),
        modifiers(0), buttons(0) { }
};

}

#endif
