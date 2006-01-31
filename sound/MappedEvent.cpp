// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <kstddirs.h>

#include "MappedEvent.h"
#include "BaseProperties.h"
#include "Midi.h"
#include "MidiTypes.h"
 
#define DEBUG_MAPPEDEVENT 1

namespace Rosegarden
{

MappedEvent::MappedEvent(InstrumentId id,
                         const Event &e,
                         const RealTime &eventTime,
                         const RealTime &duration):
    m_trackId(0),
    m_instrument(id),
    m_type(MidiNote),
    m_data1(0),
    m_data2(0),
    m_eventTime(eventTime),
    m_duration(duration),
    m_audioStartMarker(0, 0),
    m_dataBlockId(0),
    m_isPersistent(false),
    m_runtimeSegmentId(-1),
    m_autoFade(false),
    m_fadeInTime(Rosegarden::RealTime::zeroTime),
    m_fadeOutTime(Rosegarden::RealTime::zeroTime),
    m_recordedChannel(0),
    m_recordedDevice(0)
    
{
    try {

	// For each event type, we set the properties in a particular
	// order: first the type, then whichever of data1 and data2 fits
	// less well with its default value.  This way if one throws an
	// exception for no data, we still have a good event with the
	// defaults set.

	if (e.isa(Note::EventType))
            {
                m_type = MidiNoteOneShot;
                long v = MidiMaxValue;
                e.get<Int>(BaseProperties::VELOCITY, v);
                m_data2 = v;
                m_data1 = e.get<Int>(BaseProperties::PITCH);
            }
	else if (e.isa(PitchBend::EventType))
            {
                m_type = MidiPitchBend;
                PitchBend pb(e);
                m_data1 = pb.getMSB();
                m_data2 = pb.getLSB();
            }
	else if (e.isa(Controller::EventType))
            {
                m_type = MidiController;
                Controller c(e);
                m_data1 = c.getNumber();
                m_data2 = c.getValue();
            }
	else if (e.isa(ProgramChange::EventType))
            {
                m_type = MidiProgramChange;
                ProgramChange pc(e);
                m_data1 = pc.getProgram();
            }
	else if (e.isa(KeyPressure::EventType))
            {
                m_type = MidiKeyPressure;
                KeyPressure kp(e);
                m_data1 = kp.getPitch();
                m_data2 = kp.getPressure();
            }
	else if (e.isa(ChannelPressure::EventType))
            {
                m_type = MidiChannelPressure;
                ChannelPressure cp(e);
                m_data1 = cp.getPressure();
            }
	else if (e.isa(SystemExclusive::EventType))
            {
                m_type = MidiSystemMessage;
                m_data1 = Rosegarden::MIDI_SYSTEM_EXCLUSIVE;
                SystemExclusive s(e);
                std::string dataBlock = s.getRawData();
                DataBlockRepository::getInstance()->registerDataBlockForEvent(dataBlock, this);
            }
	else 
            {
                m_type = InvalidMappedEvent;
            }
    } catch (MIDIValueOutOfRange r) {

#ifdef DEBUG_MAPPEDEVENT
	std::cerr << "MIDI value out of range in MappedEvent ctor"
		  << std::endl;
#else
        ;
#endif

    } catch (Event::NoData d) {

#ifdef DEBUG_MAPPEDEVENT
	std::cerr << "Caught Event::NoData in MappedEvent ctor, message is:"
		  << std::endl << d.getMessage() << std::endl;
#else
        ;
#endif

    } catch (Event::BadType b) {

#ifdef DEBUG_MAPPEDEVENT
	std::cerr << "Caught Event::BadType in MappedEvent ctor, message is:"
		  << std::endl << b.getMessage() << std::endl;
#else
        ;
#endif

    } catch (SystemExclusive::BadEncoding e) {

#ifdef DEBUG_MAPPEDEVENT
	std::cerr << "Caught bad SysEx encoding in MappedEvent ctor"
		  << std::endl;
#else
        ;
#endif

    }
}

bool
operator<(const MappedEvent &a, const MappedEvent &b)
{
    return a.getEventTime() < b.getEventTime();
}

MappedEvent&
MappedEvent::operator=(const MappedEvent &mE)
{
    if (&mE == this) return *this;

    m_trackId = mE.getTrackId();
    m_instrument = mE.getInstrument();
    m_type = mE.getType();
    m_data1 = mE.getData1();
    m_data2 = mE.getData2();
    m_eventTime = mE.getEventTime();
    m_duration = mE.getDuration();
    m_audioStartMarker = mE.getAudioStartMarker();
    m_dataBlockId = mE.getDataBlockId();
    m_runtimeSegmentId = mE.getRuntimeSegmentId();
    m_autoFade = mE.isAutoFading();
    m_fadeInTime = mE.getFadeInTime();
    m_fadeOutTime = mE.getFadeOutTime();
    m_recordedChannel = mE.getRecordedChannel();
    m_recordedDevice = mE.getRecordedDevice();

    return *this;
}

// Do we use this?  It looks dangerous so just commenting it out - rwb
//
//const size_t MappedEvent::streamedSize = 12 * sizeof(unsigned int);

QDataStream&
operator<<(QDataStream &dS, MappedEvent *mE)
{
    dS << (unsigned int)mE->getTrackId();
    dS << (unsigned int)mE->getInstrument();
    dS << (unsigned int)mE->getType();
    dS << (unsigned int)mE->getData1();
    dS << (unsigned int)mE->getData2();
    dS << (unsigned int)mE->getEventTime().sec;
    dS << (unsigned int)mE->getEventTime().nsec;
    dS << (unsigned int)mE->getDuration().sec;
    dS << (unsigned int)mE->getDuration().nsec;
    dS << (unsigned int)mE->getAudioStartMarker().sec;
    dS << (unsigned int)mE->getAudioStartMarker().nsec;
    dS << (unsigned long)mE->getDataBlockId();
    dS << mE->getRuntimeSegmentId();
    dS << (unsigned int)mE->isAutoFading();
    dS << (unsigned int)mE->getFadeInTime().sec;
    dS << (unsigned int)mE->getFadeInTime().nsec;
    dS << (unsigned int)mE->getFadeOutTime().sec;
    dS << (unsigned int)mE->getFadeOutTime().nsec;
    dS << (unsigned int)mE->getRecordedChannel();
    dS << (unsigned int)mE->getRecordedDevice();

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, const MappedEvent &mE)
{
    dS << (unsigned int)mE.getTrackId();
    dS << (unsigned int)mE.getInstrument();
    dS << (unsigned int)mE.getType();
    dS << (unsigned int)mE.getData1();
    dS << (unsigned int)mE.getData2();
    dS << (unsigned int)mE.getEventTime().sec;
    dS << (unsigned int)mE.getEventTime().nsec;
    dS << (unsigned int)mE.getDuration().sec;
    dS << (unsigned int)mE.getDuration().nsec;
    dS << (unsigned int)mE.getAudioStartMarker().sec;
    dS << (unsigned int)mE.getAudioStartMarker().nsec;
    dS << (unsigned long)mE.getDataBlockId();
    dS << mE.getRuntimeSegmentId();
    dS << (unsigned int)mE.isAutoFading();
    dS << (unsigned int)mE.getFadeInTime().sec;
    dS << (unsigned int)mE.getFadeInTime().nsec;
    dS << (unsigned int)mE.getFadeOutTime().sec;
    dS << (unsigned int)mE.getFadeOutTime().nsec;
    dS << (unsigned int)mE.getRecordedChannel();
    dS << (unsigned int)mE.getRecordedDevice();

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent *mE)
{
    unsigned int trackId = 0, instrument = 0, type = 0, data1 = 0, data2 = 0;
    long eventTimeSec = 0, eventTimeNsec = 0, durationSec = 0, durationNsec = 0,
        audioSec = 0, audioNsec = 0;
    std::string dataBlock;
    unsigned long dataBlockId = 0;
    int runtimeSegmentId = -1;
    unsigned int autoFade = 0, 
        fadeInSec = 0, fadeInNsec = 0, fadeOutSec = 0, fadeOutNsec = 0,
        recordedChannel = 0, recordedDevice = 0;

    dS >> trackId;
    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeNsec;
    dS >> durationSec;
    dS >> durationNsec;
    dS >> audioSec;
    dS >> audioNsec;
    dS >> dataBlockId;
    dS >> runtimeSegmentId;
    dS >> autoFade;
    dS >> fadeInSec;
    dS >> fadeInNsec;
    dS >> fadeOutSec;
    dS >> fadeOutNsec;
    dS >> recordedChannel;
    dS >> recordedDevice;

    mE->setTrackId((TrackId)trackId);
    mE->setInstrument((InstrumentId)instrument);
    mE->setType((MappedEvent::MappedEventType)type);
    mE->setData1((MidiByte)data1);
    mE->setData2((MidiByte)data2);
    mE->setEventTime(RealTime(eventTimeSec, eventTimeNsec));
    mE->setDuration(RealTime(durationSec, durationNsec));
    mE->setAudioStartMarker(RealTime(audioSec, audioNsec));
    mE->setDataBlockId(dataBlockId);
    mE->setRuntimeSegmentId(runtimeSegmentId);
    mE->setAutoFade(autoFade);
    mE->setFadeInTime(RealTime(fadeInSec, fadeInNsec));
    mE->setFadeOutTime(RealTime(fadeOutSec, fadeOutNsec));
    mE->setRecordedChannel(recordedChannel);
    mE->setRecordedDevice(recordedDevice);

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent &mE)
{
    unsigned int trackId = 0, instrument = 0, type = 0, data1 = 0, data2 = 0;
    long eventTimeSec = 0, eventTimeNsec = 0, durationSec = 0, durationNsec = 0,
        audioSec = 0, audioNsec = 0;
    std::string dataBlock;
    unsigned long dataBlockId = 0;
    int runtimeSegmentId = -1;
    unsigned int autoFade = 0, 
        fadeInSec = 0, fadeInNsec = 0, fadeOutSec = 0, fadeOutNsec = 0,
        recordedChannel = 0, recordedDevice = 0;
         
    dS >> trackId;
    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeNsec;
    dS >> durationSec;
    dS >> durationNsec;
    dS >> audioSec;
    dS >> audioNsec;
    dS >> dataBlockId;
    dS >> runtimeSegmentId;
    dS >> autoFade;
    dS >> fadeInSec;
    dS >> fadeInNsec;
    dS >> fadeOutSec;
    dS >> fadeOutNsec;
    dS >> recordedChannel;
    dS >> recordedDevice;

    mE.setTrackId((TrackId)trackId);
    mE.setInstrument((InstrumentId)instrument);
    mE.setType((MappedEvent::MappedEventType)type);
    mE.setData1((MidiByte)data1);
    mE.setData2((MidiByte)data2);
    mE.setEventTime(RealTime(eventTimeSec, eventTimeNsec));
    mE.setDuration(RealTime(durationSec, durationNsec));
    mE.setAudioStartMarker(RealTime(audioSec, audioNsec));
    mE.setDataBlockId(dataBlockId);
    mE.setRuntimeSegmentId(runtimeSegmentId);
    mE.setAutoFade(autoFade);
    mE.setFadeInTime(RealTime(fadeInSec, fadeInNsec));
    mE.setFadeOutTime(RealTime(fadeOutSec, fadeOutNsec));
    mE.setRecordedChannel(recordedChannel);
    mE.setRecordedDevice(recordedDevice);

    return dS;
}

void
MappedEvent::addDataByte(MidiByte byte)
{
    DataBlockRepository::getInstance()->addDataByteForEvent(byte, this);
}

void 
MappedEvent::addDataString(const std::string& data)
{
    DataBlockRepository::getInstance()->addDataStringForEvent(data, this);
}



//--------------------------------------------------

class DataBlockFile
{
public:
    DataBlockFile(DataBlockRepository::blockid id);
    ~DataBlockFile();
    
    QString getFileName() { return m_fileName; }

    void addDataByte(MidiByte);
    void addDataString(const std::string&);
    
    void clear() { m_cleared = true; }
    bool exists();
    void setData(const std::string&);
    std::string getData();
    
protected:
    void prepareToWrite();
    void prepareToRead();

    //--------------- Data members ---------------------------------
    QString m_fileName;
    QFile m_file;
    bool m_cleared;
};

DataBlockFile::DataBlockFile(DataBlockRepository::blockid id)
    : m_fileName(KGlobal::dirs()->resourceDirs("tmp").first() + QString("/rosegarden_datablock_%1").arg(id)),
      m_file(m_fileName),
      m_cleared(false)
{
//     std::cerr << "DataBlockFile " << m_fileName.latin1() << std::endl;
}

DataBlockFile::~DataBlockFile()
{
    if (m_cleared) {
        std::cerr << "~DataBlockFile : removing " << m_fileName.latin1() << std::endl;
        QFile::remove(m_fileName);
    }
    
}

bool DataBlockFile::exists()
{
    return QFile::exists(m_fileName);
}

void DataBlockFile::setData(const std::string& s)
{
    std::cerr << "DataBlockFile::setData() : setting data to " << m_fileName << std::endl;
    prepareToWrite();

    QDataStream stream(&m_file);
    stream.writeRawBytes(s.data(), s.length());
}

std::string DataBlockFile::getData()
{
    if (!exists()) return std::string();

    prepareToRead();

    QDataStream stream(&m_file);
    std::cerr << "DataBlockFile::getData() : file size = " << m_file.size() << std::endl;
    char* tmp = new char[m_file.size()];
    stream.readRawBytes(tmp, m_file.size());
    std::string res(tmp, m_file.size());
    delete[] tmp;

    return res;
}

void DataBlockFile::addDataByte(MidiByte byte)
{
    prepareToWrite();
    m_file.putch(byte);
}

void DataBlockFile::addDataString(const std::string& s)
{
    prepareToWrite();
    QDataStream stream(&m_file);
    stream.writeRawBytes(s.data(), s.length());
}

void DataBlockFile::prepareToWrite()
{    
    std::cerr << "DataBlockFile[" << m_fileName << "]: prepareToWrite" << std::endl;
    if (!m_file.isWritable()) {
        m_file.close();
        assert(m_file.open(IO_WriteOnly | IO_Append));
    }
}

void DataBlockFile::prepareToRead()
{
    std::cerr << "DataBlockFile[" << m_fileName << "]: prepareToRead" << std::endl;
    if (!m_file.isReadable()) {
        m_file.close();
        assert(m_file.open(IO_ReadOnly));
    }
}



//--------------------------------------------------

DataBlockRepository* DataBlockRepository::getInstance()
{
    if (!m_instance) m_instance = new DataBlockRepository;
    return m_instance;
}

std::string DataBlockRepository::getDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);
    
    if (dataBlockFile.exists()) return dataBlockFile.getData();

    return std::string();
}


std::string DataBlockRepository::getDataBlockForEvent(MappedEvent* e)
{
    blockid id = e->getDataBlockId();
    if (id == 0) {
	std::cerr << "WARNING: DataBlockRepository::getDataBlockForEvent called on event with data block id 0" << std::endl;
	return "";
    }
    return getInstance()->getDataBlock(id);
}

void DataBlockRepository::setDataBlockForEvent(MappedEvent* e, const std::string& s)
{
    blockid id = e->getDataBlockId();
    if (id == 0) {
	std::cerr << "Creating new datablock for event" << std::endl;
	getInstance()->registerDataBlockForEvent(s, e);
    } else {
	std::cerr << "Writing " << s.length() << " chars to file for datablock " << id << std::endl;
	DataBlockFile dataBlockFile(id);
	dataBlockFile.setData(s);
    }
}

bool DataBlockRepository::hasDataBlock(DataBlockRepository::blockid id)
{
    return DataBlockFile(id).exists();
}

DataBlockRepository::blockid DataBlockRepository::registerDataBlock(const std::string& s)
{
    blockid id = 0;
    while (id == 0) id = (blockid)random();

    std::cerr << "DataBlockRepository::registerDataBlock: " << s.length() << " chars, id is " << id << std::endl;

    DataBlockFile dataBlockFile(id);
    dataBlockFile.setData(s);

    return id;
}

void DataBlockRepository::unregisterDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);
    
    dataBlockFile.clear();
}

void DataBlockRepository::registerDataBlockForEvent(const std::string& s, MappedEvent* e)
{
    e->setDataBlockId(registerDataBlock(s));
}

void DataBlockRepository::unregisterDataBlockForEvent(MappedEvent* e)
{
    unregisterDataBlock(e->getDataBlockId());
}

    
DataBlockRepository::DataBlockRepository()
{
}

void DataBlockRepository::clear()
{
#ifdef DEBUG_MAPPEDEVENT
    std::cerr << "DataBlockRepository::clear()\n";
#endif

    // Erase all 'datablock_*' files
    //
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").first();

    QDir segmentsDir(tmpPath, "rosegarden_datablock_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        QFile::remove(segmentName);
    }
}


void DataBlockRepository::addDataByteForEvent(MidiByte byte, MappedEvent* e)
{
    DataBlockFile dataBlockFile(e->getDataBlockId());
    dataBlockFile.addDataByte(byte);
    
}

void DataBlockRepository::addDataStringForEvent(const std::string& s, MappedEvent* e)
{
    DataBlockFile dataBlockFile(e->getDataBlockId());
    dataBlockFile.addDataString(s);
}

DataBlockRepository* DataBlockRepository::m_instance = 0;

}
