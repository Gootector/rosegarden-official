// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "rosedebug.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"

#include <klocale.h>

using Rosegarden::Composition;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;

using namespace Rosegarden::BaseProperties;


RoseXmlHandler::RoseXmlHandler(Composition &composition)
    : m_composition(composition),
      m_currentSegment(0),
      m_currentEvent(0),
      m_currentTime(0),
      m_chordDuration(0),
      m_inChord(false),
      m_inGroup(false),
      m_groupId(0),
      m_foundTempo(false)
{
//     kdDebug(KDEBUG_AREA) << "RoseXmlHandler() : composition size : "
//                          << m_composition.getNbSegments()
//                          << " addr : " << &m_composition
//                          << endl;
}

RoseXmlHandler::~RoseXmlHandler()
{
}

bool
RoseXmlHandler::startDocument()
{
    // reset state
    return true;
}

bool
RoseXmlHandler::startElement(const QString& /*namespaceURI*/,
                             const QString& /*localName*/,
                             const QString& qName, const QXmlAttributes& atts)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-data") {
        // set to some state which says it's ok to parse the rest

    } else if (lcName == "reference-segment") {

	if (m_currentSegment != 0) {
            m_errorString = i18n("Reference segment too late, one or more segments already read");
            return false;
	}

	m_currentSegment = m_composition.getReferenceSegment();
	m_currentTime = 0;

    } else if (lcName == "tempo") {

	QString tempoString = atts.value("value");
	m_composition.setTempo(tempoString.toInt());
	m_foundTempo = true;

    } else if (lcName == "segment") {

        int instrument = -1, startIndex = 0;
        QString instrumentNbStr = atts.value("instrument");
        if (instrumentNbStr) {
            instrument = instrumentNbStr.toInt();
//             kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : segment instr. nb = "
//                                  << instrumentNb << endl;
        }

        QString startIdxStr = atts.value("start");
        if (startIdxStr) {
            startIndex = startIdxStr.toInt();
//             kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : segment start idx = "
//                                  << startIndex << endl;
        }
        
        m_currentSegment = new Segment;
        m_currentSegment->setInstrument(instrument);
        m_currentSegment->setStartIndex(startIndex);
	m_currentTime = startIndex;

        m_composition.addSegment(m_currentSegment);
    
    } else if (lcName == "event") {

//        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: found event, current time is " << m_currentTime << endl;

        if (m_currentEvent) {
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: Warning: new event found at time " << m_currentTime << " before previous event has ended; previous event will be lost" << endl;
            delete m_currentEvent;
        }

        m_currentEvent = new XmlStorableEvent(atts);
        m_currentEvent->setAbsoluteTime(m_currentTime);

        if (m_inGroup) {
            m_currentEvent->setMaybe<Int>(BEAMED_GROUP_ID, m_groupId);
            m_currentEvent->set<String>(BEAMED_GROUP_TYPE, m_groupType);
	    if (m_groupType == "tupled") { //!!!
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_LENGTH, m_groupTupledLength);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_TUPLED_COUNT, m_groupTupledCount);
		m_currentEvent->set<Int>
		    (BEAMED_GROUP_UNTUPLED_LENGTH, m_groupUntupledLength);
	    }
        }
        
        if (!m_inChord) {

            m_currentTime += m_currentEvent->getDuration();

//            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: (we're not in a chord) " << endl;

        } else if (m_chordDuration == 0 &&
                   m_currentEvent->getDuration() != 0) {

            // set chord duration to the duration of the 1st element
            // with a non-null duration (if no such elements, leave it
            // to 0).

            m_chordDuration = m_currentEvent->getDuration();
        }
        
    } else if (lcName == "resync") {
	
	QString time(atts.value("time"));
	bool isNumeric;
	int numTime = time.toInt(&isNumeric);
	if (isNumeric) m_currentTime = numTime;

    } else if (lcName == "chord") {

        m_inChord = true;

    } else if (lcName == "group") {

        if (!m_currentSegment) {
            m_errorString = i18n("Got group outside of a segment");
            return false;
        }
        
        m_inGroup = true;
        m_groupId = m_currentSegment->getNextId();
        m_groupType = atts.value("type");

	if (m_groupType == "tupled") { //!!!
	    m_groupTupledLength = atts.value("length").toInt();
	    m_groupTupledCount = atts.value("count").toInt();
	    m_groupUntupledLength = atts.value("untupled").toInt();
	}

    } else if (lcName == "property") {
        
        if (!m_currentEvent) {
            kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement: Warning: Found property outside of event at time " << m_currentTime << ", ignoring" << endl;
        } else {
            m_currentEvent->setPropertiesFromAttributes(atts);
        }
        
    } else {
        kdDebug(KDEBUG_AREA) << "RoseXmlHandler::startElement : Don't know how to parse this : " << qName << endl;
    }

    return true;
}

bool
RoseXmlHandler::endElement(const QString& /*namespaceURI*/,
                           const QString& /*localName*/, const QString& qName)
{
    QString lcName = qName.lower();

    if (lcName == "event") {

        if (m_currentSegment && m_currentEvent) {
            m_currentSegment->insert(m_currentEvent);
            m_currentEvent = 0;
        } else if (!m_currentSegment && m_currentEvent) {
            m_errorString = i18n("Got event outside of a Segment");
            return false;
        }
        
    } else if (lcName == "chord") {

        m_currentTime += m_chordDuration;
        m_inChord = false;
        m_chordDuration = 0;

    } else if (lcName == "group") {

        m_inGroup = false;

    } else if (lcName == "segment") {

        m_currentSegment = 0;

    }

    return true;
}

bool
RoseXmlHandler::characters(const QString&)
{
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
    return QXmlDefaultHandler::error( exception );
}

bool
RoseXmlHandler::fatalError(const QXmlParseException& exception)
{
    return QXmlDefaultHandler::fatalError( exception );
}

bool
RoseXmlHandler::endDocument()
{
  if (m_foundTempo == false)
    m_composition.setTempo(120.0);

  return true;
}


