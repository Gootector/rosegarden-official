
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_GRACECOMMAND_H_
#define _RG_GRACECOMMAND_H_

#include "document/BasicCommand.h"
#include <QString>
#include "base/Event.h"
#include <QObject>


class Make;


namespace Rosegarden
{

class EventSelection;
class CommandRegistry;

/*!!!
class GraceCommand : public BasicCommand
{
public:
    GraceCommand(EventSelection &selection);

    static QString getGlobalName() { return QObject::tr("Make &Grace Notes"); }

    static void registerCommand(CommandRegistry *r);

protected:
    virtual void modifySegment();
    timeT getEffectiveEndTime(EventSelection &);
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
*/

}

#endif
