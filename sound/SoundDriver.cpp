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

#include <stdlib.h>

#include "SoundDriver.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"

//#define DEBUG_PLAYABLE 1

namespace Rosegarden
{

PlayableAudioFile::PlayableAudioFile(InstrumentId instrumentId,
                                     AudioFile *audioFile,
                                     const RealTime &startTime,
                                     const RealTime &startIndex,
                                     const RealTime &duration,
                                     unsigned int playBufferSize, // size of the required return buffer
                                     RingBuffer *ringBuffer):     // external ringbuffer
        m_startTime(startTime),
        m_startIndex(startIndex),
        m_duration(duration),
        m_status(IDLE),
        m_file(0),
        m_audioFile(audioFile),
        m_instrumentId(instrumentId),
        m_ringBuffer(ringBuffer),
        m_ringBufferThreshold(0),
        m_playBuffer(0),
        m_playBufferSize(playBufferSize),
        m_initialised(false),
        m_externalRingbuffer(false),
        m_runtimeSegmentId(-1)
{
#define DEBUG_PLAYABLE_CONSTRUCTION
#ifdef DEBUG_PLAYABLE_CONSTRUCTION
    std::cout << "PlayableAudioFile::PlayableAudioFile - creating " << this << std::endl;
#endif

}


PlayableAudioFile::PlayableAudioFile(const PlayableAudioFile &pAF)
{
    m_startTime = pAF.getStartTime();
    m_startIndex = pAF.getStartIndex();
    m_duration = pAF.getDuration();
    m_status = pAF.getStatus();
    m_file = 0;
    m_audioFile = pAF.getAudioFile();
    m_instrumentId = pAF.getInstrument();
    m_ringBuffer = pAF.getRingBuffer();
    m_ringBufferThreshold = pAF.getRingBufferThreshold();
    m_playBuffer = 0;
    m_playBufferSize = pAF.getPlayBufferSize();
    m_initialised = false;
    m_externalRingbuffer = false;
    m_runtimeSegmentId = pAF.getRuntimeSegmentId();

#ifdef DEBUG_PLAYABLE_CONSTRUCTION
    std::cout << "PlayableAudioFile::PlayableAudioFile - creating " << this
              << " copied from " << &pAF << std::endl;
#endif
}


void
PlayableAudioFile::initialise()
{
    if (m_initialised) return;

#ifdef DEBUG_PLAYABLE_CONSTRUCTION
    std::cout << "PlayableAudioFile::initialise() " << this << std::endl;
#endif


    m_file = new std::ifstream(m_audioFile->getFilename().c_str(),
                               std::ios::in | std::ios::binary);


    if (!*m_file)
        throw(std::string("PlayableAudioFile - can't open file"));

    // Scan to the beginning of the data chunk we need
    //
    std::cout << "PlayableAudioFile::initialise - scanning to " << m_startIndex << std::endl;
    scanTo(m_startIndex);

    // if no external ringbuffer then create one
    if (m_ringBuffer == 0)
    {
        int bufferSize = 32767; // 32k ringbuffer as default size

        // Default to a quarter of a second's worth of data in the buffer
        //
        if (m_audioFile) bufferSize = m_audioFile->getSampleRate() * m_audioFile->getBytesPerFrame() / 4;

        m_ringBuffer = new RingBuffer(bufferSize);
    }
    else
        m_externalRingbuffer = true;

    int size = m_ringBuffer->getSize();

    m_playBuffer = new char[m_playBufferSize];

    // Put a random amount of something into the buffer to start with
    //
    int initialSize = size / 4 + int(double(size)/2.0 * double(rand()) / double(RAND_MAX));
    //std::cout << "PlayableAudioFile::initialise - initial buffer size = " << initialSize << std::endl;

    m_ringBufferThreshold = size / 4;

    // hard code to output buffer size initialy
    initialSize = m_playBufferSize * 2;
    fillRingBuffer(initialSize);

    // ensure we can't do this again
    m_initialised = true;
}

PlayableAudioFile::~PlayableAudioFile()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
    }

    if (m_externalRingbuffer == false) delete m_ringBuffer;
    if (m_playBuffer) delete [] m_playBuffer;

#ifdef DEBUG_PLAYABLE_CONSTRUCTION
    std::cout << "PlayableAudioFile::~PlayableAudioFile - destroying - " << this << std::endl;
#endif
}
 
bool
PlayableAudioFile::scanTo(const RealTime &time)
{
    if (m_audioFile)
    {
        return m_audioFile->scanTo(m_file, time);
    }
    return false;
}


// Get some sample frames using this object's RingBuffer access to
// the file itself.
//
char *
PlayableAudioFile::getSampleFrames(unsigned int frames)
{
    assert(m_initialised);

    if (m_audioFile)
    {
        int bytes = frames * getBytesPerSample();
        size_t count = m_ringBuffer->read(m_playBuffer, bytes);

        if (count != size_t(bytes))
        {
            // Zero any output buffer that didn't get filled
            //
            for (unsigned int i = count; i < m_playBufferSize; ++i)
                m_playBuffer[i] = 0;

#ifdef DEBUG_PLAYABLE
            std::cerr << "PlayableAudioFile::getSampleFrames - "
                      << "got fewer audio file bytes than required : "
                      << count << " out of " << bytes
                      << std::endl;
#endif
        }
    }

    return m_playBuffer;
}

// Get a sample file slice using this object's file handle
//
std::string
PlayableAudioFile::getSampleFrameSlice(const RealTime &time)
{
    assert(m_initialised);

    if (m_audioFile)
    {
        return m_audioFile->getSampleFrameSlice(m_file, time);
    }
    return std::string("");
}

// Self appointed way of working out whether we need to stream in from disk
// or not - we let audio files manage this themselves so that we can work in
// a random factor and hence hopefully force the audio files to use disk
// reading for varying amounts of time _at_ varying amounts of time.  This
// is in the case where we have n simultaneous audio files.
//
void
PlayableAudioFile::fillRingBuffer()
{
    if (!m_initialised) return;

    // Only fill if we are below the threshold
    //
    if (int(m_ringBuffer->readSpace()) < m_ringBufferThreshold)
    {
        int fetchSize = m_ringBuffer->getSize() / 8 + 
            int(double(m_ringBuffer->getSize()) / 4.0 * double(rand()) / double(RAND_MAX));

        // just a fixed small size for now
        fetchSize =  m_playBufferSize;

        if (fetchSize > int(m_ringBuffer->writeSpace()))
        {
            fetchSize = m_ringBuffer->writeSpace();
            std::cerr << "PlayableAudioFile::fillRingBuffer - buffer maxed out" << std::endl;
        }

        fillRingBuffer(fetchSize);

        /*
        std::cerr << "PlayableAudioFile::fillRingBuffer - buffer size = " << m_ringBuffer->readSpace()
                  << std::endl;
                  */
    }
}

void
PlayableAudioFile::fillRingBuffer(int bytes)
{
#ifdef DEBUG_PLAYABLE
    std::cerr << "PlayableAudioFile::fillRingBuffer - " << bytes << std::endl;
#endif

    if (m_audioFile) // && !m_audioFile->isEof())
    {
        int frames = bytes / getBytesPerSample();

        std::string data;

        // get the frames
        try
        {
            data = m_audioFile->getSampleFrames(m_file, frames);
        }
        catch(std::string e)
        {
            // most likely we've run out of data in the file -
            // we can live with this - just write out what we
            // have got.
#ifdef DEBUG_PLAYABLE
            std::cerr << "PlayableAudioFile::fillRingBuffer - "
                      << e << std::endl;
#endif
            return;

        }

        size_t writtenBytes = m_ringBuffer->write(data);

#ifdef DEBUG_PLAYABLE
        std::cerr << "PlayableAudioFile::fillRingBuffer - "
                  << "written " << writtenBytes << " bytes" << std::endl;
#else
	(void)writtenBytes; // avoid warning
#endif

    }
#ifdef DEBUG_PLAYABLE
    else
    {
        std::cerr << "PlayableAudioFile::fillRingBuffer - no file" << std::endl;
    }
#endif


}


// How many channels in the base AudioFile?
//
unsigned int
PlayableAudioFile::getChannels()
{
    if (m_audioFile)
    {
        return m_audioFile->getChannels();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getBytesPerSample()
{
    if (m_audioFile)
    {
        return m_audioFile->getBytesPerFrame();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getSampleRate()
{
    if (m_audioFile)
    {
        return m_audioFile->getSampleRate();
    }
    return 0;
}


// How many bits per sample in the base AudioFile?
//
unsigned int
PlayableAudioFile::getBitsPerSample()
{
    if (m_audioFile)
    {
        return m_audioFile->getBitsPerSample();
    }
    return 0;
}

// ------------- RecordableAudioFile -------------
//
//
RecordableAudioFile::RecordableAudioFile(const std::string &/* filePath */,
                                         InstrumentId /* instrumentId */,
                                         AudioFile * /* audioFile */)
{
}


void 
RecordableAudioFile::fillRingBuffer(const std::string &/* data */)
{
}




// ---------- SoundDriver -----------
//

SoundDriver::SoundDriver(MappedStudio *studio, const std::string &name):
    m_name(name),
    m_driverStatus(NO_DRIVER),
    m_playStartPosition(0, 0),
    m_startPlayback(false),
    m_playing(false),
    m_midiRecordDevice(0),
    m_recordStatus(ASYNCHRONOUS_MIDI),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase),
    m_audioMonitoringInstrument(Rosegarden::AudioInstrumentBase),
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_studio(studio),
    m_mmcEnabled(false),
    m_mmcMaster(false),
    m_mmcId(0),           // default MMC id of 0
    m_midiClockEnabled(false),
    m_midiClockInterval(0),
    m_midiClockSendTime(Rosegarden::RealTime::zeroTime),
    m_midiSongPositionPointer(0)
{
    // Do some preallocating of the audio vector to minimise overhead
    //
    m_audioPlayQueue.reserve(100);
}


SoundDriver::~SoundDriver()
{
    std::cout << "SoundDriver::~SoundDriver" << std::endl;
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return 0;
}

// Generates a list of queued PlayableAudioFiles that aren't marked
// as defunct (and hence likely to be garbage collected by another
// thread).
//
std::vector<PlayableAudioFile*>
SoundDriver::getAudioPlayQueueNotDefunct()
{
    std::vector<PlayableAudioFile*> rq;
    std::vector<PlayableAudioFile*>::const_iterator it;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
        if ((*it)->getStatus() != PlayableAudioFile::DEFUNCT) {
            rq.push_back(*it);
            if (!(*it)->isInitialised())
                (*it)->initialise(); // start audio buffering
        }
    }

    return rq;
}


void
SoundDriver::queueAudio(PlayableAudioFile *audioFile)
{
    // Push to the back of the thread queue and then we must
    // process this across to the proper audio queue when
    // it's safe to do so - use the method below
    //
    m_audioPlayQueue.push_back(audioFile);
}

void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == mI->getId())
        {
            (*it)->setChannel(mI->getChannel());
            (*it)->setType(mI->getType());
            delete mI;
            return;
        }
    }

    // else create a new one
    m_instruments.push_back(mI);

    std::cout << "SoundDriver: setMappedInstrument() : "
              << "type = " << mI->getType() << " : "
              << "channel = " << (int)(mI->getChannel()) << " : "
              << "id = " << mI->getId() << std::endl;

}

unsigned int
SoundDriver::getDevices()
{
    return m_devices.size();
}

MappedDevice
SoundDriver::getMappedDevice(DeviceId id)
{
    MappedDevice retDevice;
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    std::vector<MappedDevice*>::iterator dIt = m_devices.begin();
    for (; dIt != m_devices.end(); dIt++)
    {
        if ((*dIt)->getId() == id) retDevice = **dIt;
    }

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getDevice() == id)
            retDevice.push_back(*it);
    }

    std::cout << "SoundDriver::getMappedDevice(" << id << ") - "
              << "name = \"" << retDevice.getName() 
              << "\" type = " << retDevice.getType()
              << " direction = " << retDevice.getDirection()
              << " connection = \"" << retDevice.getConnection()
              << "\"" << std::endl;

    return retDevice;
}


// Clear down the audio play queue
//
void
SoundDriver::clearAudioPlayQueue()
{
    std::vector<PlayableAudioFile*>::iterator it;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); it++)
        delete (*it);

    m_audioPlayQueue.erase(m_audioPlayQueue.begin(), m_audioPlayQueue.end());
}

// Not sure if we need this yet - too hungover and too close to 0.9 to
// start getting involved in complicated stuff.
//
void
SoundDriver::clearDefunctFromAudioPlayQueue()
{
    std::vector<PlayableAudioFile*>::const_iterator it;
    std::vector<PlayableAudioFile*> newQueue;

    std::vector<std::vector<PlayableAudioFile*>::iterator> dList;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
        if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT)
        {
#ifdef DEBUG_SOUNDRIVER
            std::cout << "SoundDriver::clearDefunctFromAudioPlayQueue - "
                      << "clearing down " << *it << std::endl;
#endif 

            delete *it;
        }
        else
            newQueue.push_back(*it);
    }

    // clear existing queue
    m_audioPlayQueue.clear();

    for (it = newQueue.begin(); it != newQueue.end(); ++it)
        m_audioPlayQueue.push_back(*it);

}

bool
SoundDriver::addAudioFile(const std::string &fileName, unsigned int id)
{
    AudioFile *ins = new WAVAudioFile(id, fileName, fileName);
    try
    {
        ins->open();
    }
    catch(std::string s)
    {
        return false;
    }

    m_audioFiles.push_back(ins);

    std::cout << "Sequencer::addAudioFile() = \"" << fileName << "\"" << std::endl;

    return true;
}

bool
SoundDriver::removeAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            std::cout << "Sequencer::removeAudioFile() = \"" <<
                          (*it)->getFilename() << "\"" << std::endl;

            delete (*it);
            m_audioFiles.erase(it);
            return true;
        }
    }

    return false;
}

AudioFile*
SoundDriver::getAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
            return *it;
    }

    return 0;
}

void
SoundDriver::clearAudioFiles()
{
    std::cout << "SoundDriver::clearAudioFiles() - clearing down audio files"
              << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}


bool
SoundDriver::queueAudio(InstrumentId instrumentId,
                        AudioFileId audioFileId,
                        const RealTime &absoluteTime,
                        const RealTime &audioStartMarker,
                        const RealTime &duration,
                        const RealTime &playLatency)
{
    AudioFile* audioFile = getAudioFile(audioFileId);

    if (audioFile == 0)
        return false;

    std::cout << "queueAudio() - queuing Audio event at time "
              << absoluteTime + playLatency << std::endl;

    // register the AudioFile in the playback queue
    //
    PlayableAudioFile *newAF =
                         new PlayableAudioFile(instrumentId,
                                               audioFile,
                                               absoluteTime + playLatency,
                                               audioStartMarker - absoluteTime,
                                               duration,
                                               4096);

    queueAudio(newAF);

    return true;
}



// Close down any playing audio files - we can manipulate the
// live play stack as we're only changing state.  Switch on
// segment id first and then drop to instrument/audio file.
//
void
SoundDriver::cancelAudioFile(MappedEvent *mE)
{
    std::vector<PlayableAudioFile*>::iterator it;

    for (it = m_audioPlayQueue.begin();
         it != m_audioPlayQueue.end();
         it++)
    {
        if (mE->getRuntimeSegmentId() == -1)
        {
            if((*it)->getInstrument() == mE->getInstrument() &&
               (*it)->getAudioFile()->getId() == mE->getAudioID())
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
        else
        {
            if ((*it)->getRuntimeSegmentId() == mE->getRuntimeSegmentId())
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
    }
}

}


