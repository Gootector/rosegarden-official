
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

#ifndef _NOTATION_PROPERTIES_H_
#define _NOTATION_PROPERTIES_H_

// Property names for properties that are computed and cached within
// the notation module, but that need not necessarily be saved with
// the file

#define  P_QUANTIZED_DURATION    "QuantizedDuration"

#define  P_NOTE_TYPE             "NoteType"
#define  P_NOTE_DOTTED           "NoteDotted"

#define  P_HEIGHT_ON_STAFF       "HeightOnStaff"
#define  P_MIN_WIDTH             "MinWidth"

#define  P_ACCIDENTAL            "NoteComputedAccidental"
#define  P_STALK_UP              "NoteStalkUp"
#define  P_DRAW_TAIL             "NoteDrawTail"
#define  P_NOTE_NAME             "NoteName"

#endif

