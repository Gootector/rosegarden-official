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

#ifndef _RG_CHANGERECORDDEVICECOMMAND_H_
#define _RG_CHANGERECORDDEVICECOMMAND_H_

#include "base/Studio.h"
#include <klocale.h>
#include <kcommand.h>

namespace Rosegarden
{

class ChangeRecordDeviceCommand : public KNamedCommand
{
public:
    ChangeRecordDeviceCommand(Rosegarden::DeviceId deviceId, bool action) :
        KNamedCommand(i18n("Change Record Device")),
        m_deviceId(deviceId), m_action(action) { }
    
    virtual void execute() { swap(); }
    virtual void unexecute() { swap(); }

private:
    Rosegarden::DeviceId m_deviceId;
    bool m_action;
    void swap();

};

}

#endif /*CHANGERECORDDEVICECOMMAND_H_*/
