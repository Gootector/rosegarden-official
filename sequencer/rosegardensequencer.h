// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.2
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

#ifndef _ROSEGARDEN_SEQUENCER_APP_H_
#define _ROSEGARDEN_SEQUENCER_APP_H_
 
// RosegardenSequencerApp is the sequencer application for Rosegarden.
// It owns a Rosegarden::Sequencer object which wraps the aRTS level
// funtionality.  At this level we deal with comms with the Rosegarden
// GUI application, the high level marshalling of data and main event
// loop of the sequencer.  [rwb]
//
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "Composition.h"
#include "rosegardendcop.h"
#include "rosegardensequenceriface.h"
#include "MappedComposition.h"
#include "Sequencer.h"
#include "Event.h"
#include "MappedStudio.h"

class KURL;
class KRecentFilesAction;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;

namespace Rosegarden { class MappedInstrument; }

class RosegardenSequencerApp : public KMainWindow,
                               virtual public RosegardenSequencerIface
{
    Q_OBJECT

public:
    RosegardenSequencerApp();
    ~RosegardenSequencerApp();

    //      -------- START OF DCOP INTERFACE METHODS --------
    //
    //


    // Quit
    virtual void quit();

    // Based on RealTime timestamps
    //
    int play(const Rosegarden::RealTime &position,
             const Rosegarden::RealTime &playLatency,
             const Rosegarden::RealTime &fetchLatency,
             const Rosegarden::RealTime &readAhead);

    // recording
    int record(const Rosegarden::RealTime &position,
               const Rosegarden::RealTime &playLatency,
               const Rosegarden::RealTime &fetchLatency,
               const Rosegarden::RealTime &readAhead,
               int recordMode);

    // looping
    void setLoop(const Rosegarden::RealTime &loopStart,
                 const Rosegarden::RealTime &loopEnd);


    // Play wrapper for DCOP
    //
    virtual int play(long timeSec,
                     long timeUsec,
                     long playLatencySec,
                     long playLatencyUSec,
                     long fetchLatencySec,
                     long fetchLatencyUSec,
                     long readAheadSec,
                     long readAheadUSec);

    // Record wrapper for DCOP
    //
    virtual int record(long timeSec,
                       long timeUSec,
                       long playLatencySec,
                       long playLatencyUSec,
                       long fetchLatencySec,
                       long fetchLatencyUSec,
                       long readAheadSec,
                       long readAheadUSec,
                       int recordMode);

    
    // Jump to a pointer in the playback (uses longs instead
    // of RealTime for DCOP)
    //
    //
    virtual void jumpTo(long posSec, long posUsec);

    // Set a loop on the Sequencer
    //
    virtual void setLoop(long loopStartSec, long loopStartUSec,
                         long loopEndSec, long loopEndUSec);
 
    // Return the Sound system status (audio/MIDI)
    //
    virtual unsigned int getSoundDriverStatus();

    // Add and remove Audio files on the sequencer
    //
    virtual int addAudioFile(const QString &fileName, int id);
    virtual int removeAudioFile(int id);

    // Deletes all the audio files and clears down any flapping i/o handles
    //
    virtual void clearAllAudioFiles();

    // stops the sequencer
    //
    virtual void stop();

    // Set a MappedInstrument at the Sequencer
    //
    virtual void setMappedInstrument(int type, unsigned char channel,
                                     unsigned int id);

    // The sequencer will process the MappedComposition as soon as it
    // gets the chance.
    //
    virtual void processSequencerSlice(Rosegarden::MappedComposition mC);

    // Yeuch!
    //
    virtual void processMappedEvent(unsigned int id,
                                    int type,
                                    unsigned char pitch,
                                    unsigned char velocity,
                                    long absTimeSec,
                                    long absTimeUsec,
                                    long durationSec,
                                    long durationUsec,
                                    long audioStartMarkerSec,
                                    long audioStartMarkerUSec);

    // And now do it properly
    //
    virtual void processMappedEvent(Rosegarden::MappedEvent mE);

    virtual unsigned int getDevices();
    virtual Rosegarden::MappedDevice getMappedDevice(unsigned int id);

    // The GUI tells us that it's alive
    //
    virtual void alive();

    // Audio monitoring
    //
    virtual void setAudioMonitoring(long value);
    virtual void setAudioMonitoringInstrument(unsigned int id);

    // Audio latencies
    //
    virtual Rosegarden::MappedRealTime getAudioPlayLatency();
    virtual Rosegarden::MappedRealTime getAudioRecordLatency();

    // Set a MappedObject 
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   int value);

    // Get a MappedObject for a type
    //
    virtual int getMappedObjectId(int type);

    // Get a Property list from an Object
    //
    virtual QValueVector<QString> getPropertyList(int id,
                                                  const QString &property);

    //
    //
    //
    //      -------- END OF DCOP INTERFACE --------




    void setStatus(TransportStatus status)
            { m_transportStatus = status; }
    TransportStatus getStatus() { return m_transportStatus; }
   
    // Process the first chunk of Sequencer events
    bool startPlaying();

    // Process all subsequent events
    bool keepPlaying();

    // Update internal clock and send GUI position pointer movement
    void updateClocks(bool clearToSend);

    // Sends status changes up to GUI
    void notifySequencerStatus();

    // These two methods process any pending MIDI or audio
    // and send them up to the gui for storage and display
    //
    void processRecordedMidi();
    void processRecordedAudio();

    // Called during stopped or playing operation to process
    // any pending incoming MIDI events that aren't being
    // recorded (i.e. for display in Transport or on Mixer)
    //
    void processAsynchronousEvents();

    // Are we looping?
    //
    bool isLooping() const { return !(m_loopStart == m_loopEnd); }

    // Do we send the "alive" call to the gui ?
    //
    bool sendAlive() const { return m_sendAlive; }

    // the call itself
    void sequencerAlive();

    /*
    // Audio latencies
    //
    Rosegarden::RealTime getAudioPlaybackLatency()
        { return m_audioPlayLatency; }
    void setAudioPlaybackLatency(const Rosegarden::RealTime &latency)
        { m_audioPlayLatency = latency; }

    Rosegarden::RealTime getAudioRecordLatency()
        { return m_audioRecordLatency; }
    void setAudioRecordLatency(const Rosegarden::RealTime &latency)
        { m_audioRecordLatency = latency; }
    */

    // Clear-To-Send flag
    //
    bool clearToSend() { return m_clearToSend; }
    void setClearToSend(bool value) { m_clearToSend = value; }

    // Initialise the virtual studio at this end of the link
    //
    void initialiseStudio();

private:

    // get events whilst handling loop
    //
    Rosegarden::MappedComposition*
        fetchEvents(const Rosegarden::RealTime &start,
                    const Rosegarden::RealTime &end,
                    bool firstFetch);

    // just get a slice of events between markers
    //
    Rosegarden::MappedComposition* getSlice(const Rosegarden::RealTime &start,
                                            const Rosegarden::RealTime &end,
                                            bool firstFetch);

    Rosegarden::Sequencer *m_sequencer;
    TransportStatus m_transportStatus;

    // Position pointer
    Rosegarden::RealTime m_songPosition;
    Rosegarden::RealTime m_lastFetchSongPosition;

    // Latency - m_fetchLatency - how long before we run out of events that
    //                            we go and fetch some more
    //
    //         - m_playLatency  - how long we add to all events to make
    //                            sure they play in a synchonised manner
    //
    //         - m_readAhead    - how large a slice of events we read at a time
    //                            
    //
    // We can throttle these values internally at first, make them
    // user defineable or even auto-throttle them possibly.
    //
    //
    Rosegarden::RealTime m_fetchLatency;
    Rosegarden::RealTime m_playLatency;
    Rosegarden::RealTime m_readAhead;

    /*

    // Not required at the sequencer

    // Two more latencies for audio play and record - when we 
    // use an unsynchronised audio and MIDI system such as
    // ALSA and JACK we need to use these additional values
    // to help time-keeping.
    //
    Rosegarden::RealTime m_audioPlayLatency;
    Rosegarden::RealTime m_audioRecordLatency;

    */


    Rosegarden::RealTime m_loopStart;
    Rosegarden::RealTime m_loopEnd;

    std::vector<Rosegarden::MappedInstrument*> m_instruments;

    // If we should poll the GUI to see if it's alive
    //
    bool m_sendAlive;
    int  m_guiCount;

    // Are we clear to send after a call()?
    //
    bool m_clearToSend;

    // MappedStudio holds all of our session-persistent information -
    // sliders and what have you.  It's also streamable over DCOP
    // so you can reconstruct it at either end of the link for 
    // presentation, storage etc.
    //
    Rosegarden::MappedStudio *m_studio;

};
 
#endif // _ROSEGARDEN_SEQUENCER_APP_H_
