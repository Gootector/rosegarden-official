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

#include "MappedInstrument.h"

namespace Rosegarden
{


MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte channel,
                                   InstrumentId id):
    m_type(type),
    m_channel(channel),
    m_id(id),
    m_name(std::string("")),
    m_subOrdering(0)
{
}

MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte channel,
                                   InstrumentId id,
                                   MappedInstrumentSubOrdering subOrdering,
                                   const std::string &name,
                                   DeviceId device):
    m_type(type),
    m_channel(channel),
    m_id(id),
    m_name(name),
    m_device(device),
    m_subOrdering(subOrdering)
{
}

MappedInstrument::MappedInstrument(const Instrument &instr):
    m_type(instr.getType()),
    m_channel(instr.getMidiChannel()),
    m_id(instr.getId()),
    m_name(instr.getName()),
    m_device((instr.getDevice())->getId()),
    m_subOrdering(instr.getSubOrdering())
{
}

MappedInstrument::MappedInstrument(Instrument *instr):
    m_type(instr->getType()),
    m_channel(instr->getMidiChannel()),
    m_id(instr->getId()),
    m_name(instr->getName()),
    m_device(instr->getDevice()->getId()),
    m_subOrdering(instr->getSubOrdering())
{
}


MappedInstrument::~MappedInstrument()
{
}

MappedInstrument::MappedInstrument(MappedInstrument *mI)
{
    m_type = mI->m_type;
    m_channel = mI->m_channel;
    m_id = mI->m_id;
    m_name = mI->m_name;
    m_subOrdering = mI->m_subOrdering;
}


}

