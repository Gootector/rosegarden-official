
// -*- c-basic-offset: 4 -*-

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

#include "JackDriver.h"
#include "AlsaDriver.h"
#include "MappedStudio.h"
#include "AudioProcess.h"
#include "Profiler.h"
#include "AudioLevel.h"

#ifdef HAVE_ALSA
#ifdef HAVE_LIBJACK

//!!! need to get this merged with alsa driver's audit trail

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

JackDriver::JackDriver(AlsaDriver *alsaDriver) :
    m_client(0),
    m_outputPortLeft(0),
    m_outputPortRight(0),
    m_bufferSize(0),
    m_sampleRate(0),
    m_tempOutBuffer(0),
    m_jackTransportEnabled(false),
    m_jackTransportMaster(false),
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_waiting(false),
    m_waitingState(JackTransportStopped),
    m_waitingToken(0),
    m_mixer(0),
    m_fileReader(0),
    m_fileWriter(0),
    m_alsaDriver(alsaDriver),
    m_ok(false)
{
    assert(sizeof(sample_t) == sizeof(float));
    initialise();
}

JackDriver::~JackDriver()
{
    std::cout << "JackDriver::~JackDriver" << std::endl;

    if (m_client)
    {
#ifdef DEBUG_ALSA
        std::cerr << "JackDriver::shutdown - closing JACK client"
                  << std::endl;
#endif

        if (jack_deactivate(m_client))
        {
#ifdef DEBUG_ALSA
            std::cerr << "JackDriver::shutdown - deactivation failed"
                      << std::endl;
#endif
        }

        for (unsigned int i = 0; i < m_inputPorts.size(); ++i)
        {
            if (jack_port_unregister(m_client, m_inputPorts[i]))
            {
#ifdef DEBUG_ALSA
                std::cerr << "JackDriver::shutdown - "
                          << "can't unregister input port " << i + 1
                          << std::endl;
#endif
            }
        }

        if (jack_port_unregister(m_client, m_outputPortLeft))
        {
#ifdef DEBUG_ALSA
            std::cerr << "JackDriver::shutdown - "
                      << "can't unregister output port left" << std::endl;
#endif
        }

        if (jack_port_unregister(m_client, m_outputPortRight))
        {
#ifdef DEBUG_ALSA
            std::cerr << "JackDriver::shutdown - "
                      << "can't unregister output port right" << std::endl;
#endif
        }
                
        jack_client_close(m_client);
        m_client = 0;
    }

    std::cout << "JackDriver::~JackDriver: deleting disk and mix managers" << std::endl;
    delete m_fileReader;
    delete m_fileWriter;
    delete m_mixer;

    std::cout << "JackDriver::~JackDriver exiting" << std::endl;
}

void
JackDriver::initialise()
{
    AUDIT_START;
    AUDIT_STREAM << std::endl;

    std::string jackClientName = "rosegarden";

    // attempt connection to JACK server
    //
    if ((m_client = jack_client_new(jackClientName.c_str())) == 0)
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "JACK server not running"
                     << std::endl;
        return;
    }

    // set callbacks
    //
    jack_set_process_callback(m_client, jackProcessStatic, this);
    jack_set_buffer_size_callback(m_client, jackBufferSize, this);
    jack_set_sample_rate_callback(m_client, jackSampleRate, this);
    jack_on_shutdown(m_client, jackShutdown, this);
    jack_set_graph_order_callback(m_client, jackGraphOrder, this);
    //jack_set_xrun_callback(m_client, jackXRun, this);
    jack_set_sync_callback(m_client, jackSyncCallback, this);

    // get and report the sample rate and buffer size
    //
    m_sampleRate = jack_get_sample_rate(m_client);
    m_bufferSize = jack_get_buffer_size(m_client);

    AUDIT_STREAM << "JackDriver::initialiseAudio - JACK sample rate = "
                 << m_sampleRate << "Hz, buffer size = " << m_bufferSize
		 << std::endl;

    // Get the initial buffer size before we activate the client
    //

    // create processing buffer(s)
    //
    m_tempOutBuffer = new sample_t[m_bufferSize];

    // Create output ports
    //
    m_outputPortLeft = jack_port_register(m_client,
					  "master_left",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput,
					  0);

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "added output port 1 (left)" << std::endl;

    m_outputPortRight = jack_port_register(m_client,
					   "master_right",
					   JACK_DEFAULT_AUDIO_TYPE,
					   JackPortIsOutput,
					   0);

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "added output port 2 (right)" << std::endl;

    InstrumentId instrumentBase;
    int instruments;
    m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instruments);

    m_outputFaderPorts = new jack_port_t*[instruments * 2];

/*!!!
    for (int i = 0; i < instruments; ++i) {
	char namebuffer[20];
	snprintf(namebuffer, 19, "submaster_%d_left", i);
	m_outputFaderPorts[i * 2] = jack_port_register(m_client,
						       namebuffer,
						       JACK_DEFAULT_AUDIO_TYPE,
						       JackPortIsOutput,
						       0);
	snprintf(namebuffer, 19, "submaster_%d_right", i);
	m_outputFaderPorts[i*2+1] = jack_port_register(m_client,
						       namebuffer,
						       JACK_DEFAULT_AUDIO_TYPE,
						       JackPortIsOutput,
						       0);
    }
*/
    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "creating disk thread" << std::endl;

    m_fileReader = new AudioFileReader(m_alsaDriver, m_sampleRate);
    m_fileWriter = new AudioFileWriter(m_alsaDriver, m_sampleRate);
    m_mixer = new AudioMixer(m_alsaDriver, m_fileReader, m_sampleRate,
			     m_bufferSize < 1024 ? 1024 : m_bufferSize);

    // Create and connect (default) two audio inputs - activating as
    // we go
    //
    createInputPorts(2, false);

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "created input ports" << std::endl;

    std::string playback_1, playback_2;

    // Assign ports directly if they were specified
    //
/*!!!
    if (m_args.size() >= 2)
    {
        playback_1 = std::string(m_args[0].data());
        playback_2 = std::string(m_args[1].data());
    }
    else // match from JACK
    {*/
        const char **ports =
            jack_get_ports(m_client, NULL, NULL,
                           JackPortIsPhysical | JackPortIsInput);

        if (ports)
        {
            if (ports[0]) playback_1 = std::string(ports[0]);
            if (ports[1]) playback_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++);
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "found " << i << " JACK physical outputs"
                         << std::endl;
        }
        else
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "no JACK physical outputs found"
                         << std::endl;
        free(ports);
//!!!    }

    if (playback_1 != "")
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_outputPortLeft)
                     << "\" to \"" << playback_1.c_str() << "\""
                     << std::endl;

        // connect our client up to the ALSA ports - first left output
        //
        if (jack_connect(m_client, jack_port_name(m_outputPortLeft),
                         playback_1.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
            return;
        }
    }

    if (playback_2 != "")
    {
        AUDIT_STREAM << "JackDriver::initialiseAudio - "
                     << "connecting from "
                     << "\"" << jack_port_name(m_outputPortLeft)
                     << "\" to \"" << playback_2.c_str() << "\""
                     << std::endl;

        if (jack_connect(m_client, jack_port_name(m_outputPortRight),
                         playback_2.c_str()))
        {
            AUDIT_STREAM << "JackDriver::initialiseAudio - "
                         << "cannot connect to JACK output port" << std::endl;
            return;
        }
    }

    // Get the latencies from JACK and set them as RealTime
    //
    jack_nframes_t outputLatency =
        jack_port_get_total_latency(m_client, m_outputPortLeft);

    jack_nframes_t inputLatency = 
        jack_port_get_total_latency(m_client, m_inputPorts[0]);

    double latency = double(outputLatency) / double(m_sampleRate);

    // Set the audio latencies ready for collection by the GUI
    //
    m_audioPlayLatency = RealTime(int(latency),
				  int((latency - int(latency)) * 1e9));

    latency = double(inputLatency) / double(m_sampleRate);

    m_audioRecordLatency = RealTime(int(latency),
				    int((latency - int(latency)) * 1e9));

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "JACK playback latency " << m_audioPlayLatency << std::endl;

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "JACK record latency " << m_audioRecordLatency << std::endl;

    AUDIT_STREAM << "JackDriver::initialiseAudio - "
                 << "initialised JACK audio subsystem"
                 << std::endl;

    m_ok = true;

    AUDIT_UPDATE;
}

void
JackDriver::createInputPorts(unsigned int totalPorts, bool deactivate)
{
    if (!m_client) return;

    AUDIT_START;

    // Out port connections if we need them
    //
    const char **outLeftPort = 0, **outRightPort = 0;

    // deactivate client
    //
    if (deactivate)
    {
        // store output connections for reconnect after port mods
        //
        outLeftPort = jack_port_get_connections(m_outputPortLeft);
        outRightPort = jack_port_get_connections(m_outputPortRight);

        if (jack_deactivate(m_client))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "client deactivation failed" << std::endl;
        }
    }

    // Unregister any we already have connected
    //
    for (unsigned int i = 0; i < m_inputPorts.size(); ++i)
        jack_port_unregister(m_client, m_inputPorts[i]);
    m_inputPorts.clear();

    jack_port_t *inputPort;
    char portName[10];
    for (unsigned int i = 0; i < totalPorts; ++i)
    {
        sprintf(portName, "in_%d", i + 1);
        inputPort = jack_port_register(m_client,
                                       portName,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsInput|JackPortIsTerminal,
                                       0);
         m_inputPorts.push_back(inputPort);


         AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                      << "adding input port " << i + 1 << std::endl;
    }
    
    // reactivate client
    //
    if (jack_activate(m_client))
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "client deactivation failed" << std::endl;
    }

    std::string capture_1, capture_2;

    // Assign port directly if they were specified
    //
/*!!!
    if (m_args.size() >= 4)
    {
        capture_1 = std::string(m_args[2].data());
        capture_2 = std::string(m_args[3].data());
    }
    else // match from JACK
    {
*/
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "getting ports" << std::endl;
        const char **ports =
            jack_get_ports(m_client, NULL, NULL,
                           JackPortIsPhysical|JackPortIsOutput);

        if (ports)
        {
            if (ports[0]) capture_1 = std::string(ports[0]);
            if (ports[1]) capture_2 = std::string(ports[1]);

            // count ports
            unsigned int i = 0;
            for (i = 0; ports[i]; i++);
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "found " << i << " JACK physical inputs"
                         << std::endl;
        }
        else
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "no JACK physical inputs found"
                         << std::endl;
        free(ports);
//!!!    }

    if (capture_1 != "")
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_1.c_str() << "\" to \""
                     << jack_port_name(m_inputPorts[0]) << "\""
                     << std::endl;

        // now input
        if (jack_connect(m_client, capture_1.c_str(),
                         jack_port_name(m_inputPorts[0])))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

    if (capture_2 != "")
    {
        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "connecting from "
                     << "\"" << capture_2.c_str() << "\" to \""
                     << jack_port_name(m_inputPorts[1]) << "\""
                     << std::endl;

        if (jack_connect(m_client, capture_2.c_str(),
                         jack_port_name(m_inputPorts[1])))
        {
            AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                         << "cannot connect to JACK input port" << std::endl;
        }
    }

    // Reconnect out ports
    if (deactivate)
    {

        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (left)"
                     << std::endl;

        if (outLeftPort && outLeftPort[0])
        {
            if (jack_connect(m_client,
                             jack_port_name(m_outputPortLeft),
                             outLeftPort[0]))
            {
                AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (left)"
                             << std::endl;
            }
        }

        AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                     << "reconnecting JACK output port (right)"
                     << std::endl;

        if (outRightPort && outRightPort[0])
        {
            if (jack_connect(m_client, 
                             jack_port_name(m_outputPortRight),
                             outRightPort[0]))
            {
                AUDIT_STREAM << "JackDriver::createJackInputPorts - "
                             << "cannot reconnect JACK output port (right)"
                             << std::endl;
            }
        }
    }

    AUDIT_UPDATE;
}


// The "process" callback is where we do all the work of turning a sample
// file into a sound.  We de-interleave a WAV and send it out as needs be.
//
// We perform basic mixing at this level - adding the samples together
// using the temp buffers and then read out to the audio buffer at the
// end of the mix stage.  More details supplied within.
//
//
int
JackDriver::jackProcessStatic(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);
    if (inst) return inst->jackProcess(nframes);
    else return 0;
}

int
JackDriver::jackProcess(jack_nframes_t nframes)
{
    sample_t *leftBuffer = static_cast<sample_t*>
	(jack_port_get_buffer(getOutputPortLeft(), nframes));
    sample_t *rightBuffer = static_cast<sample_t*>
	(jack_port_get_buffer(getOutputPortRight(), nframes));

    memset(leftBuffer, 0, sizeof(sample_t) * nframes);
    memset(rightBuffer, 0, sizeof(sample_t) * nframes);

    SequencerDataBlock *sdb = m_alsaDriver->getSequencerDataBlock();

    if (!m_mixer) {
	return 0;
    }

    bool wroteSomething = false;
    
//    Rosegarden::Profiler profiler("JackProcess, clocks running");

    jack_position_t position;
    if (m_jackTransportEnabled) {
	jack_transport_state_t state = jack_transport_query(m_client, &position);

//	std::cout << "process: jack state is " << state << std::endl;
	if (state == JackTransportStopped) {
	    if (m_alsaDriver->isPlaying() &&
		m_alsaDriver->areClocksRunning()) {
		ExternalTransport *transport = 
		    m_alsaDriver->getExternalTransportControl();
		if (transport) {
		    m_waitingToken =
			transport->transportJump
			(ExternalTransport::TransportStopAtTime,
			 RealTime::frame2RealTime(position.frame,
						  position.frame_rate));
		}
	    }
	    return 0;
	} else if (state == JackTransportStarting) {
	    return 0;
	} else if (state == JackTransportRolling) {
	    if (m_waiting) {
		m_alsaDriver->startClocksApproved();
		m_waiting = false;
	    }
	}
    } else if (!m_alsaDriver->areClocksRunning()) {
	return 0;
    }

    InstrumentId instrumentBase;
    int instruments;
    m_alsaDriver->getAudioInstrumentNumbers(instrumentBase, instruments);

    if (m_alsaDriver->getRecordStatus() == RECORD_AUDIO &&
	m_alsaDriver->areClocksRunning()) {

	//!!! This absolutely should not be done here, as it involves
	//taking out a pthread lock -- either do it in the record
	//thread only, or cache it
	MappedAudioFader *fader =
	    m_alsaDriver->getMappedStudio()->
	    getAudioFader(m_alsaDriver->getAudioMonitoringInstrument());

	int channels = 1;
	int connection = 0;

	if (fader) {
	    float f = 2;
	    (void)fader->getProperty(MappedAudioFader::Channels, f);
	    int channels = (int)f;
	}

	// Get input buffer
	//
	sample_t *inputBufferLeft = 0, *inputBufferRight = 0;

	inputBufferLeft = static_cast<sample_t*>
	    (jack_port_get_buffer(getInputPort(connection * channels), nframes));
	
	if (channels == 2) {
	    inputBufferRight = static_cast<sample_t*>
		(jack_port_get_buffer(getInputPort(connection * channels + 1), nframes));
	}
	
	//!!! want an actual instrument id
	//!!! want file writer to apply volume from fader?

	m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
			    inputBufferLeft, 0, nframes);

	if (channels == 2) {
	    m_fileWriter->write(m_alsaDriver->getAudioMonitoringInstrument(),
				inputBufferLeft, 1, nframes);
	}

	wroteSomething = true;

	sample_t totalLeft = 0.0, totalRight = 0.0;

	for (size_t i = 0; i < nframes; ++i) {
	    leftBuffer[i] = inputBufferLeft[i];
	    totalLeft += leftBuffer[i];
	}

	if (channels == 2) {
	    for (size_t i = 0; i < nframes; ++i) {
		rightBuffer[i] = inputBufferRight[i];
		totalRight += rightBuffer[i];
	    }
	}

	if (sdb) {
	    Rosegarden::TrackLevelInfo info;
	    info.level = AudioLevel::multiplier_to_fader
		(totalLeft / nframes, 127, AudioLevel::LongFader);
	    info.levelRight = AudioLevel::multiplier_to_fader
		(totalRight / nframes, 127, AudioLevel::LongFader);
	    sdb->setRecordLevel(info);
	}
    }

    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instruments;
	 ++id) {

	bool dormant = m_mixer->isInstrumentDormant(id);
	float peakLeft = 0.0;
	float peakRight = 0.0;

	RingBuffer<AudioMixer::sample_t> *rb = m_mixer->getRingBuffer(id, 0);
	
	if (rb) {
	    if (dormant) rb->skip(m_bufferSize);
	    else {
		size_t actual = rb->read(m_tempOutBuffer, m_bufferSize);
		if (actual < m_bufferSize) {
		    std::cerr << "WARNING: buffer underrun in left  mix ringbuffer " << id << " (wanted " << m_bufferSize << ", got " << actual << ")" << std::endl;
		}

		for (size_t i = 0; i < m_bufferSize; ++i) {
		    float sample = m_tempOutBuffer[i];
		    leftBuffer[i] += sample;
		    if (sample > peakLeft) peakLeft = sample;
		}
	    }
	}

	rb = m_mixer->getRingBuffer(id, 1);

	if (rb) {
	    if (dormant) rb->skip(m_bufferSize);
	    else {
		size_t actual = rb->read(m_tempOutBuffer, m_bufferSize);
		if (actual < m_bufferSize) {
		    std::cerr << "WARNING: buffer underrun in right mix ringbuffer " << id << " (wanted " << m_bufferSize << ", got " << actual << ")" << std::endl;
		}

		for (size_t i = 0; i < m_bufferSize; ++i) {
		    float sample = m_tempOutBuffer[i];
		    rightBuffer[i] += sample;
		    if (sample > peakRight) peakRight = sample;
		}
	    }

	} else if (!dormant) {
	    // copy left to right
	    for (size_t i = 0; i < m_bufferSize; ++i) {
		rightBuffer[i] += m_tempOutBuffer[i];
	    }
	}	    

	if (sdb) {
	    Rosegarden::TrackLevelInfo info;
	    info.level = AudioLevel::multiplier_to_fader
		(peakLeft, 127, AudioLevel::LongFader);
	    info.levelRight = AudioLevel::multiplier_to_fader
		(peakRight, 127, AudioLevel::LongFader);
	    sdb->setTrackLevelsForInstrument(id, info);
	}
    }

    if (m_alsaDriver->isPlaying()) m_mixer->signal();
    if (wroteSomething) m_fileWriter->signal(); //???

    return 0;
}

int
JackDriver::jackSyncCallback(jack_transport_state_t state,
			     jack_position_t *position,
			     void *arg)
{
    JackDriver *inst = (JackDriver *)arg;
    if (!inst) return true; // or rather, return "huh?"

    if (!inst->m_jackTransportEnabled) return true; // ignore

    ExternalTransport *transport =
	inst->m_alsaDriver->getExternalTransportControl();
    if (!transport) return true;

    std::cout << "jackSyncCallback(" << state << ", " << position->frame << "), m_waiting " << inst->m_waiting << ", playing " << inst->m_alsaDriver->isPlaying() << std::endl;

    ExternalTransport::TransportRequest request =
	ExternalTransport::TransportNoChange;

    if (inst->m_alsaDriver->isPlaying()) {

	if (state == JackTransportStarting) {
	    request = ExternalTransport::TransportJumpToTime;
	} else if (state == JackTransportStopped) {
	    request = ExternalTransport::TransportStop;
	}

    } else {

	if (state == JackTransportStarting) {
	    request = ExternalTransport::TransportStartAtTime;
	} else if (state == JackTransportStopped) {
	    request = ExternalTransport::TransportNoChange;
	}
    }

    if (!inst->m_waiting || inst->m_waitingState != state) {

	if (request == ExternalTransport::TransportJumpToTime ||
	    request == ExternalTransport::TransportStartAtTime) {

	    RealTime rt = RealTime::frame2RealTime(position->frame,
						   position->frame_rate);

	    std::cout << "Requesting jump to " << rt << std::endl;
	    
	    inst->m_waitingToken = transport->transportJump(request, rt);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;

	} else if (request == ExternalTransport::TransportStop) {

	    std::cout << "Requesting state change to " << request << std::endl;
	    
	    inst->m_waitingToken = transport->transportChange(request);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;

	} else if (request == ExternalTransport::TransportNoChange) {

	    std::cout << "Requesting no state change!" << std::endl;

	    inst->m_waitingToken = transport->transportChange(request);

	    std::cout << "My token is " << inst->m_waitingToken << std::endl;
	}

	inst->m_waiting = true;
	inst->m_waitingState = state;
	return 0;

    } else {

	if (transport->isTransportSyncComplete(inst->m_waitingToken)) {
	    std::cout << "Sync complete" << std::endl;
	    return 1;
	} else {
	    std::cout << "Sync not complete" << std::endl;
	    return 0;
	}
    }
}

bool
JackDriver::start()
{
    prebufferAudio();

    // m_waiting is true if we are waiting for the JACK transport
    // to finish a change of state.

    if (m_jackTransportEnabled) {

	// Where did this request come from?  Are we just responding
	// to an external sync?

	ExternalTransport *transport =
	m_alsaDriver->getExternalTransportControl();
	
	if (transport) {
	    if (transport->isTransportSyncComplete(m_waitingToken)) {

		// Nope, this came from Rosegarden

		std::cout << "start: asking for start, setting waiting" << std::endl;
		m_waiting = true;
		m_waitingState = JackTransportStarting;
		jack_transport_locate(m_client,
				      RealTime::realTime2Frame
				      (m_alsaDriver->getSequencerTime(),
				       m_sampleRate));
		jack_transport_start(m_client);
	    } else {
		std::cout << "start: waiting already" << std::endl;
	    }
	}
	return false;
    }
    
    std::cout << "start: not on transport" << std::endl;
    return true;
}

void
JackDriver::stop()
{
    flushAudio();

    if (m_jackTransportEnabled) {

	// Where did this request come from?  Is this a result of our
	// sync to a transport that has in fact already stopped?

	ExternalTransport *transport =
	    m_alsaDriver->getExternalTransportControl();

	if (transport) {
	    if (transport->isTransportSyncComplete(m_waitingToken)) {
		// No, we have no outstanding external requests; this
		// must have genuinely been requested from within
		// Rosegarden, so:
		jack_transport_stop(m_client);
	    } else {
		// Nothing to do
	    }
	}
    }

    if (m_mixer) m_mixer->resetAllPlugins();
}


// Pick up any change of buffer size
//
int
JackDriver::jackBufferSize(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackBufferSize - buffer size changed to "
              << nframes << std::endl;
#endif 

    inst->m_bufferSize = nframes;

    // Recreate our temporary mix buffers to the new size
    //
    //!!! need buffer size change callbacks on plugins (so long as they
    // have internal buffers) and the mix manager, with locks acquired
    // appropriately

    delete [] inst->m_tempOutBuffer;
    inst->m_tempOutBuffer = new sample_t[inst->m_bufferSize];

    return 0;
}

// Sample rate change
//
int
JackDriver::jackSampleRate(jack_nframes_t nframes, void *arg)
{
    JackDriver *inst = static_cast<JackDriver*>(arg);

#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackSampleRate - sample rate changed to "
               << nframes << std::endl;
#endif
    inst->m_sampleRate = nframes;

    return 0;
}

void
JackDriver::jackShutdown(void * /*arg*/)
{
#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackShutdown() - callback received - doing nothing yet" << std::endl;
#endif

    /*
    JackDriver *inst = static_cast<JackDriver*>(arg);
    if (inst) inst->shutdown();
    delete inst;
    */
}

int
JackDriver::jackGraphOrder(void *)
{
    //std::cerr << "JackDriver::jackGraphOrder" << std::endl;
    return 0;
}

int
JackDriver::jackXRun(void *)
{
#ifdef DEBUG_ALSA
    std::cerr << "JackDriver::jackXRun" << std::endl;
#endif
    return 0;
}

void
JackDriver::prebufferAudio()
{
    //!!! Hmm.  This will need to be cleverer to cope with JACK transport
    // sync, as the next slice start is not so predictable (anyone can set
    // the transport to any frame count).  We could just query it?

    if (m_mixer) {
	m_mixer->fillBuffers
	    (getNextSliceStart(m_alsaDriver->getSequencerTime()));
    }
}

void
JackDriver::flushAudio()
{
    if (m_mixer) {
	m_mixer->emptyBuffers();
    }
}
 
void
JackDriver::kickAudio()
{
    if (m_fileReader) m_fileReader->kick();
    if (m_mixer) m_mixer->kick();
    if (m_fileWriter) m_fileWriter->kick();
}


RealTime
JackDriver::getNextSliceStart(const RealTime &now) const
{
    jack_nframes_t frame = RealTime::realTime2Frame(now, m_sampleRate);
    jack_nframes_t rounded = frame;
    rounded /= m_bufferSize;
    rounded *= m_bufferSize;

    if (rounded == frame)
	return RealTime::frame2RealTime(rounded, m_sampleRate);
    else
	return RealTime::frame2RealTime(rounded + m_bufferSize, m_sampleRate);
}


int
JackDriver::getAudioQueueLocks()
{
    // We have to lock the mix manager first, because the mix manager
    // can try to lock the disk manager from within its own locked
    // section -- so if we locked the disk manager first we would risk
    // deadlock when trying to acquire the mix manager lock

    int rv = 0;
    if (m_mixer) {
	std::cerr << "JackDriver::getAudioQueueLocks: trying to lock mix manager" << std::endl;
	rv = m_mixer->getLock();
	if (rv) return rv;
    }
    if (m_fileReader) {
	std::cerr << "ok, now trying for disk reader" << std::endl;
	rv = m_fileReader->getLock();
	if (rv) return rv;
    }
    if (m_fileWriter) {
	std::cerr << "ok, now trying for disk writer" << std::endl;
	rv = m_fileWriter->getLock();
    }
    std::cerr << "ok" << std::endl;
    return rv;
}

int
JackDriver::tryAudioQueueLocks()
{
    int rv = 0;
    if (m_mixer) {
	rv = m_mixer->tryLock();
	if (rv) return rv;
    }
    if (m_fileReader) {
	rv = m_fileReader->tryLock();
	if (rv) {
	    if (m_mixer) {
		m_mixer->releaseLock();
	    }
	}
    }
    if (m_fileWriter) {
	rv = m_fileWriter->tryLock();
	if (rv) {
	    if (m_fileReader) {
		m_fileReader->releaseLock();
	    }
	    if (m_mixer) {
		m_mixer->releaseLock();
	    }
	}
    }
    return rv;
}

int
JackDriver::releaseAudioQueueLocks()
{
    int rv = 0;
    std::cerr << "JackDriver::releaseAudioQueueLocks" << std::endl;
    if (m_fileWriter) rv = m_fileWriter->releaseLock();
    if (m_fileReader) rv = m_fileReader->releaseLock();
    if (m_mixer) rv = m_mixer->releaseLock();
    return rv;
}

//!!! should really eliminate these and call on mixer directly from
// driver I s'pose

void
JackDriver::setPluginInstance(InstrumentId id, unsigned long pluginId,
			      int position)
{
    if (m_mixer) m_mixer->setPlugin(id, position, pluginId);
}

void
JackDriver::removePluginInstance(InstrumentId id, int position)
{
    if (m_mixer) m_mixer->removePlugin(id, position);
}

void
JackDriver::removePluginInstances()
{
    if (m_mixer) m_mixer->removeAllPlugins();
}

void
JackDriver::setPluginInstancePortValue(InstrumentId id, int position,
				       unsigned long portNumber,
				       float value)
{
    if (m_mixer) m_mixer->setPluginPortValue(id, position, portNumber, value);
}

void
JackDriver::setPluginInstanceBypass(InstrumentId id, int position, bool value)
{
    if (m_mixer) m_mixer->setPluginBypass(id, position, value);
}

//!!! and these
bool
JackDriver::createRecordFile(const std::string &filename)
{
    //!!! want an actual instrument
    if (m_fileWriter) {
	return m_fileWriter->createRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), filename);
    } else return false;
}

bool
JackDriver::closeRecordFile(AudioFileId &returnedId)
{
    if (m_fileWriter) {
	return m_fileWriter->closeRecordFile(m_alsaDriver->getAudioMonitoringInstrument(), returnedId);
    } else return false;
}

}

#endif // HAVE_LIBJACK
#endif // HAVE_ALSA
