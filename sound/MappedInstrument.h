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

#include "Instrument.h"

#ifndef _MAPPEDINSTRUMENT_H_
#define _MAPPEDINSTRUMENT_H_

// A scaled-down version of an Instrument that we keep Sequencer
// side.  IDs match with those on the GUI.
//
//

namespace Rosegarden
{

class MappedInstrument
{
public:
    MappedInstrument(Instrument::InstrumentType type, MidiByte channel, InstrumentId id);
    ~MappedInstrument();

    InstrumentId getID() const { return m_id; }
    MidiByte getChannel() const { return m_channel; }
    Instrument::InstrumentType getType() const { return m_type; }

    void setChannel(MidiByte channel) { m_channel = channel; }
    void setType(Instrument::InstrumentType type) { m_type = type; }

private:

    Instrument::InstrumentType  m_type;
    MidiByte                    m_channel;
    InstrumentId                m_id;

};

}

#endif // _MAPPEDINSTRUMENT_H_
