// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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
#include <set>

#include "config.h"

#ifdef HAVE_ALSA

#include <alsa/asoundlib.h> // ALSA

#include "SoundDriver.h"
#include "Instrument.h"
#include "Device.h"

// jack includes
//
#ifdef HAVE_LIBJACK
#include <jack/jack.h>

// convenience typedef
//
typedef jack_default_audio_sample_t sample_t;

#endif


#ifdef HAVE_LADSPA

#include <ladspa.h>

// LADSPA plugin instance
//
class LADSPAPluginInstance
{
public:
    LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                         unsigned long ladspaId,
                         int position,
                         const LADSPA_Descriptor* descriptor);

    ~LADSPAPluginInstance();

    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }
    unsigned long getLADSPAId() const { return m_ladspaId; }
    int getPosition() const { return m_position; }

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts(LADSPA_Data *dataIn1,
                      LADSPA_Data *dataIn2,
                      LADSPA_Data *dataOut1,
                      LADSPA_Data *dataOut2);

    // Set control ports
    //
    void setPortValue(unsigned long portNumber, LADSPA_Data value);

    // Plugin control
    //
    void instantiate(unsigned long sampleRate);
    void activate();
    void run(unsigned long sampleCount);
    void deactivate();
    void cleanup();

    // Audio channels out to mix
    unsigned int getAudioChannelsOut() const { return m_audioPortsOut.size(); }

    // During operation we need to know which plugins have
    // already been processed as part of the playable audio
    // file loop and which should still be run() outside 
    // this loop.
    //
    bool hasBeenProcessed() const { return m_processed; }
    void setProcessed(bool value) { m_processed = value; }

    // Do we want to bypass this plugin?
    //
    bool isBypassed() const { return m_bypassed; }
    void setBypassed(bool value) { m_bypassed = value; }

    // Order by instrument and then position
    //
    struct PluginCmp
    {
        bool operator()(const LADSPAPluginInstance *p1,
                        const LADSPAPluginInstance *p2)
        {
            if (p1->getInstrument() != p2->getInstrument())
                return p1->getInstrument() < p2->getInstrument();
            else
                return p1->getPosition() < p2->getPosition();
        }
    };

protected:
    
    Rosegarden::InstrumentId  m_instrument;
    unsigned long             m_ladspaId;
    int                       m_position;
    LADSPA_Handle             m_instanceHandle;
    const LADSPA_Descriptor  *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPorts;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    bool                      m_processed;
    bool                      m_bypassed;

};

typedef std::vector<LADSPAPluginInstance*> PluginInstances;
typedef std::vector<LADSPAPluginInstance*>::iterator PluginIterator;
typedef std::multiset<LADSPAPluginInstance*,
                      LADSPAPluginInstance::PluginCmp> OrderedPluginList;
typedef std::multiset<LADSPAPluginInstance*,
                      LADSPAPluginInstance::PluginCmp>::iterator OrderedPluginIterator;

#endif // HAVE_LADSPA



// Specialisation of SoundDriver to support ALSA (http://www.alsa-project.org)
// Currently supports version 0.9beta12
//
//


#ifndef _ALSADRIVER_H_
#define _ALSADRIVER_H_

namespace Rosegarden
{


// An AlsaPort represents one or more (usually 16) MappedInstruments.
//
//

class AlsaPort
{
public:
    AlsaPort(InstrumentId startId,
             InstrumentId endId,
             const std::string &name,
             int client,
             int port,
             bool duplex):
        m_startId(startId),
        m_endId(endId),
        m_name(name),
        m_client(client),
        m_port(port),
        m_duplex(duplex),
        m_type(0) {;}

    ~AlsaPort() {;}

    InstrumentId m_startId;
    InstrumentId m_endId;
    std::string  m_name;
    int          m_client;
    int          m_port;
    bool         m_duplex;   // is an input port as well?
    unsigned int m_type;     // MIDI, synth or what?
};


typedef std::pair<int, int> ClientPortPair;

class AlsaDriver : public SoundDriver
{
public:
    AlsaDriver(MappedStudio *studio);
    virtual ~AlsaDriver();

    virtual void initialiseMidi();
    virtual void initialiseAudio();
    virtual void initialisePlayback(const RealTime &position);
    virtual void stopPlayback();
    virtual void resetPlayback(const RealTime &position,
                               const RealTime &latency);
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);

    virtual RealTime getSequencerTime();

    virtual MappedComposition*
        getMappedComposition(const RealTime &playLatency);
    
    virtual void record(const RecordStatus& recordStatus);

    void addInstrumentsForPort(Instrument::InstrumentType type,
                               const std::string &name, 
                               int client,
                               int port,
                               bool duplex);

    virtual void processEventsOut(const MappedComposition &mC,
                                  const Rosegarden::RealTime &playLatency, 
                                  bool now);

    // Return the sample rate
    //
    virtual unsigned int getSampleRate() const;

    // Some stuff to help us debug this
    //
    void getSystemInfo();
    void showQueueStatus(int queue);

    // Process pending
    //
    virtual void processPending(const RealTime &playLatency);

    // We can return audio control signals to the gui using MappedEvents.
    // Meter levels or audio file completions can go in here.
    //
    void insertMappedEventForReturn(MappedEvent *mE);


    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId id,
                                   unsigned long pluginId,
                                   int position);

    virtual void removePluginInstance(InstrumentId id, int position);

    virtual void setPluginInstancePortValue(InstrumentId id,
                                            int position,
                                            unsigned long portNumber,
                                            float value);

    virtual void setPluginInstanceBypass(InstrumentId id,
                                         int position,
                                         bool value);

    virtual bool checkForNewClients();

    virtual void setLoop(const RealTime &loopStart, const RealTime &loopEnd);

    // Set the record device
    //
    virtual void setRecordDevice(Rosegarden::DeviceId id);


    // ----------------------- End of Virtuals ----------------------

    // Create and send an MMC command
    //
    void sendMMC(Rosegarden::MidiByte command);

#ifdef HAVE_LADSPA

    LADSPAPluginInstance* getPlugin(InstrumentId id, int position);

    // Return a list of plugin instances that haven't been run()
    //
    PluginInstances getUnprocessedPlugins();

    // Return an ordered list of plugins for an Instrument
    //
    PluginInstances getInstrumentPlugins(InstrumentId id);

    // Reset all plugin processed states (start of a new process() loop)
    //
    void setAllPluginsToUnprocessed();

    // Reset all plugins audio state (when looping or when playback stops)
    //
    void resetAllPlugins();

#endif // HAVE_LADSPA


#ifdef HAVE_LIBJACK

    jack_port_t* getJackInputPortLeft() { return m_audioInputPortLeft; }
    jack_port_t* getJackInputPortRight() { return m_audioInputPortRight; }

    jack_port_t* getJackOutputPortLeft() { return m_audioOutputPortLeft; }
    jack_port_t* getJackOutputPortRight() { return m_audioOutputPortRight; }

    // clear down audio connections if we're restarting
    //
    void shutdownAudio();

    // A new audio file for storage of our recorded samples - the
    // file stays open so we can append samples at will.  We must
    // explicitly close the file eventually though to make sure
    // the integrity is correct (sample sizes must be written).
    //
    // These public methods are required in this file because we
    // need access to this file from the static jackProcess member.
    //
    bool createAudioFile(const std::string &fileName);
    void appendToAudioFile(const std::string &buffer);

#endif


protected:
    ClientPortPair getFirstDestination(bool duplex);
    ClientPortPair getPairForMappedInstrument(InstrumentId id);


    virtual void generateInstruments();
    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &playLatency,
                                bool now);

    virtual void processAudioQueue(const RealTime &playLatency,
                                   bool now);

#ifdef HAVE_LIBJACK

    // Gathers JACK transport information from current driver
    // status and informs JACK server.
    //
    void sendJACKTransportState();

    // Get a JACk frame from a RealTime
    //
    jack_nframes_t getJACKFrame(const RealTime &time);

#endif // HAVE_LIBJACK


private:
    RealTime getAlsaTime();

    // Locally convenient to control our devices
    //
    void sendDeviceController(const ClientPortPair &device,
                              MidiByte byte1,
                              MidiByte byte2);


    std::vector<AlsaPort*>       m_alsaPorts;


    // ALSA MIDI/Sequencer stuff
    //
    snd_seq_t                   *m_midiHandle;
    int                          m_client;
    int                          m_port;
    int                          m_queue;
    int                          m_maxClients;
    int                          m_maxPorts;
    int                          m_maxQueues;

    // ALSA Audio/PCM stuff
    //
    snd_pcm_t                   *m_audioHandle;
    snd_pcm_stream_t             m_audioStream;

    NoteOffQueue                 m_noteOffQueue;

    // Because this can fail even if the driver's up (if
    // another service is using the port say)
    //
    bool                         m_midiInputPortConnected;

    RealTime                     m_alsaPlayStartTime;
    RealTime                     m_alsaRecordStartTime;

    RealTime                     m_loopStartTime;
    RealTime                     m_loopEndTime;
    bool                         m_looping;

    // used when deciding what device number we're on
    // (multiple ports for a client)
    //
    ClientPortPair               m_currentPair;

    // added metronome yet?
    bool                         m_addedMetronome;

    // sending of audio meter data in MappedComposition return stream
    //
    bool                         m_audioMeterSent;

#ifdef HAVE_LIBJACK

    static int  jackProcess(jack_nframes_t nframes, void *arg);
    static int  jackBufferSize(jack_nframes_t nframes, void *arg);
    static int  jackSampleRate(jack_nframes_t nframes, void *arg);
    static void jackShutdown(void *arg);
    static int  jackGraphOrder(void *);
    static int  jackXRun(void *);

    jack_client_t               *m_audioClient;
    jack_port_t                 *m_audioInputPortLeft;
    jack_port_t                 *m_audioInputPortRight;
    jack_port_t                 *m_audioOutputPortLeft;
    jack_port_t                 *m_audioOutputPortRight;

    jack_nframes_t               m_transportPosition;

#endif // HAVE_LIBJACK

#ifdef HAVE_LADSPA

    // Change this container to something a bit more efficient for
    // finding lots of plugins once we have lots of plugins available.
    //
    PluginInstances              m_pluginInstances;

#endif // HAVE_LADSPA

};

}

#endif // _SOUNDDRIVER_H_

#endif // HAVE_ALSA
