// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2005
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

#ifndef HAVE_ALSA

#include <map>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include <arts/artsflow.h>     // aRts audio subsys
#include <arts/artsmodules.h>  // aRts wav modules
#include <arts/qiomanager.h>

#include "MidiRecord.h"        // local MIDI record implementation
#include "SoundDriver.h"


// Specialisation of SoundDriver to support aRts (http://www.arts-project.org)
// Supports version 0.6.0
//
//

#ifndef _ARTSDRIVER_H_
#define _ARTSDRIVER_H_

namespace Rosegarden
{

class ArtsDriver : public SoundDriver
{
public:
    ArtsDriver(MappedStudio *studio);
    virtual ~ArtsDriver();

    virtual void initialise();
    virtual void initialisePlayback(const RealTime &position);
    virtual void stopPlayback();
    virtual void resetPlayback(const RealTime &oldPosition, const RealTime &position);
    virtual void allNotesOff();
    virtual void processNotesOff(const RealTime &time);

    virtual RealTime getSequencerTime();

    virtual MappedComposition *getMappedComposition();
    
    virtual void processEventsOut(const MappedComposition &mC);

    virtual bool record(RecordStatus recordStatus);

    virtual void processPending();

    virtual bool areClocksRunning() const { return true; }

    // Not supported
    //
    virtual unsigned int getSampleRate() const { return 0; }

    // initialise
    void initialiseMidi();
    void initialiseAudio();

    void processMidiIn(const Arts::MidiCommand &midiCommand,
                       const Arts::TimeStamp &timeStamp);

    virtual void processEventsOut(const MappedComposition &mC,
				  const RealTime &sliceStart,
				  const RealTime &sliceEnd);

    // Some Arts helper methods 'cos the basic Arts::TimeStamp
    // method is a bit unhelpful
    //
    Arts::TimeStamp aggregateTime(const Arts::TimeStamp &ts1,
                                  const Arts::TimeStamp &ts2);

    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
    {
        return (aggregateTime(deltaTime(ts, m_artsRecordStartTime),
                Arts::TimeStamp(m_playStartPosition.sec,
				m_playStartPosition.usec())));
    }

    // Plugin instance management - do nothing for the moment
    //
    virtual void setPluginInstance(InstrumentId /*id*/,
                                   QString /*pluginIdent*/,
                                   int /*position*/) {;}

    virtual void removePluginInstance(InstrumentId /*id*/,
                                      int /*position*/) {;}

    virtual void removePluginInstances() {;}

    virtual void setPluginInstancePortValue(InstrumentId /*id*/,
                                            int /*position*/,
                                            unsigned long /*portNumber*/,
                                            float /*value*/) {;}

    virtual float getPluginInstancePortValue(InstrumentId id,
					     int position,
					     unsigned long portNumber) { return 0.0; }

    virtual void setPluginInstanceBypass(InstrumentId /*id*/,
                                         int /*position*/,
                                         bool /*value*/) {;}

    virtual QStringList getPluginInstancePrograms(InstrumentId id,
						  int position) { return QStringList(); }

    virtual QString getPluginInstanceProgram(InstrumentId id,
					     int position) { return QString::null; }

    virtual QString getPluginInstanceProgram(InstrumentId id,
					     int position,
					     int bank,
					     int program) { return QString::null; }

    virtual unsigned long getPluginInstanceProgram(InstrumentId id,
						   int position,
						   QString name) { return 0; }
    
    virtual void setPluginInstanceProgram(InstrumentId id,
					  int position,
					  QString program) {;}

    virtual QString configurePlugin(InstrumentId id,
				    int position,
				    QString key,
				    QString value) { return QString::null; }

    virtual void setAudioBussLevels(int bussId,
				    float dB,
				    float pan) {}

    virtual void setAudioInstrumentLevels(InstrumentId id,
					  float dB,
					  float pan) {}


    virtual bool checkForNewClients() { return false; }
    
    virtual void setLoop(const RealTime &/*loopStart*/,
                         const RealTime &/*loopEnd*/) {;}

    virtual void getAudioInstrumentNumbers(InstrumentId &audioInstrumentBase,
					   int &audioInstrumentCount) {
	audioInstrumentBase = AudioInstrumentBase;
	audioInstrumentCount = 16;
    }
 
    virtual void getSoftSynthInstrumentNumbers(InstrumentId &ssInstrumentBase,
					       int &ssInstrumentCount) {
	ssInstrumentBase = SoftSynthInstrumentBase;
	ssInstrumentCount = 0;
    }

    virtual void claimUnwantedPlugin(void *plugin) { }
    virtual void scavengePlugins() { }

    virtual std::vector<PlayableAudioFile*> getPlayingAudioFiles() 
        { return std::vector<PlayableAudioFile*>(); }

protected:
    virtual void generateInstruments();
    virtual void processAudioQueue(bool now);

    virtual void processMidiOut(const MappedComposition &mC,
                                const RealTime &sliceStart,
				const RealTime &sliceEnd);

    void sendDeviceController(MidiByte controller, MidiByte value);
    std::map<unsigned int, MappedEvent*>        m_noteOnMap;
    
private:
    // aRts sound server reference
    //
    Arts::SoundServerV2      m_soundServer;

    // aRts MIDI devices
    //
    Arts::MidiManager        m_midiManager;
    Arts::QIOManager        *m_qIOManager;
    Arts::Dispatcher        *m_dispatcher;
    Arts::MidiClient         m_midiPlayClient;
    Arts::MidiClient         m_midiRecordClient;
    RosegardenMidiRecord     m_midiRecordPort;
    Arts::MidiPort           m_midiPlayPort;

    // aRts Audio devices
    //
    Arts::Synth_AMAN_PLAY    m_amanPlay;
    Arts::Synth_AMAN_RECORD  m_amanRecord;

    Arts::TimeStamp          m_artsPlayStartTime;
    Arts::TimeStamp          m_artsRecordStartTime;

};

}

#endif // _ARTSDRIVER_H_

#endif // HAVE_ALSA
