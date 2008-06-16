
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

#ifndef _RG_MODIFYDEVICEMAPPINGCOMMAND_H_
#define _RG_MODIFYDEVICEMAPPINGCOMMAND_H_

#include "base/Device.h"
#include "base/Track.h"
#include <kcommand.h>
#include <qstring.h>
#include <vector>
#include <klocale.h>


class Modify;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;
class Composition;


class ModifyDeviceMappingCommand : public KNamedCommand
{
public:
    ModifyDeviceMappingCommand(RosegardenGUIDoc *doc,
                               DeviceId fromDevice,
                               DeviceId toDevice);

    static QString getGlobalName() { return i18n("Modify &Device Mapping"); }

    virtual void execute();
    virtual void unexecute();
protected:
    Composition *m_composition;
    Studio      *m_studio;
    DeviceId     m_fromDevice;
    DeviceId     m_toDevice;

    std::vector<std::pair<TrackId, InstrumentId> > m_mapping;
};


}

#endif
