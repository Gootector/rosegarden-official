// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#include "MappedInstrument.h"

namespace Rosegarden
{


MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte channel,
                                   InstrumentId id):
    m_type(type),
    m_channel(channel),
    m_id(id),
    m_name(std::string(""))
{
}

MappedInstrument::MappedInstrument(Instrument::InstrumentType type,
                                   MidiByte channel,
                                   InstrumentId id,
                                   const std::string &name):
    m_type(type),
    m_channel(channel),
    m_id(id),
    m_name(name)
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
}


}

