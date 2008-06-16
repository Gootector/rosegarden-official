
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

#ifndef _RG_INCREMENTDISPLACEMENTSCOMMAND_H_
#define _RG_INCREMENTDISPLACEMENTSCOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <qstring.h>
#include <klocale.h>




namespace Rosegarden
{

class EventSelection;


class IncrementDisplacementsCommand : public BasicSelectionCommand
{
public:
    IncrementDisplacementsCommand(EventSelection &selection,
                                  long dx, long dy) :
        BasicSelectionCommand(getGlobalName(), selection, true),
        m_selection(&selection),
        m_dx(dx),
        m_dy(dy) { }

    static QString getGlobalName() { return i18n("Fine Reposition"); }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    long m_dx;
    long m_dy;
};


}

#endif
