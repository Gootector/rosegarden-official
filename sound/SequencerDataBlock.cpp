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

#include "SequencerDataBlock.h"
#include "MappedComposition.h"

namespace Rosegarden
{
      
SequencerDataBlock::SequencerDataBlock(bool initialise)
{
    if (initialise) {
	m_controlBlock = 0;
	m_positionSec = 0;
	m_positionNsec = 0;
	m_visualEventIndex = 0;
	*((MappedEvent *)&m_visualEvent) = MappedEvent();
	m_haveVisualEvent = false;
	m_recordEventIndex = 0;
	m_recordLevel.level = 0;
	m_recordLevel.levelRight = 0;
	memset(m_levels, 0,
	       SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS * sizeof(LevelInfo));
    }
}

bool
SequencerDataBlock::getVisual(MappedEvent &ev) const
{
    static int eventIndex = 0;

    if (!m_haveVisualEvent) {
	return false;
    } else {
	int thisEventIndex = m_visualEventIndex;
	if (thisEventIndex == eventIndex) return false;
	ev = *((MappedEvent *)&m_visualEvent);
	eventIndex = thisEventIndex;
	return true;
    }
}

void
SequencerDataBlock::setVisual(const MappedEvent *ev)
{
    m_haveVisualEvent = false;
    if (ev) {
	*((MappedEvent *)&m_visualEvent) = *ev;
	++m_visualEventIndex;
	m_haveVisualEvent = true;
    }
}

int
SequencerDataBlock::getRecordedEvents(MappedComposition &mC) const
{
    static int readIndex = 0;

    int currentIndex = m_recordEventIndex;
    int count = 0;
    
    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    while (readIndex != currentIndex) {
	mC.insert(new MappedEvent(recordBuffer[readIndex]));
	if (++readIndex == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE) readIndex = 0;
	++count;
    }

    return count;
}

void
SequencerDataBlock::addRecordedEvents(MappedComposition *mC)
{
    // ringbuffer
    int index = m_recordEventIndex;
    MappedEvent *recordBuffer = (MappedEvent *)m_recordBuffer;

    for (MappedComposition::iterator i = mC->begin(); i != mC->end(); ++i) {
	recordBuffer[index] = **i;
	if (++index == SEQUENCER_DATABLOCK_RECORD_BUFFER_SIZE) index = 0;
    }

    m_recordEventIndex = index;
}

int
SequencerDataBlock::instrumentToIndex(InstrumentId id) const
{
    int i;

    for (i = 0; i < m_knownInstrumentCount; ++i) {
	if (m_knownInstruments[i] == id) return i;
    }

    return -1;
}

int
SequencerDataBlock::instrumentToIndexCreating(InstrumentId id)
{
    int i;

    for (i = 0; i < m_knownInstrumentCount; ++i) {
	if (m_knownInstruments[i] == id) return i;
    }

    if (i == SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS) {
	std::cerr << "ERROR: SequencerDataBlock::instrumentToIndexCreating("
		  << id << "): out of instrument index space" << std::endl;
	return -1;
    }

    m_knownInstruments[i] = id;
    ++m_knownInstrumentCount;
    return i;
}

bool
SequencerDataBlock::getInstrumentLevel(InstrumentId id, LevelInfo &info) const
{
    static int lastUpdateIndex[SEQUENCER_DATABLOCK_MAX_NB_INSTRUMENTS];

    int index = instrumentToIndex(id);
    if (index < 0) {
	info.level = info.levelRight = 0;
	return false;
    }

    int currentUpdateIndex = m_levelUpdateIndices[index];
    info = m_levels[index];

    if (lastUpdateIndex[index] != currentUpdateIndex) {
	lastUpdateIndex[index]  = currentUpdateIndex;
	return true;
    } else {
	return false; // no change
    }
}

void
SequencerDataBlock::setInstrumentLevel(InstrumentId id, const LevelInfo &info)
{
    int index = instrumentToIndexCreating(id);
    if (index < 0) return;

    m_levels[index] = info;
    ++m_levelUpdateIndices[index];
}

bool
SequencerDataBlock::getRecordLevel(LevelInfo &level) const
{
    static int lastUpdateIndex = 0;

    int currentIndex = m_recordLevelUpdateIndex;
    level = m_recordLevel;

    if (lastUpdateIndex != currentIndex) {
	lastUpdateIndex  = currentIndex;
	return true;
    } else {
	return false;
    }
}

void
SequencerDataBlock::setRecordLevel(const LevelInfo &info)
{
    m_recordLevel = info;
    ++m_recordLevelUpdateIndex;
}

void
SequencerDataBlock::setTrackLevel(TrackId id, const LevelInfo &info)
{
    if (m_controlBlock) {
	setInstrumentLevel(m_controlBlock->getInstrumentForTrack(id), info);
    }
}

bool
SequencerDataBlock::getTrackLevel(TrackId id, LevelInfo &info) const
{
    info.level = info.levelRight = 0;

    if (m_controlBlock) {
	return getInstrumentLevel(m_controlBlock->getInstrumentForTrack(id),
				  info);
    }

    return false;
}
    

}

