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

#include <string>
#include <vector>

#ifndef _DEVICE_H_
#define _DEVICE_H_

namespace Rosegarden
{

class Instrument;

class Device
{
public:
    typedef enum 
    {
        Internal,
        Audio
    } DeviceType;

    Device(std::string name):m_name(name) {;}
    ~Device() {;}

    DeviceType getType() const { return m_type; }
    std::string getName() const { return m_name; }

private:
    std::vector<Instrument *> m_instruments;
    std::string               m_name;
    DeviceType                m_type;

};

}

#endif // _DEVICE_H_
