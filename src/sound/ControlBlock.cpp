/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cstring>

#include "ControlBlock.h"

#include "base/Instrument.h"
#include "document/RosegardenDocument.h"

namespace Rosegarden
{

ControlBlock *
ControlBlock::getInstance()
{
    static ControlBlock *instance = 0;
    if (!instance) instance = new ControlBlock();
    return instance;
}

ControlBlock::ControlBlock() :
    m_doc(0),
    m_maxTrackId(0),
    m_solo(false),
    m_routing(true),
    m_thruFilter(0),
    m_recordFilter(0),
    m_selectedTrack(0)
{
    m_metronomeInfo.muted = true;
    m_metronomeInfo.instrumentId = 0;
    for (unsigned int i = 0; i < CONTROLBLOCK_MAX_NB_TRACKS; ++i) {
        m_trackInfo[i].muted = true;
        m_trackInfo[i].deleted = true;
        m_trackInfo[i].armed = true;
        m_trackInfo[i].instrumentId = 0;
    }
}

void
ControlBlock::setDocument(RosegardenDocument *doc)
{
    m_doc = doc;
    m_maxTrackId = m_doc->getComposition().getMaxTrackId();
    
    Composition& comp = m_doc->getComposition();

    for (Composition::trackiterator i = comp.getTracks().begin();
	 i != comp.getTracks().end(); ++i) {
        Track *track = i->second;
        if (!track) continue;
	updateTrackData(track);
    }

    setMetronomeMuted(!comp.usePlayMetronome());

    setThruFilter(m_doc->getStudio().getMIDIThruFilter());
    setRecordFilter(m_doc->getStudio().getMIDIRecordFilter());
}

void
ControlBlock::updateTrackData(Track* t)
{
    if (t) {
        setInstrumentForTrack(t->getId(), t->getInstrument());
        setTrackArmed(t->getId(), t->isArmed());
        setTrackMuted(t->getId(), t->isMuted());
        setTrackDeleted(t->getId(), false);
        setTrackChannelFilter(t->getId(), t->getMidiInputChannel());
        setTrackDeviceFilter(t->getId(), t->getMidiInputDevice());
        if (t->getId() > m_maxTrackId)
            m_maxTrackId = t->getId();
    }
}

void
ControlBlock::setInstrumentForTrack(TrackId trackId, InstrumentId instId)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].instrumentId = instId;
}

InstrumentId 
ControlBlock::getInstrumentForTrack(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].instrumentId;
    return 0;
}

// Return the natural channel of instrument, meaningful only for
// fixed-channel MIDI instruments.
int
ControlBlock::
getNaturalChannelForInstrument(InstrumentId id) const
{
    if (!m_doc)
        { return -1; }

    Instrument *instrument = m_doc->getStudio().getInstrumentById(id);
    if (!instrument)
        { return -1; }
    if (instrument->getType() != Instrument::Midi)
        { return -1; }
    if (!instrument->hasFixedChannel())
        { return -1; }

    return instrument->getNaturalChannel();
}

void
ControlBlock::setTrackMuted(TrackId trackId, bool mute)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].muted = mute;
}

bool ControlBlock::isTrackMuted(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].muted;
    return true;
}

void
ControlBlock::setTrackArmed(TrackId trackId, bool armed)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].armed = armed;
}

bool 
ControlBlock::isTrackArmed(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].armed;
    return false;
}

void
ControlBlock::setTrackDeleted(TrackId trackId, bool deleted)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].deleted = deleted;
}

bool 
ControlBlock::isTrackDeleted(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].deleted;
    return true;
}

void
ControlBlock::setTrackChannelFilter(TrackId trackId, char channel)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].channelFilter = channel;
}

char
ControlBlock::getTrackChannelFilter(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].channelFilter;
    return -1;
}

void
ControlBlock::setTrackDeviceFilter(TrackId trackId, DeviceId device)
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        m_trackInfo[trackId].deviceFilter = device;
}

DeviceId 
ControlBlock::getTrackDeviceFilter(TrackId trackId) const
{
    if (trackId < CONTROLBLOCK_MAX_NB_TRACKS)
        return m_trackInfo[trackId].deviceFilter;
    return Device::ALL_DEVICES;
}

bool 
ControlBlock::isInstrumentMuted(InstrumentId instrumentId) const
{
    for (unsigned int i = 0; i <= m_maxTrackId; ++i) {
        if (m_trackInfo[i].instrumentId == instrumentId &&
                !m_trackInfo[i].deleted && !m_trackInfo[i].muted)
            return false;
    }
    return true;
}

bool 
ControlBlock::isInstrumentUnused(InstrumentId instrumentId) const
{
    for (unsigned int i = 0; i <= m_maxTrackId; ++i) {
        if (m_trackInfo[i].instrumentId == instrumentId &&
                !m_trackInfo[i].deleted)
            return false;
    }
    return true;
}

InstrumentId 
ControlBlock::getInstrumentForEvent(unsigned int dev, unsigned int chan)
{
    for (unsigned int i = 0; i <= m_maxTrackId; ++i) {
        if (!m_trackInfo[i].deleted && m_trackInfo[i].armed) {
            if (((m_trackInfo[i].deviceFilter == Device::ALL_DEVICES) ||
		 (m_trackInfo[i].deviceFilter == dev)) &&
		((m_trackInfo[i].channelFilter == -1) ||
		 (m_trackInfo[i].channelFilter == int(chan))))
                return m_trackInfo[i].instrumentId;
        }
    }
    // There is not a matching filter, return the selected track instrument
    return getInstrumentForTrack(getSelectedTrack());
}


}
