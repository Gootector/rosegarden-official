
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

#ifndef XMLSTORABLEELEMENT_H
#define XMLSTORABLEELEMENT_H

#include <qxml.h>

#include "Event.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  *
  * Currently defined types (event->type()) :
  *
  * "note", "rest", "keychange"
  *
  * Currently defined properties :
  *
  * "pitch", "QuantizedDuration", "Notation::NoteType", "Notation:Accident"
  *
  */
class XMLStorableEvent : public Event
{
public:
    XMLStorableEvent(const QXmlAttributes& atts);
    XMLStorableEvent(const Event&);

    QString        toXMLString() const;
    static QString toXMLString(const Event&);
    
protected:
/*!
    timeT noteName2Duration(const QString &noteName);
    void initMap();

    typedef hash_map<string, timeT, hashstring, eqstring> namedurationmap;

    static namedurationmap m_noteName2DurationMap;
*/
};

#endif
