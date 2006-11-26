
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

#ifndef _RG_ADDTIMESIGNATURECOMMAND_H_
#define _RG_ADDTIMESIGNATURECOMMAND_H_

#include "base/NotationTypes.h"
#include <kcommand.h>
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>




namespace Rosegarden
{

class Composition;


class AddTimeSignatureCommand : public KNamedCommand
{
public:
    AddTimeSignatureCommand(Composition *composition,
                            timeT time,
                            TimeSignature timeSig);
    virtual ~AddTimeSignatureCommand();

    static QString getGlobalName() { return i18n("Add Time Si&gnature Change..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Composition *m_composition;
    timeT m_time;
    TimeSignature m_timeSignature;

    TimeSignature *m_oldTimeSignature; // for undo
    int m_timeSigIndex; // for undo
};    




}

#endif
