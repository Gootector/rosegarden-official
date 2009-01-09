
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

#ifndef _RG_TEXTCHANGECOMMAND_H_
#define _RG_TEXTCHANGECOMMAND_H_

#include "base/NotationTypes.h"
#include "document/BasicCommand.h"




namespace Rosegarden
{

class Segment;
class Event;


class TextChangeCommand : public BasicCommand
{
public:
    TextChangeCommand(Segment &segment,
                      Event *event,
                      Text text);
    virtual ~TextChangeCommand();

protected:
    virtual void modifySegment();
    Event *m_event; // only used first time through
    Text m_text;
};

/*
 * Inserts a key change into a single segment, taking segment transpose into
 * account (fixes #1520716) if desired.
 */

}

#endif
