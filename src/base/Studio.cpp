// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <iostream>

#include "base/Studio.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "SoftSynthDevice.h"
#include "Instrument.h"

#include "base/Segment.h"
#include "Track.h"
#include "Composition.h"

#if (__GNUC__ < 3)
#include <strstream>
#define stringstream strstream
#else
#include <sstream>
#endif

using std::cerr;
using std::endl;


namespace Rosegarden
{

Studio::Studio() :
    m_midiThruFilter(0),
    m_midiRecordFilter(0),
    m_mixerDisplayOptions(0),
    m_metronomeDevice(0)
{
    // We _always_ have a buss with id zero, for the master out
    m_busses.push_back(new Buss(0));

    // And we always create one audio record in
    m_recordIns.push_back(new RecordIn());
}

Studio::~Studio()
{
    DeviceListIterator dIt = m_devices.begin();

    for (; dIt != m_devices.end(); ++dIt)
        delete(*dIt);

    m_devices.clear();

    for (size_t i = 0; i < m_busses.size(); ++i) {
	delete m_busses[i];
    }

    for (size_t i = 0; i < m_recordIns.size(); ++i) {
	delete m_recordIns[i];
    }
}

void
Studio::addDevice(const std::string &name,
                  DeviceId id,
                  Device::DeviceType type)
{
    switch(type)
    {
        case Device::Midi:
            m_devices.push_back(new MidiDevice(id, name, MidiDevice::Play));
            break;

        case Device::Audio:
            m_devices.push_back(new AudioDevice(id, name));
            break;

        case Device::SoftSynth:
            // This was never handled here before, which caused a compiler
            // warning about the enum not being handled in the switch.  Since
            // this feature works in spite of doing nothing special for this
            // case, I have put Device::SoftSynth in as effectively a second
            // default: to shut up the compiler warning.
        default:
            std::cerr << "Studio::addDevice() - unrecognised device"
                      << endl;
            break;
    }
}

void
Studio::addDevice(Device *device)
{
    m_devices.push_back(device);
}

void
Studio::removeDevice(DeviceId id)
{
    DeviceListIterator it;
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
	if ((*it)->getId() == id) {
	    delete *it;
	    m_devices.erase(it);
	    return;
	}
    }
}

InstrumentList
Studio::getAllInstruments()
{
    InstrumentList list, subList;

    DeviceListIterator it;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        // get sub list
        subList = (*it)->getAllInstruments();

        // concetenate
        list.insert(list.end(), subList.begin(), subList.end());
    }

    return list;

}

InstrumentList
Studio::getPresentationInstruments()
{
    InstrumentList list, subList;

    std::vector<Device*>::iterator it;
    MidiDevice *midiDevice;

    // Append lists
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
	{
	    // skip read-only devices
	    if (midiDevice->getDirection() == MidiDevice::Record)
		continue;
	}
	
        // get sub list
        subList = (*it)->getPresentationInstruments();

        // concatenate
        list.insert(list.end(), subList.begin(), subList.end());
    }

    return list;

}

Instrument*
Studio::getInstrumentById(InstrumentId id)
{
    std::vector<Device*>::iterator it;
    InstrumentList list;
    InstrumentList::iterator iit;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        list = (*it)->getAllInstruments();

        for (iit = list.begin(); iit != list.end(); iit++)
            if ((*iit)->getId() == id)
                return (*iit);
    }

    return 0;

}

// From a user selection (from a "Presentation" list) return
// the matching Instrument
//
Instrument*
Studio::getInstrumentFromList(int index)
{
    std::vector<Device*>::iterator it;
    InstrumentList list;
    InstrumentList::iterator iit;
    int count = 0;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        MidiDevice *midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
	{
          // skip read-only devices
          if (midiDevice->getDirection() == MidiDevice::Record)
              continue;
        }

        list = (*it)->getPresentationInstruments();

        for (iit = list.begin(); iit != list.end(); iit++)
        {
            if (count == index)
                return (*iit);

            count++;
        }
    }

    return 0;

}

Instrument *
Studio::getInstrumentFor(Segment *segment)
{
    if (!segment) return 0;
    if (!segment->getComposition()) return 0;
    TrackId tid = segment->getTrack();
    Track *track = segment->getComposition()->getTrackById(tid);
    if (!track) return 0;
    return getInstrumentFor(track);
}

Instrument *
Studio::getInstrumentFor(Track *track)
{
    if (!track) return 0;
    InstrumentId iid = track->getInstrument();
    return getInstrumentById(iid);
}

BussList
Studio::getBusses()
{
    return m_busses;
}

Buss *
Studio::getBussById(BussId id)
{
    for (BussList::iterator i = m_busses.begin(); i != m_busses.end(); ++i) {
	if ((*i)->getId() == id) return *i;
    }
    return 0;
}

void
Studio::addBuss(Buss *buss)
{
    m_busses.push_back(buss);
}

PluginContainer *
Studio::getContainerById(InstrumentId id)
{
    PluginContainer *pc = getInstrumentById(id);
    if (pc) return pc;
    else return getBussById(id);
}

RecordIn *
Studio::getRecordIn(int number)
{
    if (number >= 0 && number < int(m_recordIns.size())) return m_recordIns[number];
    else return 0;
}

// Clear down the devices  - the devices will clear down their
// own Instruments.
//
void
Studio::clear()
{
    InstrumentList list;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
        delete *it;

    m_devices.erase(m_devices.begin(), m_devices.end());
}

std::string
Studio::toXmlString()
{
    return toXmlString(std::vector<DeviceId>());
}

std::string
Studio::toXmlString(const std::vector<DeviceId> &devices)
{
    std::stringstream studio;

    studio << "<studio thrufilter=\"" << m_midiThruFilter
           << "\" recordfilter=\"" << m_midiRecordFilter
	   << "\" audioinputpairs=\"" << m_recordIns.size()
	   << "\" mixerdisplayoptions=\"" << m_mixerDisplayOptions
           << "\" metronomedevice=\"" << m_metronomeDevice
           << "\">" << endl << endl;

    studio << endl;

    InstrumentList list;

    // Get XML version of devices
    //
    if (devices.empty()) { // export all devices and busses

	for (DeviceListIterator it = m_devices.begin();
	     it != m_devices.end(); it++) {
	    studio << (*it)->toXmlString() << endl << endl;
	}

	for (BussList::iterator it = m_busses.begin();
	     it != m_busses.end(); ++it) {
	    studio << (*it)->toXmlString() << endl << endl;
	}

    } else {
	for (std::vector<DeviceId>::const_iterator di(devices.begin());
	     di != devices.end(); ++di) {
	    Device *d = getDevice(*di);
	    if (!d) {
		std::cerr << "WARNING: Unknown device id " << (*di)
			  << " in Studio::toXmlString" << std::endl;
	    } else {
		studio << d->toXmlString() << endl << endl;
	    }
	}
    }

    studio << endl << endl;

#if (__GNUC__ < 3)
    studio << "</studio>" << endl << std::ends;
#else
    studio << "</studio>" << endl;
#endif

    return studio.str();
}

// Run through the Devices checking for MidiDevices and
// returning the first Metronome we come across
//
const MidiMetronome*
Studio::getMetronomeFromDevice(DeviceId id)
{
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++) {

        MidiDevice *midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice && 
            midiDevice->getId() == id &&
            midiDevice->getMetronome()) {
            return midiDevice->getMetronome();
        }

	SoftSynthDevice *ssDevice = dynamic_cast<SoftSynthDevice *>(*it);
        if (ssDevice && 
            ssDevice->getId() == id &&
            ssDevice->getMetronome()) {
            return ssDevice->getMetronome();
        }
    }

    return 0;
}

// Scan all MIDI devices for available channels and map
// them to a current program

Instrument*
Studio::assignMidiProgramToInstrument(MidiByte program,
				      int msb, int lsb,
				      bool percussion)
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    // Instruments that we may return
    //
    Rosegarden::Instrument *newInstrument = 0;
    Rosegarden::Instrument *firstInstrument = 0;

    bool needBank = (msb >= 0 || lsb >= 0);
    if (needBank) {
	if (msb < 0) msb = 0;
	if (lsb < 0) lsb = 0;
    }

    // Pass one - search through all MIDI instruments looking for
    // a match that we can re-use.  i.e. if we have a matching 
    // Program Change then we can use this Instrument.
    //
    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice && midiDevice->getDirection() == MidiDevice::Play)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                if (firstInstrument == 0)
                    firstInstrument = *iit;

                // If we find an Instrument sending the right program already.
                //
                if ((*iit)->sendsProgramChange() &&
                    (*iit)->getProgramChange() == program &&
		    (!needBank || ((*iit)->sendsBankSelect() &&
				   (*iit)->getMSB() == msb &&
				   (*iit)->getLSB() == lsb &&
				   (*iit)->isPercussion() == percussion)))
                {
                    return (*iit);
                }
                else
                {
                    // Ignore the program change and use the percussion
                    // flag.
                    //
                    if ((*iit)->isPercussion() && percussion)
                    {
                        return (*iit);
                    }

                    // Otherwise store the first Instrument for
                    // possible use later.
                    //
                    if (newInstrument == 0 &&
                        (*iit)->sendsProgramChange() == false &&
			(*iit)->sendsBankSelect() == false &&
			(*iit)->isPercussion() == percussion)
                        newInstrument = *iit;
                }
            }
        }
    }

    
    // Okay, if we've got this far and we have a new Instrument to use
    // then use it.
    //
    if (newInstrument != 0)
    {
        newInstrument->setSendProgramChange(true);
        newInstrument->setProgramChange(program);

	if (needBank) {
	    newInstrument->setSendBankSelect(true);
	    newInstrument->setPercussion(percussion);
	    newInstrument->setMSB(msb);
	    newInstrument->setLSB(lsb);
	}
    }
    else // Otherwise we just reuse the first Instrument we found
        newInstrument = firstInstrument;


    return newInstrument;
}

// Just make all of these Instruments available for automatic
// assignment in the assignMidiProgramToInstrument() method
// by invalidating the ProgramChange flag.
//
// This method sounds much more dramatic than it actually is - 
// it could probably do with a rename.
//
//
void
Studio::unassignAllInstruments()
{
    MidiDevice *midiDevice;
    AudioDevice *audioDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;
    int channel = 0;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getPresentationInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                // Only for true MIDI Instruments - not System ones
                //
                if ((*iit)->getId() >= MidiInstrumentBase)
                {
                    (*iit)->setSendBankSelect(false);
                    (*iit)->setSendProgramChange(false);
                    (*iit)->setMidiChannel(channel);
                    channel = ( channel + 1 ) % 16;

                    (*iit)->setSendPan(false);
                    (*iit)->setSendVolume(false);
                    (*iit)->setPan(MidiMidValue);
                    (*iit)->setVolume(100);

                }
            }
        }
        else
        {
            audioDevice = dynamic_cast<AudioDevice*>(*it);

            if (audioDevice)
            {
                instList = (*it)->getPresentationInstruments();

                for (iit = instList.begin(); iit != instList.end(); iit++)
                    (*iit)->emptyPlugins();
            }
        }
    }
}

void
Studio::clearMidiBanksAndPrograms()
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            midiDevice->clearProgramList();
            midiDevice->clearBankList();
        }
    }
}

void
Studio::clearBusses()
{
    for (size_t i = 0; i < m_busses.size(); ++i) {
	delete m_busses[i];
    }
    m_busses.clear();
    m_busses.push_back(new Buss(0));
}

void
Studio::clearRecordIns()
{
    for (size_t i = 0; i < m_recordIns.size(); ++i) {
	delete m_recordIns[i];
    }
    m_recordIns.clear();
    m_recordIns.push_back(new RecordIn());
}

Device*
Studio::getDevice(DeviceId id)
{
    //cerr << "Studio[" << this << "]::getDevice(" << id << ")... ";

    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++) {

//	if (it != m_devices.begin()) cerr << ", ";

//	cerr << (*it)->getId();
        if ((*it)->getId() == id) {
	    //cerr << ". Found" << endl;
	    return (*it);
	}
    }

    //cerr << ". Not found" << endl;

    return 0;
}

std::string
Studio::getSegmentName(InstrumentId id)
{
    MidiDevice *midiDevice;
    std::vector<Device*>::iterator it;
    Rosegarden::InstrumentList::iterator iit;
    Rosegarden::InstrumentList instList;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        midiDevice = dynamic_cast<MidiDevice*>(*it);

        if (midiDevice)
        {
            instList = (*it)->getAllInstruments();

            for (iit = instList.begin(); iit != instList.end(); iit++)
            {
                if ((*iit)->getId() == id)
                {
                    if ((*iit)->sendsProgramChange())
                    {
			return (*iit)->getProgramName();
                    }
                    else
                    {
                        return midiDevice->getName() + " " + (*iit)->getName();
                    }
                }
            }
        }
    }

    return std::string("");
}

InstrumentId
Studio::getAudioPreviewInstrument()
{
    AudioDevice *audioDevice;
    std::vector<Device*>::iterator it;

    for (it = m_devices.begin(); it != m_devices.end(); it++)
    {
        audioDevice = dynamic_cast<AudioDevice*>(*it);

        // Just the first one will do - we can make this more
        // subtle if we need to later.
        //
        if (audioDevice)
            return audioDevice->getPreviewInstrument();
    }

    // system instrument -  won't accept audio
    return 0;
}

bool
Studio::haveMidiDevices() const
{
    Rosegarden::DeviceListConstIterator it = m_devices.begin();
    for (; it != m_devices.end(); it++)
    {
        if ((*it)->getType() == Device::Midi) return true;
    }
    return false;
}
    

}

