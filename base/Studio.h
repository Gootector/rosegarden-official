// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Device.h"
#include <string>

// The Studio is where Midi and Audio devices live.  We can query
// them for a list of Instruments, connect them together or to
// effects units (eventually) and generally do real studio-type
// stuff to them.
//
//

#include "Instrument.h"

#ifndef _STUDIO_H_
#define _STUDIO_H_

namespace Rosegarden
{

typedef std::vector<Instrument *> InstrumentList;

class Studio
{

public:
    Studio();
    ~Studio();

    void addDevice(const std::string &name, Device::DeviceType type);
    void addDevice(Device *device);

    // Return the combined instrument list from all devices
    //
    InstrumentList getInstruments();

    // Return an Instrument
    Instrument* getInstrumentByIndex(InstrumentId id);

private:

    std::vector<Device*> m_devices;

};

}

#endif // _STUDIO_H_
