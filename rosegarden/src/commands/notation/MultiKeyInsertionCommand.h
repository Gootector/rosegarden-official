
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

#ifndef _RG_MULTIKEYINSERTIONCOMMAND_H_
#define _RG_MULTIKEYINSERTIONCOMMAND_H_

#include "base/NotationTypes.h"
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>
#include "misc/Strings.h"
#include <kcommand.h>
#include "document/RosegardenGUIDoc.h"


class Add;


namespace Rosegarden
{

//class Composition;
class RosegardenGUIDoc;


class MultiKeyInsertionCommand : public KMacroCommand
{
public:
    
    MultiKeyInsertionCommand(RosegardenGUIDoc* doc,
                             timeT time,
                             Key key,
                             bool shouldConvert,
                             bool shouldTranspose,
                             bool shouldTransposeKey,
			     bool shouldIgnorePercussion); 
    virtual ~MultiKeyInsertionCommand();

    static QString getGlobalName(Key *key = 0) {
        if (key) {
            return i18n("Change all to &Key %1...").arg(strtoqstr(key->getName()));
        } else {
            return i18n("Add &Key Change...");
        }
    }
};


}

#endif
