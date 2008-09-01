
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

#ifndef _RG_PASTESEGMENTSCOMMAND_H_
#define _RG_PASTESEGMENTSCOMMAND_H_

#include "base/Track.h"
#include "document/Command.h"
#include <QString>
#include <vector>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class Segment;
class Composition;
class Clipboard;


/// Paste one or more segments from the clipboard into the composition

class PasteSegmentsCommand : public NamedCommand
{
public:
    PasteSegmentsCommand(Composition *composition,
                         Clipboard *clipboard,
                         timeT pasteTime,
                         TrackId baseTrack,
                         bool useExactTracks);

    virtual ~PasteSegmentsCommand();

    static QString getGlobalName() { return i18n("&Paste"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition *m_composition;
    Clipboard *m_clipboard;
    timeT m_pasteTime;
    TrackId m_baseTrack;
    bool m_exactTracks;
    std::vector<Segment *> m_addedSegments;
    bool m_detached;
};



}

#endif
