// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "MappedCommon.h"
#include "MappedStudio.h"
#include "MappedComposition.h"
#include "MappedInstrument.h"

#ifndef _STUDIOCONTROL_H_
#define _STUDIOCONTROL_H_

// Access and control the sequencer studio from the gui
//

namespace Rosegarden
{

typedef std::pair<Rosegarden::MidiByte, Rosegarden::MidiByte> MidiControlPair;

class StudioControl
{
public:

    // Object management
    //
    static MappedObjectId
        createStudioObject(MappedObject::MappedObjectType type);
    static MappedObjectId
        getStudioObjectByType(MappedObject::MappedObjectType type);
    static bool destroyStudioObject(MappedObjectId id);

    // Properties
    //
    static MappedObjectPropertyList
                  getStudioObjectProperty(MappedObjectId id,
                                          const MappedObjectProperty &property);

    // Set a value to a value
    //
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        MappedObjectValue value);

    // Set a value to a list
    //
    static bool setStudioObjectProperty(MappedObjectId id,
                                        const MappedObjectProperty &property,
                                        MappedObjectValueList value);

    // cheat so we can avoid making calls() during playback
    //
    static void setStudioPluginPort(MappedObjectId pluginId,
                                    unsigned long portId,
                                    MappedObjectValue value);

    // Connection
    //
    static void connectStudioObjects(MappedObjectId id1,
				     MappedObjectId id2);
    static void disconnectStudioObjects(MappedObjectId id1,
					MappedObjectId id2);
    static void disconnectStudioObject(MappedObjectId id);

    // Send controllers and other one off MIDI events using these
    // interfaces.
    //
    static void sendMappedEvent(const Rosegarden::MappedEvent& mE);
    static void sendMappedComposition(const Rosegarden::MappedComposition &mC);

    // MappedInstrument
    //
    static void sendMappedInstrument(const Rosegarden::MappedInstrument &mI);

    // Send the Quarter Note Length has changed to the sequencer
    //
    static void sendQuarterNoteLength(const Rosegarden::RealTime &length);

    // Convenience wrappers for RPNs and NRPNs
    //
    static void sendRPN(Rosegarden::InstrumentId instrumentId,
                        Rosegarden::MidiByte paramMSB,
                        Rosegarden::MidiByte paramLSB,
                        Rosegarden::MidiByte controller,
                        Rosegarden::MidiByte value);

    static void sendNRPN(Rosegarden::InstrumentId instrumentId,
                         Rosegarden::MidiByte paramMSB,
                         Rosegarden::MidiByte paramLSB,
                         Rosegarden::MidiByte controller,
                         Rosegarden::MidiByte value);
};

}

#endif // _STUDIOCONTROL_H_

