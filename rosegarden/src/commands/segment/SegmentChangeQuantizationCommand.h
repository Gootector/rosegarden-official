
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

#ifndef _RG_SEGMENTCHANGEQUANTIZATIONCOMMAND_H_
#define _RG_SEGMENTCHANGEQUANTIZATIONCOMMAND_H_

#include <kcommand.h>
#include <qstring.h>
#include <vector>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;


class SegmentChangeQuantizationCommand : public KNamedCommand
{
public:
    /// Set quantization on segments.  If unit is zero, switch quantization off
    SegmentChangeQuantizationCommand(timeT);
    virtual ~SegmentChangeQuantizationCommand();

    void addSegment(Segment *s);

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName(timeT);

private:
    struct SegmentRec {
        Segment *segment;
        timeT oldUnit;
        bool wasQuantized;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;

    timeT m_unit;
};



}

#endif
