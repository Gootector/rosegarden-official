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


#include "SegmentChangeQuantizationCommand.h"

#include "base/Segment.h"
#include "gui/editors/notation/NotationStrings.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentChangeQuantizationCommand::SegmentChangeQuantizationCommand(timeT unit) :
        KNamedCommand(getGlobalName(unit)),
        m_unit(unit)
{
    // nothing
}

SegmentChangeQuantizationCommand::~SegmentChangeQuantizationCommand()
{
    // nothing
}

void
SegmentChangeQuantizationCommand::execute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

        SegmentRec &rec = m_records[i];

        if (m_unit) {

            rec.oldUnit = rec.segment->getQuantizer()->getUnit();
            rec.segment->setQuantizeLevel(m_unit);

            rec.wasQuantized = rec.segment->hasQuantization();
            rec.segment->setQuantization(true);

        } else {

            rec.wasQuantized = rec.segment->hasQuantization();
            rec.segment->setQuantization(false);
        }
    }
}

void
SegmentChangeQuantizationCommand::unexecute()
{
    for (unsigned int i = 0; i < m_records.size(); ++i) {

        SegmentRec &rec = m_records[i];

        if (m_unit) {

            if (!rec.wasQuantized)
                rec.segment->setQuantization(false);
            rec.segment->setQuantizeLevel(rec.oldUnit);

        } else {

            if (rec.wasQuantized)
                rec.segment->setQuantization(true);
        }
    }
}

void
SegmentChangeQuantizationCommand::addSegment(Segment *s)
{
    SegmentRec rec;
    rec.segment = s;
    rec.oldUnit = 0; // shouldn't matter what we initialise this to
    rec.wasQuantized = false; // shouldn't matter what we initialise this to
    m_records.push_back(rec);
}

QString
SegmentChangeQuantizationCommand::getGlobalName(timeT unit)
{
    if (!unit) {
        return "Unquantize";
    } else {
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(unit, true, error);
        return QString("Quantize to %1").arg(label);
    }
}

}
