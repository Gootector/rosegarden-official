
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _COLOURS_H_
#define _COLOURS_H_

#include <qcolor.h>

/**
 * Definitions of colours to be used throughout the Rosegarden GUI.
 * 
 * They're in this file not so much to allow them to be easily
 * changed, as to ensure a certain amount of consistency between
 * colours for different functions that might end up being seen
 * at the same time.
 */

namespace RosegardenGUIColours
{
    extern const QColor SegmentBorder;
    extern const QColor SegmentBlock;

    extern const QColor StaffConnectingLine;
    extern const QColor StaffConnectingTerminatingLine;
    extern const QColor StaffRulerBackground;
    
    extern const QColor PositionCursor;
    extern const QColor SelectionRectangle;

    extern const QColor NoteFontRed;
    extern const QColor NoteFontGreen;
    extern const QColor NoteFontBlue;
}

#endif

