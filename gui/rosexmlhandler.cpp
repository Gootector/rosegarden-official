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

#include <qtextstream.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include "rosestrings.h"
#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "rosegardengui.h"
#include "xmlstorableevent.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include "Track.h"

#include "MidiDevice.h"
#include "AudioDevice.h"
#include "MappedStudio.h"
#include "Instrument.h"
#include "Midi.h"
#include "AudioLevel.h"
#include "MidiTypes.h"
#include "Segment.h"

#include "widgets.h"
#include "rosestrings.h"
#include "dialogs.h"
#include "audiopluginmanager.h"
#include "studiocontrol.h"
#include "kstartuplogo.h"
#include "rgapplication.h"

using Rosegarden::Composition;
using Rosegarden::Studio;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::timeT;
using Rosegarden::Device;
using Rosegarden::DeviceList;
using Rosegarden::DeviceListIterator;

using namespace Rosegarden::BaseProperties;

class XmlSubHandler
{
public:
    XmlSubHandler();
    virtual ~XmlSubHandler();
    
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) = 0;

    /**
     * @param finished : if set to true on return, means that
     * the handler should be deleted
     */
    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName,
                            bool& finished) = 0;

    virtual bool characters(const QString& ch) = 0;
};

XmlSubHandler::XmlSubHandler()
{
}

XmlSubHandler::~XmlSubHandler()
{
}

//----------------------------------------

class ConfigurationXmlSubHandler : public XmlSubHandler
{
public:
    ConfigurationXmlSubHandler(const QString &elementName,
			       Rosegarden::Configuration *configuration);
    
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName,
                            bool& finished);

    virtual bool characters(const QString& ch);

    //--------------- Data members ---------------------------------

    Rosegarden::Configuration *m_configuration;

    QString m_elementName;
    QString m_propertyName;
    QString m_propertyType;
};

ConfigurationXmlSubHandler::ConfigurationXmlSubHandler(const QString &elementName,
						       Rosegarden::Configuration *configuration)
    : m_configuration(configuration),
      m_elementName(elementName)
{
}

bool ConfigurationXmlSubHandler::startElement(const QString&, const QString&,
                                              const QString& lcName,
                                              const QXmlAttributes& atts)
{
    m_propertyName = lcName;
    m_propertyType = atts.value("type");

    if (m_propertyName == "property") {
	// handle alternative encoding for properties with arbitrary names
	m_propertyName = atts.value("name");
	QString value = atts.value("value");
	if (value) {
	    m_propertyType = "String";
	    m_configuration->set<String>(qstrtostr(m_propertyName),
					 qstrtostr(value));
	}
    }

    return true;
}

bool ConfigurationXmlSubHandler::characters(const QString& chars)
{
    QString ch = chars.stripWhiteSpace();
    // this method is also called on newlines - skip these cases
    if (ch.isEmpty()) return true;


    if (m_propertyType == "Int") {
        long i = ch.toInt();
        RG_DEBUG << "\"" << m_propertyName << "\" "
                 << "value = " << i << endl;
        m_configuration->set<Int>(qstrtostr(m_propertyName), i);

        return true;
    }
    
    if (m_propertyType == "RealTime") {
        Rosegarden::RealTime rt;
        int sepIdx = ch.find(',');
        
        rt.sec = ch.left(sepIdx).toInt();
        rt.nsec = ch.mid(sepIdx + 1).toInt();

        RG_DEBUG << "\"" << m_propertyName << "\" "
                 << "sec = " << rt.sec << ", nsec = " << rt.nsec << endl;

        m_configuration->set<Rosegarden::RealTimeT>(qstrtostr(m_propertyName), rt);

        return true;
    }

    if (m_propertyType == "Bool") {
        QString chLc = ch.lower();
        
        bool b = (chLc == "true" ||
                  chLc == "1"    ||
                  chLc == "on");
        
        m_configuration->set<Rosegarden::Bool>(qstrtostr(m_propertyName), b);

        return true;
    }

    if (!m_propertyType ||
	m_propertyType == "String") {
        
        m_configuration->set<Rosegarden::String>(qstrtostr(m_propertyName),
						 qstrtostr(ch));

        return true;
    }
    

    return true;
}

bool
ConfigurationXmlSubHandler::endElement(const QString&,
                                       const QString&,
                                       const QString& lcName,
                                       bool& finished)
{
    m_propertyName = "";
    m_propertyType = "";
    finished = (lcName == m_elementName);
    return true;
}


//----------------------------------------



RoseXmlHandler::RoseXmlHandler(RosegardenGUIDoc *doc,
                               unsigned int elementCount,
			       bool createNewDevicesWhenNeeded)
    : ProgressReporter(0),
      m_doc(doc),
      m_currentSegment(0),
      m_currentEvent(0),
      m_currentTime(0),
      m_chordDuration(0),
      m_segmentEndMarkerTime(0),
      m_inChord(false),
      m_inGroup(false),
      m_inComposition(false),
      m_groupId(0),
      m_foundTempo(false),
      m_section(NoSection),
      m_device(0),
      m_deviceRunningId(Rosegarden::Device::NO_DEVICE),
      m_msb(0),
      m_lsb(0),
      m_instrument(0),
      m_plugin(0),
      m_pluginId(0),
      m_totalElements(elementCount),
      m_elementsSoFar(0),
      m_subHandler(0),
      m_deprecation(false),
      m_createDevices(createNewDevicesWhenNeeded),
      m_haveControls(false),
      m_cancelled(false),
      m_skipAllAudio(false)
{
}

RoseXmlHandler::~RoseXmlHandler()
{
    delete m_subHandler;
}

bool
RoseXmlHandler::startDocument()
{
    // Clear tracks
    //
    getComposition().clearTracks();

    // And the loop
    //
    getComposition().setLoopStart(0);
    getComposition().setLoopEnd(0);

    // All plugins
    //
    m_doc->clearAllPlugins();

    // reset state
    return true;
}

bool
RoseXmlHandler::startElement(const QString& namespaceURI,
                             const QString& localName,
                             const QString& qName, const QXmlAttributes& atts)
{
    // First check if user pressed cancel button on the progress
    // dialog
    //
    if (isOperationCancelled()) {
        // Ideally, we'd throw here, but at this point Qt is in the stack
        // and Qt is very often compiled without exception support.
        //
        m_cancelled = true;
        return false;
    }

    QString lcName = qName.lower();

    if (getSubHandler()) {
        return getSubHandler()->startElement(namespaceURI, localName, lcName, atts);
    }

    if (lcName == "event") {

//        RG_DEBUG << "RoseXmlHandler::startElement: found event, current time is " << m_currentTime << endl;

        if (m_currentEvent) {
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: new event found at time " << m_currentTime << " before previous event has ended; previous event will be lost" << endl;
            delete m_currentEvent;
        }

        m_currentEvent = new XmlStorableEvent(atts, m_currentTime);

	if (m_currentEvent->has(BEAMED_GROUP_ID)) {
	    
	    // remap -- we want to ensure that the segment's nextId
	    // is always used (and incremented) in preference to the
	    // stored id
	    
	    if (!m_currentSegment) {
		m_errorString = "Got grouped event outside of a segment";
		return false;
	    }
	    
	    long storedId = m_currentEvent->get<Int>(BEAMED_GROUP_ID);

	    if (m_groupIdMap.find(storedId) == m_groupIdMap.end()) {
		m_groupIdMap[storedId] = m_currentSegment->getNextId();
	    }
	    
	    m_currentEvent->set<Int>(BEAMED_GROUP_ID, m_groupIdMap[storedId]);

        } else if (m_inGroup) {
            m_currentEvent->set<Int>(BEAMED_GROUP_ID, m_groupId);
            m_currentEvent->set<String>(BEAMED_GROUP_TYPE, m_groupType);
	    if (m_groupType == GROUP_TYPE_TUPLED) {
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLET_BASE, m_groupTupletBase);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_UNTUPLED_COUNT, m_groupUntupledCount);
	    }
        }

	timeT duration = m_currentEvent->getDuration();
        
        if (!m_inChord) {

            m_currentTime = m_currentEvent->getAbsoluteTime() + duration;

//            RG_DEBUG << "RoseXmlHandler::startElement: (we're not in a chord) " << endl;

        } else if (duration != 0) {

            // set chord duration to the duration of the shortest
            // element with a non-null duration (if no such elements,
            // leave it as 0).

	    if (m_chordDuration == 0 || duration < m_chordDuration) {
		m_chordDuration = duration;
	    }
        }

    } else if (lcName == "property") {
        
        if (!m_currentEvent) {
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: Found property outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertyFromAttributes(atts, true);
        }

    } else if (lcName == "nproperty") {
        
        if (!m_currentEvent) {
            RG_DEBUG << "RoseXmlHandler::startElement: Warning: Found nproperty outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertyFromAttributes(atts, false);
        }

    } else if (lcName == "chord") {

        m_inChord = true;

    } else if (lcName == "group") {

        if (!m_currentSegment) {
            m_errorString = "Got group outside of a segment";
            return false;
        }
        
        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"group\".  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        m_inGroup = true;
        m_groupId = m_currentSegment->getNextId();
        m_groupType = qstrtostr(atts.value("type"));

	if (m_groupType == GROUP_TYPE_TUPLED) {
	    m_groupTupletBase = atts.value("base").toInt();
	    m_groupTupledCount = atts.value("tupled").toInt();
	    m_groupUntupledCount = atts.value("untupled").toInt();
	}

    } else if (lcName == "rosegarden-data") {

	// FILE FORMAT VERSIONING -- see comments in
	// rosegardenguidoc.cpp.  We only care about major and minor
	// here, not point.

	QString version = atts.value("version");
	QString smajor = atts.value("format-version-major");
	QString sminor = atts.value("format-version-minor");

	if (smajor) {

	    int major = smajor.toInt();
	    int minor = sminor.toInt();

	    if (major > RosegardenGUIDoc::FILE_FORMAT_VERSION_MAJOR) {
		m_errorString = i18n("This file was written by Rosegarden %1, and it uses\na different file format that cannot be read by this version.").arg(version);
		return false;
	    }

	    if (major == RosegardenGUIDoc::FILE_FORMAT_VERSION_MAJOR &&
		minor >  RosegardenGUIDoc::FILE_FORMAT_VERSION_MINOR) {

                CurrentProgressDialog::freeze();
	        KStartupLogo::hideIfStillThere();

		KMessageBox::information(0, i18n("This file was written by Rosegarden %1, which is more recent than this version.\nThere may be some incompatibilities with the file format.").arg(version));

		CurrentProgressDialog::thaw();
	    }
	}

    } else if (lcName == "studio") {

        if (m_section != NoSection)
        {
            m_errorString = "Found Studio in another section";
            return false;
        }

        // In the Studio we clear down everything apart from Devices and
        // Instruments before we reload.  Instruments are derived from
        // the Sequencer, the bank/program information is loaded from
        // the file we're currently examining.
        //
        getStudio().clearMidiBanksAndPrograms();
	getStudio().clearBusses();
	getStudio().clearRecordIns();

        m_section = InStudio; // set top level section

        // Get and set MIDI filters
        //
        QString thruStr = atts.value("thrufilter");

        if (thruStr)
            getStudio().setMIDIThruFilter(thruStr.toInt());

        QString recordStr = atts.value("recordfilter");

        if (recordStr)
            getStudio().setMIDIRecordFilter(recordStr.toInt());

	QString inputStr = atts.value("audioinputpairs");
	
	if (inputStr) {
	    int inputs = inputStr.toInt();
	    if (inputs < 1) inputs = 1; // we simply don't permit no inputs
	    while (int(getStudio().getRecordIns().size()) < inputs) {
		getStudio().addRecordIn(new Rosegarden::RecordIn());
	    }
	}

	QString mixerStr = atts.value("mixerdisplayoptions");
	
	if (mixerStr) {
	    unsigned int mixer = mixerStr.toUInt();
	    getStudio().setMixerDisplayOptions(mixer);
	}

        QString metronomeStr = atts.value("metronomedevice");

        if (metronomeStr) {
            Rosegarden::DeviceId metronome = metronomeStr.toUInt();
            getStudio().setMetronomeDevice(metronome);
        }

    } else if (lcName == "timesignature") {

        if (m_inComposition == false)
        {
            m_errorString = "TimeSignature object found outside Composition";
            return false;
        }

	timeT t = 0;
	QString timeStr = atts.value("time");
	if (timeStr) t = timeStr.toInt();

	int num = 4;
	QString numStr = atts.value("numerator");
	if (numStr) num = numStr.toInt();

	int denom = 4;
	QString denomStr = atts.value("denominator");
	if (denomStr) denom = denomStr.toInt();

	bool common = false;
	QString commonStr = atts.value("common");
	if (commonStr) common = (commonStr == "true");

	bool hidden = false;
	QString hiddenStr = atts.value("hidden");
	if (hiddenStr) hidden = (hiddenStr == "true");

	bool hiddenBars = false;
	QString hiddenBarsStr = atts.value("hiddenbars");
	if (hiddenBarsStr) hiddenBars = (hiddenBarsStr == "true");

	getComposition().addTimeSignature
	    (t, Rosegarden::TimeSignature(num, denom, common, hidden, hiddenBars));

    } else if (lcName == "tempo") {

	timeT t = 0;
	QString timeStr = atts.value("time");
	if (timeStr) t = timeStr.toInt();
	
	int bph = 120 * 60; // arbitrary
	QString bphStr = atts.value("bph");
	if (bphStr) bph = bphStr.toInt();

	getComposition().addRawTempo(t, bph);

    } else if (lcName == "composition") {

        if (m_section != NoSection)
        {
            m_errorString = "Found Composition in another section";
            return false;
        }
        
        // set Segment
        m_section = InComposition;

        // Get and set the record track
        //
        int recordTrack = -1;
        QString recordStr = atts.value("recordtrack");

        if (recordStr) {
            recordTrack = recordStr.toInt();
        }

        getComposition().setRecordTrack(recordTrack);

        // Get and ste the position pointer
        //
        int position = 0;
        QString positionStr = atts.value("pointer");
        if (positionStr) {
            position = positionStr.toInt();
        }

        getComposition().setPosition(position);


        // Get and (eventually) set the tempo
        //
        double tempo = 120.0;
        QString tempoStr = atts.value("defaultTempo");
        if (tempoStr) {
            tempo = tempoStr.toDouble();
        }

        getComposition().setDefaultTempo(tempo);
        
        // set the composition flag
        m_inComposition = true;


        // Set the loop
        //
        QString loopStartStr = atts.value("loopstart");
        QString loopEndStr = atts.value("loopend");

        if (loopStartStr && loopEndStr)
        {
            int loopStart = loopStartStr.toInt();
            int loopEnd = loopEndStr.toInt();

            getComposition().setLoopStart(loopStart);
            getComposition().setLoopEnd(loopEnd);
        }

        QString selectedTrackStr = atts.value("selected");

        if (selectedTrackStr)
        {
            Rosegarden::TrackId selectedTrack =
                (Rosegarden::TrackId)selectedTrackStr.toInt();

            getComposition().setSelectedTrack(selectedTrack);
        }

        QString soloTrackStr = atts.value("solo");
        if (soloTrackStr)
        {
            if (soloTrackStr.toInt() == 1)
                getComposition().setSolo(true);
            else
                getComposition().setSolo(false);
        }


        QString playMetStr = atts.value("playmetronome");
        if (playMetStr)
        {
            if (playMetStr.toInt())
               getComposition().setPlayMetronome(true);
            else
               getComposition().setPlayMetronome(false);
        }

        QString recMetStr = atts.value("recordmetronome");
        if (recMetStr)
        {
            if (recMetStr.toInt())
               getComposition().setRecordMetronome(true);
            else
               getComposition().setRecordMetronome(false);
        }

	QString nextTriggerIdStr = atts.value("nexttriggerid");
	if (nextTriggerIdStr) {
	    getComposition().setNextTriggerSegmentId(nextTriggerIdStr.toInt());
	}

	QString copyrightStr = atts.value("copyright");
	if (copyrightStr)
	{
	    getComposition().setCopyrightNote(qstrtostr(copyrightStr));
	}

        QString startMarkerStr = atts.value("startMarker");
        QString endMarkerStr = atts.value("endMarker");

        if (startMarkerStr)
        {
            getComposition().setStartMarker(startMarkerStr.toInt());
        }

        if (endMarkerStr)
        {
            getComposition().setEndMarker(endMarkerStr.toInt());
        }

    } else if (lcName == "track") {

        if (m_section != InComposition)
        {
            m_errorString = "Track object found outside Composition";
            return false;
        }

        int id = -1;
        int position = -1;
        int instrument = -1;
        std::string label;
        bool muted = false;

        QString trackNbStr = atts.value("id");
        if (trackNbStr) {
            id = trackNbStr.toInt();
        }

        QString labelStr = atts.value("label");
        if (labelStr) {
            label = qstrtostr(labelStr);
        }

        QString mutedStr = atts.value("muted");
        if (mutedStr) {
            if (mutedStr == "true")
                muted = true;
            else
                muted = false;
        }

        QString positionStr = atts.value("position");
        if (positionStr) {
            position = positionStr.toInt();
        }

        QString instrumentStr = atts.value("instrument");
        if (instrumentStr) {
            instrument = instrumentStr.toInt();
        }
       
        Rosegarden::Track *track = new Rosegarden::Track(id,
                                                         instrument,
                                                         position,
                                                         label,
                                                         muted);
        getComposition().addTrack(track);


    } else if (lcName == "segment") {

        if (m_section != NoSection)
        {
            m_errorString = "Found Segment in another section";
            return false;
        }
        
        // set Segment
        m_section = InSegment;

        int track = -1, startTime = 0;
        unsigned int colourindex = 0;
        QString trackNbStr = atts.value("track");
        if (trackNbStr) {
            track = trackNbStr.toInt();
        }

        QString startIdxStr = atts.value("start");
        if (startIdxStr) {
            startTime = startIdxStr.toInt();
        }

        QString segmentType = (atts.value("type")).lower();
        if (segmentType) {
            if (segmentType == "audio")
            {
                int audioFileId = atts.value("file").toInt();

                // check this file id exists on the AudioFileManager

                if(getAudioFileManager().fileExists(audioFileId) == false)
                {
                    // We don't report an error as this audio file might've
                    // been excluded deliberately as we could't actually
                    // find the audio file itself.
                    //
                    return true;
                }

                // Create an Audio segment and add its reference
                //
                m_currentSegment = new Segment(Rosegarden::Segment::Audio);
                m_currentSegment->setAudioFileId(audioFileId);
                m_currentSegment->setStartTime(startTime);
            }
            else
            {
                // Create a (normal) internal Segment
                m_currentSegment = new Segment(Rosegarden::Segment::Internal);
            }

        } else {
	    // for the moment we default
            m_currentSegment = new Segment(Rosegarden::Segment::Internal);
	}
    
        QString repeatStr = atts.value("repeat");
        if (repeatStr.lower() == "true") {
            m_currentSegment->setRepeating(true);
        }

        QString delayStr = atts.value("delay");
        if (delayStr) {
	    RG_DEBUG << "Delay string is \"" << delayStr << "\"" << endl;
	    long delay = delayStr.toLong();
	    RG_DEBUG << "Delay is " << delay << endl;
	    m_currentSegment->setDelay(delay);
	}

	QString rtDelaynSec = atts.value("rtdelaynsec");
	QString rtDelayuSec = atts.value("rtdelayusec");
	QString rtDelaySec = atts.value("rtdelaysec");
	if (rtDelaySec && (rtDelaynSec || rtDelayuSec)) {
	    if (rtDelaynSec) {
		m_currentSegment->setRealTimeDelay
		    (Rosegarden::RealTime(rtDelaySec.toInt(),
					  rtDelaynSec.toInt()));
	    } else {
		m_currentSegment->setRealTimeDelay 
		    (Rosegarden::RealTime(rtDelaySec.toInt(),
					  rtDelayuSec.toInt() * 1000));
	    }
	}

        QString transposeStr = atts.value("transpose");
        if (transposeStr) m_currentSegment->setTranspose(transposeStr.toInt());

	// fill in the label
        QString labelStr = atts.value("label");
        if (labelStr)
            m_currentSegment->setLabel(qstrtostr(labelStr));
        
        m_currentSegment->setTrack(track);
        //m_currentSegment->setStartTime(startTime);

        QString colourIndStr = atts.value("colourindex");
        if (colourIndStr) {
            colourindex = colourIndStr.toInt();
        }

        m_currentSegment->setColourIndex(colourindex);

        QString snapGridSizeStr = atts.value("snapgridsize");
        if (snapGridSizeStr) {
            m_currentSegment->setSnapGridSize(snapGridSizeStr.toInt());
        }

        QString viewFeaturesStr = atts.value("viewfeatures");
        if (viewFeaturesStr) {
            m_currentSegment->setViewFeatures(viewFeaturesStr.toInt());
        }

	m_currentTime = startTime;

	QString triggerIdStr = atts.value("triggerid");
	QString triggerPitchStr = atts.value("triggerbasepitch");
	QString triggerVelocityStr = atts.value("triggerbasevelocity");
	QString triggerRetuneStr = atts.value("triggerretune");
	QString triggerAdjustTimeStr = atts.value("triggeradjusttimes");

	if (triggerIdStr) {
	    int pitch = -1;
	    if (triggerPitchStr) pitch = triggerPitchStr.toInt();
	    int velocity = -1;
	    if (triggerVelocityStr) velocity = triggerVelocityStr.toInt();
	    Rosegarden::TriggerSegmentRec *rec =
		getComposition().addTriggerSegment(m_currentSegment,
						   triggerIdStr.toInt(),
						   pitch, velocity);
	    if (rec) {
		if (triggerRetuneStr)
		    rec->setDefaultRetune(triggerRetuneStr.lower() == "true");
		if (triggerAdjustTimeStr)
		    rec->setDefaultTimeAdjust(qstrtostr(triggerAdjustTimeStr));
	    }
	    m_currentSegment->setStartTimeDataMember(startTime);
	} else {
	    getComposition().addSegment(m_currentSegment);
	    getComposition().setSegmentStartTime(m_currentSegment, startTime);
	}

	QString endMarkerStr = atts.value("endmarker");
	if (endMarkerStr) {
	    delete m_segmentEndMarkerTime;
	    m_segmentEndMarkerTime = new timeT(endMarkerStr.toInt());
	}

	m_groupIdMap.clear();

    } else if (lcName == "gui") {

        if (m_section != InSegment)
        {
            m_errorString = "Found GUI element outside Segment";
            return false;
        }

    } else if (lcName == "controller") {

        if (m_section != InSegment)
        {
            m_errorString = "Found Controller element outside Segment";
            return false;
        }

        QString type = atts.value("type");
        //RG_DEBUG << "RoseXmlHandler::startElement - controller type = " << type << endl;

        if (type == strtoqstr(Rosegarden::PitchBend::EventType))
            m_currentSegment->addEventRuler(Rosegarden::PitchBend::EventType);
        else if (type == strtoqstr(Rosegarden::Controller::EventType))
        {
            QString value = atts.value("value");

            if (value != "")
                m_currentSegment->addEventRuler(Rosegarden::Controller::EventType, value.toInt());
        }

    } else if (lcName == "resync") {

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"resync\".  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;
	
	QString time(atts.value("time"));
	bool isNumeric;
	int numTime = time.toInt(&isNumeric);
	if (isNumeric) m_currentTime = numTime;

    } else if (lcName == "audio") {

        if (m_section != InAudioFiles)
        {
            m_errorString = "Audio object found outside Audio section";
            return false;
        }

        if (m_skipAllAudio)
        {
            std::cout << "SKIPPING audio file" << std::endl;
            return true;
        }

	QString id(atts.value("id"));
        QString file(atts.value("file"));
        QString label(atts.value("label"));

        if (id.isEmpty() || file.isEmpty() || label.isEmpty())
        {
            m_errorString = "Audio object has empty parameters";
            return false;
        }

        // attempt to insert file into AudioFileManager
        // (this checks the integrity of the file at the
        // same time)
        //
        if(getAudioFileManager().insertFile(qstrtostr(label),
                                         qstrtostr(file),
                                         id.toInt()) == false)
        {
            // Ok, now attempt to use the KFileDialog saved default
            // value for the AudioPath.
            //
            QString thing;
            KURL url = KFileDialog::getStartURL(QString(":WAVS"), thing);
            getAudioFileManager().setAudioPath(url.path().latin1());

            /*
            RG_DEBUG << "ATTEMPTING TO FIND IN PATH = " 
                << url.path() << endl;
                */

            if(getAudioFileManager().
                    insertFile(qstrtostr(label), 
                        qstrtostr(file), id.toInt()) == false)
            {

                // Freeze the progress dialog
                CurrentProgressDialog::freeze();

	        // Hide splash screen if present on startup
	        KStartupLogo::hideIfStillThere();

                // Create a locate file dialog - give it the file name
                // and the AudioFileManager path that we've already
                // tried.  If we manually locate the file then we reset
                // the audiofilepath to the new value and see if this
                // helps us locate the rest of the files.
                //

                QString newFilename = "";
                QString newPath = "";
            
                do
                {

                    FileLocateDialog fL((RosegardenGUIApp *)m_doc->parent(),
				        file,
				        QString(getAudioFileManager().getAudioPath().c_str()));
                    int result = fL.exec();

                    if (result == QDialog::Accepted)
                    {
                        newFilename = fL.getFilename();
                        newPath = fL.getDirectory();
                    }
                    else if (result == QDialog::Rejected)
                    {
                        // just skip the file
                        break;
                    } else
                    {
                        // don't process any more audio files
                        m_skipAllAudio = true;
                        CurrentProgressDialog::thaw();
                        return true;
                    }

    
                } while(getAudioFileManager().insertFile(qstrtostr(label),
                                                qstrtostr(newFilename),
                                                id.toInt()) == false);

                if (newPath != "")
                {
                    getAudioFileManager().setAudioPath(qstrtostr(newPath));
                    // Set a document post-modify flag
                    //m_doc->setModified(true);
                }

                getAudioFileManager().print();

                // Restore progress dialog's normal state
                CurrentProgressDialog::thaw();
            }
            else
            {
                // AudioPath is modified so set a document post modify flag
                //
                //m_doc->setModified(true);
            }

        }
        
    } else if (lcName == "audiopath") {

        if (m_section != InAudioFiles)
        {
            m_errorString = "Audiopath object found outside AudioFiles section";
            return false;
        }

        QString search(atts.value("value"));

        if (search.isEmpty())
        {
            m_errorString = "Audiopath has no value";
            return false;
        }

        getAudioFileManager().setAudioPath(qstrtostr(search));

    } else if (lcName == "begin") {
        float marker = atts.value("index").toFloat();

        if (!m_currentSegment)
        {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = "Found audio begin index in non audio segment";
            return false;
        }

        // convert to RealTime from float
        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        m_currentSegment->setAudioStartTime(Rosegarden::RealTime(sec, usec * 1000));


    } else if (lcName == "end") {
        float marker = atts.value("index").toFloat();

        if (!m_currentSegment)
        {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = "found audio end index in non audio segment";
            return false;
        }

        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        Rosegarden::RealTime markerTime(sec, usec * 1000);

        if (markerTime < m_currentSegment->getAudioStartTime())
        {
            m_errorString = "Audio end index before audio start marker";
            return false;
        }

        m_currentSegment->setAudioEndTime(markerTime);

        // Ensure we set end time according to correct RealTime end of Segment
        //
        Rosegarden::RealTime realEndTime = getComposition().
                getElapsedRealTime(m_currentSegment->getStartTime()) +
                m_currentSegment->getAudioEndTime() -
                m_currentSegment->getAudioStartTime();

        timeT absEnd = getComposition().getElapsedTimeForRealTime(realEndTime);
	m_currentSegment->setEndTime(absEnd);

    } else if (lcName == "fadein") {

        if (!m_currentSegment)
        {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = "found fade in time in non audio segment";
            return false;
        }

        float marker = atts.value("time").toFloat();
        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        Rosegarden::RealTime markerTime(sec, usec * 1000);

        m_currentSegment->setFadeInTime(markerTime);
        m_currentSegment->setAutoFade(true);


    } else if (lcName == "fadeout") {

        if (!m_currentSegment)
        {
            // Don't fail - as this segment could be defunct if we
            // skipped loading the audio file
            //
            return true;
        }

        if (m_currentSegment->getType() != Rosegarden::Segment::Audio)
        {
            m_errorString = "found fade out time in non audio segment";
            return false;
        }

        float marker = atts.value("time").toFloat();
        int sec = (int)marker;
        int usec = (int)((marker - ((float)sec)) * 1000000.0);
        Rosegarden::RealTime markerTime(sec, usec * 1000);

        m_currentSegment->setFadeOutTime(markerTime);
        m_currentSegment->setAutoFade(true);

    } else if (lcName == "device") {

        if (m_section != InStudio)
        {
            m_errorString = "Found Device outside Studio";
            return false;
        }

	m_haveControls = false;

        QString type = (atts.value("type")).lower();
        QString idString = atts.value("id");
        QString nameStr = atts.value("name");
            
        if (idString.isNull())
        {
            m_errorString = "No ID on Device tag";
            return false;
        }
        int id = idString.toInt();

        if (type == "midi")
        {
	    QString direction = atts.value("direction").lower();

	    if (direction.isNull() ||
		direction == "" ||
		direction == "play") { // ignore inputs

		// This will leave m_device set only if there is a
		// valid play midi device to modify:
		skipToNextPlayDevice();

		if (m_device) {
		    if (nameStr && nameStr != "") {
			m_device->setName(qstrtostr(nameStr));
		    }
		} else if (nameStr && nameStr != "") {
		    addMIDIDevice(nameStr, m_createDevices); // also sets m_device
		}
	    }

	    QString connection = atts.value("connection");
	    if (m_createDevices && m_device &&
		!connection.isNull() && connection != "") {
		setMIDIDeviceConnection(connection);
	    }

	    QString vstr = atts.value("variation").lower();
	    Rosegarden::MidiDevice::VariationType variation =
		Rosegarden::MidiDevice::NoVariations;
	    if (!vstr.isNull()) {
		if (vstr == "lsb") {
		    variation = Rosegarden::MidiDevice::VariationFromLSB;
		} else if (vstr == "msb") {
		    variation = Rosegarden::MidiDevice::VariationFromMSB;
		} else if (vstr == "") {
		    variation = Rosegarden::MidiDevice::NoVariations;
		}
	    }
	    Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>
		(m_device);
	    if (md) {
		md->setVariationType(variation);
	    }
	}
	else if (type == "softsynth")
        {
            m_device = getStudio().getDevice(id);

            if (m_device && m_device->getType() == Rosegarden::Device::SoftSynth)
                m_device->setName(qstrtostr(nameStr));
        }
	else if (type == "audio")
        {
            m_device = getStudio().getDevice(id);

            if (m_device && m_device->getType() == Rosegarden::Device::Audio)
                m_device->setName(qstrtostr(nameStr));
        }
        else
        {
            m_errorString = "Found unknown Device type";
            return false;
        }

    } else if (lcName == "librarian") {

        // The contact details for the maintainer of the banks/programs
        // information.
        //
        if (m_device && m_device->getType() == Rosegarden::Device::Midi)
        {
            QString name = atts.value("name");
            QString email = atts.value("email");

            dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
                setLibrarian(qstrtostr(name), qstrtostr(email));
        }

    } else if (lcName == "bank") {

        if (m_device) // only if we have a device
        {
            if (m_section != InStudio && m_section != InInstrument)
            {
                m_errorString = "Found Bank outside Studio or Instrument";
                return false;
            }

            QString nameStr = atts.value("name");
	    m_percussion = (atts.value("percussion").lower() == "true");
            m_msb = (atts.value("msb")).toInt();
            m_lsb = (atts.value("lsb")).toInt();

            // To actually create a bank
            //
            if (m_section == InStudio)
            {
                // Create a new bank
                Rosegarden::MidiBank bank(m_percussion,
					  m_msb,
					  m_lsb,
					  qstrtostr(nameStr));
    
                if (m_device->getType() == Rosegarden::Device::Midi)
                {
                    // Insert the bank
                    //
                    dynamic_cast<Rosegarden::MidiDevice*>(m_device)->addBank(bank);
                }
            }
            else // otherwise we're referencing it in an instrument
            if (m_section == InInstrument)
            {
                if (m_instrument)
                {
		    m_instrument->setPercussion(m_percussion);
                    m_instrument->setMSB(m_msb);
                    m_instrument->setLSB(m_lsb);
                    m_instrument->setSendBankSelect(true);
                }
            }
        }

    } else if (lcName == "program") {

        if (m_device) // only if we have a device
        {
            if (m_section == InStudio)
            {
                QString nameStr = (atts.value("name"));
                Rosegarden::MidiByte pc = atts.value("id").toInt();

                // Create a new program
                Rosegarden::MidiProgram program
		    (Rosegarden::MidiBank(m_percussion,
					  m_msb,
					  m_lsb),
		     pc,
		     qstrtostr(nameStr));

                if (m_device->getType() == Rosegarden::Device::Midi)
                {
                    // Insert the program
                    //
                    dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
                         addProgram(program);
                }

            }
            else if (m_section == InInstrument)
            {
                if (m_instrument)
                {
                    Rosegarden::MidiByte id = atts.value("id").toInt();
                    m_instrument->setProgramChange(id);
                    m_instrument->setSendProgramChange(true);
                }
            }
            else
            {
                m_errorString = "Found Program outside Studio and Instrument";
                return false;
            }
        }

    } else if (lcName == "controls") {

        // Only clear down the controllers list if we have found some controllers in the RG file
        //
        if (m_device)
        {
            dynamic_cast<Rosegarden::MidiDevice*>(m_device)->clearControlList();
        }

	m_haveControls = true;

    } else if (lcName == "control") {

        if (m_section != InStudio)
        {
            m_errorString = "Found ControlParameter outside Studio";
            return false;
        }

	if (!m_device) {
//!!! ach no, we can't give this warning -- we might be in a <device> elt
// but have no sequencer support, for example.  we need a separate m_inDevice
// flag
//	    m_deprecation = true;
//	    std::cerr << "WARNING: This Rosegarden file uses a deprecated control parameter structure.  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;

	} else if (m_device->getType() == Rosegarden::Device::Midi) {

	    if (!m_haveControls) {
		m_errorString = "Found ControlParameter outside Controls block";
		return false;
	    }

	    QString name = atts.value("name");
	    QString type = atts.value("type");
	    QString descr = atts.value("description");
	    QString min = atts.value("min");
	    QString max = atts.value("max");
	    QString def = atts.value("default");
	    QString conVal = atts.value("controllervalue");
	    QString colour = atts.value("colourindex");
	    QString ipbPosition = atts.value("ipbposition");
	    
	    Rosegarden::ControlParameter con(qstrtostr(name),
					     qstrtostr(type),
					     qstrtostr(descr),
					     min.toInt(),
					     max.toInt(),
					     def.toInt(),
					     Rosegarden::MidiByte(conVal.toInt()),
					     colour.toInt(),
					     ipbPosition.toInt());

            dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
		addControlParameter(con);
	}

    } else if (lcName == "reverb") { // deprecated but we still read 'em

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"reverb\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Reverb outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_REVERB, value);


    } else if (lcName == "chorus") { // deprecated but we still read 'em

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"chorus\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Chorus outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_CHORUS, value);

    } else if (lcName == "filter") { // deprecated but we still read 'em

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"filter\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Filter outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_FILTER, value);


    } else if (lcName == "resonance") { // deprecated but we still read 'em

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"resonance\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Resonance outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_RESONANCE, value);


    } else if (lcName == "attack") { // deprecated but we still read 'em

        if (!m_deprecation) 
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"attack\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Attack outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_ATTACK, value);

    } else if (lcName == "release") { // deprecated but we still read 'em

        if (!m_deprecation)
            std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"release\" (now replaced by a control parameter).  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
        m_deprecation = true;

        if (m_section != InInstrument)
        {
            m_errorString = "Found Release outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
            m_instrument->setControllerValue(Rosegarden::MIDI_CONTROLLER_RELEASE, value);

    } else if (lcName == "pan") {

        if (m_section != InInstrument && m_section != InBuss)
        {
            m_errorString = "Found Pan outside Instrument or Buss";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

	if (m_section == InInstrument) {
	    if (m_instrument)
	    {
		m_instrument->setPan(value);
		m_instrument->setSendPan(true);
	    }
	} else if (m_section == InBuss) {
	    if (m_buss) {
		m_buss->setPan(value);
	    }
	}

        // keep "velocity" so we're backwards compatible
    } else if (lcName == "velocity" || lcName == "volume") {

	if (lcName == "velocity") {
            if (!m_deprecation)
                std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"velocity\" for an overall MIDI instrument level (now replaced by \"volume\").  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
            m_deprecation = true;
	}

        if (m_section != InInstrument)
        {
            m_errorString = "Found Volume outside Instrument";
            return false;
        }

        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
        {
	    if (m_instrument->getType() == Rosegarden::Instrument::Audio ||
		m_instrument->getType() == Rosegarden::Instrument::SoftSynth) {
		// Backward compatibility: "volume" was in a 0-127
		// range and we now store "level" (float dB) instead.
		// Note that we have no such compatibility for
		// "recordLevel", whose range has changed silently.
		if (!m_deprecation) std::cerr << "WARNING: This Rosegarden file uses the deprecated element \"volume\" for an audio instrument (now replaced by \"level\").  We recommend re-saving the file from this version of Rosegarden to assure your ability to re-load it in future versions" << std::endl;
		m_deprecation = true;
		m_instrument->setLevel
		    (Rosegarden::AudioLevel::multiplier_to_dB(float(value) / 100.0));
	    } else {
		m_instrument->setVolume(value);
		m_instrument->setSendVolume(true);
	    }
        }

    } else if (lcName == "level") {
	
        if (m_section != InBuss &&
	    (m_section != InInstrument ||
	     (m_instrument &&
	      m_instrument->getType() != Rosegarden::Instrument::Audio &&
	      m_instrument->getType() != Rosegarden::Instrument::SoftSynth))) {
            m_errorString = "Found Level outside (audio) Instrument or Buss";
            return false;
        }

	float value = atts.value("value").toFloat();

	if (m_section == InBuss) {
	    if (m_buss) m_buss->setLevel(value);
	} else {
	    if (m_instrument) m_instrument->setLevel(value);
	}

    } else if (lcName == "controlchange") {

        if (m_section != InInstrument)
        {
            m_errorString = "Found ControlChange outside Instrument";
            return false;
        }

        Rosegarden::MidiByte type = atts.value("type").toInt();
        Rosegarden::MidiByte value = atts.value("value").toInt();

        if (m_instrument)
        {
            m_instrument->setControllerValue(type, value);
        }

    } else if (lcName == "plugin" || lcName == "synth") {

        if (m_section != InInstrument)
        {
	    if (m_section == InBuss) {
		m_instrument = 0;
		// and continue, effectively ignoring the plugin
	    } else {
		m_errorString = "Found Plugin outside Instrument";
		return false;
	    }
        }

        // Despite being InInstrument we might not actually have a valid
        // one.
        //
        if (m_instrument)
        {
            // Get the details
	    int position;
	    if (lcName == "synth") {
		position = Rosegarden::Instrument::SYNTH_PLUGIN_POSITION;
	    } else {
		position = atts.value("position").toInt();
	    }

	    bool bypassed = false;
	    QString bpStr = atts.value("bypassed");
	    if (bpStr.lower() == "true")
		bypassed = true;

	    std::string program = "";
	    QString progStr = atts.value("program");
	    if (progStr) {
		program = qstrtostr(progStr);
	    }

	    // Plugins are identified by a structured identifier
	    // string, but we will accept a LADSPA UniqueId if there's
	    // no identifier, for backward compatibility

	    QString identifier = atts.value("identifier");

            Rosegarden::AudioPlugin *plugin = 0;
	    Rosegarden::AudioPluginManager *apm = getAudioPluginManager();

	    if (!identifier) {
		if (atts.value("id")) {
		    unsigned long id = atts.value("id").toULong();
		    if (apm) plugin = apm->getPluginByUniqueId(id);
		}
	    } else {
		if (apm) plugin = apm->getPluginByIdentifier(identifier);
	    }

            // If we find the plugin all is well and good but if
            // we don't we just skip it.
            //
            if (plugin)
            {
                m_plugin = m_instrument->getPlugin(position);
		if (!m_plugin) {
		    RG_DEBUG << "WARNING: RoseXmlHandler: instrument "
			     << m_instrument->getId() << " has no plugin position "
			     << position << endl;
		} else {
		    m_plugin->setAssigned(true);
		    m_plugin->setBypass(bypassed);
		    m_plugin->setIdentifier(plugin->getIdentifier().data());
		    if (program != "") {
			m_plugin->setProgram(program);
		    }
		}
            } else {
		// we shouldn't be halting import of the RG file just because
		// we can't match a plugin
		//
		if (identifier) {
		    RG_DEBUG << "WARNING: RoseXmlHandler: plugin " << identifier << " not found" << endl;
		    m_pluginsNotFound.insert(identifier);
		} else if (atts.value("id")) {
		    RG_DEBUG << "WARNING: RoseXmlHandler: plugin uid " << atts.value("id") << " not found" << endl;
		} else {
		    m_errorString = "No plugin identifier or uid specified";
		    return false;
		}
            }
        } else { // no instrument

	    if (lcName == "synth") {
		QString identifier = atts.value("identifier");
		if (identifier) {
		    RG_DEBUG << "WARNING: RoseXmlHandler: no instrument for plugin " << identifier << endl;
		    m_pluginsNotFound.insert(identifier);
		}
	    }
	}

        m_section = InPlugin;

    } else if (lcName == "port") {

        if (m_section != InPlugin)
        {
            m_errorString = "Found Port outside Plugin";
            return false;
        }
        unsigned long portId = atts.value("id").toULong();
        Rosegarden::MappedObjectValue value = atts.value("value").toFloat();

        if (m_plugin)
        {
            m_plugin->addPort(portId, value);
        }

    } else if (lcName == "configure") {

        if (m_section != InPlugin)
        {
            m_errorString = "Found Configure outside Plugin";
            return false;
        }

        QString key = atts.value("key");
        QString value = atts.value("value");

        if (m_plugin)
        {
            m_plugin->setConfigurationValue(qstrtostr(key), qstrtostr(value));
        }

    } else if (lcName == "metronome") {

        if (m_section != InStudio)
        {
            m_errorString = "Found Metronome outside Studio";
            return false;
        }

        // Only create if we have a device
        //
        if (m_device && m_device->getType() == Rosegarden::Device::Midi)
        {
            Rosegarden::InstrumentId instrument =
                atts.value("instrument").toInt();

	    Rosegarden::MidiMetronome metronome(instrument);
	    
	    if (atts.value("pitch"))
		metronome.setPitch(atts.value("pitch").toInt());
	    if (atts.value("depth"))
		metronome.setDepth(atts.value("depth").toInt());
	    if (atts.value("barvelocity"))
		metronome.setBarVelocity(atts.value("barvelocity").toInt());
	    if (atts.value("beatvelocity"))
		metronome.setBeatVelocity(atts.value("beatvelocity").toInt());
	    if (atts.value("subbeatvelocity"))
		metronome.setSubBeatVelocity(atts.value("subbeatvelocity").toInt());

	    dynamic_cast<Rosegarden::MidiDevice*>(m_device)->
		setMetronome(metronome);
        }

    } else if (lcName == "instrument") {

        if (m_section != InStudio)
        {
            m_errorString = "Found Instrument outside Studio";
            return false;
        }

        m_section = InInstrument;

        Rosegarden::InstrumentId id = atts.value("id").toInt();
        std::string stringType = qstrtostr(atts.value("type"));
        Rosegarden::Instrument::InstrumentType type;

        if (stringType == "midi")
            type = Rosegarden::Instrument::Midi;
        else if (stringType == "audio")
            type = Rosegarden::Instrument::Audio;
        else if (stringType == "softsynth")
            type = Rosegarden::Instrument::SoftSynth;
        else
        {
            m_errorString = "Found unknown Instrument type";
            return false;
        }
            
        // Try and match an Instrument in the file with one in
        // our studio
        //
        Rosegarden::Instrument *instrument = getStudio().getInstrumentById(id);

        // If we've got an instrument and the types match then
        // we use it from now on.
        //
        if (instrument && instrument->getType() == type)
        {
            m_instrument = instrument;

            // We can also get the channel from this tag
            //
            Rosegarden::MidiByte channel =
                    (Rosegarden::MidiByte)atts.value("channel").toInt();
            m_instrument->setMidiChannel(channel);
        }

    } else if (lcName == "buss") {

        if (m_section != InStudio)
        {
            m_errorString = "Found Buss outside Studio";
            return false;
        }

        m_section = InBuss;

        Rosegarden::BussId id = atts.value("id").toInt();
        Rosegarden::Buss *buss = getStudio().getBussById(id);

        // If we've got a buss then we use it from now on.
        //
        if (buss) {
            m_buss = buss;
        } else {
	    m_buss = new Rosegarden::Buss(id);
	    getStudio().addBuss(m_buss);
	}

    } else if (lcName == "audiofiles") {

        if (m_section != NoSection)
        {
            m_errorString = "Found AudioFiles inside another section";
            return false;
        }

        m_section = InAudioFiles;


    } else if (lcName == "configuration") {

        setSubHandler(new ConfigurationXmlSubHandler
		      (lcName, &m_doc->getConfiguration()));

    } else if (lcName == "metadata") {
	
	if (m_section != InComposition) {
            m_errorString = "Found Metadata outside Composition";
            return false;
        }

        setSubHandler(new ConfigurationXmlSubHandler
		      (lcName, &getComposition().getMetadata()));

    } else if (lcName == "recordlevel") {

        if (m_section != InInstrument)
        {
            m_errorString = "Found recordLevel outside Instrument";
            return false;
        }

        float value = atts.value("value").toFloat();

	// if the value retrieved is greater than (say) 15 then we
	// must have an old-style 0-127 value instead of a shiny new
	// dB value, so convert it
	if (value > 15.0) {
	    value = Rosegarden::AudioLevel::multiplier_to_dB(value / 100);
	}

        if (m_instrument) m_instrument->setRecordLevel(value);

    } else if (lcName == "audioinput") {

        if (m_section != InInstrument)
        {
            m_errorString = "Found audioInput outside Instrument";
            return false;
        }

        int value = atts.value("value").toInt();
	int channel = atts.value("channel").toInt();

	QString type = atts.value("type");
	if (type) {
	    if (type.lower() == "buss") {
		if (m_instrument) m_instrument->setAudioInputToBuss(value, channel);
	    } else if (type.lower() == "record") {
		if (m_instrument) m_instrument->setAudioInputToRecord(value, channel);
	    }
	}

    } else if (lcName == "audiooutput") {

        if (m_section != InInstrument)
        {
            m_errorString = "Found audioOutput outside Instrument";
            return false;
        }

        int value = atts.value("value").toInt();
	if (m_instrument) m_instrument->setAudioOutput(value);

    } else if (lcName == "appearance") {

        m_section = InAppearance;

    } else if (lcName == "colourmap") {

        if (m_section == InAppearance) {
            QString mapName = atts.value("name");
            m_inColourMap = true;
            if (mapName == "segmentmap")
            {
                m_colourMap = &m_doc->getComposition().getSegmentColourMap();
            }
            else
            if (mapName == "generalmap")
            {
                m_colourMap = &m_doc->getComposition().getGeneralColourMap();
            }
            else
            { // This will change later once we get more of the Appearance code sorted out
                RG_DEBUG << "RoseXmlHandler::startElement : Found colourmap with unknown name\n";
            }
        }
        else {
            m_errorString = "Found colourmap outside Appearance";
            return false;
        }

    } else if (lcName == "colourpair") {

        if (m_inColourMap && m_colourMap)
        {
            unsigned int id = atts.value("id").toInt();
            QString name = atts.value("name");
            unsigned int red = atts.value("red").toInt();
            unsigned int blue = atts.value("blue").toInt();
            unsigned int green = atts.value("green").toInt();
            Rosegarden::Colour colour(red, green, blue);
            m_colourMap->addItem(colour, qstrtostr(name), id);
        }
        else
        {
            m_errorString = "Found colourpair outside ColourMap";
            return false;
        }

    } else if (lcName == "markers") {

        if (!m_inComposition)
        {
            m_errorString = "Found Markers outside Composition";
            return false;
        }

        // clear down any markers
        getComposition().clearMarkers();

    } else if (lcName == "marker") {
        if (!m_inComposition)
        {
            m_errorString = "Found Marker outside Composition";
            return false;
        }
        int time = atts.value("time").toInt();
        QString name = atts.value("name");
        QString descr = atts.value("description");

        Rosegarden::Marker *marker = 
            new Rosegarden::Marker(time,
                                   qstrtostr(name),
                                   qstrtostr(descr));

        getComposition().addMarker(marker);
    }
    else
    {
        RG_DEBUG << "RoseXmlHandler::startElement : Don't know how to parse this : " << qName << endl;
    }

    return true;
}

bool
RoseXmlHandler::endElement(const QString& namespaceURI,
                           const QString& localName,
                           const QString& qName)
{
    if (getSubHandler()) {
        bool finished;
        bool res = getSubHandler()->endElement(namespaceURI, localName, qName.lower(), finished);
        if (finished) setSubHandler(0);
        return res;
    }

    // Set percentage done
    //
    if ((m_totalElements > m_elementsSoFar) &&
	(++m_elementsSoFar % 300 == 0)) {

        emit setProgress(int(double(m_elementsSoFar) / double(m_totalElements) * 100.0));
        RosegardenProgressDialog::processEvents();
    }

    QString lcName = qName.lower();

    if (lcName == "rosegarden-data") {

	getComposition().updateTriggerSegmentReferences();

    } else if (lcName == "event") {

        if (m_currentSegment && m_currentEvent) {
            m_currentSegment->insert(m_currentEvent);
            m_currentEvent = 0;
        } else if (!m_currentSegment && m_currentEvent) {
            m_errorString = "Got event outside of a Segment";
            return false;
        }
        
    } else if (lcName == "chord") {

        m_currentTime += m_chordDuration;
        m_inChord = false;
        m_chordDuration = 0;

    } else if (lcName == "group") {

        m_inGroup = false;

    } else if (lcName == "segment") {

	if (m_currentSegment && m_segmentEndMarkerTime) {
	    m_currentSegment->setEndMarkerTime(*m_segmentEndMarkerTime);
	    delete m_segmentEndMarkerTime;
	    m_segmentEndMarkerTime = 0;
	}

        m_currentSegment = 0;
        m_section = NoSection;

    } else if (lcName == "bar-segment" || lcName == "tempo-segment") {
	
	m_currentSegment = 0;

    } else if (lcName == "composition") {
        m_inComposition = false;
        m_section = NoSection;

    } else if (lcName == "studio") {

        m_section = NoSection;

    } else if (lcName == "buss") {

        m_section = InStudio;
	m_buss = 0;

    } else if (lcName == "instrument") {

        m_section = InStudio;
        m_instrument = 0;

    } else if (lcName == "plugin") {

        m_section = InInstrument;
        m_plugin = 0;
        m_pluginId = 0;

    } else if (lcName == "device") {

        m_device = 0;

    } else if (lcName == "audiofiles") {

        m_section = NoSection;

    } else if (lcName == "appearance") {

        m_section = NoSection;

    } else if (lcName == "colourmap") {
        m_inColourMap = false;
        m_colourMap = 0;
    }

    return true;
}

bool
RoseXmlHandler::characters(const QString& s)
{
    if (m_subHandler)
        return m_subHandler->characters(s);

    return true;
}

QString
RoseXmlHandler::errorString()
{
    return m_errorString;
}

// Not used yet
bool
RoseXmlHandler::error(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::error( exception );
}

bool
RoseXmlHandler::fatalError(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::fatalError( exception );
}

bool
RoseXmlHandler::endDocument()
{
    if (m_foundTempo == false) getComposition().setDefaultTempo(120.0);

    return true;
}


void
RoseXmlHandler::setSubHandler(XmlSubHandler* sh)
{
    delete m_subHandler;
    m_subHandler = sh;
}


void
RoseXmlHandler::addMIDIDevice(QString name, bool createAtSequencer)
{
    unsigned int deviceId = 0;

    if (createAtSequencer) {

	QByteArray data;
	QByteArray replyData;
	QCString replyType;
	QDataStream arg(data, IO_WriteOnly);

	arg << (int)Device::Midi;
	arg << (unsigned int)Rosegarden::MidiDevice::Play;

	if (!rgapp->sequencerCall("addDevice(int, unsigned int)", replyType, replyData, data)) {
	    SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
			 << "can't call sequencer addDevice" << endl;
	    return;
	}

	if (replyType == "unsigned int") {
	    QDataStream reply(replyData, IO_ReadOnly);
	    reply >> deviceId;
	} else {
	    SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
			 << "got unknown returntype from addDevice()" << endl;
	    return;
	}

	if (deviceId == Device::NO_DEVICE) {
	    SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
			 << "sequencer addDevice failed" << endl;
	    return;
	}

	SEQMAN_DEBUG << "RoseXmlHandler::addMIDIDevice - "
		     << " added device " << deviceId << endl;

    } else {
	// Generate a new device id at the base Studio side only.
	// This may not correspond to any given device id at the
	// sequencer side.  We should _never_ do this in a document
	// that's actually intended to be retained for use, only
	// in temporary documents for device import etc.
	int tempId = -1;
	for (Rosegarden::DeviceListIterator i = getStudio().getDevices()->begin();
	     i != getStudio().getDevices()->end(); ++i) {
	    if (int((*i)->getId()) > tempId) tempId = int((*i)->getId());
	}
	deviceId = tempId + 1;
    }

    // add the device, so we can name it and set our pointer to it --
    // instruments will be sync'd later in the natural course of things
    getStudio().addDevice(qstrtostr(name), deviceId, Device::Midi);
    m_device = getStudio().getDevice(deviceId);
    m_deviceRunningId = deviceId;
}

void
RoseXmlHandler::skipToNextPlayDevice()
{
    SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice; m_deviceRunningId is " << m_deviceRunningId << endl;

    for (Rosegarden::DeviceList::iterator i = getStudio().getDevices()->begin();
	 i != getStudio().getDevices()->end(); ++i) {

	Rosegarden::MidiDevice *md =
	    dynamic_cast<Rosegarden::MidiDevice *>(*i);
	
	if (md && md->getDirection() == Rosegarden::MidiDevice::Play) {
	    if (m_deviceRunningId == Rosegarden::Device::NO_DEVICE ||
		md->getId() > m_deviceRunningId) {

		SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice: found next device: id " << md->getId() << endl;

		m_device = md;
		m_deviceRunningId = md->getId();
		return;
	    }
	}
    }

    SEQMAN_DEBUG << "RoseXmlHandler::skipToNextPlayDevice: fresh out of devices" << endl;

    m_device = 0;
}

void
RoseXmlHandler::setMIDIDeviceConnection(QString connection)
{
    SEQMAN_DEBUG << "RoseXmlHandler::setMIDIDeviceConnection(" << connection << ")" << endl;

    Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>(m_device);
    if (!md) return;
		    
    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << (unsigned int)md->getId();
    arg << connection;

    rgapp->sequencerCall("setPlausibleConnection(unsigned int, QString)",
                         replyType, replyData, data);
    // connection should be sync'd later in the natural course of things
}


