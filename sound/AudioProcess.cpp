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

#include "AudioProcess.h"

#include "config.h"

#include "RunnablePluginInstance.h"
#include "PlayableAudioFile.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "Profiler.h"
#include "AudioLevel.h"
#include "AudioPlayQueue.h"
#include "PluginFactory.h"

#include <sys/time.h>
#include <pthread.h>

#include <cmath>

//#define DEBUG_THREAD_CREATE_DESTROY 1
//#define DEBUG_BUSS_MIXER 1
//#define DEBUG_MIXER 1
//#define DEBUG_MIXER_LIGHTWEIGHT 1
//#define DEBUG_LOCKS 1
//#define DEBUG_READER 1
//#define DEBUG_WRITER 1

namespace Rosegarden
{

AudioThread::AudioThread(std::string name,
			 SoundDriver *driver,
			 unsigned int sampleRate) :
    m_name(name),
    m_driver(driver),
    m_sampleRate(sampleRate),
    m_thread(0),
    m_running(false),
    m_exiting(false)
{
    pthread_mutex_t  initialisingMutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&m_lock, &initialisingMutex, sizeof(pthread_mutex_t));

    pthread_cond_t initialisingCondition = PTHREAD_COND_INITIALIZER;
    memcpy(&m_condition, &initialisingCondition, sizeof(pthread_cond_t));
}

AudioThread::~AudioThread()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << "AudioThread::~AudioThread()" << std::endl;
#endif

    if (m_thread) {
	pthread_mutex_destroy(&m_lock);
	m_thread = 0;
    }

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << "AudioThread::~AudioThread() exiting" << std::endl;
#endif
}

void
AudioThread::run()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << m_name << "::run()" << std::endl;
#endif

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int priority = getPriority();

    if (priority > 0) {

	if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {

	    std::cerr << m_name << "::run: WARNING: couldn't set FIFO scheduling "
		      << "on new thread" << std::endl;
	    pthread_attr_init(&attr); // reset to safety

	} else {
	
	    struct sched_param param;
	    memset(&param, 0, sizeof(struct sched_param));
	    param.sched_priority = priority;
	    
	    if (pthread_attr_setschedparam(&attr, &param)) {
		std::cerr << m_name << "::run: WARNING: couldn't set priority "
			  << priority << " on new thread" << std::endl;
		pthread_attr_init(&attr); // reset to safety
	    }
	}
    }

    int rv = pthread_create(&m_thread, &attr, staticThreadRun, this);

    if (rv != 0 && priority > 0) {
	std::cerr << m_name << "::run: WARNING: unable to start RT thread;"
		  << "\ntrying again with normal scheduling" << std::endl;
	pthread_attr_init(&attr);
	rv = pthread_create(&m_thread, &attr, staticThreadRun, this);
    }

    if (rv != 0) {
	std::cerr << m_name << "::run: ERROR: failed to start thread!" << std::endl;
    }	

    m_running = true;

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << m_name << "::run() done" << std::endl;
#endif
}

void
AudioThread::terminate()
{
#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::string name = m_name;
    std::cerr << name << "::terminate()" << std::endl;
#endif

    m_running = false;

    if (m_thread) {

	pthread_cancel(m_thread);

#ifdef DEBUG_THREAD_CREATE_DESTROY
	std::cerr << name << "::terminate(): cancel requested" << std::endl;
#endif

	int rv = pthread_join(m_thread, 0);

#ifdef DEBUG_THREAD_CREATE_DESTROY
	std::cerr << name << "::terminate(): thread exited with return value " << rv << std::endl;
#endif
    }

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << name << "::terminate(): done" << std::endl;
#endif
}    

void *
AudioThread::staticThreadRun(void *arg)
{
    AudioThread *inst = static_cast<AudioThread *>(arg);
    if (!inst) return 0;

    pthread_cleanup_push(staticThreadCleanup, arg);
    
    inst->getLock();
    inst->threadRun();

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << inst->m_name << "::staticThreadRun(): threadRun exited" << std::endl;
#endif

    inst->releaseLock();
    pthread_cleanup_pop(0);

    return 0;
}

void 
AudioThread::staticThreadCleanup(void *arg)
{
    AudioThread *inst = static_cast<AudioThread *>(arg);
    if (!inst || inst->m_exiting) return;

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::string name = inst->m_name;
    std::cerr << name << "::staticThreadCleanup()" << std::endl;
#endif

    inst->m_exiting = true;
    inst->releaseLock();

#ifdef DEBUG_THREAD_CREATE_DESTROY
    std::cerr << name << "::staticThreadCleanup() done" << std::endl;
#endif
}

int
AudioThread::getLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << m_name << "::getLock()" << std::endl;
#endif
    rv = pthread_mutex_lock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

int
AudioThread::tryLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << m_name << "::tryLock()" << std::endl;
#endif
    rv = pthread_mutex_trylock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK (rv is " << rv << ")" << std::endl;
#endif
    return rv;
}

int
AudioThread::releaseLock()
{
    int rv;
#ifdef DEBUG_LOCKS
    std::cerr << m_name << "::releaseLock()" << std::endl;
#endif
    rv = pthread_mutex_unlock(&m_lock);
#ifdef DEBUG_LOCKS
    std::cerr << "OK" << std::endl;
#endif
    return rv;
}

void
AudioThread::signal()
{
#ifdef DEBUG_LOCKS
    std::cerr << m_name << "::signal()" << std::endl;
#endif
    pthread_cond_signal(&m_condition);
}


AudioBussMixer::AudioBussMixer(SoundDriver *driver,
			       AudioInstrumentMixer *instrumentMixer,
			       unsigned int sampleRate, 
			       unsigned int blockSize) :
    AudioThread("AudioBussMixer", driver, sampleRate),
    m_instrumentMixer(instrumentMixer),
    m_blockSize(blockSize),
    m_bussCount(0)
{ 
    // nothing else here
}

AudioBussMixer::~AudioBussMixer()
{
    for (unsigned int i = 0; i < m_processBuffers.size(); ++i) {
	delete[] m_processBuffers[i];
    }
}

AudioBussMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i) delete buffers[i];
}

void
AudioBussMixer::generateBuffers()
{
#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::generateBuffers" << std::endl;
#endif

    // This returns one too many, as the master is counted as buss 0
    m_bussCount =
	m_driver->getMappedStudio()->getObjectCount(MappedStudio::AudioBuss) - 1;

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::generateBuffers: have " << m_bussCount << " busses" << std::endl;
#endif

    //!!! should be different
    RealTime bufferLength = m_driver->getAudioMixBufferLength();
    int bufferSamples = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;

    for (int i = 0; i < m_bussCount; ++i) {
	
	BufferRec &rec = m_bufferMap[i];

	if (rec.buffers.size() == 2) continue;

	for (unsigned int ch = 0; ch < 2; ++ch) {
	    RingBuffer<sample_t> *rb = new RingBuffer<sample_t>(bufferSamples);
	    if (!rb->mlock()) {
//		std::cerr << "WARNING: AudioBussMixer::generateBuffers: couldn't lock ring buffer into real memory, performance may be impaired" << std::endl;
	    }
	    rec.buffers.push_back(rb);
	}

	MappedAudioBuss *mbuss =
	    m_driver->getMappedStudio()->getAudioBuss(i + 1); // master is 0
	
	if (mbuss) {

	    float level = 0.0;
	    (void)mbuss->getProperty(MappedAudioBuss::Level, level);
	    
	    float pan = 0.0;
	    (void)mbuss->getProperty(MappedAudioBuss::Pan, pan);
	    
	    setBussLevels(i + 1, level, pan);
	}
    }

    if (m_processBuffers.size() == 0) {
	m_processBuffers.push_back(new sample_t[m_blockSize]);
	m_processBuffers.push_back(new sample_t[m_blockSize]);
    }
}	

void
AudioBussMixer::fillBuffers(const RealTime &currentTime)
{
#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::fillBuffers" << std::endl;
#endif
    emptyBuffers();
    m_instrumentMixer->fillBuffers(currentTime);
    kick();
}

void
AudioBussMixer::emptyBuffers()
{
    getLock();

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::emptyBuffers" << std::endl;
#endif

    generateBuffers();

    for (int i = 0; i < m_bussCount; ++i) {
	m_bufferMap[i].dormant = true;
	for (int ch = 0; ch < 2; ++ch) {
	    if (int(m_bufferMap[i].buffers.size()) > ch) {
		m_bufferMap[i].buffers[ch]->reset();
	    }
	}
    }

    releaseLock();
}
    
void
AudioBussMixer::kick(bool wantLock)
{
    if (wantLock) getLock();

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::kick" << std::endl;
#endif

    processBlocks();

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::kick: processed" << std::endl;
#endif

    m_instrumentMixer->signal();
    
    if (wantLock) releaseLock();
}
	    	    
void
AudioBussMixer::setBussLevels(int bussId, float dB, float pan)
{
    if (bussId == 0) return; // master
    int buss = bussId - 1;

    BufferRec &rec = m_bufferMap[buss];

    float volume = AudioLevel::dB_to_multiplier(dB);
    
    rec.gainLeft  = volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
    rec.gainRight = volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
}

void
AudioBussMixer::processBlocks()
{
    if (m_bussCount == 0) return;

#ifdef DEBUG_BUSS_MIXER
    if (m_driver->isPlaying())
	std::cerr << "AudioBussMixer::processBlocks" << std::endl;
#endif

    std::set<InstrumentId> processedInstruments;

    int minBlocks = 0;
    bool haveMinBlocks = false;

    for (int buss = 0; buss < m_bussCount; ++buss) {

	MappedAudioBuss *mbuss =
	    m_driver->getMappedStudio()->getAudioBuss(buss + 1); // master is 0

	if (!mbuss) {
#ifdef DEBUG_BUSS_MIXER
	    std::cerr << "AudioBussMixer::processBlocks: buss " << buss << " not found" << std::endl;
#endif
	    continue;
	}

	BufferRec &rec = m_bufferMap[buss];
	
	float gain[2];
	gain[0] = rec.gainLeft;
	gain[1] = rec.gainRight;

	std::vector<InstrumentId> instruments = mbuss->getInstruments();

#ifdef DEBUG_BUSS_MIXER
	std::cerr << "AudioBussMixer::processBlocks: buss " << buss << ": " << instruments.size() << " instruments" << std::endl;
#endif

	// The dormant calculation here depends on the buffer length
	// for this mixer being the same as that for the instrument mixer

	size_t minSpace = 0;

	for (int ch = 0; ch < 2; ++ch) {

	    size_t w = rec.buffers[ch]->getWriteSpace();
	    if (ch == 0 || w < minSpace) minSpace = w;

#ifdef DEBUG_BUSS_MIXER
	    std::cerr << "AudioBussMixer::processBlocks: buss " << buss << ": write space " << w << " on channel " << ch << std::endl;
#endif

	    if (minSpace == 0) break;
	    
	    for (std::vector<InstrumentId>::iterator ii = instruments.begin();
		 ii != instruments.end(); ++ii) {

		if (m_instrumentMixer->isInstrumentEmpty(*ii)) continue;

		RingBuffer<sample_t, 2> *rb =
		    m_instrumentMixer->getRingBuffer(*ii, ch);
		if (rb) {
		    size_t r = rb->getReadSpace(1);
		    if (r < minSpace) minSpace = r;

#ifdef DEBUG_BUSS_MIXER
		    if (*ii == 1000) {
			std::cerr << "AudioBussMixer::processBlocks: buss " << buss << ": read space " << r << " on instrument " << *ii << ", channel " << ch << std::endl;
		    }
#endif

		    if (minSpace == 0) break;
		}
	    }

	    if (minSpace == 0) break;
	}

	int blocks = minSpace / m_blockSize;
	if (!haveMinBlocks || (blocks < minBlocks)) {
	    minBlocks = blocks;
	    haveMinBlocks = true;
	}
	
#ifdef DEBUG_BUSS_MIXER
	if (m_driver->isPlaying())
	    std::cerr << "AudioBussMixer::processBlocks: doing " << blocks << " blocks at block size " << m_blockSize << std::endl;
#endif

	for (int block = 0; block < blocks; ++block) {

	    memset(m_processBuffers[0], 0, m_blockSize * sizeof(sample_t));
	    memset(m_processBuffers[1], 0, m_blockSize * sizeof(sample_t));

	    bool dormant = true;

	    for (std::vector<InstrumentId>::iterator ii = instruments.begin();
		 ii != instruments.end(); ++ii) {

		if (processedInstruments.find(*ii) !=
		    processedInstruments.end()) {
		    // we aren't set up to process any instrument to
		    // more than one buss
		    continue;
		} else {
		    processedInstruments.insert(*ii);
		}

		if (m_instrumentMixer->isInstrumentEmpty(*ii)) continue;

		if (m_instrumentMixer->isInstrumentDormant(*ii)) {

		    for (int ch = 0; ch < 2; ++ch) {
			RingBuffer<sample_t, 2> *rb =
			    m_instrumentMixer->getRingBuffer(*ii, ch);

			if (rb) rb->skip(m_blockSize,
					 1);
		    }
		} else {
		    dormant = false;

		    for (int ch = 0; ch < 2; ++ch) {
			RingBuffer<sample_t, 2> *rb =
			    m_instrumentMixer->getRingBuffer(*ii, ch);

			if (rb) rb->readAdding(m_processBuffers[ch],
					       m_blockSize,
					       1);
		    }
		}
	    }

	    for (int ch = 0; ch < 2; ++ch) {
		if (dormant) {
		    rec.buffers[ch]->zero(m_blockSize);
		} else {
		    for (size_t i = 0; i < m_blockSize; ++i) {
			m_processBuffers[ch][i] *= gain[ch];
		    }
		    rec.buffers[ch]->write(m_processBuffers[ch], m_blockSize);
		}
	    }

	    rec.dormant = dormant;

#ifdef DEBUG_BUSS_MIXER
	if (m_driver->isPlaying())
	    std::cerr << "AudioBussMixer::processBlocks: buss " << buss << (dormant ? " dormant" : " not dormant") << std::endl;
#endif
	}
    }

    // any unprocessed instruments need to be skipped, or they'll block

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    if (int(processedInstruments.size()) != audioInstruments + synthInstruments) {

	for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

	    InstrumentId id;
	    if (i < audioInstruments) id = audioInstrumentBase + i;
	    else id = synthInstrumentBase + (i - audioInstruments);

	    if (m_instrumentMixer->isInstrumentEmpty(id)) continue;

	    if (processedInstruments.find(id) == processedInstruments.end()) {
		for (int ch = 0; ch < 2; ++ch) {
		    RingBuffer<sample_t, 2> *rb =
			m_instrumentMixer->getRingBuffer(id, ch);

		    if (rb) rb->skip(m_blockSize * minBlocks,
				     1);
		}
	    }
	}
    }
    

#ifdef DEBUG_BUSS_MIXER
    std::cerr << "AudioBussMixer::processBlocks: done" << std::endl;
#endif
}

void
AudioBussMixer::threadRun()
{
    while (!m_exiting) {

	if (m_driver->areClocksRunning()) {
	    kick(false);
	}

	RealTime t = m_driver->getAudioMixBufferLength();
	t = t / 2;
	if (t < RealTime(0, 10000000)) t = RealTime(0, 10000000); // 10ms minimum

	struct timeval now;
	gettimeofday(&now, 0);
	t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	struct timespec timeout;
	timeout.tv_sec = t.sec;
	timeout.tv_nsec = t.nsec;

	pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
	pthread_testcancel();
    }
}


AudioInstrumentMixer::AudioInstrumentMixer(SoundDriver *driver,
					   AudioFileReader *fileReader,
					   unsigned int sampleRate,
					   unsigned int blockSize) :
    AudioThread("AudioInstrumentMixer", driver, sampleRate),
    m_fileReader(fileReader),
    m_bussMixer(0),
    m_blockSize(blockSize)
{
    // Leave the buffer map and process buffer list empty for now.
    // The number of channels per fader can change between plays, so
    // we always examine the buffers in fillBuffers and are prepared
    // to regenerate from scratch if necessary.
}

AudioInstrumentMixer::~AudioInstrumentMixer()
{
    std::cerr << "AudioInstrumentMixer::~AudioInstrumentMixer" << std::endl;
    // BufferRec dtor will handle the BufferMap

    removeAllPlugins();
    
    for (std::vector<sample_t *>::iterator i = m_processBuffers.begin();
	 i != m_processBuffers.end(); ++i) {
	delete *i;
    }

    std::cerr << "AudioInstrumentMixer::~AudioInstrumentMixer exiting" << std::endl;
}

AudioInstrumentMixer::BufferRec::~BufferRec()
{
    for (size_t i = 0; i < buffers.size(); ++i) delete buffers[i];
}


void
AudioInstrumentMixer::setPlugin(InstrumentId id, int position, QString identifier)
{
    getLock();

    std::cerr << "AudioInstrumentMixer::setPlugin(" << id << ", " << position << ", " << identifier << ")" << std::endl;

    int channels = 2;
    if (m_bufferMap.find(id) != m_bufferMap.end()) {
	channels = m_bufferMap[id].channels;
    }

    RunnablePluginInstance *instance = 0;

    PluginFactory *factory = PluginFactory::instanceFor(identifier);
    if (factory) {
	instance = factory->instantiatePlugin(identifier,
					      id,
					      position,
					      m_sampleRate,
					      m_blockSize,
					      channels);
	if (instance && !instance->isOK()) {
	    std::cerr << "AudioInstrumentMixer::setPlugin(" << id << ", " << position
		      << ": instance is not OK" << std::endl;
	    delete instance;
	    instance = 0;
	}
    }

    RunnablePluginInstance *oldInstance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {

	oldInstance = m_synths[id];
	m_synths[id] = instance;

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    oldInstance = i->second;
	}

	m_plugins[id][position] = instance;
    }

    if (instance) {
	instance->activate();
    }

    if (oldInstance) {
	oldInstance->deactivate();
	delete oldInstance;
    }

    releaseLock();
}
    
void 
AudioInstrumentMixer::removePlugin(InstrumentId id, int position)
{
    getLock();

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {

	if (m_synths[id]) {
	    m_synths[id]->deactivate();
	    delete m_synths[id];
	    m_synths[id] = 0;
	}

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {

	    RunnablePluginInstance *instance = i->second;

	    if (instance) {
		instance->deactivate();
		delete instance;
	    }

	    m_plugins[id].erase(i);
	    releaseLock();
	    return;
	}
    }

    releaseLock();
}

void
AudioInstrumentMixer::removeAllPlugins()
{
    getLock();

    for (SynthPluginMap::iterator i = m_synths.begin();
	 i != m_synths.end(); ++i) {
	if (i->second) {
	    i->second->deactivate();
	    delete i->second;
	}
    }
    m_synths.clear();

    for (PluginMap::iterator j = m_plugins.begin();
	 j != m_plugins.end(); ++j) {

	InstrumentId id = j->first;

	for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end();) {

	    RunnablePluginInstance *instance = i->second;

	    if (instance) {
		instance->deactivate();
		delete instance;
	    }

	    PluginList::iterator k = i;
	    ++k;
	    m_plugins[id].erase(i);
	    i = k;
	}
    }

    releaseLock();
}


void
AudioInstrumentMixer::setPluginPortValue(InstrumentId id, int position,
			       unsigned int port, float value)
{
    //!!! surely we don't need a lock for something like this?
    getLock();
    
    RunnablePluginInstance *instance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
	
	instance = m_synths[id];

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    instance = i->second;
	}
    }

    if (instance) {
	std::cerr << "Setting plugin port " << port << " to value " << value << std::endl;
	instance->setPortValue(port, value);
    }
    
    releaseLock();
}

void
AudioInstrumentMixer::setPluginBypass(InstrumentId id, int position, bool bypass)
{
    getLock();
    
    RunnablePluginInstance *instance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
	
	instance = m_synths[id];

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    instance = i->second;
	}
    }
    
    if (instance) instance->setBypassed(bypass);
    
    releaseLock();
}

QStringList
AudioInstrumentMixer::getPluginPrograms(InstrumentId id, int position)
{
    QStringList programs;

    getLock();
    
    RunnablePluginInstance *instance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
	
	instance = m_synths[id];

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    instance = i->second;
	}
    }
    
    if (instance) programs = instance->getPrograms();
    
    releaseLock();

    return programs;
}

QString
AudioInstrumentMixer::getPluginProgram(InstrumentId id, int position)
{
    QString program;

    getLock();
    
    RunnablePluginInstance *instance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
	
	instance = m_synths[id];

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    instance = i->second;
	}
    }
    
    if (instance) program = instance->getCurrentProgram();
    
    releaseLock();

    return program;
}

void
AudioInstrumentMixer::setPluginProgram(InstrumentId id, int position, QString program)
{
    getLock();
    
    RunnablePluginInstance *instance = 0;

    if (position == int(Instrument::SYNTH_PLUGIN_POSITION)) {
	
	instance = m_synths[id];

    } else {

	PluginList::iterator i = m_plugins[id].find(position);
	if (i != m_plugins[id].end()) {
	    instance = i->second;
	}
    }
    
    if (instance) instance->selectProgram(program);
    
    releaseLock();
}

void
AudioInstrumentMixer::resetAllPlugins()
{
    getLock();

    std::cerr << "AudioInstrumentMixer::resetAllPlugins!" << std::endl;

    for (SynthPluginMap::iterator j = m_synths.begin();
	 j != m_synths.end(); ++j) {

	InstrumentId id = j->first;

	int channels = 2;
	if (m_bufferMap.find(id) != m_bufferMap.end()) {
	    channels = m_bufferMap[id].channels;
	}

	RunnablePluginInstance *instance = j->second;

	if (instance) {
	    instance->deactivate();
	    std::cerr << "AudioInstrumentMixer::resetAllPlugins: setting " << channels << " channels on synth for instrument " << id << std::endl;
	    instance->setIdealChannelCount(channels);
	    instance->activate();
	}
    }	

    for (PluginMap::iterator j = m_plugins.begin();
	 j != m_plugins.end(); ++j) {

	InstrumentId id = j->first;

	int channels = 2;
	if (m_bufferMap.find(id) != m_bufferMap.end()) {
	    channels = m_bufferMap[id].channels;
	}

	for (PluginList::iterator i = m_plugins[id].begin();
	     i != m_plugins[id].end(); ++i) {

	    RunnablePluginInstance *instance = i->second;

	    if (instance) {
		instance->deactivate();
		std::cerr << "AudioInstrumentMixer::resetAllPlugins: setting " << channels << " channels on plugin for instrument " << id << std::endl;
		instance->setIdealChannelCount(channels);
		instance->activate();
	    }
	}
    }

    releaseLock();
}

void
AudioInstrumentMixer::generateBuffers()
{
    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    unsigned int maxChannels = 0;

    RealTime bufferLength = m_driver->getAudioMixBufferLength();
    int bufferSamples = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / m_blockSize) + 1) * m_blockSize;

#ifdef DEBUG_MIXER
    std::cerr << "AudioInstrumentMixer::generateBuffers: Buffer length is " << bufferLength << "; buffer samples " << bufferSamples << " (sample rate " << m_sampleRate << ")" << std::endl;
#endif
    
    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

	InstrumentId id;
	if (i < audioInstruments) id = audioInstrumentBase + i;
	else id = synthInstrumentBase + (i - audioInstruments);

	// Get a fader for this instrument - if we can't then this
	// isn't a valid audio track.
	MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader(id);

	if (!fader) {
#ifdef DEBUG_MIXER
	    std::cerr << "AudioInstrumentMixer::generateBuffers: no fader for audio instrument " << id << std::endl;
#endif
	    continue;
	}

	float fch = 2;
	(void)fader->getProperty(MappedAudioFader::Channels, fch);
	unsigned int channels = (unsigned int)fch;

	BufferRec &rec = m_bufferMap[id];

	rec.channels = channels;

	// We always have stereo buffers (for output of pan)
	// even on a mono instrument.
	if (channels < 2) channels = 2;
	if (channels > maxChannels) maxChannels = channels;

	for (size_t i = 0; i < rec.buffers.size(); ++i) {
	    delete rec.buffers[i];
	}
	rec.buffers.clear();

	for (unsigned int i = 0; i < channels; ++i) {

	    // All our ringbuffers are set up for two readers: the
	    // buss mix thread and the main process thread for
	    // e.g. JACK.  The main process thread gets the zero-id
	    // reader, so it gets the same API as if this was a
	    // single-reader buffer; the buss mixer has to remember to
	    // explicitly request reader 1.

	    RingBuffer<sample_t, 2> *rb =
		new RingBuffer<sample_t, 2>(bufferSamples);

	    if (!rb->mlock()) {
//		std::cerr << "WARNING: AudioInstrumentMixer::generateBuffers: couldn't lock ring buffer into real memory, performance may be impaired" << std::endl;
	    }
	    rec.buffers.push_back(rb);
	}	    

	float level = 0.0;
	(void)fader->getProperty(MappedAudioFader::FaderLevel, level);

	float pan = 0.0;
	(void)fader->getProperty(MappedAudioFader::Pan, pan);

	setInstrumentLevels(id, level, pan);
    }

    while (m_processBuffers.size() > maxChannels) {
	std::vector<sample_t *>::iterator bi = m_processBuffers.end();
	--bi;
	delete[] *bi;
	m_processBuffers.erase(bi);
    }
    while (m_processBuffers.size() < maxChannels) {
	m_processBuffers.push_back(new sample_t[m_blockSize]);
    }
}

void
AudioInstrumentMixer::fillBuffers(const RealTime &currentTime)
{
    emptyBuffers(currentTime);

    getLock();

#ifdef DEBUG_MIXER
    std::cerr << "AudioInstrumentMixer::fillBuffers(" << currentTime <<")" << std::endl;
#endif

    bool discard;
    processBlocks(discard);

    releaseLock();
}

void
AudioInstrumentMixer::emptyBuffers(RealTime currentTime)
{
    getLock();

#ifdef DEBUG_MIXER
    std::cerr << "AudioInstrumentMixer::emptyBuffers(" << currentTime <<")" << std::endl;
#endif

    generateBuffers();

    InstrumentId audioInstrumentBase;
    int audioInstruments;
    m_driver->getAudioInstrumentNumbers(audioInstrumentBase, audioInstruments);

    InstrumentId synthInstrumentBase;
    int synthInstruments;
    m_driver->getSoftSynthInstrumentNumbers(synthInstrumentBase, synthInstruments);

    for (int i = 0; i < audioInstruments + synthInstruments; ++i) {

	InstrumentId id;
	if (i < audioInstruments) id = audioInstrumentBase + i;
	else id = synthInstrumentBase + (i - audioInstruments);

	m_bufferMap[id].dormant = true;
	m_bufferMap[id].zeroFrames = 0;
	m_bufferMap[id].filledTo = currentTime;

	for (size_t i = 0; i < m_bufferMap[id].buffers.size(); ++i) {
	    m_bufferMap[id].buffers[i]->reset();
	}
    }

    releaseLock();
}

/* Branch-free optimizer-resistant denormal killer courtesy of Simon
   Jenkins on LAD: */

static inline float flushToZero(volatile float f)
{
   f += 9.8607615E-32f;
   return f - 9.8607615E-32f;
}

static inline void denormalKill(float *buffer, int size)
{
    for (int i = 0; i < size; ++i) {
	buffer[i] = flushToZero(buffer[i]);
    }
}

void
AudioInstrumentMixer::setInstrumentLevels(InstrumentId id, float dB, float pan)
{
    BufferRec &rec = m_bufferMap[id];
    
    float volume = AudioLevel::dB_to_multiplier(dB);

    rec.gainLeft  = volume * ((pan > 0.0) ? (1.0 - (pan / 100.0)) : 1.0);
    rec.gainRight = volume * ((pan < 0.0) ? ((pan + 100.0) / 100.0) : 1.0);
    rec.volume = volume;
}

void
AudioInstrumentMixer::processBlocks(bool &readSomething)
{
#ifdef DEBUG_MIXER
    if (m_driver->isPlaying())
	std::cerr << "AudioInstrumentMixer::processBlocks" << std::endl;
#endif

    const AudioPlayQueue *queue = m_driver->getAudioQueue();

    for (BufferMap::iterator i = m_bufferMap.begin();
	 i != m_bufferMap.end(); ++i) {

	InstrumentId id = i->first;
	BufferRec &rec = i->second;

	rec.empty = (m_plugins[id].empty() && m_synths[id] == 0 &&
		     !queue->haveFilesForInstrument(id));

	// For a while we were setting empty to true if the volume on
	// the track was zero, but that breaks continuity if there is
	// actually a file on the track -- processEmptyBlocks won't
	// read it, so it'll fall behind if we put the volume up again.
    }

    bool more = true;
    
    AudioPlayQueue::FileSet playing;

    RealTime blockDuration = RealTime::frame2RealTime(m_blockSize, m_sampleRate);

    while (more) {

	more = false;

	for (BufferMap::iterator i = m_bufferMap.begin();
	     i != m_bufferMap.end(); ++i) {

	    InstrumentId id = i->first;
	    BufferRec &rec = i->second;

	    if (rec.empty) {
//!!!		processEmptyBlocks(id);
		continue;
	    }

	    queue->getPlayingFilesForInstrument(rec.filledTo,
						blockDuration,
						id, playing);
	    
	    if (processBlock(id, playing, readSomething)) {
		more = true;
	    }
	}
    }
}


void
AudioInstrumentMixer::processEmptyBlocks(InstrumentId id)
{
#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	if (id == 1000) std::cerr << "AudioInstrumentMixer::processEmptyBlocks(" << id << ")" << std::endl;
    }
#endif
  
    BufferRec &rec = m_bufferMap[id];
    unsigned int channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) return; // buffers just haven't been set up yet

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    size_t minWriteSpace = 0;
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	size_t thisWriteSpace = rec.buffers[ch]->getWriteSpace();
	if (ch == 0 || thisWriteSpace < minWriteSpace) {
	    minWriteSpace = thisWriteSpace;
	    if (minWriteSpace < m_blockSize) {
#ifdef DEBUG_MIXER
		if (m_driver->isPlaying()) {
		    if (id == 1000) std::cerr << "AudioInstrumentMixer::processEmptyBlocks(" << id << "): only " << minWriteSpace << " write space on channel " << ch << " for block size " << m_blockSize << std::endl;
		}
#endif
		return;
	    }
	}
    }

    // unlike processBlock, we can really fill this one up here (at
    // least to the nearest block multiple, just to make things easier
    // to understand) because it's so cheap

    size_t toWrite = (minWriteSpace / m_blockSize) * m_blockSize;

    rec.zeroFrames += toWrite;
    bool dormant = true;
                
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	rec.buffers[ch]->zero(toWrite);
	if (rec.buffers[ch]->getReadSpace() > rec.zeroFrames) dormant = false;
    }

    rec.dormant = dormant;
    rec.filledTo = rec.filledTo +
	RealTime::frame2RealTime(toWrite, m_sampleRate);
}
	    

bool
AudioInstrumentMixer::processBlock(InstrumentId id,
				   AudioPlayQueue::FileSet &audioQueue,
				   bool &readSomething)
{
    BufferRec &rec = m_bufferMap[id];
    RealTime bufferTime = rec.filledTo;

#ifdef DEBUG_MIXER
    if (m_driver->isPlaying()) {
	if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): buffer time is " << bufferTime << std::endl;
    }
#endif

    unsigned int channels = rec.channels;
    if (channels > rec.buffers.size()) channels = rec.buffers.size();
    if (channels > m_processBuffers.size()) channels = m_processBuffers.size();
    if (channels == 0) {
#ifdef DEBUG_MIXER
	if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): nominal channels " << rec.channels << ", ring buffers " << rec.buffers.size() << ", process buffers " << m_processBuffers.size() << std::endl;
#endif
	return false; // buffers just haven't been set up yet
    }

    unsigned int targetChannels = channels;
    if (targetChannels < 2) targetChannels = 2; // fill at least two buffers

    size_t minWriteSpace = 0;
    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	size_t thisWriteSpace = rec.buffers[ch]->getWriteSpace();
	if (ch == 0 || thisWriteSpace < minWriteSpace) {
	    minWriteSpace = thisWriteSpace;
	    if (minWriteSpace < m_blockSize) {
#ifdef DEBUG_MIXER
		if (m_driver->isPlaying()) {
		    if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): only " << minWriteSpace << " write space on channel " << ch << " for block size " << m_blockSize << std::endl;
		}
#endif
		return false;
	    }  
	}
    }

    PluginList &plugins = m_plugins[id];
#ifdef DEBUG_MIXER
    if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id << "): have " << plugins.size() << " plugin(s)" << std::endl;
#endif

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && m_driver->isPlaying()) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): minWriteSpace is " << minWriteSpace << std::endl;
#else
#ifdef DEBUG_MIXER_LIGHTWEIGHT
    if ((id % 100) == 0 && m_driver->isPlaying()) std::cout << minWriteSpace << "/" << rec.buffers[0]->getSize() << std::endl;
#endif
#endif

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && audioQueue.size() > 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): " << audioQueue.size() << " audio file(s) to consider" << std::endl;
#endif

    bool haveBlock = true;
    bool haveMore = false;

    for (AudioPlayQueue::FileSet::iterator it = audioQueue.begin();
	 it != audioQueue.end(); ++it) {

	bool acceptable = false;

	size_t frames = (*it)->getSampleFramesAvailable();
	acceptable =
	    ((frames >= m_blockSize) || (*it)->isFullyBuffered());
	
	if (acceptable &&
	    (minWriteSpace >= m_blockSize * 2) &&
	    (frames >= m_blockSize * 2)) {
	    
#ifdef DEBUG_MIXER
	    if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): will be asking for more" << std::endl;
#endif
	    
	    haveMore = true;
	}

#ifdef DEBUG_MIXER
	if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): file has " << frames << " frames available" << std::endl;
#endif

	if (!acceptable) {
	    haveBlock = false;
	}
    }
	
    if (!haveBlock) {
	//!!! Is this actually a problem? It should mean we just block
	// on this one instrument and return to it a little later.
//	std::cerr << "WARNING: buffer underrun in file ringbuffer for instrument " << id << std::endl;
//	m_driver->reportFailure(MappedEvent::FailureDiscUnderrun);
	return false; // blocked
    }

#ifdef DEBUG_MIXER
    if (!haveMore) {
	if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): won't be asking for more" << std::endl;
    }
#endif

    for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	memset(m_processBuffers[ch], 0, sizeof(sample_t) * m_blockSize);
    }
	
    RunnablePluginInstance *synth = m_synths[id];

    if (synth && !synth->isBypassed()) {

	synth->run(bufferTime);

	unsigned int ch = 0;

	while (ch < synth->getAudioOutputCount() && ch < channels) {
	    denormalKill(synth->getAudioOutputBuffers()[ch],
			 m_blockSize);
	    memcpy(m_processBuffers[ch],
		   synth->getAudioOutputBuffers()[ch],
		   m_blockSize * sizeof(sample_t));
	    ++ch;
	}
    }

    if (haveBlock) {
	    
	// Mix in a block from each playing file on this instrument.
	    
	for (AudioPlayQueue::FileSet::iterator it = audioQueue.begin();
	     it != audioQueue.end(); ++it) {

	    size_t offset = 0;
	    size_t blockSize = m_blockSize;

	    if ((*it)->getStartTime() > bufferTime) {
		offset = RealTime::realTime2Frame
		    ((*it)->getStartTime() - bufferTime, m_sampleRate);
		if (offset < blockSize) blockSize -= offset;
		else blockSize = 0;
#ifdef DEBUG_MIXER
		std::cerr << "AudioInstrumentMixer::processBlock: file starts at offset " << offset << ", block size now " << blockSize << std::endl;
#endif
	    }

	    //!!! This addSamples call is what is supposed to signal
	    // to a playable audio file when the end of the file has
	    // been reached.  But for some playables it appears the
	    // file overruns, possibly due to rounding errors in
	    // sample rate conversion, and so we stop reading from it
	    // before it's actually done.  I don't particularly mind
	    // that from a sound quality POV (after all it's badly
	    // resampled already) but unfortunately it means we leak
	    // pooled buffers.

	    if (blockSize > 0) {
		(*it)->addSamples(m_processBuffers, channels, blockSize, offset);
		readSomething = true;
	    }
	}
    }

    // Apply plugins.  There are various copy-reducing
    // optimisations available here, but we're not even going to
    // think about them yet.  Note that we force plugins to mono
    // on a mono track, even though we have stereo output buffers
    // -- stereo only comes into effect at the pan stage, and
    // these are pre-fader plugins.

    for (PluginList::iterator pli = plugins.begin();
	 pli != plugins.end(); ++pli) {

	RunnablePluginInstance *plugin = pli->second;
	if (!plugin || plugin->isBypassed()) continue;

	unsigned int ch = 0;

	// If a plugin has more input channels than we have
	// available, we duplicate up to stereo and leave any
	// remaining channels empty.

	while (ch < plugin->getAudioInputCount()) {

	    if (ch < channels || ch < 2) {
		memcpy(plugin->getAudioInputBuffers()[ch],
		       m_processBuffers[ch % channels],
		       m_blockSize * sizeof(sample_t));
	    } else {
		memset(plugin->getAudioInputBuffers()[ch], 0,
		       m_blockSize * sizeof(sample_t));
	    }
	    ++ch;
	}

#ifdef DEBUG_MIXER
	std::cerr << "Running plugin with " << plugin->getAudioInputCount()
		  << " inputs, " << plugin->getAudioOutputCount() << " outputs" << std::endl;
#endif

	plugin->run(bufferTime);

	ch = 0;

	while (ch < plugin->getAudioOutputCount()) {

	    denormalKill(plugin->getAudioOutputBuffers()[ch],
			 m_blockSize);

	    if (ch < channels) {
		memcpy(m_processBuffers[ch],
		       plugin->getAudioOutputBuffers()[ch],
		       m_blockSize * sizeof(sample_t));
	    } else if (ch == 1) {
		// stereo output from plugin on a mono track
		for (size_t i = 0; i < m_blockSize; ++i) {
		    m_processBuffers[0][i] +=
			plugin->getAudioOutputBuffers()[ch][i];
		    m_processBuffers[0][i] /= 2;
		}
	    } else {
		break;
	    }

	    ++ch;
	}
    }

    // special handling for pan on mono tracks

    bool allZeros = true;

    if (targetChannels == 2 && channels == 1) {

	for (size_t i = 0; i < m_blockSize; ++i) {

	    sample_t sample = m_processBuffers[0][i];

	    m_processBuffers[0][i] = sample * rec.gainLeft;
	    m_processBuffers[1][i] = sample * rec.gainRight;

	    if (allZeros && sample != 0.0)
		allZeros = false;
	}
		
	rec.buffers[0]->write(m_processBuffers[0], m_blockSize);
	rec.buffers[1]->write(m_processBuffers[1], m_blockSize);

    } else {

	for (unsigned int ch = 0; ch < targetChannels; ++ch) {
		
	    float gain = ((ch == 0) ? rec.gainLeft  :
			  (ch == 1) ? rec.gainRight : rec.volume);

	    for (size_t i = 0; i < m_blockSize; ++i) {

		// handle volume and pan
		m_processBuffers[ch][i] *= gain;

		if (allZeros && m_processBuffers[ch][i] != 0.0)
		    allZeros = false;
	    }
		
	    rec.buffers[ch]->write(m_processBuffers[ch], m_blockSize);
	}
    }

    bool dormant = true;

    if (allZeros) {
	rec.zeroFrames += m_blockSize;
	for (unsigned int ch = 0; ch < targetChannels; ++ch) {
	    if (rec.buffers[ch]->getReadSpace() > rec.zeroFrames) {
		dormant = false;
	    }
	}
    } else {
	rec.zeroFrames = 0;
	dormant = false;
    }

#ifdef DEBUG_MIXER
    if ((id % 100) == 0 && m_driver->isPlaying()) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): setting dormant to " << dormant << std::endl;
#endif

    rec.dormant = dormant;
    bufferTime = bufferTime + RealTime::frame2RealTime(m_blockSize,
						       m_sampleRate);

    rec.filledTo = bufferTime;

#ifdef DEBUG_MIXER
    if ((id % 100) == 0) std::cerr << "AudioInstrumentMixer::processBlock(" << id <<"): done, returning " << haveMore << std::endl;
#endif

    return haveMore;
}

void
AudioInstrumentMixer::kick(bool wantLock)
{
    if (wantLock) getLock();

    bool readSomething = false;
    processBlocks(readSomething);
    if (readSomething) m_fileReader->signal();

    if (wantLock) releaseLock();
}


void
AudioInstrumentMixer::threadRun()
{
    while (!m_exiting) {

	if (m_driver->areClocksRunning()) {
	    kick(false);
	}

	RealTime t = m_driver->getAudioMixBufferLength();
	t = t / 2;
	if (t < RealTime(0, 10000000)) t = RealTime(0, 10000000); // 10ms minimum

	struct timeval now;
	gettimeofday(&now, 0);
	t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	struct timespec timeout;
	timeout.tv_sec = t.sec;
	timeout.tv_nsec = t.nsec;

	pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
	pthread_testcancel();
    }
}



AudioFileReader::AudioFileReader(SoundDriver *driver,
				 unsigned int sampleRate) :
    AudioThread("AudioFileReader", driver, sampleRate)
{
    // nothing else here
}

AudioFileReader::~AudioFileReader()
{
}

void
AudioFileReader::fillBuffers(const RealTime &currentTime)
{
    getLock();

    // Tell every audio file the play start time.

    const AudioPlayQueue *queue = m_driver->getAudioQueue();

    RealTime bufferLength = m_driver->getAudioReadBufferLength();
    int bufferFrames = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    
    PlayableAudioFile::setRingBufferPoolSizes
	(queue->getMaxBuffersRequired() * 2 + 4, bufferFrames);
    
    const AudioPlayQueue::FileSet &files = queue->getAllScheduledFiles();
    
#ifdef DEBUG_READER
    std::cerr << "AudioFileReader::fillBuffers: have " << files.size() << " audio files total" << std::endl;
#endif
    
    for (AudioPlayQueue::FileSet::const_iterator fi = files.begin();
	 fi != files.end(); ++fi) {
	(*fi)->clearBuffers();
    }
    
    for (AudioPlayQueue::FileSet::const_iterator fi = files.begin();
	 fi != files.end(); ++fi) {
	(*fi)->fillBuffers(currentTime);
    }
    
    releaseLock();
}

bool
AudioFileReader::kick(bool wantLock)
{
    if (wantLock) getLock();

    RealTime now = m_driver->getSequencerTime();
    const AudioPlayQueue *queue = m_driver->getAudioQueue();
    
    bool someFilled = false;
	
    // Tell files that are playing or will be playing in the next few
    // seconds to update.

    AudioPlayQueue::FileSet playing;
    
    queue->getPlayingFiles(now, RealTime(1, 0), playing);
    
    for (AudioPlayQueue::FileSet::iterator fi = playing.begin();
	 fi != playing.end(); ++fi) {
	if ((*fi)->updateBuffers()) someFilled = true;
    }
  
    if (wantLock) releaseLock();

    return someFilled;
}

void
AudioFileReader::threadRun()
{
    while (!m_exiting) {

//	struct timeval now;
//	gettimeofday(&now, 0);
//	RealTime t = RealTime(now.tv_sec, now.tv_usec * 1000);

	bool someFilled = false;

	if (m_driver->areClocksRunning()) {
	    someFilled = kick(false);
	}

	if (someFilled) {

	    releaseLock();
	    getLock();

	} else {

	    RealTime bt = m_driver->getAudioReadBufferLength();
	    bt = bt / 2;
	    if (bt < RealTime(0, 10000000)) bt = RealTime(0, 10000000); // 10ms minimum

	    struct timeval now;
	    gettimeofday(&now, 0);
	    RealTime t = bt + RealTime(now.tv_sec, now.tv_usec * 1000);

	    struct timespec timeout;
	    timeout.tv_sec = t.sec;
	    timeout.tv_nsec = t.nsec;

	    pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
	    pthread_testcancel();
	}
    }
}



AudioFileWriter::AudioFileWriter(SoundDriver *driver,
				 unsigned int sampleRate) :
    AudioThread("AudioFileWriter", driver, sampleRate)
{
    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {

	// prefill with zero files in all slots, so that we can
	// refer to the map without a lock (as the number of
	// instruments won't change)
	
	m_files[id] = FilePair(0, 0);
    }
}

AudioFileWriter::~AudioFileWriter()
{
}


bool
AudioFileWriter::createRecordFile(InstrumentId id,
				  const std::string &fileName)
{
    getLock();

    if (m_files[id].first) {
	releaseLock();
	std::cerr << "AudioFileWriter::createRecordFile: already have record file!" << std::endl;
	return false; // already have one
    }
    
    MappedAudioFader *fader = m_driver->getMappedStudio()->getAudioFader
	(m_driver->getAudioMonitoringInstrument());

    RealTime bufferLength = m_driver->getAudioWriteBufferLength();
    int bufferSamples = RealTime::realTime2Frame(bufferLength, m_sampleRate);
    bufferSamples = ((bufferSamples / 1024) + 1) * 1024;

    if (fader)
    {
	float fch = 2;
	(void)fader->getProperty(MappedAudioFader::Channels, fch);
        int channels = (int)fch;

        int bytesPerSample = 2 * channels;
        int bitsPerSample = 16;

        AudioFile *recordFile =
            new WAVAudioFile(fileName,
                             channels,             // channels
                             m_sampleRate,      // samples per second
                             m_sampleRate *
                                  bytesPerSample,  // bytes per second
                             bytesPerSample,       // bytes per sample
                             bitsPerSample);       // bits per sample

        // open the file for writing
        //
	if (!recordFile->write()) {
	    std::cerr << "AudioFileWriter::createRecordFile: failed to open " << fileName << " for writing" << std::endl;
	    delete recordFile;
	    releaseLock();
	    return false;
	}

	RecordableAudioFile *raf = new RecordableAudioFile(recordFile,
							   bufferSamples);
	m_files[id].second = raf;
	m_files[id].first = recordFile;
	releaseLock();
	return true;
    }

    std::cerr << "AudioFileWriter::createRecordFile: no audio fader for record instrument " << m_driver->getAudioMonitoringInstrument() << "!" << std::endl;
    releaseLock();
    return false;
}	


void
AudioFileWriter::write(InstrumentId id,
		       const sample_t *samples,
		       int channel,
		       size_t sampleCount)
{
    if (!m_files[id].first) return; // no file
    if (m_files[id].second->buffer(samples, channel, sampleCount) < sampleCount) {
	m_driver->reportFailure(MappedEvent::FailureDiscOverrun);
    }
}

bool
AudioFileWriter::closeRecordFile(InstrumentId id, AudioFileId &returnedId)
{
    if (!m_files[id].first) return false;

    returnedId = m_files[id].first->getId();
    m_files[id].second->setStatus(RecordableAudioFile::DEFUNCT);
#ifdef DEBUG_WRITER
    std::cerr << "AudioFileWriter::closeRecordFile: instrument " << id << " file set defunct" << std::endl;
#endif
    return true;
}
    

void
AudioFileWriter::kick(bool wantLock)
{
    if (wantLock) getLock();

    InstrumentId instrumentBase;
    int instrumentCount;
    m_driver->getAudioInstrumentNumbers(instrumentBase, instrumentCount);
    
    for (InstrumentId id = instrumentBase;
	 id < instrumentBase + instrumentCount; ++id) {

	if (!m_files[id].first) continue;

	RecordableAudioFile *raf = m_files[id].second;

	if (raf->getStatus() == RecordableAudioFile::DEFUNCT) {

#ifdef DEBUG_WRITER
	    std::cerr << "AudioFileWriter::kick: found defunct file on instrument " << id << std::endl;
#endif

	    m_files[id].first = 0;
	    delete raf; // also deletes the AudioFile
	    m_files[id].second = 0;

	} else {
#ifdef DEBUG_WRITER
	    std::cerr << "AudioFileWriter::kick: writing file on instrument " << id << std::endl;
#endif

	    raf->write();
	}
    }
    
    if (wantLock) releaseLock();
}

void
AudioFileWriter::threadRun()
{
    while (!m_exiting) {

	kick(false);

	RealTime t = m_driver->getAudioWriteBufferLength();
	t = t / 2;
	if (t < RealTime(0, 10000000)) t = RealTime(0, 10000000); // 10ms minimum

	struct timeval now;
	gettimeofday(&now, 0);
	t = t + RealTime(now.tv_sec, now.tv_usec * 1000);

	struct timespec timeout;
	timeout.tv_sec = t.sec;
	timeout.tv_nsec = t.nsec;

	pthread_cond_timedwait(&m_condition, &m_lock, &timeout);
	pthread_testcancel();
    }
}


}

