// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include "MidiDevice.h"
#include "MidiTypes.h"
#include "Instrument.h"
#include "ControlParameter.h"

#include <cstdio>
#include <iostream>
#include <set>

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif


namespace Rosegarden
{

MidiDevice::MidiDevice():
    Device(0, "Default Midi Device", Device::Midi),
    m_metronome(0),
    m_direction(Play),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    generatePresentationList();
    generateDefaultControllers();

    // create a default Metronome
    m_metronome = new MidiMetronome(MidiInstrumentBase + 9);
}

MidiDevice::MidiDevice(DeviceId id,
                       const std::string &name,
                       DeviceDirection dir):
    Device(id, name, Device::Midi),
    m_metronome(0),
    m_direction(dir),
    m_variationType(NoVariations),
    m_librarian(std::pair<std::string, std::string>("<none>", "<none>"))
{
    generatePresentationList();
    generateDefaultControllers();

    // create a default Metronome
    m_metronome = new MidiMetronome(MidiInstrumentBase + 9);
}

MidiDevice::MidiDevice(DeviceId id,
		       const MidiDevice &dev) :
    Device(id, dev.getName(), Device::Midi),
    m_programList(dev.m_programList),
    m_bankList(dev.m_bankList),
    m_controlList(dev.m_controlList),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_variationType(dev.getVariationType()),
    m_librarian(dev.getLibrarian())
{
    // Create and assign a metronome if required
    //
    if (dev.getMetronome()) {
        m_metronome = new MidiMetronome(*dev.getMetronome());
    }

    generatePresentationList();
    generateDefaultControllers();
}

MidiDevice::MidiDevice(const MidiDevice &dev) :
    Device(dev.getId(), dev.getName(), dev.getType()),
    Controllable(),
    m_programList(dev.m_programList),
    m_bankList(dev.m_bankList),
    m_controlList(dev.m_controlList),
    m_metronome(0),
    m_direction(dev.getDirection()),
    m_variationType(dev.getVariationType()),
    m_librarian(dev.getLibrarian())
{
    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
        m_metronome = new MidiMetronome(*dev.getMetronome());
    }

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();
}


MidiDevice &
MidiDevice::operator=(const MidiDevice &dev)
{
    if (&dev == this) return *this;

    m_id = dev.getId();
    m_name = dev.getName();
    m_type = dev.getType();
    m_librarian = dev.getLibrarian();

    m_programList = dev.getPrograms();
    m_bankList = dev.getBanks();
    m_controlList = dev.getControlParameters();
    m_direction = dev.getDirection();
    m_variationType = dev.getVariationType();

    // clear down instruments list
    m_instruments.clear();
    m_presentationInstrumentList.clear();

    // Create and assign a metronome if required
    //
    if (dev.getMetronome())
    {
	if (m_metronome) delete m_metronome;
	m_metronome = new MidiMetronome(*dev.getMetronome());
    }
    else
    {
	delete m_metronome;
	m_metronome = 0;
    }

    // Copy the instruments
    //
    InstrumentList insList = dev.getAllInstruments();
    InstrumentList::iterator iIt = insList.begin();
    for (; iIt != insList.end(); iIt++)
    {
        Instrument *newInst = new Instrument(**iIt);
        newInst->setDevice(this);
        m_instruments.push_back(newInst);
    }

    // generate presentation instruments
    generatePresentationList();

    return (*this);
}

MidiDevice::~MidiDevice()
{
    delete m_metronome;
    //!!! delete key mappings
}

void
MidiDevice::generatePresentationList()
{
    // Fill the presentation list for the instruments
    //
    m_presentationInstrumentList.clear();

    InstrumentList::iterator it;
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() >= MidiInstrumentBase) {
            m_presentationInstrumentList.push_back(*it);
	}
    }
}

void
MidiDevice::generateDefaultControllers()
{
    m_controlList.clear();

    static std::string controls[][9] = {
        { "Pan", Rosegarden::Controller::EventType, "<none>", "0", "127", "64", "10", "2", "0" },
        { "Chorus", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "93", "3", "1" },
        { "Volume", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "7", "1", "2" },
        { "Reverb", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "91", "3", "3" },
        { "Sustain", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "64", "4", "-1" },
        { "Expression", Rosegarden::Controller::EventType, "<none>", "0", "127", "100", "11", "2", "-1" },
        { "Modulation", Rosegarden::Controller::EventType, "<none>", "0", "127", "0", "1", "4", "-1" },
        { "PitchBend", Rosegarden::PitchBend::EventType, "<none>", "0", "16383", "8192", "1", "4", "-1" }
    };

    for (unsigned int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {

        Rosegarden::ControlParameter con(controls[i][0],
                                         controls[i][1],
                                         controls[i][2],
                                         atoi(controls[i][3].c_str()),
                                         atoi(controls[i][4].c_str()),
                                         atoi(controls[i][5].c_str()),
                                         Rosegarden::MidiByte(atoi(controls[i][6].c_str())),
                                         atoi(controls[i][7].c_str()),
                                         atoi(controls[i][8].c_str()));
        addControlParameter(con);
    }


}

void
MidiDevice::clearBankList()
{
    m_bankList.clear();
}

void
MidiDevice::clearProgramList()
{
    m_programList.clear();
}

void
MidiDevice::clearControlList()
{
    m_controlList.clear();
}

void
MidiDevice::addProgram(const MidiProgram &prog)
{
    // Refuse duplicates
    for (ProgramList::const_iterator it = m_programList.begin();
	 it != m_programList.end(); ++it) {
	if (*it == prog) return;
    }

    m_programList.push_back(prog);
}

void 
MidiDevice::addBank(const MidiBank &bank)
{
    m_bankList.push_back(bank);
}

void
MidiDevice::removeMetronome()
{
    delete m_metronome;
    m_metronome = 0;
}

void
MidiDevice::setMetronome(const MidiMetronome &metronome)
{
    delete m_metronome;
    m_metronome = new MidiMetronome(metronome);
}

BankList
MidiDevice::getBanks(bool percussion) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (it->isPercussion() == percussion) banks.push_back(*it);
    }

    return banks;
}

BankList
MidiDevice::getBanksByMSB(bool percussion, MidiByte msb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (it->isPercussion() == percussion && it->getMSB() == msb)
	    banks.push_back(*it);
    }

    return banks;
}

BankList
MidiDevice::getBanksByLSB(bool percussion, MidiByte lsb) const
{
    BankList banks;

    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (it->isPercussion() == percussion && it->getLSB() == lsb)
	    banks.push_back(*it);
    }

    return banks;
}

MidiByteList
MidiDevice::getDistinctMSBs(bool percussion, int lsb) const
{
    std::set<MidiByte> msbs;

    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (it->isPercussion() == percussion &&
	    (lsb == -1 || it->getLSB() == lsb)) msbs.insert(it->getMSB());
    }

    MidiByteList v;
    for (std::set<MidiByte>::iterator i = msbs.begin(); i != msbs.end(); ++i) {
	v.push_back(*i);
    }

    return v;
}

MidiByteList
MidiDevice::getDistinctLSBs(bool percussion, int msb) const
{
    std::set<MidiByte> lsbs;

    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (it->isPercussion() == percussion &&
	    (msb == -1 || it->getMSB() == msb)) lsbs.insert(it->getLSB());
    }

    MidiByteList v;
    for (std::set<MidiByte>::iterator i = lsbs.begin(); i != lsbs.end(); ++i) {
	v.push_back(*i);
    }

    return v;
}

ProgramList
MidiDevice::getPrograms(const MidiBank &bank) const
{
    ProgramList programs;

    for (ProgramList::const_iterator it = m_programList.begin();
	 it != m_programList.end(); ++it) {
	if (it->getBank() == bank) programs.push_back(*it);
    }

    return programs;
}

std::string
MidiDevice::getBankName(const MidiBank &bank) const
{
    for (BankList::const_iterator it = m_bankList.begin();
	 it != m_bankList.end(); ++it) {
	if (*it == bank) return it->getName();
    }
    return "";
}

void
MidiDevice::addKeyMapping(const MidiKeyMapping &mapping)
{
    //!!! handle dup names
    m_keyMappingList.push_back(mapping);
}

const MidiKeyMapping *
MidiDevice::getKeyMappingByName(const std::string &name) const
{
    for (KeyMappingList::const_iterator i = m_keyMappingList.begin();
	 i != m_keyMappingList.end(); ++i) {
	if (i->getName() == name) return &(*i);
    }
    return 0;
}

const MidiKeyMapping *
MidiDevice::getKeyMappingForProgram(const MidiProgram &program) const
{
    ProgramList::const_iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++) {
	if (*it == program) {
	    std::string kmn = it->getKeyMapping();
	    if (kmn == "") return 0;
	    return getKeyMappingByName(kmn);
	}
    }

    return 0;
}

void
MidiDevice::setKeyMappingForProgram(const MidiProgram &program,
				    std::string mapping)
{
    ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++) {
	if (*it == program) {
	    it->setKeyMapping(mapping);
	}
    }
}
    

std::string
MidiDevice::toXmlString()
{
    std::stringstream midiDevice;

    midiDevice << "    <device id=\""  << m_id 
               << "\" name=\""         << m_name 
	       << "\" direction=\""    << (m_direction == Play ?
					   "play" : "record")
	       << "\" variation=\""    << (m_variationType == VariationFromLSB ?
					   "LSB" :
					   m_variationType == VariationFromMSB ?
					   "MSB" : "")
	       << "\" connection=\""   << encode(m_connection)
               << "\" type=\"midi\">"  << std::endl << std::endl;

    midiDevice << "        <librarian name=\"" << encode(m_librarian.first)
               << "\" email=\"" << encode(m_librarian.second)
               << "\"/>" << std::endl;

    if (m_metronome)
    {
        // Write out the metronome - watch the MidiBytes
        // when using the stringstream
        //
        midiDevice << "        <metronome "
                   << "instrument=\"" << m_metronome->getInstrument() << "\" "
                   << "barpitch=\"" << (int)m_metronome->getBarPitch() << "\" "
                   << "beatpitch=\"" << (int)m_metronome->getBeatPitch() << "\" "
                   << "subbeatpitch=\"" << (int)m_metronome->getSubBeatPitch() << "\" "
                   << "depth=\"" << (int)m_metronome->getDepth() << "\" "
                   << "barvelocity=\"" << (int)m_metronome->getBarVelocity() << "\" "
                   << "beatvelocity=\"" << (int)m_metronome->getBeatVelocity() << "\" "
                   << "subbeatvelocity=\"" << (int)m_metronome->getSubBeatVelocity() 
                   << "\"/>"
                   << std::endl << std::endl;
    }

    // and now bank information
    //
    BankList::iterator it;
    InstrumentList::iterator iit;
    ProgramList::iterator pt;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
    {
        midiDevice << "        <bank "
                   << "name=\"" << encode(it->getName()) << "\" "
	           << "percussion=\"" << (it->isPercussion() ? "true" : "false") << "\" "
                   << "msb=\"" << (int)it->getMSB() << "\" "
                   << "lsb=\"" << (int)it->getLSB() << "\">"
                   << std::endl;

        // Not terribly efficient
        //
        for (pt = m_programList.begin(); pt != m_programList.end(); pt++)
        {
	    if (pt->getBank() == *it)
            {
                midiDevice << "            <program "
                           << "id=\"" << (int)pt->getProgram() << "\" "
                           << "name=\"" << encode(pt->getName()) << "\" ";
                if (!pt->getKeyMapping().empty()) {
                    midiDevice << "keymapping=\"" 
                               << encode(pt->getKeyMapping()) << "\" ";
                }
                midiDevice << "/>" << std::endl;
            }
        }

        midiDevice << "        </bank>" << std::endl << std::endl;
    }

    // Now controllers (before Instruments, which can depend on 
    // Controller colours)
    //
    midiDevice << "        <controls>" << std::endl;
    ControlList::iterator cIt;
    for (cIt = m_controlList.begin(); cIt != m_controlList.end() ; ++cIt)
        midiDevice << cIt->toXmlString();
    midiDevice << "        </controls>" << std::endl << std::endl;

    // Add instruments
    //
    for (iit = m_instruments.begin(); iit != m_instruments.end(); iit++)
        midiDevice << (*iit)->toXmlString();

    KeyMappingList::iterator kit;

    for (kit = m_keyMappingList.begin(); kit != m_keyMappingList.end(); kit++)
    {
        midiDevice << "        <keymapping "
                   << "name=\"" << encode(kit->getName()) << "\">\n";

	for (MidiKeyMapping::KeyNameMap::const_iterator nmi =
		 kit->getMap().begin(); nmi != kit->getMap().end(); ++nmi) {
	    midiDevice << "          <key number=\"" << (int)nmi->first
		       << "\" name=\"" << encode(nmi->second) << "\"/>\n";
	}

	midiDevice << "        </keymapping>\n";
    }

#if (__GNUC__ < 3)
    midiDevice << "    </device>" << std::endl << std::ends;
#else
    midiDevice << "    </device>" << std::endl;
#endif

    return midiDevice.str();
}

// Only copy across non System instruments
//
InstrumentList
MidiDevice::getAllInstruments() const
{
    return m_instruments;
}

// Omitting special system Instruments
//
InstrumentList
MidiDevice::getPresentationInstruments() const
{
    return m_presentationInstrumentList;
}

void
MidiDevice::addInstrument(Instrument *instrument)
{
    m_instruments.push_back(instrument);
    generatePresentationList();
}

std::string
MidiDevice::getProgramName(const MidiProgram &program) const
{
    ProgramList::const_iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
	if (*it == program) return it->getName();
    }

    return std::string("");
}

void
MidiDevice::replaceBankList(const BankList &bankList)
{
    m_bankList = bankList;
}

void
MidiDevice::replaceProgramList(const ProgramList &programList)
{
    m_programList = programList;
}

void
MidiDevice::replaceKeyMappingList(const KeyMappingList &keyMappingList)
{
    m_keyMappingList = keyMappingList;
}


// Merge the new bank list in without duplication
//
void
MidiDevice::mergeBankList(const BankList &bankList)
{
    BankList::const_iterator it;
    BankList::iterator oIt;
    bool clash = false;
    
    for (it = bankList.begin(); it != bankList.end(); it++)
    {
        for (oIt = m_bankList.begin(); oIt != m_bankList.end(); oIt++)
        {
	    if (*it == *oIt)
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addBank(*it);
        else
            clash = false;
    }

}

void
MidiDevice::mergeProgramList(const ProgramList &programList)
{
    ProgramList::const_iterator it;
    ProgramList::iterator oIt;
    bool clash = false;

    for (it = programList.begin(); it != programList.end(); it++)
    {
        for (oIt = m_programList.begin(); oIt != m_programList.end(); oIt++)
        {
	    if (*it == *oIt)
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addProgram(*it);
        else
            clash = false;
    }
}

void
MidiDevice::mergeKeyMappingList(const KeyMappingList &keyMappingList)
{
    KeyMappingList::const_iterator it;
    KeyMappingList::iterator oIt;
    bool clash = false;

    for (it = keyMappingList.begin(); it != keyMappingList.end(); it++)
    {
        for (oIt = m_keyMappingList.begin(); oIt != m_keyMappingList.end(); oIt++)
        {
	    if (it->getName() == oIt->getName())
            {
                clash = true;
                break;
            }
        }

        if (clash == false)
            addKeyMapping(*it);
        else
            clash = false;
    }
}

void
MidiDevice::addControlParameter(const ControlParameter &con)
{
    m_controlList.push_back(con);
}

void
MidiDevice::addControlParameter(const ControlParameter &con, int index)
{
    ControlList controls;

    // if we're out of range just add the control
    if (index >= (int)m_controlList.size())
    {
        m_controlList.push_back(con);
        return;
    }

    // add new controller in at a position
    for (int i = 0; i < (int)m_controlList.size(); ++i)
    {
        if (index == i) controls.push_back(con);
        controls.push_back(m_controlList[i]);
    }

    m_controlList = controls;
}


bool
MidiDevice::removeControlParameter(int index)
{
    ControlList::iterator it = m_controlList.begin();
    int i = 0;

    for (; it != m_controlList.end(); ++it)
    {
        if (index == i)
        {
            m_controlList.erase(it);
            return true;
        }
        i++;
    }

    return false;
}

bool
MidiDevice::modifyControlParameter(const ControlParameter &con, int index)
{
    if (index < 0 || index > (int)m_controlList.size()) return false;
    m_controlList[index] = con;
    return true;
}

void
MidiDevice::replaceControlParameters(const ControlList &con)
{
    m_controlList = con;
}


// Check to see if passed ControlParameter is unique.  Either the
// type must be unique or in the case of Controller::EventType the
// ControllerValue must be unique.
//
// Controllers (Control type)
//
//
bool 
MidiDevice::isUniqueControlParameter(const ControlParameter &con) const
{
    ControlList::const_iterator it = m_controlList.begin();

    for (; it != m_controlList.end(); ++it)
    {
        if (it->getType() == con.getType())
        {
            if (it->getType() == Rosegarden::Controller::EventType &&
                it->getControllerValue() != con.getControllerValue())
                continue;

            return false;
        }

    }

    return true;
}

// Cheat a bit here and remove the VOLUME controller here - just
// so that the MIDIMixer is made a bit easier.
//
ControlList
MidiDevice::getIPBControlParameters() const
{
    ControlList retList;

    Rosegarden::MidiByte MIDI_CONTROLLER_VOLUME = 0x07;

    for (ControlList::const_iterator it = m_controlList.begin();
         it != m_controlList.end(); ++it)
    {
        if (it->getIPBPosition() != -1 && 
            it->getControllerValue() != MIDI_CONTROLLER_VOLUME)
            retList.push_back(*it);
    }

    return retList;
}




ControlParameter *
MidiDevice::getControlParameter(int index)
{
    if (index >= 0 && ((unsigned int)index) < m_controlList.size())
        return &m_controlList[index];

    return 0;
}

const ControlParameter *
MidiDevice::getControlParameter(int index) const
{
    return ((MidiDevice *)this)->getControlParameter(index);
}

ControlParameter *
MidiDevice::getControlParameter(const std::string &type, Rosegarden::MidiByte controllerValue)
{
    ControlList::iterator it = m_controlList.begin();

    for (; it != m_controlList.end(); ++it)
    {
        if (it->getType() == type)
        {
            // Return matched on type for most events
            //
            if (type != Rosegarden::Controller::EventType) 
                return &*it;
            
            // Also match controller value for Controller events
            //
            if (it->getControllerValue() == controllerValue)
                return  &*it;
        }
    }

    return 0;
}

const ControlParameter *
MidiDevice::getControlParameter(const std::string &type, Rosegarden::MidiByte controllerValue) const
{
    return ((MidiDevice *)this)->getControlParameter(type, controllerValue);
}

}


