
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

#ifndef _RG_NOTESMENUADDTEXTMARKCOMMAND_H_
#define _RG_NOTESMENUADDTEXTMARKCOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <string>
#include <qstring.h>
#include <klocale.h>




namespace Rosegarden
{

class EventSelection;


class AddTextMarkCommand : public BasicSelectionCommand
{
public:
    AddTextMarkCommand(std::string text,
                                EventSelection &selection) :
        BasicSelectionCommand(getGlobalName(), selection, true),
        m_selection(&selection), m_text(text) { }

    static QString getGlobalName() { return i18n("Add Te&xt Mark..."); }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    std::string m_text;
};



}

#endif
