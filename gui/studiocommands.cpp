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

#include "studiocommands.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"

#include "Composition.h"
#include "Studio.h"
#include "MidiDevice.h"

ModifyDeviceCommand::ModifyDeviceCommand(
        Rosegarden::Studio *studio,
        int device,
        const std::string &name,
        std::vector<Rosegarden::MidiBank> bankList,
        std::vector<Rosegarden::MidiProgram> programList):
    KNamedCommand(getGlobalName()),
    m_studio(studio),
    m_device(device),
    m_name(name),
    m_bankList(bankList),
    m_programList(programList)
{
}

void
ModifyDeviceCommand::execute()
{
    Rosegarden::MidiDevice *device = m_studio->getMidiDevice(m_device);

    m_oldName = device->getName();
    m_oldBankList = device->getBanks();
    m_oldProgramList = device->getPrograms();

    device->setName(m_name);
    device->replaceBankList(m_bankList);
    device->replaceProgramList(m_programList);
}

void
ModifyDeviceCommand::unexecute()
{
    Rosegarden::MidiDevice *device = m_studio->getMidiDevice(m_device);

    device->setName(m_oldName);
    device->replaceBankList(m_oldBankList);
    device->replaceProgramList(m_oldProgramList);

}

// -------------------- ModifyDeviceMapping -----------------------
//

ModifyDeviceMappingCommand::ModifyDeviceMappingCommand(
        RosegardenGUIDoc *doc,
        Rosegarden::DeviceId fromDevice,
        Rosegarden::DeviceId toDevice):
            KNamedCommand(getGlobalName()),
            m_composition(&doc->getComposition()),
            m_studio(&doc->getStudio()),
            m_fromDevice(fromDevice),
            m_toDevice(toDevice)
{
}

void
ModifyDeviceMappingCommand::execute()
{
    Rosegarden::Composition::trackcontainer *tracks =
        m_composition->getTracks();
    Rosegarden::Composition::trackcontainer::iterator it = tracks->begin();
    Rosegarden::Instrument *instr = 0;
    int index = 0;

    for(; it != tracks->end(); it++)
    {
        instr = m_studio->getInstrumentById(it->second->getInstrument());

        if (instr->getDevice()->getId() == m_fromDevice)
        {
            // if source and target are MIDI
            if (m_studio->getDevice(m_fromDevice)->getType() ==
                    Rosegarden::Device::Midi &&
                m_studio->getDevice(m_toDevice)->getType() ==
                    Rosegarden::Device::Midi)
            {
                // Try to match channels on the target device
                //
                Rosegarden::MidiByte channel = instr->getMidiChannel();

                Rosegarden::InstrumentList destList = m_studio->
                    getDevice(m_toDevice)->getPresentationInstruments();

                Rosegarden::InstrumentList::iterator dIt = destList.begin();

                for (; dIt != destList.end(); dIt++)
                {
                    if ((*dIt)->getMidiChannel() == channel)
                    {
                        break;
                    }
                }

                // Failure to match anything and there's no Instruments
                // at all in the destination.  Skip to next source Instrument.
                //
                if (dIt == destList.end() || destList.size() == 0)
                    continue;


                RG_DEBUG << " Track " << it->first  
                         << ", setting Instrument to "
                         << (*dIt)->getId() << endl;

                // store "to" and "from" values
                //
                m_mapping.push_back(
                        std::pair<Rosegarden::TrackId,
                                  Rosegarden::InstrumentId>
                                  (it->first,
                                   instr->getId()));

                it->second->setInstrument((*dIt)->getId());
            }
            else // audio is involved in the mapping - use indexes
            {
                // assign by index numbers
                Rosegarden::InstrumentList destList = m_studio->
                    getDevice(m_toDevice)->getPresentationInstruments();

                // skip if we can't match
                //
                if (index > destList.size() - 1)
                    continue;

                m_mapping.push_back(
                        std::pair<Rosegarden::TrackId,
                                  Rosegarden::InstrumentId>
                                  (it->first,
                                   instr->getId()));

                it->second->setInstrument(destList[index]->getId());
            }

            index++;
        }
    }

}

void
ModifyDeviceMappingCommand::unexecute()
{
    std::vector<std::pair<Rosegarden::TrackId, Rosegarden::InstrumentId> >
        ::iterator it = m_mapping.begin();
    Rosegarden::Track *track = 0;

    for (; it != m_mapping.end(); it++)
    {
        track = m_composition->getTrackByIndex(it->first);
        track->setInstrument(it->second);
    }
}

// ----------------------- ModifyInstrumentMapping -------------------------
//

ModifyInstrumentMappingCommand::ModifyInstrumentMappingCommand(
        RosegardenGUIDoc *doc,
        Rosegarden::InstrumentId fromInstrument,
        Rosegarden::InstrumentId toInstrument):
            KNamedCommand(getGlobalName()),
            m_composition(&doc->getComposition()),
            m_studio(&doc->getStudio()),
            m_fromInstrument(fromInstrument),
            m_toInstrument(toInstrument)
{
}

void
ModifyInstrumentMappingCommand::execute()
{
    Rosegarden::Composition::trackcontainer *tracks =
        m_composition->getTracks();
    Rosegarden::Composition::trackcontainer::iterator it = tracks->begin();

    for(; it != tracks->end(); it++)
    {
        if (it->second->getInstrument() == m_fromInstrument)
        {
            m_mapping.push_back(it->first);
            it->second->setInstrument(m_toInstrument);
        }
    }

}

void
ModifyInstrumentMappingCommand::unexecute()
{
    std::vector<Rosegarden::TrackId>::iterator it = m_mapping.begin();
    Rosegarden::Track *track = 0;

    for (; it != m_mapping.end(); it++)
    {
        track = m_composition->getTrackByIndex(*it);
        track->setInstrument(m_fromInstrument);
    }
}

        




