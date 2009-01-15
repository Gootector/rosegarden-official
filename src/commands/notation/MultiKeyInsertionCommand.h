
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

#ifndef _RG_MULTIKEYINSERTIONCOMMAND_H_
#define _RG_MULTIKEYINSERTIONCOMMAND_H_

#include "base/NotationTypes.h"
#include <QString>
#include "base/Event.h"
#include <QObject>
#include "misc/Strings.h"
#include "document/Command.h"
#include "document/RosegardenGUIDoc.h"


class Add;


namespace Rosegarden
{

//class Composition;
class RosegardenGUIDoc;


class MultiKeyInsertionCommand : public MacroCommand
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
            return QObject::tr("Change all to &Key %1...").arg(strtoqstr(key->getName()));
        } else {
            return QObject::tr("Add &Key Change...");
        }
    }
};


}

#endif
