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


#include "MultiKeyInsertionCommand.h"

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "KeyInsertionCommand.h"
#include <qstring.h>


namespace Rosegarden
{

MultiKeyInsertionCommand::MultiKeyInsertionCommand(Composition &c,
        timeT time,
        Key key,
        bool convert,
        bool transpose,
        bool transposeKey) :
        KMacroCommand(getGlobalName(&key))
{
    for (Composition::iterator i = c.begin(); i != c.end(); ++i) {
        Segment *segment = *i;

        // no harm in using getEndTime instead of getEndMarkerTime here:
        if (segment->getStartTime() <= time && segment->getEndTime() > time) {
            addCommand(new KeyInsertionCommand(*segment, time, key, convert, transpose, transposeKey));
        } else if (segment->getStartTime() > time) {
            addCommand(new KeyInsertionCommand(*segment, segment->getStartTime(),
                                               key, convert, transpose, transposeKey));
        }
    }
}

MultiKeyInsertionCommand::~MultiKeyInsertionCommand()
{
    // nothing
}

}
