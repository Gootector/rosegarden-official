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

#include <vector>

#include <klocale.h>

#include "basiccommand.h"

#include "Instrument.h"
#include "Device.h"

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }

// Use the overwrite flag to overwrite banks and programs rather
// than merging (default behaviour).
//
class ModifyDeviceCommand : public KNamedCommand
{
public:
    ModifyDeviceCommand(Rosegarden::Studio *studio,
                        int device,
                        const std::string &name,
                        const std::string &librarianName,
                        const std::string &librarianEmail,
                        std::vector<Rosegarden::MidiBank> bankList,
                        std::vector<Rosegarden::MidiProgram> programList,
                        bool overwrite);

    static QString getGlobalName() { return i18n("Modify &MIDI Bank"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Studio                    *m_studio;
    int                                    m_device;
    std::string                            m_name;
    std::string                            m_librarianName;
    std::string                            m_librarianEmail;
    std::vector<Rosegarden::MidiBank>      m_bankList;
    std::vector<Rosegarden::MidiProgram>   m_programList;

    std::string                            m_oldName;
    std::vector<Rosegarden::MidiBank>      m_oldBankList;
    std::vector<Rosegarden::MidiProgram>   m_oldProgramList;
    std::string                            m_oldLibrarianName;
    std::string                            m_oldLibrarianEmail;

    bool                                   m_overwrite;

};

class ModifyDeviceMappingCommand : public KNamedCommand
{
public:
    ModifyDeviceMappingCommand(RosegardenGUIDoc *doc,
                               Rosegarden::DeviceId fromDevice,
                               Rosegarden::DeviceId toDevice);

    static QString getGlobalName() { return i18n("Modify &Device Mapping"); }

    virtual void execute();
    virtual void unexecute();
protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    Rosegarden::DeviceId     m_fromDevice;
    Rosegarden::DeviceId     m_toDevice;

    std::vector<std::pair<Rosegarden::TrackId, Rosegarden::InstrumentId> >
                             m_mapping;
};

class ModifyInstrumentMappingCommand : public KNamedCommand
{
public:
    ModifyInstrumentMappingCommand(RosegardenGUIDoc *doc,
                                   Rosegarden::InstrumentId fromInstrument,
                                   Rosegarden::InstrumentId toInstrument);

    static QString getGlobalName() { return i18n("Modify &Instrument Mapping"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    Rosegarden::InstrumentId m_fromInstrument;
    Rosegarden::InstrumentId m_toInstrument;

    std::vector<Rosegarden::TrackId> m_mapping;

};


