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


#include "SegmentChangeTransposeCommand.h"

#include "base/Segment.h"
#include "gui/editors/notation/NotationStrings.h"
#include <qstring.h>


namespace Rosegarden
{

SegmentChangeTransposeCommand::SegmentChangeTransposeCommand(int unit, Segment *segment) :
        KNamedCommand(getGlobalName(unit)),
        m_unit(unit),
        m_segment(segment)
{
    // nothing
}

SegmentChangeTransposeCommand::~SegmentChangeTransposeCommand()
{
    // nothing
}

void
SegmentChangeTransposeCommand::execute()
{
    m_oldUnit = m_segment->getTranspose();
	m_segment->setTranspose(m_unit);
}

void
SegmentChangeTransposeCommand::unexecute()
{
   	m_segment->setTranspose(m_oldUnit);
}

QString
SegmentChangeTransposeCommand::getGlobalName(int unit)
{
    if (!unit) {
        return "Undo change transposition";
    } else {
        return QString("Change transposition to %1").arg(unit);
    }
}

}
