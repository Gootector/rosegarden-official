// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
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

#include "MappedDevice.h"
#include "MappedInstrument.h"
#include <iostream>

namespace Rosegarden
{

MappedDevice::MappedDevice():
    std::vector<Rosegarden::MappedInstrument*>(),
    m_id(0),
    m_type(Rosegarden::Device::Midi),
    m_name("MappedDevice default name"),
    m_client(-1)
{
}

MappedDevice::MappedDevice(Rosegarden::DeviceId id,
                           Rosegarden::Device::DeviceType type,
                           const std::string &name):
    std::vector<Rosegarden::MappedInstrument*>(),
    m_id(id),
    m_type(type),
    m_name(name),
    m_client(-1)
{
}

MappedDevice::~MappedDevice()
{
}

MappedDevice::MappedDevice(const MappedDevice &mD):
    std::vector<Rosegarden::MappedInstrument*>()
{
    clear();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(**it));

    m_id = mD.getId();
    m_type = mD.getType();
    m_name = mD.getName();
}

void
MappedDevice::clear()
{
    MappedDeviceIterator it;

    for (it = this->begin(); it != this->end(); it++)
        delete (*it);

    this->erase(this->begin(), this->end());
}

MappedDevice&
MappedDevice::operator+(const MappedDevice &mD)
{
    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(**it));

    return *this;
}

MappedDevice&
MappedDevice::operator=(const MappedDevice &mD)
{
    if (&mD == this) return *this;

    clear();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        this->push_back(new MappedInstrument(**it));

    m_id = mD.getId();
    m_type = mD.getType();
    m_name = mD.getName();

    return *this;
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice *mD)
{
    int instruments = 0;
    dS >> instruments;

    MappedInstrument mI;
    while (!dS.atEnd() && instruments)
    {
        dS >> mI;
        mD->push_back(new MappedInstrument(mI));
        instruments--;
    }

    QString name;
    unsigned int id;
    int dType;

    dS >> id;
    dS >> dType;
    dS >> name;
    mD->setId(id);
    mD->setType(Rosegarden::Device::DeviceType(dType));
    mD->setName(std::string(name.data()));

    if (instruments)
    {
        std::cerr << "MappedDevice::operator>> - "
                  << "wrong number of events received" << std::endl;
    }

    return dS;
}


QDataStream&
operator>>(QDataStream &dS, MappedDevice &mD)
{
    int instruments;
    dS >> instruments;

    MappedInstrument mI;

    while (!dS.atEnd() && instruments)
    {
        dS >> mI;
        mD.push_back(new MappedInstrument(mI));
        instruments--;
    }

    unsigned int id;
    QString name;
    int dType;

    dS >> id;
    dS >> dType;
    dS >> name;
    mD.setId(id);
    mD.setType(Rosegarden::Device::DeviceType(dType));
    mD.setName(std::string(name.data()));

    if (instruments)
    {
        std::cerr << "MappedDevice::operator>> - "
                  << "wrong number of events received" << std::endl;
    }

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, MappedDevice *mD)
{
    dS << mD->size();

    for (MappedDeviceIterator it = mD->begin(); it != mD->end(); it++)
        dS << (*it);

    dS << (unsigned int)(mD->getId());
    dS << (int)(mD->getType());
    dS << QString(mD->getName().c_str());

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, const MappedDevice &mD)
{
    dS << mD.size();

    for (MappedDeviceConstIterator it = mD.begin(); it != mD.end(); it++)
        dS << (*it);

    dS << (unsigned int)(mD.getId());
    dS << (int)(mD.getType());
    dS << QString(mD.getName().c_str());

    return dS;
}


}

