
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
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

#ifndef _RG_SEGMENTLABELCOMMAND_H_
#define _RG_SEGMENTLABELCOMMAND_H_

#include <kcommand.h>
#include <qstring.h>
#include <vector>
#include <klocale.h>




namespace Rosegarden
{

class SegmentSelection;
class Segment;


class SegmentLabelCommand : public KNamedCommand
{
public:
    SegmentLabelCommand(SegmentSelection &segments,
                        const QString &label);
    virtual ~SegmentLabelCommand();

    static QString getGlobalName()
        { return i18n("Re&label..."); }

    virtual void execute();
    virtual void unexecute();
protected:

    std::vector<Segment*>  m_segments;
    std::vector<QString>               m_labels;
    QString                            m_newLabel;
};



}

#endif
