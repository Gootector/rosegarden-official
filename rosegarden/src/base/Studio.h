// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include <string>
#include <vector>

#include "XmlExportable.h"
#include "Instrument.h"
#include "Device.h"
#include "MidiProgram.h"
#include "ControlParameter.h"

// The Studio is where Midi and Audio devices live.  We can query
// them for a list of Instruments, connect them together or to
// effects units (eventually) and generally do real studio-type
// stuff to them.
//
//


#ifndef _STUDIO_H_
#define _STUDIO_H_

namespace Rosegarden
{

typedef std::vector<Instrument *> InstrumentList;
typedef std::vector<Device*> DeviceList;
typedef std::vector<Buss *> BussList;
typedef std::vector<RecordIn *> RecordInList;
typedef std::vector<Device*>::iterator DeviceListIterator;
typedef std::vector<Device*>::const_iterator DeviceListConstIterator;

class MidiDevice;

class Segment;
class Track;


class Studio : public XmlExportable
{

public:
    Studio();
    ~Studio();

private:
    Studio(const Studio &);
    Studio& operator=(const Studio &);
public:

    void addDevice(const std::string &name,
                   DeviceId id,
                   Device::DeviceType type);
    void addDevice(Device *device);

    void removeDevice(DeviceId id);

    // Return the combined instrument list from all devices
    // for all and presentation Instrument (Presentation is
    // just a subset of All).
    //
    InstrumentList getAllInstruments();
    InstrumentList getPresentationInstruments();

    // Return an Instrument
    Instrument* getInstrumentById(InstrumentId id);
    Instrument* getInstrumentFromList(int index);

    // Convenience functions
    Instrument *getInstrumentFor(Segment *);
    Instrument *getInstrumentFor(Track *);

    // Return a Buss
    BussList getBusses();
    Buss *getBussById(BussId id);
    void addBuss(Buss *buss);

    // Return an Instrument or a Buss
    PluginContainer *getContainerById(InstrumentId id);

    RecordInList getRecordIns() { return m_recordIns; }
    RecordIn *getRecordIn(int number);
    void addRecordIn(RecordIn *ri) { m_recordIns.push_back(ri); }

    // A clever method to best guess MIDI file program mappings
    // to available MIDI channels across all MidiDevices.
    //
    // Set the percussion flag if it's a percussion channel (mapped
    // channel) we're after.
    //
    Instrument* assignMidiProgramToInstrument(MidiByte program,
                                              bool percussion) {
        return assignMidiProgramToInstrument(program, -1, -1, percussion);
    }

    // Same again, but with bank select
    // 
    Instrument* assignMidiProgramToInstrument(MidiByte program,
                                              int msb, int lsb,
                                              bool percussion);

    // Get a suitable name for a Segment belonging to this instrument.
    // Takes into account ProgramChanges.
    //
    std::string getSegmentName(InstrumentId id);

    // Clear down all the ProgramChange flags in all MIDI Instruments
    //
    void unassignAllInstruments();

    // Clear down all MIDI banks and programs on all MidiDevices
    // prior to reloading.  The Instruments and Devices are generated
    // at the Sequencer - the Banks and Programs are loaded from the
    // RG4 file.
    //
    void clearMidiBanksAndPrograms();

    void clearBusses();
    void clearRecordIns();
    
    // Clear down
    void clear();

    // Get a MIDI metronome from a given device
    //
    const MidiMetronome* getMetronomeFromDevice(DeviceId id);

    // Return the device list
    //
    DeviceList* getDevices() { return &m_devices; }

    // Const iterators
    //
    DeviceListConstIterator begin() const { return m_devices.begin(); }
    DeviceListConstIterator end() const { return m_devices.end(); }

    // Get a device by ID
    //
    Device* getDevice(DeviceId id);

    bool haveMidiDevices() const;

    // Export as XML string
    //
    virtual std::string toXmlString();

    // Export a subset of devices as XML string.  If devices is empty,
    // exports all devices just as the above method does.
    //
    virtual std::string toXmlString(const std::vector<DeviceId> &devices);

    // Get an audio preview Instrument
    //
    InstrumentId getAudioPreviewInstrument();

    // MIDI filtering into and thru Rosegarden
    //
    void setMIDIThruFilter(MidiFilter filter) { m_midiThruFilter = filter; }
    MidiFilter getMIDIThruFilter() const { return m_midiThruFilter; }

    void setMIDIRecordFilter(MidiFilter filter) { m_midiRecordFilter = filter; }
    MidiFilter getMIDIRecordFilter() const { return m_midiRecordFilter; }

    void setMixerDisplayOptions(unsigned int options) { m_mixerDisplayOptions = options; }
    unsigned int getMixerDisplayOptions() const { return m_mixerDisplayOptions; }

    DeviceId getMetronomeDevice() const { return m_metronomeDevice; }
    void setMetronomeDevice(DeviceId device) { m_metronomeDevice = device; }

private:

    DeviceList        m_devices;

    BussList          m_busses;
    RecordInList      m_recordIns;

    int               m_audioInputs; // stereo pairs

    MidiFilter        m_midiThruFilter;
    MidiFilter        m_midiRecordFilter;

    unsigned int      m_mixerDisplayOptions;

    DeviceId          m_metronomeDevice;
};

}

#endif // _STUDIO_H_
