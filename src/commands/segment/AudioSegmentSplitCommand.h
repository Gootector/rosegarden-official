
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

#ifndef _RG_AUDIOSEGMENTSPLITCOMMAND_H_
#define _RG_AUDIOSEGMENTSPLITCOMMAND_H_

#include <string>
#include <kcommand.h>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;


class AudioSegmentSplitCommand : public KNamedCommand
{
public:
    AudioSegmentSplitCommand(Segment *segment,
                             timeT splitTime);
    virtual ~AudioSegmentSplitCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Segment *m_segment;
    Segment *m_newSegment;
    timeT m_splitTime;
    timeT *m_previousEndMarkerTime;
    bool m_detached;
    std::string m_segmentLabel;
//    RealTime m_previousEndAudioTime;
};


}

#endif
