// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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

#include "config.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_ALSA

// ALSA
#include <alsa/asoundlib.h>
#include <alsa/seq_event.h>
#include <alsa/version.h>
#include <alsa/seq.h>

#include "AlsaDriver.h"
#include "AlsaPort.h"
#include "MappedInstrument.h"
#include "Midi.h"
#include "MappedStudio.h"
#include "rosestrings.h"
#include "MappedCommon.h"

#include <qregexp.h>

#define DEBUG_ALSA 1

// This driver implements MIDI in and out via the ALSA (www.alsa-project.org)
// sequencer interface.

using std::cerr;
using std::endl;


static std::string _audit;

// I was going to prevent output to std::cout if NDEBUG was set, but actually
// it's probably a good idea to continue to send to std::cout as well -- it
// shouldn't even get noticed unless someone's actually trying to debug,
// and it's necessary if the sequencer crashes altogether (because the
// status button in the GUI won't work if the sequencer's crashed).

#define AUDIT_STREAM _auditStream
#if (__GNUC__ < 3)
#include <strstream>
#define AUDIT_START std::strstream _auditStream
#ifdef NOT_DEFINED_NDEBUG
#define AUDIT_UPDATE _auditStream << std::ends; _audit += _auditStream.str();
#else
#define AUDIT_UPDATE _auditStream << std::ends; \
                     std::string _auditChunk = _auditStream.str(); \
                     _audit += _auditChunk; std::cerr << _auditChunk;
#endif
#else
#include <sstream>
#define AUDIT_START std::stringstream _auditStream
#ifdef NOT_DEFINED_NDEBUG
#define AUDIT_UPDATE _audit += _auditStream.str();
#else
#define AUDIT_UPDATE std::string _auditChunk = _auditStream.str(); \
                     _audit += _auditChunk; std::cerr << _auditChunk;
#endif
#endif


namespace Rosegarden
{

AlsaDriver::AlsaDriver(MappedStudio *studio):
    SoundDriver(studio, std::string("alsa-lib version ") +
                        std::string(SND_LIB_VERSION_STR)),
    m_client(-1),
    m_port(-1),
    m_queue(-1),
    m_maxClients(-1),
    m_maxPorts(-1),
    m_maxQueues(-1),
    m_midiInputPortConnected(false),
    m_alsaPlayStartTime(0, 0),
    m_alsaRecordStartTime(0, 0),
    m_loopStartTime(0, 0),
    m_loopEndTime(0, 0),
    m_looping(false)
#ifdef HAVE_LIBJACK
    ,m_jackDriver(0)
#endif
    ,m_queueRunning(false)

{
    AUDIT_START;
    AUDIT_STREAM << "Rosegarden " << VERSION << " - AlsaDriver - " 
                 << m_name << std::endl;

    AUDIT_UPDATE;
}

AlsaDriver::~AlsaDriver()
{
    std::cerr << "AlsaDriver::~AlsaDriver (" << (void *)this << ")" << std::endl;
    shutdown();
    std::cerr << "AlsaDriver::~AlsaDriver exiting" << std::endl;
}

void
AlsaDriver::shutdown()
{
    AUDIT_START;
    AUDIT_STREAM << "AlsaDriver::~AlsaDriver - shutting down" << std::endl;

    if (m_midiHandle)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closing MIDI client" << std::endl;
#endif

        snd_seq_stop_queue(m_midiHandle, m_queue, 0);
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - stopped queue" << std::endl;
#endif
        snd_seq_close(m_midiHandle);
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::shutdown - closed MIDI handle" << std::endl;
#endif
        m_midiHandle = 0;
    }

#ifdef HAVE_LADSPA
#ifdef DEBUG_ALSA
    std::cout << "AlsaDriver::shutdown - unloading LADSPA" << std::endl;
#endif
    m_studio->unloadAllPluginLibraries();
#ifdef DEBUG_ALSA
    std::cout << "AlsaDriver::shutdown - unloading LADSPA done" << std::endl;
#endif
#endif // HAVE_LADSPA

#ifdef HAVE_LIBJACK
    delete m_jackDriver;
#endif

   AUDIT_UPDATE;
}

void
AlsaDriver::setLoop(const RealTime &loopStart, const RealTime &loopEnd)
{
    m_loopStartTime = loopStart;
    m_loopEndTime = loopEnd;

    // currently we use this simple test for looping - it might need
    // to get more sophisticated in the future.
    //
    if (m_loopStartTime != m_loopEndTime)
        m_looping = true;
    else
        m_looping = false;
}

void
AlsaDriver::getSystemInfo()
{
    int err;
    snd_seq_system_info_t *sysinfo;

    snd_seq_system_info_alloca(&sysinfo);

    if ((err = snd_seq_system_info(m_midiHandle, sysinfo)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "System info error: " <<  snd_strerror(err)
                  << std::endl;
#endif
        exit(EXIT_FAILURE);
    }

    m_maxQueues = snd_seq_system_info_get_queues(sysinfo); 
    m_maxClients = snd_seq_system_info_get_clients(sysinfo);
    m_maxPorts = snd_seq_system_info_get_ports(sysinfo);

}

void
AlsaDriver::showQueueStatus(int queue)
{
    int err, idx, min, max;
    snd_seq_queue_status_t *status;

    snd_seq_queue_status_alloca(&status);
    min = queue < 0 ? 0 : queue;
    max = queue < 0 ? m_maxQueues : queue + 1;

    for (idx = min; idx < max; ++idx)
    {
        if ((err = snd_seq_get_queue_status(m_midiHandle, idx, status))<0)
        {
            if (err == -ENOENT)
                continue;

#ifdef DEBUG_ALSA
            std::cerr << "Client " << idx << " info error: "
                      << snd_strerror(err) << std::endl;
#endif
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG_ALSA
        std::cerr << "Queue " << snd_seq_queue_status_get_queue(status)
                  << std::endl;

        std::cerr << "Tick       = "
                  << snd_seq_queue_status_get_tick_time(status)
                  << std::endl;

        std::cerr << "Realtime   = "
                  << snd_seq_queue_status_get_real_time(status)->tv_sec
                  << "."
                  << snd_seq_queue_status_get_real_time(status)->tv_nsec
                  << std::endl;

        std::cerr << "Flags      = 0x"
                  << snd_seq_queue_status_get_status(status)
                  << std::endl;
#endif
    }

}


void
AlsaDriver::generateTimerList()
{
    // Enumerate the available timers

    snd_timer_t *timerHandle;

    snd_timer_id_t *timerId;
    snd_timer_info_t *timerInfo;

    snd_timer_id_alloca(&timerId);
    snd_timer_info_alloca(&timerInfo);

    snd_timer_query_t *timerQuery;
    char timerName[64];

    if (snd_timer_query_open(&timerQuery, "hw", 0) >= 0) {

	snd_timer_id_set_class(timerId, SND_TIMER_CLASS_NONE);

	while (1) {

	    if (snd_timer_query_next_device(timerQuery, timerId) < 0) break;
	    if (snd_timer_id_get_class(timerId) < 0) break;

	    AlsaTimerInfo info = {
		snd_timer_id_get_class(timerId),
		snd_timer_id_get_sclass(timerId),
		snd_timer_id_get_card(timerId),
		snd_timer_id_get_device(timerId),
		snd_timer_id_get_subdevice(timerId),
		""
	    };

	    if (info.card < 0) info.card = 0;
	    if (info.device < 0) info.device = 0;
	    if (info.subdevice < 0) info.subdevice = 0;

//	    std::cerr << "got timer: class " << info.clas << std::endl;

	    sprintf(timerName, "hw:CLASS=%i,SCLASS=%i,CARD=%i,DEV=%i,SUBDEV=%i",
		    info.clas, info.sclas, info.card, info.device, info.subdevice);

	    if (snd_timer_open(&timerHandle, timerName, SND_TIMER_OPEN_NONBLOCK) < 0) {
		std::cerr << "Failed to open timer: " << timerName << std::endl;
		continue;
	    }

	    if (snd_timer_info(timerHandle, timerInfo) < 0) continue;

	    info.name = snd_timer_info_get_name(timerInfo);
	    snd_timer_close(timerHandle);

//	    std::cerr << "adding timer: " << info.name << std::endl;

	    m_timers.push_back(info);
	}

	snd_timer_query_close(timerQuery);
    }
}


void
AlsaDriver::generatePortList(AlsaPortList *newPorts)
{
    AUDIT_START;
    AlsaPortList alsaPorts;

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    AUDIT_STREAM << std::endl << "  ALSA Client information:"
		 << std::endl << std::endl;

    // Get only the client ports we're interested in and store them
    // for sorting and then device creation.
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0) continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0)
        {
	    int client = snd_seq_port_info_get_client(pinfo);
	    int port = snd_seq_port_info_get_port(pinfo);
	    unsigned int clientType = snd_seq_client_info_get_type(cinfo);
	    unsigned int portType = snd_seq_port_info_get_type(pinfo);
	    unsigned int capability = snd_seq_port_info_get_capability(pinfo);

            if (((capability & writeCap) == writeCap) ||
                ((capability &  readCap) ==  readCap))
            {
                AUDIT_STREAM << "    "
			     << client << ","
			     << port << " - ("
			     << snd_seq_client_info_get_name(cinfo) << ", "
			     << snd_seq_port_info_get_name(pinfo) << ")";

                PortDirection direction;

                if (capability & SND_SEQ_PORT_CAP_DUPLEX)
                {
                    direction = Duplex;
                    AUDIT_STREAM << "\t\t\t(DUPLEX)";
                }
                else if (capability & SND_SEQ_PORT_CAP_WRITE)
                {
                    direction = WriteOnly;
                    AUDIT_STREAM << "\t\t(WRITE ONLY)";
                }
                else
                {
                    direction = ReadOnly;
                    AUDIT_STREAM << "\t\t(READ ONLY)";
                }

		AUDIT_STREAM << " [ctype " << clientType << ", ptype " << portType << ", cap " << capability << "]";

                // Generate a unique name using the client id
                //
		char portId[40];
		sprintf(portId, "%d:%d ", client, port);

                std::string fullClientName = 
                    std::string(snd_seq_client_info_get_name(cinfo));

                std::string fullPortName = 
                    std::string(snd_seq_port_info_get_name(pinfo));

		std::string name;

		// If the first part of the client name is the same as the 
                // start of the port name, just use the port name.  otherwise
                // concatenate.
                //
                int firstSpace = fullClientName.find(" ");

                // If no space is found then we try to match the whole string
                //
                if (firstSpace < 0) firstSpace = fullClientName.length();

                if (firstSpace &&
		    int(fullPortName.length()) >= firstSpace &&
		    fullPortName.substr(0, firstSpace) ==
		    fullClientName.substr(0, firstSpace)) {
		    name = portId + fullPortName;
		} else {
		    name = portId + fullClientName + ": " + fullPortName;
		}

                // Sanity check for length
                //
                if (name.length() > 25) name = portId + fullPortName;

		if (direction == WriteOnly) {
		    name += " (write)";
		} else if (direction == ReadOnly) {
		    name += " (read)";
		} else if (direction == Duplex) {
		    name += " (duplex)";
		}

                AlsaPortDescription *portDescription = 
                    new AlsaPortDescription(
                            Instrument::Midi,
                            name,
                            client,
			    port,
			    clientType,
			    portType,
			    capability,
                            direction);

		if (newPorts &&
		    (getPortName(ClientPortPair(client, port)) == "")) {
		    newPorts->push_back(portDescription);
		}

                alsaPorts.push_back(portDescription);

                AUDIT_STREAM << std::endl;
            }
        }
    }

    AUDIT_STREAM << std::endl;

    // Ok now sort by duplexicity
    //
    std::sort(alsaPorts.begin(), alsaPorts.end(), AlsaPortCmp());
    m_alsaPorts = alsaPorts;

    AUDIT_UPDATE;
}


void
AlsaDriver::generateInstruments()
{
    // Reset these before each Instrument hunt
    //
    int audioCount = 0;
    getAudioInstrumentNumbers(m_audioRunningId, audioCount);
    m_midiRunningId = MidiInstrumentBase;

    // Clear these
    //
    m_instruments.clear();
    m_devices.clear();
    m_devicePortMap.clear();
    m_suspendedPortMap.clear();

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); it++)
    {
        /*
        std::cout << "installing device " << (*it)->m_name
             << " client = " << (*it)->m_client
             << " port = " << (*it)->m_port << endl;
             */

	if ((*it)->isWriteable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Play);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                ;
#endif
	    } else {
		addInstrumentsForDevice(device);
		m_devices.push_back(device);
	    }
	}
	if ((*it)->isReadable()) {
	    MappedDevice *device = createMidiDevice(*it, MidiDevice::Record);
	    if (!device) {
#ifdef DEBUG_ALSA
		std::cerr << "WARNING: Failed to create record device" << std::endl;
#else
                ;
#endif
	    } else {
		m_devices.push_back(device);
	    }
	}
    }

#ifdef HAVE_LIBJACK

    // Create a number of audio Instruments - these are just
    // logical Instruments anyway and so we can create as 
    // many as we like and then use them as Tracks.
    //

    MappedInstrument *instr;
    char number[100];
    std::string audioName;

    DeviceId audioDeviceId = getSpareDeviceId();

    if (m_driverStatus & AUDIO_OK)
    {
        for (int channel = 0; channel < audioCount; ++channel)
        {
            sprintf(number, " #%d", channel + 1);
            audioName = "JACK Audio" + std::string(number);
            instr = new MappedInstrument(Instrument::Audio,
                                         channel,
                                         m_audioRunningId,
                                         audioName,
                                         audioDeviceId);
            m_instruments.push_back(instr);

            // Create a fader with a matching id - this is the starting
            // point for all audio faders.
            //
            m_studio->createObject(MappedObject::AudioFader,
                                   m_audioRunningId);

            /*
            std::cerr  << "AlsaDriver::generateInstruments - "
                       << "added audio fader (id=" << m_audioRunningId
                       << ")" << std::endl;
                       */
    
            m_audioRunningId++;
        }

        // Create audio device
        //
        MappedDevice *device =
                        new MappedDevice(audioDeviceId,
                                         Device::Audio,
                                         "JACK Audio",
                                         "Audio connection");
        m_devices.push_back(device);
    }

#endif
}

MappedDevice *
AlsaDriver::createMidiDevice(AlsaPortDescription *port,
			     MidiDevice::DeviceDirection reqDirection)
{
    char deviceName[100];
    std::string connectionName("");
    AUDIT_START;

    static int unknownCounter;

    static int counters[3][2]; // [system/hardware/software][out/in]
    const int SYSTEM = 0, HARDWARE = 1, SOFTWARE = 2;
    static const char *firstNames[4][2] = {
	{ "MIDI output system device", "MIDI input system device" },
	{ "MIDI external device", "MIDI hardware input device" },
	{ "MIDI software device", "MIDI software input" }
    };
    static const char *countedNames[4][2] = {
	{ "MIDI output system device %d", "MIDI input system device %d" },
	{ "MIDI external device %d", "MIDI hardware input device %d" },
	{ "MIDI software device %d", "MIDI software input %d" }
    };

    static int specificCounters[2];
    static const char *specificNames[2] = {
	"MIDI soundcard synth", "MIDI soft synth",
    };
    static const char *specificCountedNames[2] = {
	"MIDI soundcard synth %d", "MIDI soft synth %d",
    };

    DeviceId deviceId = getSpareDeviceId();

    if (port) {

	if (reqDirection == MidiDevice::Record && !port->isReadable())  return 0;
	if (reqDirection == MidiDevice::Play   && !port->isWriteable()) return 0;

	int category = (port->m_client <  64 ? SYSTEM :
			port->m_client < 128 ? HARDWARE : SOFTWARE);

	bool haveName = false;

	if (category != SYSTEM && reqDirection == MidiDevice::Play) {

	    // No way to query whether a port is a MIDI synth, as
	    // PORT_TYPE_SYNTH actually indicates something different
	    // (ability to do direct wavetable synthesis -- nothing
	    // to do with MIDI).  But we assume GM/GS/XG/MT32 devices
	    // are synths.

	    bool isSynth = (port->m_portType &
			    (SND_SEQ_PORT_TYPE_MIDI_GM |
			     SND_SEQ_PORT_TYPE_MIDI_GS |
			     SND_SEQ_PORT_TYPE_MIDI_XG |
			     SND_SEQ_PORT_TYPE_MIDI_MT32));

	    // Because we can't discover through the API whether a
	    // port is a synth, we are instead reduced to this
	    // disgusting hack.  (At least we should make this
	    // configurable!)

	    if (!isSynth &&
		(port->m_name.find("ynth") < port->m_name.length())) isSynth = true;
	    if (!isSynth &&
		(port->m_name.find("nstrument") < port->m_name.length())) isSynth = true;
	    if (!isSynth &&
		(port->m_name.find("VSTi") < port->m_name.length())) isSynth = true;
	    
	    if (category == SYSTEM) isSynth = false;

	    if (isSynth) {
		int clientType = (category == SOFTWARE) ? 1 : 0;
		if (specificCounters[clientType] == 0) {
		    sprintf(deviceName, specificNames[clientType]);
		    ++specificCounters[clientType];
		} else {
		    sprintf(deviceName,
			    specificCountedNames[clientType],
			    ++specificCounters[clientType]);
		}
		haveName = true;
	    }
	}

	if (!haveName) {

	    if (counters[category][reqDirection] == 0) {
		sprintf(deviceName, firstNames[category][reqDirection]);
		++counters[category][reqDirection];
	    } else {
		sprintf(deviceName,
			countedNames[category][reqDirection],
			++counters[category][reqDirection]);
	    }
	}

	m_devicePortMap[deviceId] = ClientPortPair(port->m_client,
						   port->m_port);

	connectionName = port->m_name;

	AUDIT_STREAM << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode for connection " << connectionName
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;

    } else {

	sprintf(deviceName, "Anonymous MIDI device %d", ++unknownCounter);

	AUDIT_STREAM << "Creating device " << deviceId << " in "
		     << (reqDirection == MidiDevice::Play ? "Play" : "Record")
		     << " mode -- no connection available "
		     << "\nDefault device name for this device is "
		     << deviceName << std::endl;
    }
	
    MappedDevice *device = new MappedDevice(deviceId,
					    Device::Midi,
					    deviceName,
					    connectionName);
    device->setDirection(reqDirection);
    AUDIT_UPDATE;
    return device;
}

DeviceId
AlsaDriver::getSpareDeviceId()
{
    std::set<DeviceId> ids;
    for (unsigned int i = 0; i < m_devices.size(); ++i) {
	ids.insert(m_devices[i]->getId());
    }

    DeviceId id = 0;
    while (ids.find(id) != ids.end()) ++id;
    return id;
}

void
AlsaDriver::addInstrumentsForDevice(MappedDevice *device)
{
    std::string channelName;
    char number[100];

    for (int channel = 0; channel < 16; ++channel)
    {
	// Create MappedInstrument for export to GUI
	//
	// name is just number, derive rest from device at gui
	sprintf(number, "#%d", channel + 1);
	channelName = std::string(number);
	
	if (channel == 9) channelName = std::string("#10[D]");
	MappedInstrument *instr = new MappedInstrument(Instrument::Midi,
						       channel,
						       m_midiRunningId++,
						       channelName,
						       device->getId());
	m_instruments.push_back(instr);
    }
}
    

bool
AlsaDriver::canReconnect(Device::DeviceType type)
{
    return (type == Device::Midi);
}

DeviceId
AlsaDriver::addDevice(Device::DeviceType type,
		      MidiDevice::DeviceDirection direction)
{
    if (type == Device::Midi) {

	MappedDevice *device = createMidiDevice(0, direction);
	if (!device) {
#ifdef DEBUG_ALSA
	    std::cerr << "WARNING: Device creation failed" << std::endl;
#else
            ;
#endif
	} else {
	    addInstrumentsForDevice(device);
	    m_devices.push_back(device);

	    MappedEvent *mE =
		new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				0, 0);
	    insertMappedEventForReturn(mE);

	    return device->getId();
	}
    }

    return Device::NO_DEVICE;
}

void
AlsaDriver::removeDevice(DeviceId id)
{
    for (MappedDeviceList::iterator i = m_devices.end();
	 i != m_devices.begin(); ) {
	
	--i;

	if ((*i)->getId() == id) {
	    delete *i;
	    m_devices.erase(i);
	}
    }

    for (MappedInstrumentList::iterator i = m_instruments.end();
	 i != m_instruments.begin(); ) {

	--i;
	
	if ((*i)->getDevice() == id) {
	    delete *i;
	    m_instruments.erase(i);
	}
    }

    MappedEvent *mE =
	new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			0, 0);
    insertMappedEventForReturn(mE);
}

ClientPortPair
AlsaDriver::getPortByName(std::string name)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_name == name) {
	    return ClientPortPair(m_alsaPorts[i]->m_client,
				  m_alsaPorts[i]->m_port);
	}
    }
    return ClientPortPair(-1, -1);
}

std::string
AlsaDriver::getPortName(ClientPortPair port)
{
    for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {
	if (m_alsaPorts[i]->m_client == port.first &&
	    m_alsaPorts[i]->m_port == port.second) {
	    return m_alsaPorts[i]->m_name;
	}
    }
    return "";
}
    

unsigned int
AlsaDriver::getConnections(Device::DeviceType type,
			   MidiDevice::DeviceDirection direction)
{
    if (type != Device::Midi) return 0;

    int count = 0;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    ++count;
	}
    }
    
    return count;
}

QString
AlsaDriver::getConnection(Device::DeviceType type,
			  MidiDevice::DeviceDirection direction,
			  unsigned int connectionNo)
{
    if (type != Device::Midi) return "";
    
    AlsaPortList tempList;
    for (unsigned int j = 0; j < m_alsaPorts.size(); ++j) {
	if ((direction == MidiDevice::Play && m_alsaPorts[j]->isWriteable()) ||
	    (direction == MidiDevice::Record && m_alsaPorts[j]->isReadable())) {
	    tempList.push_back(m_alsaPorts[j]);
	}
    }
    
    if (connectionNo < tempList.size()) {
	return tempList[connectionNo]->m_name.c_str();
    }

    return "";
}

void
AlsaDriver::setConnection(DeviceId id, QString connection)
{
    AUDIT_START;
    ClientPortPair port(getPortByName(connection.data()));

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		m_devices[i]->setConnection(connection.data());

		MappedEvent *mE =
		    new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
				    0, 0);
		insertMappedEventForReturn(mE);

		break;
	    }
	}
    }

    AUDIT_UPDATE;
}

void
AlsaDriver::setPlausibleConnection(DeviceId id, QString idealConnection)
{
    AUDIT_START;
    ClientPortPair port(getPortByName(idealConnection.data()));

    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: connection like "
		 << idealConnection << " requested for device " << id << std::endl;

    if (port.first != -1 && port.second != -1) {

	m_devicePortMap[id] = port;

	for (unsigned int i = 0; i < m_devices.size(); ++i) {

	    if (m_devices[i]->getId() == id) {
		m_devices[i]->setConnection(idealConnection.data());
		break;
	    }
	}

	AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: exact match available"
		     << std::endl;
	AUDIT_UPDATE;
	return;
    }

    // What we want is a connection that:
    // 
    //  * is in the right "class" (the 0-63/64-127/128+ range of client id)
    //  * has at least some text in common
    //  * is not yet in use for any device.
    // 
    // To do this, we exploit our privileged position as part of AlsaDriver
    // and use our knowledge of how connection strings are made (see
    // AlsaDriver::generatePortList above) to pick out the relevant parts
    // of the requested string.
    
    int client = 0;
    int colon = idealConnection.find(":");
    if (colon >= 0) client = idealConnection.left(colon).toInt();

    int firstSpace = idealConnection.find(" ");
    int endOfText  = idealConnection.find(QRegExp("[^\\w ]"), firstSpace);

    QString text;
    if (endOfText < 2) {
	text = idealConnection.mid(firstSpace + 1);
    } else {
	text = idealConnection.mid(firstSpace + 1, endOfText - firstSpace - 2);
    }

    for (int testName = 1; testName >= 0; --testName) {

	for (unsigned int i = 0; i < m_alsaPorts.size(); ++i) {

	    AlsaPortDescription *port = m_alsaPorts[i];

	    if (client > 0 && (port->m_client / 64 != client / 64)) continue;
	    
	    if (testName && text != "" &&
		!QString(port->m_name.c_str()).contains(text)) continue;
	    
	    bool used = false;
	    for (DevicePortMap::iterator dpmi = m_devicePortMap.begin();
		 dpmi != m_devicePortMap.end(); ++dpmi) {
		if (dpmi->second.first  == port->m_client &&
		    dpmi->second.second == port->m_port) {
		    used = true;
		    break;
		}
	    }
	    if (used) continue;

	    // OK, this one will do

	    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: fuzzy match "
			 << port->m_name << " available" << std::endl;

	    m_devicePortMap[id] = ClientPortPair(port->m_client, port->m_port);

	    for (unsigned int i = 0; i < m_devices.size(); ++i) {

		if (m_devices[i]->getId() == id) {
		    m_devices[i]->setConnection(port->m_name);
		    
		    // in this case we don't request a device resync,
		    // because this is only invoked at times such as
		    // file load when the GUI is well aware that the
		    // whole situation is in upheaval anyway
		    
		    AUDIT_UPDATE;
		    return;
		}
	    }
	}
    }

    AUDIT_STREAM << "AlsaDriver::setPlausibleConnection: nothing suitable available"
		 << std::endl;
    AUDIT_UPDATE;
}

unsigned int
AlsaDriver::getTimers()
{
    return m_timers.size();
}

QString
AlsaDriver::getTimer(unsigned int n)
{
    return m_timers[n].name.c_str();
}

QString
AlsaDriver::getCurrentTimer()
{
    return m_currentTimer.c_str();
}

void
AlsaDriver::setCurrentTimer(QString timer)
{
    AUDIT_START;

    std::string name(timer.data());

    for (unsigned int i = 0; i < m_timers.size(); ++i) {
	if (m_timers[i].name == name) {

	    snd_seq_queue_timer_t *timer;
	    snd_timer_id_t *timerid;

	    snd_seq_queue_timer_alloca(&timer);
	    snd_seq_get_queue_timer(m_midiHandle, m_queue, timer);

	    snd_timer_id_alloca(&timerid);
	    snd_timer_id_set_class(timerid, m_timers[i].clas);
	    snd_timer_id_set_sclass(timerid, m_timers[i].sclas);
	    snd_timer_id_set_card(timerid, m_timers[i].card);
	    snd_timer_id_set_device(timerid, m_timers[i].device);
	    snd_timer_id_set_subdevice(timerid, m_timers[i].subdevice);

	    snd_seq_queue_timer_set_id(timer, timerid);
	    snd_seq_set_queue_timer(m_midiHandle, m_queue, timer);

	    AUDIT_STREAM << "    Current timer set to \"" << name << "\""
			 << std::endl;

	    break;
	}
    }

    AUDIT_UPDATE;
}

void
AlsaDriver::initialise()
{
    initialiseAudio();
    initialiseMidi();
}



// Set up queue, client and port
//
void
AlsaDriver::initialiseMidi()
{ 
    AUDIT_START;

    // Create a non-blocking handle.
    // ("hw" will possibly give in to other handles in future?)
    //
    if (snd_seq_open(&m_midiHandle,
                     "default",
                     SND_SEQ_OPEN_DUPLEX,
                     SND_SEQ_NONBLOCK) < 0)
    {
        AUDIT_STREAM << "AlsaDriver::initialiseMidi - "
                  << "couldn't open sequencer - " << snd_strerror(errno)
                  << std::endl;
        return;
    }

    generatePortList();
    generateInstruments();

    // Create a queue
    //
    if((m_queue = snd_seq_alloc_named_queue(m_midiHandle,
                                                "Rosegarden queue")) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't allocate queue"
                  << std::endl;
#endif
        return;
    }


    // Create a client
    //
    snd_seq_set_client_name(m_midiHandle, "Rosegarden sequencer");
    if ((m_client = snd_seq_client_id(m_midiHandle)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't create client"
                  << std::endl;
#endif
        return;
    }

    // Create a port
    //
    m_port = snd_seq_create_simple_port(m_midiHandle,
					"Rosegarden",
                                        SND_SEQ_PORT_CAP_WRITE |
                                        SND_SEQ_PORT_CAP_READ  |
					SND_SEQ_PORT_CAP_SUBS_WRITE |
                                        SND_SEQ_PORT_CAP_SUBS_READ,
                                        //SND_SEQ_PORT_CAP_NO_EXPORT,
                                        SND_SEQ_PORT_TYPE_APPLICATION);
    if (m_port < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - can't create port"
                  << std::endl;
#endif
        return;
    }

    ClientPortPair inputDevice = getFirstDestination(true); // duplex = true

    AUDIT_STREAM << "    Record port set to (" << inputDevice.first
              << ", "
              << inputDevice.second
              << ")" << std::endl << std::endl;

    AlsaPortList::iterator it;

    // Connect to all available output client/ports
    //
    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); it++)
    {
        if (snd_seq_connect_to(m_midiHandle,
                               m_port,
                               (*it)->m_client,
                               (*it)->m_port) < 0)
        {
            /*
            std::cerr << "AlsaDriver::initialiseMidi - "
                      << "can't subscribe output client/port ("
                      << (*it)->m_client << ", "
                      << (*it)->m_port << ")"
                      << std::endl;
                      */
        }
    }

    // Connect input port - enabling timestamping on the way through.
    // We have to fill out the subscription information as follows:
    //
    snd_seq_addr_t sender, dest;
    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    sender.client = inputDevice.first;
    sender.port = inputDevice.second;
    dest.client = m_client;
    dest.port = m_port;

    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    snd_seq_port_subscribe_set_queue(subs, m_queue);

    // enable time-stamp-update mode 
    //
    snd_seq_port_subscribe_set_time_update(subs, 1);

    // set so we get realtime timestamps
    //
    snd_seq_port_subscribe_set_time_real(subs, 1);

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't subscribe input client:port "
		  << int(sender.client) << ":" << int(sender.port)
                  << std::endl;
#endif
        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
    }
    else
        m_midiInputPortConnected = true;

    // Set the input queue size
    //
    if (snd_seq_set_client_pool_output(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_input(m_midiHandle, 2000) < 0 ||
        snd_seq_set_client_pool_output_room(m_midiHandle, 2000) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - "
                  << "can't modify pool parameters"
                  << std::endl;
#endif
        return;
    }

    getSystemInfo();

    // Modify status with MIDI success
    //
    m_driverStatus |= MIDI_OK;

    generateTimerList();
    
    if (m_timers.size() > 0) {
	setCurrentTimer(m_timers[0].name.c_str());
    }

    int result;

    // Start the timer
    if ((result = snd_seq_start_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::initialiseMidi - couldn't start queue - "
                  << snd_strerror(result)
                  << std::endl;
#endif
        exit(EXIT_FAILURE);
    }

    m_queueRunning = true;

    // process anything pending
    snd_seq_drain_output(m_midiHandle);

    AUDIT_STREAM << "AlsaDriver::initialiseMidi -  initialised MIDI subsystem"
              << std::endl << std::endl;
    AUDIT_UPDATE;
}

// We don't even attempt to use ALSA audio.  We just use JACK instead.
// See comment at the top of this file and jackProcess() for further
// information on how we use this.
//
void
AlsaDriver::initialiseAudio()
{
#ifdef HAVE_LIBJACK
    m_jackDriver = new JackDriver(this);

    if (m_jackDriver->isOK()) {
	m_driverStatus |= AUDIO_OK;
	m_audioPlayLatency = m_jackDriver->getAudioPlayLatency();
	m_audioRecordLatency = m_jackDriver->getAudioRecordLatency();
    }
#endif
}

void
AlsaDriver::initialisePlayback(const RealTime &position)
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver - initialisePlayback" << std::endl;
#endif

    // now that we restart the queue at each play, the origin is always zero
    m_alsaPlayStartTime = RealTime::zeroTime;

    m_playStartPosition = position;

    m_startPlayback = true;

    // If the clock is enabled then adjust for the MIDI Clock to 
    // synchronise the sequencer with the clock.
    //
    if (m_midiClockEnabled)
    {
        // Last clock sent should always be ahead of current
        // ALSA time - adjust for latency and find nearest
        // clock for start time.
        //
        RealTime alsaClockSent = m_midiClockSendTime;

        while (alsaClockSent > m_alsaPlayStartTime)
            alsaClockSent = alsaClockSent - m_midiClockInterval;

        /*
        std::cout << "START ADJUST FROM " << m_alsaPlayStartTime
             << " to " << alsaClockSent << endl;
             */

        m_alsaPlayStartTime = alsaClockSent;

        if (position == RealTime::zeroTime)
            sendSystemQueued(SND_SEQ_EVENT_START, "",
                             m_alsaPlayStartTime);
        else
            sendSystemQueued(SND_SEQ_EVENT_CONTINUE, "",
                             m_alsaPlayStartTime);

    }

    if (isMMCMaster())
    {
        sendMMC(127, MIDI_MMC_PLAY, true, "");
    }
}


void
AlsaDriver::stopPlayback()
{
#ifdef DEBUG_ALSA
    std::cerr << "AlsaDriver - stopPlayback" << std::endl;
#endif

    allNotesOff();
    m_playing = false;
    
    // reset the clock send time
    //
    m_midiClockSendTime = RealTime::zeroTime;

    // Flush the output and input queues
    //
    snd_seq_remove_events_t *info;
    snd_seq_remove_events_alloca(&info);
    snd_seq_remove_events_set_condition(info, SND_SEQ_REMOVE_INPUT|
                                              SND_SEQ_REMOVE_OUTPUT);
    snd_seq_remove_events(m_midiHandle, info);

    // Send system stop to duplex MIDI devices
    //
    if (m_midiClockEnabled) sendSystemDirect(SND_SEQ_EVENT_STOP, "");

    // send sounds-off to all client port pairs
    //
    AlsaPortList::iterator it;
    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        sendDeviceController(ClientPortPair((*it)->m_client,
                                            (*it)->m_port),
                             MIDI_CONTROLLER_SOUNDS_OFF,
                             0);
    }

#ifdef HAVE_LIBJACK
    // Close any recording file
    if (m_recordStatus == RECORD_AUDIO)
    {
	AudioFileId id;
	if (m_jackDriver->closeRecordFile(id)) {

	    // Create event to return to gui to say that we've completed
	    // an audio file and we can generate a preview for it now.
	    //
	    try
	    {
		MappedEvent *mE =
		    new MappedEvent(id,
				    MappedEvent::AudioGeneratePreview,
				    0);
		
		// send completion event
		insertMappedEventForReturn(mE);
	    }
	    catch(...) {;}
	}

        m_recordStatus = ASYNCHRONOUS_AUDIO;
    }
#endif

    // Change recorded state if any set
    //
    if (m_recordStatus == RECORD_MIDI)
        m_recordStatus = ASYNCHRONOUS_MIDI;

#ifdef HAVE_LIBJACK
    m_jackDriver->stop();

    m_jackDriver->getAudioQueueLocks();
#endif
    clearAudioPlayQueue();
#ifdef HAVE_LIBJACK
    m_jackDriver->releaseAudioQueueLocks();
#endif
}

void
AlsaDriver::resetPlayback(const RealTime &position)
{
    // Reset note offs to correct positions
    //
    RealTime modifyNoteOff = m_playStartPosition - m_alsaPlayStartTime;

    // set new
    m_playStartPosition = position;
    m_alsaPlayStartTime = getAlsaTime();

    // add
    modifyNoteOff = modifyNoteOff - m_playStartPosition + m_alsaPlayStartTime;

    // modify the note offs that exist as they're relative to the
    // playStartPosition terms.
    //
    for (NoteOffQueue::iterator i = m_noteOffQueue.begin();
                                i != m_noteOffQueue.end(); ++i)
    {

        // if we're fast forwarding then we bring the note off closer
        if (modifyNoteOff <= RealTime::zeroTime)
        {
            (*i)->setRealTime((*i)->getRealTime() + modifyNoteOff);
        }
        else // we're rewinding - kill the note immediately
        {
            (*i)->setRealTime(m_playStartPosition);
        }
    }


    // Clear down all playing audio files
    //

    std::list<PlayableAudioFile*>::iterator it;
    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::resetPlayback - stopping audio file"
                  << std::endl;
#endif // DEBUG_ALSA

        if ((*it)->getStatus() == PlayableAudioFile::PLAYING)
            (*it)->setStatus(PlayableAudioFile::DEFUNCT);
    }

#ifdef HAVE_LIBJACK
    m_jackDriver->getAudioQueueLocks();
#endif

    // clear out defunct
    clearDefunctFromAudioPlayQueue();

#ifdef HAVE_LIBJACK
    m_jackDriver->releaseAudioQueueLocks();
#endif
}


void
AlsaDriver::allNotesOff()
{
    snd_seq_event_t event;
    ClientPortPair outputDevice;
    RealTime offTime;

    // drop any pending notes
    snd_seq_drop_output_buffer(m_midiHandle);
    snd_seq_drop_output(m_midiHandle);

    // prepare the event
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_port);
    offTime = getAlsaTime();

    for (NoteOffQueue::iterator it = m_noteOffQueue.begin();
                                it != m_noteOffQueue.end(); ++it)
    {
        // Set destination according to instrument mapping to port
        //
        outputDevice = getPairForMappedInstrument((*it)->getInstrument());
	if (outputDevice.first < 0 || outputDevice.second < 0) continue;

        snd_seq_ev_set_dest(&event,
                            outputDevice.first,
                            outputDevice.second);


        /*
        snd_seq_real_time_t alsaOffTime = { offTime.sec,
                                            offTime.nsec };

        snd_seq_ev_schedule_real(&event, m_queue, 0, &alsaOffTime);
        */

        snd_seq_ev_set_noteoff(&event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               127);

        //snd_seq_event_output(m_midiHandle, &event);
        int error = snd_seq_event_output_direct(m_midiHandle, &event);

        if (error < 0)
        {
#ifdef DEBUG_ALSA
	    std::cerr << "AlsaDriver::allNotesOff - "
                      << "can't send event" << std::endl;
#endif
        }

        delete(*it);
    }
    
    m_noteOffQueue.erase(m_noteOffQueue.begin(), m_noteOffQueue.end());

    /*
    std::cerr << "AlsaDriver::allNotesOff - "
              << " queue size = " << m_noteOffQueue.size() << std::endl;
              */

    // flush
    snd_seq_drain_output(m_midiHandle);
}

void
AlsaDriver::processNotesOff(const RealTime &time)
{
    snd_seq_event_t event;

    ClientPortPair outputDevice;
    RealTime offTime;

    // prepare the event
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_port);

    NoteOffQueue::iterator it = m_noteOffQueue.begin();

    for (;it != m_noteOffQueue.end() && (*it)->getRealTime() <= time; ++it)
    {
        // Set destination according to instrument mapping to port
        //
        outputDevice = getPairForMappedInstrument((*it)->getInstrument());
	if (outputDevice.first < 0 || outputDevice.second < 0) continue;

        snd_seq_ev_set_dest(&event,
                            outputDevice.first,
                            outputDevice.second);

        offTime = (*it)->getRealTime();

        snd_seq_real_time_t alsaOffTime = { offTime.sec,
                                            offTime.nsec };

        snd_seq_ev_schedule_real(&event, m_queue, 0, &alsaOffTime);
        snd_seq_ev_set_noteoff(&event,
                               (*it)->getChannel(),
                               (*it)->getPitch(),
                               127);
        // send note off
        snd_seq_event_output(m_midiHandle, &event);
        delete(*it);
        m_noteOffQueue.erase(it);
    }

    // and flush them
    if (m_queueRunning) {
	snd_seq_drain_output(m_midiHandle);
    }

    /*
      std::cerr << "AlsaDriver::processNotesOff - "
      << " queue size = " << m_noteOffQueue.size() << std::endl;
    */
}

// Get the queue time and convert it to RealTime for the gui
// to use.
//
RealTime
AlsaDriver::getSequencerTime()
{
    RealTime t(0, 0);

    if (m_playing)
       t = getAlsaTime() + m_playStartPosition - m_alsaPlayStartTime;

//    std::cerr << "AlsaDriver::getSequencerTime: alsa time is "
//	      << getAlsaTime() << ", start time is " << m_alsaPlayStartTime << ", play start position is " << m_playStartPosition << endl;

    return t;
}

// Gets the time of the ALSA queue
//
RealTime
AlsaDriver::getAlsaTime()
{
    RealTime sequencerTime(0, 0);

    snd_seq_queue_status_t *status;
    snd_seq_queue_status_alloca(&status);

    if (snd_seq_get_queue_status(m_midiHandle, m_queue, status) < 0)
    {
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::getAlsaTime - can't get queue status"
                  << std::endl;
#endif
        return sequencerTime;
    }

    sequencerTime.sec = snd_seq_queue_status_get_real_time(status)->tv_sec;
    sequencerTime.nsec = snd_seq_queue_status_get_real_time(status)->tv_nsec;

//    std::cerr << "AlsaDriver::getAlsaTime: alsa time is " << sequencerTime << std::endl;

    return sequencerTime;
}


// Get all pending input events and turn them into a MappedComposition.
//
//
MappedComposition*
AlsaDriver::getMappedComposition()
{
    m_recordComposition.clear();
    if (!m_returnComposition.empty()) {
	for (MappedComposition::iterator i = m_returnComposition.begin();
	     i != m_returnComposition.end(); ++i) {
	    m_recordComposition.insert(new MappedEvent(**i));
	}
	m_returnComposition.clear();
    }

    if (m_recordStatus != RECORD_MIDI &&
        m_recordStatus != RECORD_AUDIO &&
        m_recordStatus != ASYNCHRONOUS_MIDI &&
        m_recordStatus != ASYNCHRONOUS_AUDIO)
    {
        return &m_recordComposition;
    }

    // If the input port hasn't connected we shouldn't poll it
    //
    if(m_midiInputPortConnected == false)
    {
        return &m_recordComposition;
    }

    RealTime eventTime(0, 0);

    snd_seq_event_t *event;

    while(snd_seq_event_input(m_midiHandle, &event) > 0)
    {
        unsigned int channel = (unsigned int)event->data.note.channel;
        unsigned int chanNoteKey = ( channel << 8 ) +
                                   (unsigned int) event->data.note.note;

        eventTime.sec = event->time.time.tv_sec;
        eventTime.nsec = event->time.time.tv_nsec;
        eventTime = eventTime - m_alsaRecordStartTime + m_playStartPosition;

        switch(event->type)
        {

            case SND_SEQ_EVENT_NOTE:
            case SND_SEQ_EVENT_NOTEON:
                if (event->data.note.velocity > 0)
                {
                    m_noteOnMap[chanNoteKey] = new MappedEvent();
                    m_noteOnMap[chanNoteKey]->setPitch(event->data.note.note);
                    m_noteOnMap[chanNoteKey]->
                        setVelocity(event->data.note.velocity);
                    m_noteOnMap[chanNoteKey]->setEventTime(eventTime);

                    // Negative duration - we need to hear the NOTE ON
                    // so we must insert it now with a negative duration
                    // and pick and mix against the following NOTE OFF
                    // when we create the recorded segment.
                    //
                    m_noteOnMap[chanNoteKey]->setDuration(RealTime(-1, 0));

                    // Create a copy of this when we insert the NOTE ON -
                    // keeping a copy alive on the m_noteOnMap.
                    //
                    // We shake out the two NOTE Ons after we've recorded
                    // them.
                    //
                    m_recordComposition.insert(
                            new MappedEvent(m_noteOnMap[chanNoteKey]));

                    break;
                }

            case SND_SEQ_EVENT_NOTEOFF:
                if (m_noteOnMap[chanNoteKey] != 0)
                {
                    // Set duration correctly on the NOTE OFF
                    //
                    RealTime duration = eventTime -
                             m_noteOnMap[chanNoteKey]->getEventTime();

                    if (duration < RealTime::zeroTime) break;

                    // Velocity 0 - NOTE OFF.  Set duration correctly
                    // for recovery later.
                    //
                    m_noteOnMap[chanNoteKey]->setVelocity(0);
                    m_noteOnMap[chanNoteKey]->setDuration(duration);

                    // force shut off of note
                    m_recordComposition.insert(m_noteOnMap[chanNoteKey]);

                    // reset the reference
                    //
                    m_noteOnMap[chanNoteKey] = 0;

                }
                break;

            case SND_SEQ_EVENT_KEYPRESS:
                {
                    // Fix for 632964 by Pedro Lopez-Cabanillas (20030523)
                    //
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiKeyPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.note.note);
                    mE->setData2(event->data.note.velocity);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CONTROLLER:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiController);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.param);
                    mE->setData2(event->data.control.value);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_PGMCHANGE:
                {
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiProgramChange);
                    mE->setEventTime(eventTime);
                    mE->setData1(event->data.control.value);
                    m_recordComposition.insert(mE);

                }
                break;

            case SND_SEQ_EVENT_PITCHBEND:
                {
                    // Fix for 711889 by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value + 8192;
                    int d1 = (s >> 7) & 0x7f; // data1 = MSB
                    int d2 = s & 0x7f; // data2 = LSB
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiPitchBend);
                    mE->setEventTime(eventTime);
                    mE->setData1(d1);
                    mE->setData2(d2);
                    m_recordComposition.insert(mE);
                }
                break;

            case SND_SEQ_EVENT_CHANPRESS:
                {
                    // Fixed by Pedro Lopez-Cabanillas (20030523)
                    //
                    int s = event->data.control.value & 0x7f;
                    MappedEvent *mE = new MappedEvent();
                    mE->setType(MappedEvent::MidiChannelPressure);
                    mE->setEventTime(eventTime);
                    mE->setData1(s);
                    m_recordComposition.insert(mE);
                }
               break;

            case SND_SEQ_EVENT_SYSEX:
               {
                   // Bundle up the data into a block on the MappedEvent
                   //
                   std::string data;
                   char *ptr = (char*)(event->data.ext.ptr);
                   for (unsigned int i = 0; i < event->data.ext.len; ++i)
                       data += *(ptr++);

#ifdef DEBUG_ALSA
                   if ((MidiByte)(data[0]) == MIDI_SYSEX_RT)
                   {
                       std::cerr << "REALTIME SYSEX" << endl;
                   }
#endif

                   MappedEvent *mE = new MappedEvent();
                   mE->setType(MappedEvent::MidiSystemExclusive);
                   // chop off SYX and EOX bytes from data block
                   // Fix for 674731 by Pedro Lopez-Cabanillas (20030601)
                   DataBlockRepository::setDataBlockForEvent(mE, data.substr(1, data.length() - 2));
                   mE->setEventTime(eventTime);
                   m_recordComposition.insert(mE);
               }
               break;


            case SND_SEQ_EVENT_SENSING: // MIDI device is still there
               break;

            case SND_SEQ_EVENT_CLOCK:
               /*
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got realtime MIDI clock" << std::endl;
                         */
               break;

            case SND_SEQ_EVENT_START:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "START" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_CONTINUE:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "CONTINUE" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_STOP:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "STOP" << std::endl;
#endif
               break;

            case SND_SEQ_EVENT_SONGPOS:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "SONG POSITION" << std::endl;
#endif
               break;

               // these cases are handled by checkForNewClients
               //
            case SND_SEQ_EVENT_PORT_SUBSCRIBED:
            case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
                   break;

            case SND_SEQ_EVENT_TICK:
            default:
#ifdef DEBUG_ALSA
               std::cerr << "AlsaDriver::getMappedComposition - "
                         << "got unhandled MIDI event type from ALSA sequencer"
                         << "(" << int(event->type) << ")" << std::endl;
#endif
               break;


        }
    }

    return &m_recordComposition;
}
    
void
AlsaDriver::processMidiOut(const MappedComposition &mC,
                           bool now)
{
    RealTime midiRelativeTime;
    RealTime midiRelativeStopTime;
    MappedInstrument *instrument;
    ClientPortPair outputDevice;
    MidiByte channel;
    snd_seq_event_t event;

    // These won't change in this slice
    //
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_port);

    if ((mC.begin() != mC.end()) && getSequencerDataBlock()) {
	getSequencerDataBlock()->setVisual(*mC.begin());
    }

    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
        if ((*i)->getType() >= MappedEvent::Audio)
            continue;

        midiRelativeTime = (*i)->getEventTime() - m_playStartPosition +
                           m_alsaPlayStartTime;

	RealTime alsaTimeNow = getAlsaTime();
	std::cerr << "processMidiOut: event is at " << midiRelativeTime << " (" << midiRelativeTime - alsaTimeNow << " ahead of queue time)" << std::endl;

        // Second and nanoseconds for ALSA
        //
        snd_seq_real_time_t time = { midiRelativeTime.sec,
                                     midiRelativeTime.nsec };

        // millisecond note duration
        //
        unsigned int eventDuration = (*i)->getDuration().sec * 1000
             + (*i)->getDuration().msec();

        // Set destination according to Instrument mapping
        //
        outputDevice = getPairForMappedInstrument((*i)->getInstrument());
	if (outputDevice.first < 0 && outputDevice.second < 0) continue;

	std::cout << "processMidiOut: instrument " << (*i)->getInstrument() << " -> output device " << outputDevice.first << ":" << outputDevice.second << std::endl;

        snd_seq_ev_set_dest(&event,
                            outputDevice.first,
                            outputDevice.second);

        snd_seq_ev_schedule_real(&event, m_queue, 0, &time);
        instrument = getMappedInstrument((*i)->getInstrument());

        // set the stop time for Note Off
        //
        midiRelativeStopTime = midiRelativeTime + (*i)->getDuration();
 
        if (instrument != 0)
            channel = instrument->getChannel();
        else
        {
#ifdef DEBUG_ALSA
            std::cerr << "processMidiOut() - couldn't get Instrument for Event"
                      << std::endl;
#endif
            channel = 0;
        }

        switch((*i)->getType())
        {
            case MappedEvent::MidiNoteOneShot:
                {
                    // Just an arbitrary duration for one-shot Notes
                    // for the moment until we work out the timing
                    // conversion.
                    //
                    int duration = 100;
                    snd_seq_ev_set_note(&event,
                                        channel,
                                        (*i)->getPitch(),
                                        (*i)->getVelocity(),
                                        duration);
                }
                break;

            case MappedEvent::MidiNote:
                // If we've got an "infinite" note then just noteon -
                // else send the duration.
                //
                if ((*i)->getDuration() == RealTime(-1, 0))
                {
                    snd_seq_ev_set_noteon(&event,
                                          channel,
                                          (*i)->getPitch(),
                                          (*i)->getVelocity());
                }
                else
                {
                    if ((*i)->getVelocity() == 0)
                    {
                        snd_seq_ev_set_noteoff(&event,
                                               channel,
                                               (*i)->getPitch(),
                                               (*i)->getVelocity());
                    }
                    else
                    {
                        snd_seq_ev_set_note(&event,
                                            channel,
                                            (*i)->getPitch(),
                                            (*i)->getVelocity(),
                                            eventDuration);

                        /*
                        std::cerr << "NOTE OUT : pitch = " 
                                  << int((*i)->getPitch())
                                  << ", velocity = "
                                  << int((*i)->getVelocity())
                                  << " at time " 
                                  << (*i)->getEventTime() << std::endl;
                                  */
                    }
                }
		if (getSequencerDataBlock()) {
		    Rosegarden::LevelInfo info;
		    info.level = (*i)->getVelocity();
		    info.levelRight = 0;
		    getSequencerDataBlock()->setInstrumentLevel
			((*i)->getInstrument(), info);
		}
                break;

            case MappedEvent::MidiProgramChange:
                snd_seq_ev_set_pgmchange(&event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiKeyPressure:
                snd_seq_ev_set_keypress(&event,
                                        channel,
                                        (*i)->getData1(),
                                        (*i)->getData2());
                break;

            case MappedEvent::MidiChannelPressure:
                snd_seq_ev_set_chanpress(&event,
                                         channel,
                                         (*i)->getData1());
                break;

            case MappedEvent::MidiPitchBend:
                {
                    int d1 = (int)((*i)->getData1());
                    int d2 = (int)((*i)->getData2());
                    int value = ((d1 << 7) | d2) - 8192;

                    // keep within -8192 to +8192
                    //
                    // if (value & 0x4000)
                    //    value -= 0x8000;

                    snd_seq_ev_set_pitchbend(&event,
                                             channel,
                                             value);
                }
                break;

            case MappedEvent::MidiSystemExclusive:
                {
                    // pack data between start and end blocks
                    //

                    char out[2];
                    sprintf(out, "%c", MIDI_SYSTEM_EXCLUSIVE);
                    std::string data = out;

                    data += DataBlockRepository::getDataBlockForEvent((*i));

                    sprintf(out, "%c", MIDI_END_OF_EXCLUSIVE);
                    data += out;

                    snd_seq_ev_set_sysex(&event,
                                         data.length(),
                                         (char*)(data.c_str()));
                }
                break;

            case MappedEvent::MidiController:
                snd_seq_ev_set_controller(&event,
                                          channel,
                                          (*i)->getData1(),
                                          (*i)->getData2());
                break;

            case MappedEvent::Audio:
            case MappedEvent::AudioCancel:
            case MappedEvent::AudioLevel:
            case MappedEvent::AudioStopped:
            case MappedEvent::SystemUpdateInstruments:
	case MappedEvent::SystemJackTransport: //???
            case MappedEvent::SystemMMCTransport:
            case MappedEvent::SystemMIDIClock:
                break;

            default:
            case MappedEvent::InvalidMappedEvent:
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processMidiOut - "
                          << "skipping unrecognised or invalid MappedEvent type"
                          << std::endl;
#endif
                continue;
        }

        int error = 0;
        if (now || m_playing == false)
        {
            RealTime nowTime = getAlsaTime();
            snd_seq_real_time_t outTime = { nowTime.sec,
                                            nowTime.nsec };
            snd_seq_ev_schedule_real(&event, m_queue, 0, &outTime);
            error = snd_seq_event_output_direct(m_midiHandle, &event);
        }
        else
            error = snd_seq_event_output(m_midiHandle, &event);

#ifdef DEBUG_ALSA
        if (error < 0)
        {
            std::cerr << "AlsaDriver::processMidiOut - "
                      << "failed to send ALSA event ("
                      << error << ")" <<  std::endl;
        }
#endif

        // Add note to note off stack
        //
        if ((*i)->getType() == MappedEvent::MidiNote)
        {
            bool extended = false;
            NoteOffQueue::iterator it;

            for (it = m_noteOffQueue.begin(); it != m_noteOffQueue.end(); ++it)
            {
                if ((*it)->getPitch() == (*i)->getPitch() &&
                    (*it)->getChannel() == channel &&
                    (*it)->getInstrument() == (*i)->getInstrument())
                {
                    (*it)->setRealTime(midiRelativeStopTime);
                    extended = true;
                }
            }

            if (!extended)
            {
                NoteOffEvent *noteOffEvent =
                    new NoteOffEvent(midiRelativeStopTime, // already calculated
                                     (*i)->getPitch(),
                                     channel,
                                     (*i)->getInstrument());
                m_noteOffQueue.insert(noteOffEvent);
            }
        }
    }

    if (m_queueRunning) {
	snd_seq_drain_output(m_midiHandle);
    }
}

void
AlsaDriver::startClocks()
{
    int result;

    std::cerr << "AlsaDriver::startClocks" << std::endl;

#ifdef HAVE_LIBJACK
    // Don't need any locks on this, except for those that the
    // driver methods take and hold for themselves

    if (!m_jackDriver->start()) { // need to wait for transport sync
	return;
    }
#endif

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
	//!!! bit excessive?
#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::startClocks - couldn't start queue - "
		  << snd_strerror(result)
		  << std::endl;
#endif
	exit(EXIT_FAILURE);
    }

    std::cerr << "AlsaDriver::startClocks: started clocks" << std::endl;

    m_queueRunning = true;

    // process pending MIDI events
    snd_seq_drain_output(m_midiHandle);
}

void
AlsaDriver::startClocksApproved()
{
    int result;

    std::cerr << "AlsaDriver::startClocks: startClocksApproved" << std::endl;

    // Restart the timer
    if ((result = snd_seq_continue_queue(m_midiHandle, m_queue, NULL)) < 0)
    {
	//!!! bit excessive?
#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::startClocks - couldn't start queue - "
		  << snd_strerror(result)
		  << std::endl;
#endif
	exit(EXIT_FAILURE);
    }

    m_queueRunning = true;

    //!!! too slow, guess we need the off-by-1 trick

    // process pending MIDI events
    snd_seq_drain_output(m_midiHandle);
}

void
AlsaDriver::stopClocks()
{
    int result;
    
    std::cerr << "AlsaDriver::stopClocks" << std::endl;

    if ((result = snd_seq_stop_queue(m_midiHandle, m_queue, NULL)) < 0) {
	//!!! bit excessive?
#ifdef DEBUG_ALSA
	std::cerr << "AlsaDriver::stopClocks - couldn't stop queue - "
		  << snd_strerror(result)
		  << std::endl;
#endif
	exit(EXIT_FAILURE);
    }

    m_queueRunning = false;

#ifdef HAVE_LIBJACK
    m_jackDriver->stop();
#endif
    
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    snd_seq_real_time_t z = { 0, 0 };
    snd_seq_ev_set_queue_pos_real(&event, m_queue, &z);
    snd_seq_control_queue(m_midiHandle, m_queue, SND_SEQ_EVENT_SETPOS_TIME,
			  0, &event);

    // process that
    snd_seq_drain_output(m_midiHandle);

    std::cerr << "AlsaDriver::stopClocks: ALSA time now is " << getAlsaTime() << std::endl;

    m_alsaPlayStartTime = RealTime::zeroTime;
}
	

// This is based on the aRts driver version.
//
void
AlsaDriver::processEventsOut(const MappedComposition &mC,
                             bool now)
{
    if (m_startPlayback)
    {
        m_startPlayback = false;
	// This only records whether we're playing in principle,
	// not whether the clocks are actually ticking.  Contrariwise,
	// areClocksRunning tells us whether the clocks are ticking
	// but not whether we're actually playing (the clocks go even
	// when we're not).  Check both if you want to know whether
	// we're really rolling.
        m_playing = true;
    }

    AudioFile *audioFile = 0;
    bool haveNewAudio = false;

    // insert audio events if we find them
    for (MappedComposition::iterator i = mC.begin(); i != mC.end(); ++i)
    {
#ifdef HAVE_LIBJACK

        // Play an audio file
        //
        if ((*i)->getType() == MappedEvent::Audio)
        {
            // Check for existence of file - if the sequencer has died
            // and been restarted then we're not always loaded up with
            // the audio file references we should have.  In the future
            // we could make this just get the gui to reload our files
            // when (or before) this fails.
            //
            audioFile = getAudioFile((*i)->getAudioID());

            if (audioFile)
            { 
                RealTime adjustedEventTime =
                    (*i)->getEventTime();

                /*
                    adjustedEventTime == RealTime::zeroTime &&
                    getSequencerTime() > RealTime(1, 0))
                    */

                // If we're playing, the event time is two minutes or
                // more in the past we've sent an async audio event -
                // if we're playing we have to reset this time to
                // some point in our playing future - otherwise we
                // just reset to zero.
                //
                if (adjustedEventTime <= RealTime(-120, 0))
                {
		    adjustedEventTime = getSequencerTime();
//!!!er...
//		    adjustedEventTime = getAlsaTime() + RealTime(0, 500000000);
			//!!! need to cause a fillBuffers call to happen on the mixer
//!!!                    else
//!!!                        adjustedEventTime = RealTime::zeroTime;
                }

                // Create this event in this thread and push it onto audio queue.
                AudioFile *aF = getAudioFile((*i)->getAudioID());

		MappedAudioFader *fader =
		    dynamic_cast<MappedAudioFader*>
		    (getMappedStudio()->getAudioFader((*i)->getInstrument()));

		if (!fader) {
		    std::cerr << "WARNING: AlsaDriver::processEventsOut: no fader for audio instrument " << (*i)->getInstrument() << std::endl;
		    continue;
		}

		unsigned int channels = fader->getPropertyList(
		    MappedAudioFader::Channels)[0].toInt();

		RealTime bufferLength = getAudioReadBufferLength();
		int bufferFrames = RealTime::realTime2Frame
		    (bufferLength, m_jackDriver->getSampleRate());
		if (bufferFrames % m_jackDriver->getBufferSize()) {
		    bufferFrames /= m_jackDriver->getBufferSize();
		    bufferFrames ++;
		    bufferFrames *= m_jackDriver->getBufferSize();
		}

/*
		std::cout << "Creating playable audio file: id " << aF->getId() << ", event time " << adjustedEventTime << ", time now " << getAlsaTime() << ", start marker " << (*i)->getAudioStartMarker() << ", duration " << (*i)->getDuration() << ", instrument " << (*i)->getInstrument() << " channels " << channels <<  std::endl;

		std::cout << "Read buffer length is " << bufferLength << " (" << RealTime::realTime2Frame(bufferLength, m_jackDriver->getSampleRate()) << " frames)" << std::endl;
*/
                PlayableAudioFile *audioFile =
                    new PlayableAudioFile((*i)->getInstrument(),
                                          aF,
                                          adjustedEventTime,
                                          (*i)->getAudioStartMarker(),
                                          (*i)->getDuration(),
					  bufferFrames,
					  getSmallFileSize() * 1024,
					  channels,
					  m_jackDriver->getSampleRate());

                // segment runtime id
                audioFile->setRuntimeSegmentId((*i)->getRuntimeSegmentId());

                // Queue the audio file on the driver stack
                //
                queueAudio(audioFile);

		haveNewAudio = true;
            }
            else
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "can't find audio file reference" 
                          << std::endl;

                std::cerr << "AlsaDriver::processEventsOut - "
                          << "try reloading the current Rosegarden file"
                          << std::endl;
#else 
                ;
#endif
            }
        }

        // Cancel a playing audio file preview (this is predicated on
        // runtime segment ID and optionally start time)
        //
        if ((*i)->getType() == MappedEvent::AudioCancel)
        {
            cancelAudioFile(*i);
        }

#endif // HAVE_LIBJACK

        if ((*i)->getType() == MappedEvent::SystemMIDIClock)
        {
            if ((*i)->getData1())
            {
                m_midiClockEnabled = true;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK ENABLED"
                          << std::endl;
#endif
            }
            else
            {
                m_midiClockEnabled = false;
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "Rosegarden MIDI CLOCK DISABLED"
                          << std::endl;
#endif
            }
        }

#ifdef HAVE_LIBJACK

        // Set the JACK transport 
        if ((*i)->getType() == MappedEvent::SystemJackTransport)
        {
            bool enabled = false;
            bool master = false;

            switch ((int)(*i)->getData1())
            {
                case 2:
                    master = true;
		    enabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to follow JACK transport and request JACK timebase master role (not yet implemented)"
                              << std::endl;
#endif
                    break;

                case 1:
		    enabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to follow JACK transport"
                              << std::endl;
#endif
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden to ignore JACK transport"
                              << std::endl;
#endif
                    break;
            }

	    m_jackDriver->setTransportEnabled(enabled);
	    m_jackDriver->setTransportMaster(master);
        }
#endif // HAVE_LIBJACK


        if ((*i)->getType() == MappedEvent::SystemMMCTransport)
        {
            m_mmcMaster = false;
            m_mmcEnabled = false;

            switch ((int)(*i)->getData1())
            {
                case 2:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC MASTER"
                              << std::endl;
#endif
                    m_mmcMaster = true;
                    m_mmcEnabled = true;
                    break;

                case 1:
                    m_mmcEnabled = true;
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden is MMC SLAVE"
                              << std::endl;
#endif
                    break;

                case 0:
                default:
#ifdef DEBUG_ALSA
                    std::cerr << "AlsaDriver::processEventsOut - "
                              << "Rosegarden MMC Transport DISABLED"
                              << std::endl;
#endif
                    break;
            }
        }

        if ((*i)->getType() == MappedEvent::SystemRecordDevice)
        {
            DeviceId recordDevice =
               (DeviceId)((*i)->getData1());

            // Unset connections
            //
            unsetRecordDevices();

            // Special case to set for all record ports
            //
            if (recordDevice == Device::ALL_DEVICES)
            {
                /* set all record devices */
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::processEventsOut - "
                          << "set all record devices - not implemented"
                          << std::endl;
#endif

                /*
		MappedDeviceList::iterator it = m_devices.begin();
                std::vector<int> ports;
                std::vector<int>::iterator pIt;

                for (; it != m_devices.end(); ++it)
                {
                    std::cout << "DEVICE = " << (*it)->getName() << " - DIR = "
                         << (*it)->getDirection() << endl;
                    // ignore ports we can't connect to
                    if ((*it)->getDirection() == MidiDevice::WriteOnly) continue;

                    std::cout << "PORTS = " << ports.size() << endl;
                    ports = (*it)->getPorts();
                    for (pIt = ports.begin(); pIt != ports.end(); ++pIt)
                    {
                        setRecordDevice((*it)->getClient(), *pIt);
                    }
                }
                */
            }
            else
            {
                // Otherwise just for the one device and port
                //
                setRecordDevice(recordDevice);
            }
        }

        if ((*i)->getType() == MappedEvent::SystemAudioInputs)
        {
#ifndef HAVE_LIBJACK
#ifdef DEBUG_ALSA
            std::cerr << "AlsaDriver::processEventsOut - "
                      << "MappedEvent::SystemAudioInputs - no audio subsystem"
                      << std::endl;
#endif
#endif
        }
    }

    // Process Midi and Audio
    //
    processMidiOut(mC, now);

#ifdef HAVE_LIBJACK
    if (haveNewAudio) {
	if (now) {
	    m_jackDriver->prebufferAudio();
	}
	if (m_queueRunning) {
	    m_jackDriver->kickAudio();
	}
    }
    m_jackDriver->updateAudioLevels();
#endif
}

bool
AlsaDriver::record(RecordStatus recordStatus)
{
    if (recordStatus == RECORD_MIDI)
    {
        // start recording
        m_recordStatus = RECORD_MIDI;
//!!!        m_alsaRecordStartTime = getAlsaTime();
	m_alsaRecordStartTime = RealTime::zeroTime;
    }
    else if (recordStatus == RECORD_AUDIO)
    {
#ifdef HAVE_LIBJACK
        if (m_jackDriver->createRecordFile(m_recordingFilename))
        {
            m_recordStatus = RECORD_AUDIO;
        }
        else
        {
            m_recordStatus = ASYNCHRONOUS_MIDI;
	    return false;
        }
#else
#ifdef DEBUG_ALSA
        std::cerr << "AlsaDriver::record - can't record audio without JACK"
                  << std::endl;
#endif
#endif

    }
    else
    if (recordStatus == ASYNCHRONOUS_MIDI)
    {
        m_recordStatus = ASYNCHRONOUS_MIDI;
    }
    else if (recordStatus == ASYNCHRONOUS_AUDIO)
    {
        m_recordStatus = ASYNCHRONOUS_AUDIO;
    }
#ifdef DEBUG_ALSA
    else
    {
        std::cerr << "ArtsDriver::record - unsupported recording mode"
                  << std::endl;
    }
#endif

    return true;
}

ClientPortPair
AlsaDriver::getFirstDestination(bool duplex)
{
    ClientPortPair destPair(-1, -1);
    AlsaPortList::iterator it;

    for (it = m_alsaPorts.begin(); it != m_alsaPorts.end(); ++it)
    {
        destPair.first = (*it)->m_client;
        destPair.second = (*it)->m_port;

        // If duplex port is required then choose first one
        //
        if (duplex)
        {
            if ((*it)->m_direction == Duplex)
                return destPair;
        }
        else
        {
            // If duplex port isn't required then choose first
            // specifically non-duplex port (should be a synth)
            //
            if ((*it)->m_direction != Duplex)
                return destPair;
        }
    }

    return destPair;
}


// Sort through the ALSA client/port pairs for the range that
// matches the one we're querying.  If none matches then send
// back -1 for each.
//
ClientPortPair
AlsaDriver::getPairForMappedInstrument(InstrumentId id)
{
    MappedInstrument *instrument = getMappedInstrument(id);
    if (instrument)
    {
	DeviceId device = instrument->getDevice();
	DevicePortMap::iterator i = m_devicePortMap.find(device);
	if (i != m_devicePortMap.end())
	{
	    return i->second;
	}
    }
#ifdef DEBUG_ALSA
    /*
    else
    {
	cerr << "WARNING: AlsaDriver::getPairForMappedInstrument: couldn't find instrument for id " << id << ", falling through" << endl;
    }
    */
#endif

    return ClientPortPair(-1, -1);
}

// Send a direct controller to the specified port/client
//
void
AlsaDriver::sendDeviceController(const ClientPortPair &device,
                                 MidiByte controller,
                                 MidiByte value)
{
    snd_seq_event_t event;

    // These won't change in this slice
    //
    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, m_port);

    snd_seq_ev_set_dest(&event,
                        device.first,
                        device.second);

    for (int i = 0; i < 16; i++)
    {
        snd_seq_ev_set_controller(&event,
                                  i,
                                  controller,
                                  value);
        snd_seq_event_output_direct(m_midiHandle, &event);
    }

    // we probably don't need this:
    snd_seq_drain_output(m_midiHandle);
}

// We only process note offs in this section
//
void
AlsaDriver::processPending()
{
    if (!m_playing)
        processAudioQueue(true);
    else
        processNotesOff(getAlsaTime());
}

void
AlsaDriver::insertMappedEventForReturn(MappedEvent *mE)
{
    // Insert the event ready for return at the next opportunity
    //
    m_returnComposition.insert(mE);
}


// At some point make this check for just different numbers of clients
//
bool
AlsaDriver::checkForNewClients()
{
    AUDIT_START;
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int  client;
    unsigned int currentPortCount = 0,
                     oldPortCount = m_alsaPorts.size();

    unsigned int writeCap = SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_WRITE;
    unsigned int readCap = SND_SEQ_PORT_CAP_SUBS_READ|SND_SEQ_PORT_CAP_READ;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_client_info_set_client(cinfo, -1);

    // Count current ports
    //
    while (snd_seq_query_next_client(m_midiHandle, cinfo) >= 0)
    {
        client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        // Ignore ourselves and the system client
        //
        if (client == m_client || client == 0) continue;

        while (snd_seq_query_next_port(m_midiHandle, pinfo) >= 0)
        {
            if (((snd_seq_port_info_get_capability(pinfo) & writeCap)
                        == writeCap) ||
                ((snd_seq_port_info_get_capability(pinfo) & readCap)
                        == readCap))
                currentPortCount++;
        }
    }

    if (oldPortCount == currentPortCount) return false;

    AUDIT_STREAM << "AlsaDriver: number of ports changed ("
	      << currentPortCount << " now, " << oldPortCount << " before)"
	      << std::endl;
    
    AlsaPortList newPorts;
    generatePortList(&newPorts);

    // If any devices have connections that no longer exist,
    // clear those connections and stick them in the suspended
    // port map in case they come back online later.

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	
	ClientPortPair pair(m_devicePortMap[(*i)->getId()]);
	
	bool found = false;
	for (AlsaPortList::iterator j = m_alsaPorts.begin();
	     j != m_alsaPorts.end(); ++j) {
	    if ((*j)->m_client == pair.first &&
		(*j)->m_port == pair.second) {
		found = true;
		break;
	    }
	}
	
	if (!found) {
	    m_suspendedPortMap[pair] = (*i)->getId();
	    m_devicePortMap[(*i)->getId()] = ClientPortPair(-1, -1);
	    (*i)->setConnection("");
	}
    }
    
    // If we've increased the number of connections, we need
    // to assign the new connections to existing devices that
    // have none, where possible, and create new devices for
    // any left over.
    
    if (newPorts.size() > 0) {

	AUDIT_STREAM << "New ports:" << std::endl;

	for (AlsaPortList::iterator i = newPorts.begin();
	     i != newPorts.end(); ++i) {

	    AUDIT_STREAM << (*i)->m_name << std::endl;
	    std::string portName = (*i)->m_name;
	    ClientPortPair portPair = ClientPortPair((*i)->m_client,
						     (*i)->m_port);

	    if (m_suspendedPortMap.find(portPair) != m_suspendedPortMap.end()) {

		DeviceId id = m_suspendedPortMap[portPair];

		AUDIT_STREAM << "(Reusing suspended device " << id << ")" << std::endl;

		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getId() == id) (*j)->setConnection(portName);
		}

		m_suspendedPortMap.erase(m_suspendedPortMap.find(portPair));
		m_devicePortMap[id] = portPair;
		continue;
	    }
	    
	    bool needPlayDevice = true, needRecordDevice = true;

	    if ((*i)->isReadable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Record) {
			AUDIT_STREAM << "(Reusing record device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			(*j)->setConnection(portName);
			needRecordDevice = false;
			break;
		    }
		}
	    } else {
		needRecordDevice = false;
	    }

	    if ((*i)->isWriteable()) {
		for (MappedDeviceList::iterator j = m_devices.begin();
		     j != m_devices.end(); ++j) {
		    if ((*j)->getType() == Device::Midi &&
			(*j)->getConnection() == "" &&
			(*j)->getDirection() == MidiDevice::Play) {
			AUDIT_STREAM << "(Reusing play device " << (*j)->getId()
				  << ")" << std::endl;
			m_devicePortMap[(*j)->getId()] = portPair;
			(*j)->setConnection(portName);
			needPlayDevice = false;
			break;
		    }
		}
	    } else {
		needPlayDevice = false;
	    }

	    if (needRecordDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Record);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create record device" << std::endl;
#else 
                    ;
#endif
		} else {
		    AUDIT_STREAM << "(Created new record device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		}
	    }

	    if (needPlayDevice) {
		MappedDevice *device = createMidiDevice(*i, MidiDevice::Play);
		if (!device) {
#ifdef DEBUG_ALSA
		    std::cerr << "WARNING: Failed to create play device" << std::endl;
#else
                    ;
#endif
		} else {
		    AUDIT_STREAM << "(Created new play device " << device->getId() << ")" << std::endl;
		    addInstrumentsForDevice(device);
		    m_devices.push_back(device);
		}
	    }
	}
    }
    
    MappedEvent *mE =
	new MappedEvent(0, MappedEvent::SystemUpdateInstruments,
			0, 0);
    // send completion event
    insertMappedEventForReturn(mE);

    AUDIT_UPDATE;
    return true;
}


// From a DeviceId get a client/port pair for connecting as the
// MIDI record device.
//
void
AlsaDriver::setRecordDevice(DeviceId id)
{
    AUDIT_START;

    // Locate a suitable port
    //
    if (m_devicePortMap.find(id) == m_devicePortMap.end()) {
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "couldn't match device id (" << id << ") to ALSA port"
		     << std::endl;
        return;
    }

    ClientPortPair pair = m_devicePortMap[id];

    snd_seq_addr_t sender, dest;
    sender.client = pair.first;
    sender.port = pair.second;

    for (MappedDeviceList::iterator i = m_devices.begin();
	 i != m_devices.end(); ++i) {
	if ((*i)->getId() == id) {
	    if ((*i)->getDirection() != MidiDevice::Record) {
		AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
			     << "attempting to set play device (" << id 
			     << ") to record device" << std::endl;
		return;
	    }
	    break;
	}
    }

    snd_seq_port_subscribe_t *subs;
    snd_seq_port_subscribe_alloca(&subs);

    dest.client = m_client;
    dest.port = m_port;

    // Set destinations and senders
    //
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_port_subscribe_set_queue(subs, m_queue);

    // enable time-stamp-update mode 
    //
    snd_seq_port_subscribe_set_time_update(subs, 1);

    // set so we get realtime timestamps
    //
    snd_seq_port_subscribe_set_time_real(subs, 1);

    if (snd_seq_subscribe_port(m_midiHandle, subs) < 0)
    {
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "can't subscribe input client:port"
		     << int(sender.client) << ":" << int(sender.port)
		     << std::endl;

        // Not the end of the world if this fails but we
        // have to flag it internally.
        //
        m_midiInputPortConnected = false;
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "failed to subscribe device " 
		     << id << " as record port" << std::endl;
    }
    else
    {
        m_midiInputPortConnected = true;
        AUDIT_STREAM << "AlsaDriver::setRecordDevice - "
		     << "successfully subscribed device "
		     << id << " as record port" << std::endl;
    }

}

// Clear any record device connections
//
void
AlsaDriver::unsetRecordDevices()
{
    snd_seq_addr_t dest;
    dest.client = m_client;
    dest.port = m_port;

    snd_seq_query_subscribe_t *qSubs;
    snd_seq_addr_t tmp_addr;
    snd_seq_query_subscribe_alloca(&qSubs);

    tmp_addr.client = m_client;
    tmp_addr.port = m_port;

    // Unsubsribe any existing connections
    //
    snd_seq_query_subscribe_set_type(qSubs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(qSubs, 0);
    snd_seq_query_subscribe_set_root(qSubs, &tmp_addr);

    while (snd_seq_query_port_subscribers(m_midiHandle, qSubs) >= 0)
    {
        tmp_addr = *snd_seq_query_subscribe_get_addr(qSubs);

        snd_seq_port_subscribe_t *dSubs;
        snd_seq_port_subscribe_alloca(&dSubs);

        snd_seq_addr_t dSender;
        dSender.client = tmp_addr.client;
        dSender.port = tmp_addr.port;

        snd_seq_port_subscribe_set_sender(dSubs, &dSender);
        snd_seq_port_subscribe_set_dest(dSubs, &dest);

        int error = snd_seq_unsubscribe_port(m_midiHandle, dSubs);

#ifdef DEBUG_ALSA
        if (error < 0)
        {
            std::cerr << "AlsaDriver::unsetRecordDevices - "
                      << "can't unsubscribe record port" << std::endl;

        }
#endif

        snd_seq_query_subscribe_set_index(qSubs,
                snd_seq_query_subscribe_get_index(qSubs) + 1);
    }
}

void
AlsaDriver::sendMMC(MidiByte deviceArg,
                    MidiByte instruction,
                    bool isCommand,
                    const std::string &data)
{
    MappedComposition mC;
    MappedEvent *mE;

    DeviceId deviceId = Device::NO_DEVICE;
    
    for (MappedInstrumentList::iterator i = m_instruments.begin();
	 i != m_instruments.end(); ++i) {

	if ((*i)->getDevice() == deviceId) continue;
	deviceId = (*i)->getDevice();

	if ((*i)->getType() != Instrument::Midi) continue;

	// Create a plain SysEx
	//
	mE = new MappedEvent((*i)->getId(),
			     MappedEvent::MidiSystemExclusive);

	// Make it a RealTime SysEx
	mE->addDataByte(MIDI_SYSEX_RT);
	
	// Add the destination
	mE->addDataByte(deviceArg);
	
	// Add the command type
	if (isCommand)
	    mE->addDataByte(MIDI_SYSEX_RT_COMMAND);
	else
	    mE->addDataByte(MIDI_SYSEX_RT_RESPONSE);
	
	// Add the command
	mE->addDataByte(instruction);
	
	// Add any data
	mE->addDataString(data);
    
	mC.insert(mE);

#ifdef NOT_DEFINED
    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            try
            {
                // Create a plain SysEx
                //
		mE = new MappedEvent(0, //!!! should be iterating over devices?  (*it)->m_startId,
				     MappedEvent::MidiSystemExclusive);

		// Make it a RealTime SysEx
		mE->addDataByte(MIDI_SYSEX_RT);
		
		// Add the destination
		mE->addDataByte(deviceId);
		
		// Add the command type
		if (isCommand)
		    mE->addDataByte(MIDI_SYSEX_RT_COMMAND);
		else
		    mE->addDataByte(MIDI_SYSEX_RT_RESPONSE);
		
		// Add the command
		mE->addDataByte(instruction);
		
		// Add any data
		mE->addDataString(data);
            }
            catch(...)
            {
#ifdef DEBUG_ALSA
                std::cerr << "AlsaDriver::sendMMC - "
                          << "couldn't create MMC message" << std::endl;
#endif
                return;
            }

            mC.insert(mE);
        }
#endif
    }

    processMidiOut(mC, true);
}

// Send a system real-time message
//
void
AlsaDriver::sendSystemDirect(MidiByte command, const std::string &args)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            snd_seq_event_t event;
            memset(&event, 0, sizeof(&event));

            // Set destination and sender
            dest.client = (*it)->m_client;
            dest.port = (*it)->m_port;
        
            event.dest = dest;
            event.source = sender;
            event.queue = SND_SEQ_QUEUE_DIRECT;

            // set the command
            event.type = command;

            // set args if we have them
            switch(args.length())
            {
                case 1:
                    event.data.control.value = args[0];
                    event.data.control.value = args[0];
                    break;

                case 2:
                    event.data.control.param = args[0];
                    event.data.control.value = args[0];
                    break;

                default: // do nothing
                    break;
            }

            int error = snd_seq_event_output_direct(m_midiHandle, &event);

#ifdef DEBUG_ALSA
            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemDirect - "
                          << "can't send event (" << int(command) << ")"
                          << std::endl;
            }
#endif
        }
    }

    // we probably don't need this here
    snd_seq_drain_output(m_midiHandle);
}


void
AlsaDriver::sendSystemQueued(MidiByte command,
                             const std::string &args,
                             const RealTime &time)
{
    snd_seq_addr_t sender, dest;
    sender.client = m_client;
    sender.port = m_port;
    snd_seq_real_time_t sendTime = { time.sec, time.nsec };

    AlsaPortList::iterator it = m_alsaPorts.begin();
    for (; it != m_alsaPorts.end(); ++it)
    {
        // One message per writeable port
        //
        if ((*it)->m_port == 0 && (*it)->isWriteable())
        {
            snd_seq_event_t event;
            memset(&event, 0, sizeof(&event));

            // Set destination and sender
            dest.client = (*it)->m_client;
            dest.port = (*it)->m_port;
        
            event.dest = dest;
            event.source = sender;

            // Schedule the command
            //
            event.type = command;

            // useful for debugging
            //snd_seq_ev_set_note(&event, 0, 64, 127, 100);

            snd_seq_ev_schedule_real(&event, m_queue, 0, &sendTime);

            // set args if we have them
            switch(args.length())
            {
                case 1:
                    event.data.control.value = args[0];
                    event.data.control.value = args[0];
                    break;

                case 2:
                    event.data.control.param = args[0];
                    event.data.control.value = args[0];
                    break;

                default: // do nothing
                    break;
            }

            int error = snd_seq_event_output(m_midiHandle, &event);

#ifdef DEBUG_ALSA
            if (error < 0)
            {
                std::cerr << "AlsaDriver::sendSystemQueued - "
                          << "can't send event (" << int(command) << ")"
                          << " - error = (" << error << ")"
                          << std::endl;
            }
#endif
        }
    }

    if (m_queueRunning) {
	snd_seq_drain_output(m_midiHandle);
    }
}

// Send the MIDI clock signal
//
void
AlsaDriver::sendMidiClock()
{
    // Don't send the clock if it's disabled
    //
    if (!m_midiClockEnabled) return;

    // Get the number of ticks in (say) two seconds
    //
    unsigned int numTicks =
        (unsigned int)(RealTime(10, 0) / m_midiClockInterval);

    // First time through set the clock send time - this will also
    // ensure we send the first batch of clock events
    //
    if (m_midiClockSendTime == RealTime::zeroTime)
    {
        m_midiClockSendTime = getAlsaTime();
        /*
        std::cout << "INITIAL ALSA TIME = " << m_midiClockSendTime << endl;
        */
    }

    // If we're within a tenth of a second of running out of clock
    // then send a new batch of clock signals.
    //
    if ((getAlsaTime()) >
        (m_midiClockSendTime - RealTime(0, 100000000)))
    {
        /*
        std::cout << "SENDING " << numTicks
             << " CLOCK TICKS @ " << m_midiClockSendTime << endl;
             */

        for (unsigned int i = 0; i < numTicks; i++)
        {
            sendSystemQueued(SND_SEQ_EVENT_CLOCK, "", m_midiClockSendTime);

            // increment send time
            m_midiClockSendTime = m_midiClockSendTime + m_midiClockInterval;
        }
    }

    // If we're playing then send the song position pointer.
    //
    if (m_playing)
    {
        // Get time from current alsa time to start of alsa timing -
        // add the initial starting point and divide by the total
        // single clock length.  Divide this result by 6 for the SPP
        // position.
        //
        long spp =
          long(((getAlsaTime() - m_alsaPlayStartTime + m_playStartPosition) /
                         m_midiClockInterval) / 6.0);

        // Only send if it's changed
        //
        if (m_midiSongPositionPointer != spp)
        {
            m_midiSongPositionPointer = spp;
            MidiByte lsb = spp & 0x7f;
            MidiByte msb = (spp << 8) & 0x7f;
            std::string args;
            args += lsb;
            args += msb;

            sendSystemDirect(SND_SEQ_EVENT_SONGPOS, args);
        }

    }


}

QString
AlsaDriver::getStatusLog()
{
    return QString::fromUtf8(_audit.c_str());
}


void
AlsaDriver::sleep(const RealTime &rt)
{
    int npfd = snd_seq_poll_descriptors_count(m_midiHandle, POLLIN);
    struct pollfd *pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
    snd_seq_poll_descriptors(m_midiHandle, pfd, npfd, POLLIN);
    poll(pfd, npfd, rt.sec * 1000 + rt.msec());
}

}


#endif // HAVE_ALSA
