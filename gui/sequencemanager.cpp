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

#include <qpushbutton.h>
#include <qcursor.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kmessagebox.h>

#include "rgapplication.h"
#include "constants.h"
#include "audiopluginmanager.h"
#include "ktmpstatusmsg.h"
#include "rosegardenguidoc.h"
#include "rosegardentransportdialog.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"
#include "SoundDriver.h"
#include "BaseProperties.h"
#include "studiocontrol.h"
#include "MidiDevice.h"
#include "rosestrings.h"
#include "mmapper.h"
// #include "widgets.h"
// #include "dialogs.h"
#include "diskspace.h"
#include "sequencermapper.h"


namespace Rosegarden
{


//----------------------------------------

SequenceManager::SequenceManager(RosegardenGUIDoc *doc,
                                 RosegardenTransportDialog *transport):
    m_doc(doc),
    m_compositionMmapper(new CompositionMmapper(m_doc)),
    m_controlBlockMmapper(new ControlBlockMmapper(m_doc)),
    m_metronomeMmapper(SegmentMmapperFactory::makeMetronome(m_doc)),
    m_tempoSegmentMmapper(SegmentMmapperFactory::makeTempo(m_doc)),
    m_timeSigSegmentMmapper(SegmentMmapperFactory::makeTimeSig(m_doc)),
    m_transportStatus(STOPPED),
    m_soundDriverStatus(NO_DRIVER),
    m_transport(transport),
    m_lastRewoundAt(clock()),
    m_countdownDialog(0),
    m_countdownTimer(new QTimer(doc)),
    m_shownOverrunWarning(false),
    m_recordTime(new QTime()),
    m_compositionRefreshStatusId(m_doc->getComposition().getNewRefreshStatusId()),
    m_updateRequested(true),
    m_sequencerMapper(0)
{
    m_compositionMmapper->cleanup();

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());
    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    connect(doc->getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));

    m_doc->getComposition().addObserver(this);

    // This check may throw an exception
    checkSoundDriverStatus();

    // Try to map the sequencer file
    //
    mapSequencer();
}


SequenceManager::~SequenceManager()
{
    m_doc->getComposition().removeObserver(this);

    SEQMAN_DEBUG << "SequenceManager::~SequenceManager()\n";   
    delete m_compositionMmapper;
    delete m_controlBlockMmapper;
    delete m_metronomeMmapper;
    delete m_tempoSegmentMmapper;
    delete m_timeSigSegmentMmapper;
    delete m_sequencerMapper;
}

void SequenceManager::setDocument(RosegardenGUIDoc* doc)
{
    SEQMAN_DEBUG << "SequenceManager::setDocument()\n";

    DataBlockRepository::clear();

    m_doc->getComposition().removeObserver(this);
    disconnect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()));
    
    m_segments.clear();
    m_doc = doc;

    // Must recreate and reconnect the countdown timer and dialog
    // (bug 729039)
    //
    delete m_countdownDialog;
    delete m_countdownTimer;

    m_countdownDialog = new CountdownDialog(dynamic_cast<QWidget*>
                                (m_doc->parent())->parentWidget());

    m_countdownTimer = new QTimer(m_doc);

    // Connect this for use later
    //
    connect(m_countdownTimer, SIGNAL(timeout()),
            this, SLOT(slotCountdownTimerTimeout()));

    if (m_doc) {
	m_compositionRefreshStatusId =
	    m_doc->getComposition().getNewRefreshStatusId();
        m_doc->getComposition().addObserver(this);

        connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
                this, SLOT(update()));

	for (Rosegarden::Composition::iterator i = m_doc->getComposition().begin();
	     i != m_doc->getComposition().end(); ++i) {
	    m_segments.insert(SegmentRefreshMap::value_type
			      (*i, (*i)->getNewRefreshStatusId()));
	}
    }

    resetCompositionMmapper();

    // Try to map the sequencer file
    //
    mapSequencer();
}

RosegardenGUIDoc* SequenceManager::getDocument()
{
    return m_doc;
}


void
SequenceManager::setTransportStatus(const TransportStatus &status)
{
    m_transportStatus = status;
}

void 
SequenceManager::mapSequencer()
{
    if (m_sequencerMapper) return;

    try
    {
        m_sequencerMapper = new SequencerMapper(
            KGlobal::dirs()->resourceDirs("tmp").last() + "/rosegarden_sequencer_timing_block");
    }
    catch(Rosegarden::Exception)
    {
        m_sequencerMapper = 0;
    }
}

bool
SequenceManager::play()
{
    mapSequencer();

    Composition &comp = m_doc->getComposition();

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
  
    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
        {
            stopping();
            return true;
        }

    // This check may throw an exception
    checkSoundDriverStatus();

    // Align Instrument lists and send initial program changes
    //
    preparePlayback();

    // Update play metronome status
    // 
    m_controlBlockMmapper->updateMetronomeData
	(m_metronomeMmapper->getMetronomeInstrument());
    m_controlBlockMmapper->updateMetronomeForPlayback();

    // Send audio latencies
    //
    //sendAudioLatencies();

    // make sure we toggle the play button
    // 
    m_transport->PlayButton()->setOn(true);

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0) {
        comp.setDefaultTempo(120.0);

        SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
    } else {
            SEQMAN_DEBUG << "SequenceManager::play() - starting to play\n";
    }

    // Send initial tempo
    //
    double qnD = 60.0/comp.getTempo();
    Rosegarden::RealTime qnTime =
        Rosegarden::RealTime(long(qnD), 
                long((qnD - double(long(qnD))) * 1000000000.0));
    StudioControl::sendQuarterNoteLength(qnTime);

    // set the tempo in the transport
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer
    RealTime startPos = comp.getElapsedRealTime(comp.getPosition());

    // If we're looping then jump to loop start
    if (comp.isLooping())
        startPos = comp.getElapsedRealTime(comp.getLoopStart());

    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.nsec;

    // event read ahead
    streamOut << config->readLongNumEntry("readaheadsec", 0);
    streamOut << config->readLongNumEntry("readaheadusec", 80000) * 1000;

    // read ahead slice
    streamOut << config->readLongNumEntry("audiomixsec", 0);
    streamOut << config->readLongNumEntry("audiomixusec", 60000) * 1000;

    // read ahead slice
    streamOut << config->readLongNumEntry("audioreadsec", 0);
    streamOut << config->readLongNumEntry("audioreadusec", 80000) * 1000;

    // read ahead slice
    streamOut << config->readLongNumEntry("audiowritesec", 0);
    streamOut << config->readLongNumEntry("audiowriteusec", 200000) * 1000;

    // small file size
    streamOut << config->readLongNumEntry("smallaudiofilekbytes", 128);

    // Send Play to the Sequencer
    if (!rgapp->sequencerCall("play(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int)",
                              replyType, replyData, data)) {
        m_transportStatus = STOPPED;
        return false;
    }

    // ensure the return type is ok
    QDataStream streamIn(replyData, IO_ReadOnly);
    int result;
    streamIn >> result;
  
    if (result) {
        // completed successfully 
        m_transportStatus = STARTING_TO_PLAY;
    } else {
        m_transportStatus = STOPPED;
        throw(Rosegarden::Exception("Failed to start playback"));
    }

    return false;
}

void
SequenceManager::stopping()
{
    // Do this here rather than in stop() to avoid any potential
    // race condition (we use setPointerPosition() during stop()).
    //
    if (m_transportStatus == STOPPED)
    {
        if (m_doc->getComposition().isLooping())
            m_doc->setPointerPosition(m_doc->getComposition().getLoopStart());
        else
            m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());

        return;
    }

    // Disarm recording and drop back to STOPPED
    //
    if (m_transportStatus == RECORDING_ARMED)
    {
        m_transportStatus = STOPPED;
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());
        return;
    }

    SEQMAN_DEBUG << "SequenceManager::stopping() - preparing to stop\n";

    stop();

    m_shownOverrunWarning = false;
}

void
SequenceManager::stop()
{
    // Toggle off the buttons - first record
    //
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        m_transport->RecordButton()->setOn(false);
        m_transport->MetronomeButton()->
            setOn(m_doc->getComposition().usePlayMetronome());

        // Remove the countdown dialog and stop the timer (if being used)
        //
        m_countdownDialog->hide();
        m_countdownTimer->stop();
    }

    // Now playback
    m_transport->PlayButton()->setOn(false);

    // "call" the sequencer with a stop so we get a synchronous
    // response - then we can fiddle about with the audio file
    // without worrying about the sequencer causing problems
    // with access to the same audio files.
    //

    // wait cursor
    //
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    QCString replyType;
    QByteArray replyData;

    if (!rgapp->sequencerCall("stop()", replyType, replyData)) {
        throw(Rosegarden::Exception("Failed to contact Rosegarden sequencer with stop command.   Please save your composition and restart Rosegarden to continue."));
    }

    // restore
    QApplication::restoreOverrideCursor();

    TransportStatus status = m_transportStatus;
    
    // set new transport status first, so that if we're stopping
    // recording we don't risk the record segment being restored by a
    // timer while the document is busy trying to do away with it
    m_transportStatus = STOPPED;

    // if we're recording MIDI or Audio then tidy up the recording Segment
    if (status == RECORDING_MIDI)
    {
        m_doc->stopRecordingMidi();

        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording MIDI\n";
    }

    if (status == RECORDING_AUDIO)
    {
        m_doc->stopRecordingAudio();
        SEQMAN_DEBUG << "SequenceManager::stop() - stopped recording audio\n";
    }

    // always untoggle the play button at this stage
    //
    m_transport->PlayButton()->setOn(false);
    SEQMAN_DEBUG << "SequenceManager::stop() - stopped playing\n";

    // We don't reset controllers at this point - what happens with static 
    // controllers the next time we play otherwise?  [rwb]
    //resetControllers();
}

// Jump to previous bar
//
void
SequenceManager::rewind()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    std::pair<timeT, timeT> barRange =
	composition.getBarRangeForTime(position - 1);

    if (m_transportStatus == PLAYING) {

	// if we're playing and we had a rewind request less than 200ms
	// ago and we're some way into the bar but less than half way
	// through it, rewind two barlines instead of one

	clock_t now = clock();
	int elapsed = (now - m_lastRewoundAt) * 1000 / CLOCKS_PER_SEC;

	SEQMAN_DEBUG << "That was " << m_lastRewoundAt << ", this is " << now << ", elapsed is " << elapsed << endl;

	if (elapsed >= 0 && elapsed <= 200) {
	    if (position > barRange.first &&
		position < barRange.second &&
		position <= (barRange.first + (barRange.second -
					       barRange.first) / 2)) {
		barRange = composition.getBarRangeForTime(barRange.first - 1);
	    }
	}

	m_lastRewoundAt = now;
    }
    
    if (barRange.first < composition.getStartMarker()) {
	m_doc->setPointerPosition(composition.getStartMarker());
    } else {
	m_doc->setPointerPosition(barRange.first);
    }
}


// Jump to next bar
//
void
SequenceManager::fastforward()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition() + 1;
    timeT newPosition = composition.getBarRangeForTime(position).second;

    // Don't skip past end marker
    //
    if (newPosition > composition.getEndMarker())
        newPosition = composition.getEndMarker();

    m_doc->setPointerPosition(newPosition);

}


// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void
SequenceManager::notifySequencerStatus(TransportStatus status)
{
    // for the moment we don't do anything fancy
    m_transportStatus = status;

}


void
SequenceManager::sendSequencerJump(const Rosegarden::RealTime &time)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << time.sec;
    streamOut << time.nsec;

    rgapp->sequencerSend("jumpTo(long int, long int)", data);
}



// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the sequencer mmapper
// also called via DCOP
//
//

void
SequenceManager::record(bool toggled)
{
    mapSequencer();

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Studio &studio = m_doc->getStudio();
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    bool punchIn = false; // are we punching in?

    // Rather clumsy additional check for audio subsys when we start
    // recording - once we enforce audio subsystems then this will
    // become redundant.
    //
    if (!(m_soundDriverStatus & AUDIO_OK)) {
        int rID = comp.getRecordTrack();
        Rosegarden::InstrumentId instrId =
            comp.getTrackById(rID)->getInstrument();
        Rosegarden::Instrument *instr = studio.getInstrumentById(instrId);

        if (!instr || instr->getType() == Rosegarden::Instrument::Audio) {
            m_transport->RecordButton()->setOn(false);
            throw(Rosegarden::Exception("Audio subsystem is not available - can't record audio"));
        }
    }

    if (toggled) {
        if (m_transportStatus == RECORDING_ARMED) {
            SEQMAN_DEBUG << "SequenceManager::record - unarming record\n";
            m_transportStatus = STOPPED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.usePlayMetronome());
            m_transport->RecordButton()->setOn(false);

            return;
        }

        if (m_transportStatus == STOPPED) {
            SEQMAN_DEBUG << "SequenceManager::record - armed record\n";
            m_transportStatus = RECORDING_ARMED;

            // Toggle the buttons
            m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());
            m_transport->RecordButton()->setOn(true);

            return;
        }

        if (m_transportStatus == RECORDING_MIDI ||
            m_transportStatus == RECORDING_AUDIO) {
            SEQMAN_DEBUG << "SequenceManager::record - stop recording and keep playing\n";

            QByteArray data;
            QCString replyType;
            QByteArray replyData;

            // Send Record to the Sequencer to signal it to drop out of record mode
            //
            if (!rgapp->sequencerCall("play(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int)",
                                  replyType, replyData, data))
            {
                m_transportStatus = STOPPED;
                return;
            }

            m_doc->stopRecordingMidi();
            m_transportStatus = PLAYING;

            return;
        }

        if (m_transportStatus == PLAYING) {
            SEQMAN_DEBUG << "SequenceManager::record - punch in recording\n";
            punchIn = true;
            goto punchin;
        }

    } else {

punchin:

        // Get the record track and check the Instrument type
        int rID = comp.getRecordTrack();
        Rosegarden::InstrumentId inst =
            comp.getTrackById(rID)->getInstrument();

        // If no matching record instrument
        //
        if (studio.getInstrumentById(inst) == 0) {
            m_transport->RecordButton()->setDown(false);
            throw(Rosegarden::Exception("No Record instrument selected"));
        }


        // may throw an exception
        checkSoundDriverStatus();

        // toggle the Metronome button if it's in use
        m_transport->MetronomeButton()->setOn(comp.useRecordMetronome());

	// Update record metronome status
	// 
	m_controlBlockMmapper->updateMetronomeData
	    (m_metronomeMmapper->getMetronomeInstrument());
	m_controlBlockMmapper->updateMetronomeForRecord();

        // If we are looping then jump to start of loop and start recording,
        // if we're not take off the number of count-in bars and start 
        // recording.
        //
        if(comp.isLooping())
            m_doc->setPointerPosition(comp.getLoopStart());
        else {
            if (m_transportStatus != RECORDING_ARMED && punchIn == false) {
                int startBar = comp.getBarNumber(comp.getPosition());
                startBar -= config->readUnsignedNumEntry("countinbars", 2);
                m_doc->setPointerPosition(comp.getBarRange(startBar).first);
            }
        }

        // Some locals
        //
        TransportStatus recordType;
        QByteArray data;
        QCString replyType;
        QByteArray replyData;

        switch (studio.getInstrumentById(inst)->getType()) {

        case Rosegarden::Instrument::Midi:
            recordType = STARTING_TO_RECORD_MIDI;
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record MIDI\n";
            break;

        case Rosegarden::Instrument::Audio: {
            // check for disk space available
            Rosegarden::DiskSpace *space;
            Rosegarden::AudioFileManager &afm = 
                m_doc->getAudioFileManager();
            QString audioPath = strtoqstr(afm.getAudioPath());

            try {
                space = new Rosegarden::DiskSpace(audioPath);
            }
            catch(QString e)
                {
                    // Add message and re-throw
                    //
                    QString m = i18n("Audio record path \"") +
                        audioPath + QString("\". ") + e + QString("\n") +
                        i18n("Edit your audio path properties (Edit->Edit Document Properties->Audio)");
                    throw(m);
                }

            // Check the disk space available is within current
            // audio recording limit
            //
            config->setGroup(Rosegarden::SequencerOptionsConfigGroup);
//            int audioRecordMinutes = config->
//                readNumEntry("audiorecordminutes", 5);

//            Rosegarden::AudioPluginManager *apm = 
//                m_doc->getPluginManager();

            // Ok, check disk space and compare to limits
            //space->getFreeKBytes()

            recordType = STARTING_TO_RECORD_AUDIO;
            SEQMAN_DEBUG << "SequenceManager::record() - "
                         << "starting to record Audio\n";
            break;
        }
            
        default:
            SEQMAN_DEBUG << "SequenceManager::record() - unrecognised instrument type " << int(studio.getInstrumentById(inst)->getType()) << " for instrument " << inst << "\n";
            return;
            break;
        }

        // set the buttons
        m_transport->RecordButton()->setOn(true);
        m_transport->PlayButton()->setOn(true);

        // write the start position argument to the outgoing stream
        //
        QDataStream streamOut(data, IO_WriteOnly);

        if (comp.getTempo() == 0) {
            SEQMAN_DEBUG << "SequenceManager::play() - setting Tempo to Default value of 120.000\n";
            comp.setDefaultTempo(120.0);
        } else {
            SEQMAN_DEBUG << "SequenceManager::record() - starting to record\n";
        }

        // set the tempo in the transport
        //
        m_transport->setTempo(comp.getTempo());

        // The arguments for the Sequencer  - record is similar to playback,
        // we must being playing to record.
        //
        Rosegarden::RealTime startPos =
            comp.getElapsedRealTime(comp.getPosition());

        // playback start position
        streamOut << startPos.sec;
        streamOut << startPos.nsec;
    
        // set group
        config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

        // read ahead slice
        streamOut << config->readLongNumEntry("readaheadsec", 0);
        streamOut << config->readLongNumEntry("readaheadusec", 80000) * 1000;

	// read ahead slice
	streamOut << config->readLongNumEntry("audiomixsec", 0);
	streamOut << config->readLongNumEntry("audiomixusec", 60000) * 1000;

	// read ahead slice
	streamOut << config->readLongNumEntry("audioreadsec", 0);
	streamOut << config->readLongNumEntry("audioreadusec", 100000) * 1000;

	// read ahead slice
	streamOut << config->readLongNumEntry("audiowritesec", 0);
	streamOut << config->readLongNumEntry("audiowriteusec", 200000) * 1000;

	// small file size
	streamOut << config->readLongNumEntry("smallaudiofilekbytes", 128);
    
        // record type
        streamOut << (int)recordType;
    
        // Send Play to the Sequencer
        if (!rgapp->sequencerCall("record(long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, long int, int)",
                                  replyType, replyData, data)) {
            // failed
            m_transportStatus = STOPPED;
            return;
        }

        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result) {
            // completed successfully 
            m_transportStatus = recordType;

            if (recordType == STARTING_TO_RECORD_AUDIO) {
                // Create the countdown timer dialog for limiting the
                // audio recording time.
                //
                KConfig* config = kapp->config();
                config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

                int seconds = 60 * 
                    (config->readNumEntry("audiorecordminutes", 5));

                // re-initialise
                m_countdownDialog->setTotalTime(seconds);

                connect(m_countdownDialog, SIGNAL(stopped()),
                        this, SLOT(slotCountdownCancelled()));

                connect(m_countdownDialog, SIGNAL(completed()),
                        this, SLOT(slotCountdownStop()));

                // Create the timer
                //
                m_recordTime->start();

                // Start an elapse timer for updating the dialog -
                // it will fire every second.
                //
                m_countdownTimer->start(1000);

                // Pop-up the dialog (don't use exec())
                //
                m_countdownDialog->show();

            }
        } else {
            // Stop immediately - turn off buttons in parent
            //
            m_transportStatus = STOPPED;

            if (recordType == STARTING_TO_RECORD_AUDIO) {
                throw(Rosegarden::Exception("Couldn't start recording audio.  Ensure your audio record path is valid\nin Document Properties (Edit->Edit Document Properties->Audio)"));
            } else {
                throw(Rosegarden::Exception("Couldn't start recording MIDI"));
            }

        }
    }
}


// Process unexpected MIDI events at the GUI - send them to the
// Transport, or to a MIDI mixer for display purposes, or to an editor
// for use in step recording.
//
void
SequenceManager::processAsynchronousMidi(const MappedComposition &mC,
                                         Rosegarden::AudioManagerDialog
                                             *audioManagerDialog)
{
    static bool boolShowingWarning = false;

    if (m_doc == 0 || mC.size() == 0) return;

    Rosegarden::MappedComposition::iterator i;

    // Thru filtering is done at the sequencer for the actual sound
    // output, but here we need both filtered (for OUT display) and
    // unfiltered (for insertable note callbacks) compositions, so
    // we've received the unfiltered copy and will filter here
    Rosegarden::MappedComposition tempMC =
	applyFiltering(mC,
		       Rosegarden::MappedEvent::MappedEventType(
			   m_doc->getStudio().getMIDIThruFilter()));
    
    // send to the MIDI labels (which can only hold one event at a time)
    i = mC.begin();
    if (i != mC.end()) {
	m_transport->setMidiInLabel(*i);
    }

    i = tempMC.begin();
    if (i != tempMC.end()) {
	m_transport->setMidiOutLabel(*i);
    }

    for (i = mC.begin(); i != mC.end(); ++i )
    {
	if ((*i)->getType() >= Rosegarden::MappedEvent::Audio)
	{
	    if ((*i)->getType() == Rosegarden::MappedEvent::AudioStopped)
	    {
		/*
		  SEQMAN_DEBUG << "AUDIO FILE ID = "
		  << int((*i)->getData1())
		  << " - FILE STOPPED - " 
		  << "INSTRUMENT = "
		  << (*i)->getInstrument()
		  << endl;
		*/
		
		if (audioManagerDialog && (*i)->getInstrument() == 
		    m_doc->getStudio().getAudioPreviewInstrument())
		{
		    audioManagerDialog->
			closePlayingDialog(
			    Rosegarden::AudioFileId((*i)->getData1()));
		}
	    }
	    
	    if ((*i)->getType() == Rosegarden::MappedEvent::AudioLevel)
		sendAudioLevel(*i);
	    
	    if ((*i)->getType() == 
		Rosegarden::MappedEvent::AudioGeneratePreview)
	    {
		m_doc->finalizeAudioFile(
		    Rosegarden::AudioFileId((*i)->getData1()));
	    }
	    
	    if ((*i)->getType() ==
		Rosegarden::MappedEvent::SystemUpdateInstruments)
	    {
		// resync Devices and Instruments
		//
		m_doc->syncDevices();
	    }

            if (m_transportStatus == PLAYING ||
                m_transportStatus == RECORDING_MIDI ||
                m_transportStatus == RECORDING_AUDIO)
            {
		if ((*i)->getType() == Rosegarden::MappedEvent::SystemFailure) {

		    SEQMAN_DEBUG << "Failure of some sort..." << endl;

		    if ((*i)->getData1() == Rosegarden::MappedEvent::FailureJackDied) {
                    // Something horrible has happened to JACK or we got
                    // bumped out of the graph.  Either way stop playback.
                    //
                    stopping();

                    KMessageBox::error(
                        dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
                        i18n("JACK Audio subsystem has died or it has stopped Rosegarden from processing audio.\nPlease restart Rosegarden to continue working with audio.\nQuitting other running applications may improve Rosegarden's performance."));

		    } else if ((*i)->getData1() == Rosegarden::MappedEvent::FailureXRuns) {
			if (!boolShowingWarning) {

			    QString message = i18n("JACK Audio subsystem is losing resolution.");
			    boolShowingWarning = true;
		    
			    KMessageBox::information
//				(dynamic_cast<QWidget*>(m_doc->parent())->parentWidget(),
				(0, 
				 message);
			    boolShowingWarning = false;
			}
		    } else if (!m_shownOverrunWarning) {
			
			QString message;
			    
			switch ((*i)->getData1()) {
			    
			case Rosegarden::MappedEvent::FailureDiscUnderrun:
			    message = i18n("Cannot read from disc fast enough to service the audio subsystem.\nConsider increasing the disc read buffer size in the sequencer configuration.");
			    break;

			case Rosegarden::MappedEvent::FailureDiscOverrun:
			    message = i18n("Cannot write to disc fast enough to service the audio subsystem.\nConsider increasing the disc write buffer size in the sequencer configuration.");
			    break;

			case Rosegarden::MappedEvent::FailureBussMixUnderrun:
			case Rosegarden::MappedEvent::FailureMixUnderrun:
			    message = i18n("The audio subsystem is failing to keep up.\nConsider increasing the mix buffer size in the sequencer configuration.");
			    break;

			default:
			    message = i18n("Unknown sequencer failure mode!");
			    break;
			}

			m_shownOverrunWarning = true;
			KMessageBox::information(0, message);
		    }
		}
	    }
        }
    }
    
    // if we aren't playing or recording, consider invoking any
    // step-by-step clients (using unfiltered composition)
    
    if (m_transportStatus == STOPPED ||
	m_transportStatus == RECORDING_ARMED) {

	for (i = mC.begin(); i != mC.end(); ++i )
	{
	    if ((*i)->getType() == Rosegarden::MappedEvent::MidiNote) {
		if ((*i)->getVelocity() == 0) {
		    emit insertableNoteOffReceived((*i)->getPitch(), (*i)->getVelocity());
		} else {
		    emit insertableNoteOnReceived((*i)->getPitch(), (*i)->getVelocity());
		}
	    }
	}
    }
}


void
SequenceManager::rewindToBeginning()
{
    SEQMAN_DEBUG << "SequenceManager::rewindToBeginning()\n";
    m_doc->setPointerPosition(m_doc->getComposition().getStartMarker());
}


void
SequenceManager::fastForwardToEnd()
{
    SEQMAN_DEBUG << "SequenceManager::fastForwardToEnd()\n";

    Composition &comp = m_doc->getComposition();
    m_doc->setPointerPosition(comp.getDuration());
}


// Called from the LoopRuler (usually a double click)
// to set position and start playing
//
void
SequenceManager::setPlayStartTime(const timeT &time)
{

    // If already playing then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stopping();
        return;
    }
    
    // otherwise off we go
    //
    m_doc->setPointerPosition(time);
    play();
}


void
SequenceManager::setLoop(const timeT &lhs, const timeT &rhs)
{
    // Let the sequencer know about the loop markers
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    Rosegarden::RealTime loopStart =
            m_doc->getComposition().getElapsedRealTime(lhs);
    Rosegarden::RealTime loopEnd =
            m_doc->getComposition().getElapsedRealTime(rhs);

    streamOut << loopStart.sec;
    streamOut << loopStart.nsec;
    streamOut << loopEnd.sec;
    streamOut << loopEnd.nsec;
  
    rgapp->sequencerSend("setLoop(long int, long int, long int, long int)", data);
}

void
SequenceManager::checkSoundDriverStatus()
{
    QCString replyType;
    QByteArray replyData;

    if (! rgapp->sequencerCall("getSoundDriverStatus()", replyType, replyData)) {
	m_soundDriverStatus = NO_DRIVER;
        return;
    }

    QDataStream streamIn(replyData, IO_ReadOnly);
    unsigned int result;
    streamIn >> result;
    m_soundDriverStatus = result;

    if (m_soundDriverStatus == NO_DRIVER)
        throw(Rosegarden::Exception("MIDI and Audio subsystems have failed to initialise"));

    if (!(m_soundDriverStatus & MIDI_OK))
        throw(Rosegarden::Exception("MIDI subsystem has failed to initialise"));

    /*
      if (!(m_soundDriverStatus & AUDIO_OK))
      throw(Rosegarden::Exception("Audio subsystem has failed to initialise"));
    */
}

// Send Instrument list to Sequencer and ensure that initial program
// changes follow them.  Sending the instruments ensures that we have
// channels available on the Sequencer and then the program changes
// are sent to those specific channel (referenced by Instrument ID)
//
void
SequenceManager::preparePlayback(bool forceProgramChanges)
{
    Rosegarden::Studio &studio = m_doc->getStudio();
    Rosegarden::InstrumentList list = studio.getAllInstruments();
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    // Send the MappedInstruments (minimal Instrument information
    // required for Performance) to the Sequencer
    //
    InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        Rosegarden::StudioControl::sendMappedInstrument(MappedInstrument(*it));

        // Send program changes for MIDI Instruments
        //
        if ((*it)->getType() == Instrument::Midi)
        {
            // send bank select always before program change
            //
            if ((*it)->sendsBankSelect())
            {
                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                     (*it)->getMSB());
                mC.insert(mE);

                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                     (*it)->getLSB());
                mC.insert(mE);
            }

            // send program change
            //
            if ((*it)->sendsProgramChange() || forceProgramChanges)
            {
                RG_DEBUG << "SequenceManager::preparePlayback() : sending prg change for "
                         << (*it)->getPresentationName().c_str() << endl;

                mE = new MappedEvent((*it)->getId(),
                                     Rosegarden::MappedEvent::MidiProgramChange,
                                     (*it)->getProgramChange());
                mC.insert(mE);
            }

        }
/*!!! ??? send fader & buss levels? or do we send them routinely anyway?
        else if ((*it)->getType() == Instrument::Audio)
        {
            Rosegarden::StudioControl::setStudioObjectProperty(
                    (*it)->getId(), "value", (*it)->getVolume());
        }
*/
        else
        {
            RG_DEBUG << "SequenceManager::preparePlayback - "
                     << "unrecognised instrument type" << endl;
        }


    }

    // Send the MappedComposition if it's got anything in it
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);

    // Set up the audio playback latency
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

    int jackSec = config->readLongNumEntry("jackplaybacklatencysec", 0);
    int jackUSec = config->readLongNumEntry("jackplaybacklatencyusec", 0);
    m_playbackAudioLatency = Rosegarden::RealTime(jackSec, jackUSec * 1000);

}

void
SequenceManager::sendAudioLevel(Rosegarden::MappedEvent *mE)
{
    RosegardenGUIView *v;
    QList<RosegardenGUIView>& viewList = m_doc->getViewList();

    for (v = viewList.first(); v != 0; v = viewList.next())
    {
        v->showVisuals(mE);
    }

}

void
SequenceManager::resetControllers()
{
    SEQMAN_DEBUG << "SequenceManager::resetControllers - resetting\n";

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    InstrumentList list = m_doc->getStudio().getPresentationInstruments();
    InstrumentList::iterator it;

    for (it = list.begin(); it != list.end(); it++)
    {
        if ((*it)->getType() == Instrument::Midi)
        {
            Rosegarden::MappedEvent 
                mE((*it)->getId(),
                   Rosegarden::MappedEvent::MidiController,
                   MIDI_CONTROLLER_RESET,
                   0);
            Rosegarden::StudioControl::sendMappedEvent(mE);
        }
    }

    //showVisuals(mC);
}

void
SequenceManager::resetMidiNetwork()
{
    SEQMAN_DEBUG << "SequenceManager::resetMidiNetwork - resetting\n";
    Rosegarden::MappedComposition mC;

    // Should do all Midi Instrument - not just guess like this is doing
    // currently.

    for (unsigned int i = 0; i < 16; i++)
    {
        Rosegarden::MappedEvent *mE =
            new Rosegarden::MappedEvent(Rosegarden::MidiInstrumentBase + i,
                                        Rosegarden::MappedEvent::MidiController,
                                        MIDI_SYSTEM_RESET,
                                        0);

        mC.insert(mE);
    }
    showVisuals(mC);
    Rosegarden::StudioControl::sendMappedComposition(mC);
}

void
SequenceManager::getSequencerPlugins(Rosegarden::AudioPluginManager *aPM)
{
    Rosegarden::MappedObjectId id =
        Rosegarden::StudioControl::getStudioObjectByType(
                Rosegarden::MappedObject::AudioPluginManager);

    SEQMAN_DEBUG << "getSequencerPlugins - getting plugin information" << endl;
    
    Rosegarden::MappedObjectPropertyList seqPlugins
        = Rosegarden::StudioControl::getStudioObjectProperty(
                id, Rosegarden::MappedAudioPluginManager::Plugins);

    SEQMAN_DEBUG << "getSequencerPlugins - got "
                 << seqPlugins.size() << " items" << endl;

    /*
    Rosegarden::MappedObjectPropertyList seqPluginIds
        = getSequencerPropertyList(id,
                               Rosegarden::MappedAudioPluginManager::PluginIds);
                               */

    //Rosegarden::MappedObjectPropertyList::iterator it;

    unsigned int i = 0;

    while (i < seqPlugins.size())
    {
        Rosegarden::MappedObjectId id = seqPlugins[i++].toInt();
        QString name = seqPlugins[i++];
        unsigned long uniqueId = seqPlugins[i++].toLong();
        QString label = seqPlugins[i++];
        QString author = seqPlugins[i++];
        QString copyright = seqPlugins[i++];
        QString category = seqPlugins[i++];
        unsigned int portCount = seqPlugins[i++].toInt();

        AudioPlugin *aP = aPM->addPlugin(id,
                                         name,
                                         uniqueId,
                                         label,
                                         author,
                                         copyright,
					 category);

        // SEQMAN_DEBUG << "PLUGIN = \"" << name << "\"" << endl;

        for (unsigned int j = 0; j < portCount; j++)
        {
            id = seqPlugins[i++].toInt();
            name = seqPlugins[i++];
            Rosegarden::PluginPort::PortType type =
                Rosegarden::PluginPort::PortType(seqPlugins[i++].toInt());
            Rosegarden::PluginPort::PortDisplayHint hint =
                Rosegarden::PluginPort::PortDisplayHint(seqPlugins[i++].toInt());
            Rosegarden::PortData lowerBound = seqPlugins[i++].toFloat();
            Rosegarden::PortData upperBound = seqPlugins[i++].toFloat();
	    Rosegarden::PortData defaultValue = seqPlugins[i++].toFloat();

//	     SEQMAN_DEBUG << "DEFAULT =  " << defaultValue << endl;
//             SEQMAN_DEBUG << "ADDED PORT = \"" << name << "\" id " << id << endl;
            aP->addPort(id,
                        name,
                        type,
                        hint,
                        lowerBound,
                        upperBound,
			defaultValue);

        }

        // SEQMAN_DEBUG << " = " << seqPlugins[i] << endl;

        /*
        Rosegarden::MappedObjectPropertyList author =
            getSequencerPropertyList(seqPluginIds[i].toInt(), "author");

        if (author.size() == 1)
            SEQMAN_DEBUG << "PLUGIN AUTHOR = \"" << author[0] << "\"" << std::endl;
            */
    }
}

// Clear down all temporary (non read-only) objects and then
// add the basic audio faders only (one per instrument).
//
void
SequenceManager::reinitialiseSequencerStudio()
{
    // Send the MIDI recording device to the sequencer
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    QString recordDeviceStr = config->readEntry("midirecorddevice");

    if (recordDeviceStr)
    {
        int recordDevice = recordDeviceStr.toInt();

        if (recordDevice >= 0)
        {
            Rosegarden::MappedEvent mE(Rosegarden::MidiInstrumentBase, // InstrumentId
                                       Rosegarden::MappedEvent::SystemRecordDevice,
                                       Rosegarden::MidiByte(recordDevice));

            Rosegarden::StudioControl::sendMappedEvent(mE);
            SEQMAN_DEBUG << "set MIDI record device to "
                         << recordDevice << endl;
        }
    }

    // Toggle JACK audio ports appropriately
    //
    bool submasterOuts = config->readBoolEntry("audiosubmasterouts", false);
    bool faderOuts = config->readBoolEntry("audiofaderouts", false);

    Rosegarden::MidiByte ports = 0;
    if (faderOuts) {
	ports |= Rosegarden::MappedEvent::FaderOuts;
    }
    if (submasterOuts) {
	ports |= Rosegarden::MappedEvent::SubmasterOuts;
    }
    Rosegarden::MappedEvent mEports
	(Rosegarden::MidiInstrumentBase,
	 Rosegarden::MappedEvent::SystemAudioPorts,
	 ports);

    Rosegarden::StudioControl::sendMappedEvent(mEports);


    // Set the studio from the current document
    //
    m_doc->initialiseStudio();
}

// Clear down all playing notes and reset controllers
//
void
SequenceManager::panic()
{
    SEQMAN_DEBUG << "panic button\n";

    Studio &studio = m_doc->getStudio();

    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;

    int maxDevices = 0, device = 0;
    for (it = list.begin(); it != list.end(); it++)
        if ((*it)->getType() == Instrument::Midi)
            maxDevices++;

    for (it = list.begin(); it != list.end(); it++)
    {
        if ((*it)->getType() == Instrument::Midi)
        {
            for (unsigned int i = 0; i < 128; i++)
            {
                MappedEvent 
                    mE((*it)->getId(),
                                Rosegarden::MappedEvent::MidiNote,
                                i,
                                0);

                Rosegarden::StudioControl::sendMappedEvent(mE);
            }

            device++;
        }

        emit setProgress(int(90.0 * (double(device) / double(maxDevices))));
    }

    resetControllers();
}

// In this case we only route MIDI events to the transport ticker
//
void
SequenceManager::showVisuals(const Rosegarden::MappedComposition &mC)
{
    MappedComposition::iterator it = mC.begin();
    if (it != mC.end()) m_transport->setMidiOutLabel(*it);
}


// Filter a MappedComposition by Type.
//
Rosegarden::MappedComposition
SequenceManager::applyFiltering(const Rosegarden::MappedComposition &mC,
                                Rosegarden::MappedEvent::MappedEventType filter)
{
    Rosegarden::MappedComposition retMc;
    Rosegarden::MappedComposition::iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        if (!((*it)->getType() & filter))
            retMc.insert(new MappedEvent(*it));
    }

    return retMc;
}

void SequenceManager::resetCompositionMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetCompositionMmapper()\n";
    delete m_compositionMmapper;
    m_compositionMmapper = new CompositionMmapper(m_doc);

    resetMetronomeMmapper();
    resetTempoSegmentMmapper();
    resetTimeSigSegmentMmapper();
    resetControlBlockMmapper();
}

void SequenceManager::resetMetronomeMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetMetronomeMmapper()\n";

    delete m_metronomeMmapper;
    m_metronomeMmapper = SegmentMmapperFactory::makeMetronome(m_doc);
}

void SequenceManager::resetTempoSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTempoSegmentMmapper()\n";

    delete m_tempoSegmentMmapper;
    m_tempoSegmentMmapper = SegmentMmapperFactory::makeTempo(m_doc);
}

void SequenceManager::resetTimeSigSegmentMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetTimeSigSegmentMmapper()\n";

    delete m_timeSigSegmentMmapper;
    m_timeSigSegmentMmapper = SegmentMmapperFactory::makeTimeSig(m_doc);
}

void SequenceManager::resetControlBlockMmapper()
{
    SEQMAN_DEBUG << "SequenceManager::resetControlBlockMmapper()\n";

    m_controlBlockMmapper->setDocument(m_doc);
}


bool SequenceManager::event(QEvent *e)
{
    if (e->type() == QEvent::User) {
	SEQMAN_DEBUG << "SequenceManager::event() with user event\n";
	if (m_updateRequested) {
	    SEQMAN_DEBUG << "SequenceManager::event(): update requested\n";
	    checkRefreshStatus();
	    m_updateRequested = false;
	}
	return true;
    } else {
	return QObject::event(e);
    }
}


void SequenceManager::update()
{
    SEQMAN_DEBUG << "SequenceManager::update()\n";
    // schedule a refresh-status check for the next event loop
    QEvent *e = new QEvent(QEvent::User);
    m_updateRequested = true;
    QApplication::postEvent(this, e);
}


void SequenceManager::checkRefreshStatus()
{
    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus()\n";

    std::vector<Segment*>::iterator i;

    // Check removed segments first
    for (i = m_removedSegments.begin(); i != m_removedSegments.end(); ++i) {
        processRemovedSegment(*i);
    }
    m_removedSegments.clear();

    SEQMAN_DEBUG << "SequenceManager::checkRefreshStatus: we have "
		 << m_segments.size() << " segments" << endl;
    
    // then the ones which are still there
    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
	if (i->first->getRefreshStatus(i->second).needsRefresh()) {
	    segmentModified(i->first);
	    i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
	}
    }

    // then added ones
    for (i = m_addedSegments.begin(); i != m_addedSegments.end(); ++i) {
        processAddedSegment(*i);
    }
    m_addedSegments.clear();
}

void SequenceManager::segmentModified(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentModified(" << s << ")\n";

    bool sizeChanged = m_compositionMmapper->segmentModified(s);

    SEQMAN_DEBUG << "SequenceManager::segmentModified() : size changed = "
                 << sizeChanged << endl;

    if ((m_transportStatus == PLAYING) && sizeChanged) {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << m_compositionMmapper->getSegmentFileName(s);
	streamOut << m_compositionMmapper->getSegmentFileSize(s);
        
        SEQMAN_DEBUG << "SequenceManager::segmentModified() : DCOP-call sequencer remapSegment"
                     << m_compositionMmapper->getSegmentFileName(s) << endl;

        rgapp->sequencerSend("remapSegment(QString, size_t)", data);
    }
}

void SequenceManager::segmentAdded(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentAdded(" << s << ")\n";
    m_addedSegments.push_back(s);
}

void SequenceManager::segmentRemoved(const Composition*, Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRemoved(" << s << ")\n";
    m_removedSegments.push_back(s);
}

void SequenceManager::segmentRepeatChanged(const Composition*, Segment* s, bool repeat)
{
    SEQMAN_DEBUG << "SequenceManager::segmentRepeatChanged(" << s << ", " << repeat << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentEventsTimingChanged(const Composition*, Segment * s, timeT t, RealTime)
{
    SEQMAN_DEBUG << "SequenceManager::segmentEventsTimingChanged(" << s << ", " << t << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::segmentTransposeChanged(const Composition*, Segment *s, int transpose)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTransposeChanged(" << s << ", " << transpose << ")\n";
    segmentModified(s);
}

void SequenceManager::segmentTrackChanged(const Composition*, Segment *s, TrackId id)
{
    SEQMAN_DEBUG << "SequenceManager::segmentTrackChanged(" << s << ", " << id << ")\n";
    segmentModified(s);
    if (s && s->getType() == Segment::Audio && m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::processAddedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processAddedSegment(" << s << ")\n";

    m_compositionMmapper->segmentAdded(s);

    if (m_transportStatus == PLAYING) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << m_compositionMmapper->getSegmentFileName(s);
        
        if (!rgapp->sequencerSend("addSegment(QString)", data)) {
            m_transportStatus = STOPPED;
        }
    }

    // Add to segments map
    int id = s->getNewRefreshStatusId();
    m_segments.insert(SegmentRefreshMap::value_type(s, id));

}

void SequenceManager::processRemovedSegment(Segment* s)
{
    SEQMAN_DEBUG << "SequenceManager::processRemovedSegment(" << s << ")\n";

    QString filename = m_compositionMmapper->getSegmentFileName(s);
    m_compositionMmapper->segmentDeleted(s);

    if (m_transportStatus == PLAYING) {

        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << filename;

        if (!rgapp->sequencerSend("deleteSegment(QString)", data)) {
            // failed
            m_transportStatus = STOPPED;
        }
    }

    // Remove from segments map
    m_segments.erase(s);
}

void SequenceManager::endMarkerTimeChanged(const Composition *, bool /*shorten*/)
{
    resetMetronomeMmapper();
}

void SequenceManager::timeSignatureChanged(const Composition *)
{
    resetMetronomeMmapper();
}

void SequenceManager::compositionDeleted(const Composition *)
{
    // do nothing
}

void SequenceManager::trackChanged(const Composition *, Track* t)
{
    m_controlBlockMmapper->updateTrackData(t);

    if (m_transportStatus == PLAYING) {
	QByteArray data;
        rgapp->sequencerSend("remapTracks()", data);
    }
}

void SequenceManager::metronomeChanged(Rosegarden::InstrumentId id,
				       bool regenerateTicks)
{
    // This method is called when the user has changed the
    // metronome instrument, pitch etc

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged (simple)"
                 << ", instrument = "
                 << id
                 << endl;

    if (regenerateTicks) resetMetronomeMmapper();

    m_controlBlockMmapper->updateMetronomeData(id);
    if (m_transportStatus == PLAYING) {
	m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
	m_controlBlockMmapper->updateMetronomeForRecord();
    }

    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();
}

void SequenceManager::metronomeChanged(const Composition *)
{
    // This method is called when the muting status in the composition
    // has changed -- the metronome itself has not actually changed

    SEQMAN_DEBUG << "SequenceManager::metronomeChanged " 
                 << ", instrument = "
                 << m_metronomeMmapper->getMetronomeInstrument()
                 << endl;

    m_controlBlockMmapper->updateMetronomeData
	(m_metronomeMmapper->getMetronomeInstrument());

    if (m_transportStatus == PLAYING) {
	m_controlBlockMmapper->updateMetronomeForPlayback();
    } else {
	m_controlBlockMmapper->updateMetronomeForRecord();
    }
}

void SequenceManager::filtersChanged(Rosegarden::MidiFilter thruFilter,
				     Rosegarden::MidiFilter recordFilter)
{
    m_controlBlockMmapper->updateMidiFilters(thruFilter, recordFilter);
}

void SequenceManager::soloChanged(const Composition *, bool solo, TrackId selectedTrack)
{
    m_controlBlockMmapper->updateSoloData(solo, selectedTrack);
}

void SequenceManager::tempoChanged(const Composition *c)
{
    SEQMAN_DEBUG << "SequenceManager::tempoChanged()\n";

    // Refresh all segments
    //
    for (SegmentRefreshMap::iterator i = m_segments.begin();
	 i != m_segments.end(); ++i) {
        segmentModified(i->first);
    }

    // and metronome, time sig and tempo
    //
    m_metronomeMmapper->refresh();
    m_timeSigSegmentMmapper->refresh();
    m_tempoSegmentMmapper->refresh();

    if (c->isLooping()) setLoop(c->getLoopStart(), c->getLoopEnd());
}

void
SequenceManager::sendTransportControlStatuses()
{
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::SequencerOptionsConfigGroup);

    // Get the config values
    //
    bool jackTransport = config->readBoolEntry("jacktransport", false);
    bool jackMaster = config->readBoolEntry("jackmaster", false);

    bool mmcTransport = config->readBoolEntry("mmctransport", false);
    bool mmcMaster = config->readBoolEntry("mmcmaster", false);

    bool midiClock = config->readBoolEntry("midiclock", false);

    // Send JACK transport
    //
    int jackValue = 0;
    if (jackTransport && jackMaster)
        jackValue = 2;
    else 
    {
        if (jackTransport)
            jackValue = 1;
        else
            jackValue = 0;
    }

    Rosegarden::MappedEvent mEjackValue(Rosegarden::MidiInstrumentBase, // InstrumentId
                                        Rosegarden::MappedEvent::SystemJackTransport,
                                        Rosegarden::MidiByte(jackValue));
    Rosegarden::StudioControl::sendMappedEvent(mEjackValue);


    // Send MMC transport
    //
    int mmcValue = 0;
    if (mmcTransport && mmcMaster)
        mmcValue = 2;
    else
    {
        if (mmcTransport)
            mmcValue = 1;
        else
            mmcValue = 0;
    }

    Rosegarden::MappedEvent mEmccValue(Rosegarden::MidiInstrumentBase, // InstrumentId
                                       Rosegarden::MappedEvent::SystemMMCTransport,
                                       Rosegarden::MidiByte(mmcValue));

    Rosegarden::StudioControl::sendMappedEvent(mEmccValue);


    // Send MIDI Clock
    //
    Rosegarden::MappedEvent mEmidiClock(Rosegarden::MidiInstrumentBase, // InstrumentId
                                        Rosegarden::MappedEvent::SystemMIDIClock,
                                        Rosegarden::MidiByte(midiClock));

    Rosegarden::StudioControl::sendMappedEvent(mEmidiClock);

}

void
SequenceManager::slotCountdownCancelled()
{
    SEQMAN_DEBUG << "SequenceManager::slotCountdownCancelled - "
                 << "stopping" << endl;

    // stop timer
    m_countdownTimer->stop();
    m_countdownDialog->hide();

    // stop recording
    stopping();
}

void
SequenceManager::slotCountdownTimerTimeout()
{
    // Set the elapsed time in seconds
    //
    m_countdownDialog->setElapsedTime(m_recordTime->elapsed() / 1000);
}

// The countdown has completed - stop recording
//
void
SequenceManager::slotCountdownStop()
{
    SEQMAN_DEBUG << "SequenceManager::slotCountdownStop - "
                 << "countdown timed out - automatically stopping recording"
                 << endl;

    stopping(); // erm - simple as that
}



}
