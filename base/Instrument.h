// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_

#include <string>

// An Instrument connects a Track (containing or comparable to
// a list of Segments) to a device that can play that Track.
//
//

namespace Rosegarden
{

class Instrument
{
public:
    enum InstrumentType { Midi, Audio };

    Instrument();
    ~Instrument();

    string getName() { return m_name; }
    InstrumentType getInstrumentType() { return m_type; }
    int getNumber() { return m_number; }

    int getMidiChannel() { return m_midiChannel; }
    int getMidiTranspose() { return m_midiTranspose; }

    void setMidiChannel(const int &mC) { m_midiChannel = mC; }
    void setMidiTranspose(const int &mT) { m_midiTranspose = mT; }

private:

    int m_number;
    string m_name;
    InstrumentType m_type;
    
    int m_midiChannel;
    int m_midiTranspose;

};

}

#endif // _INSTRUMENT_H_
