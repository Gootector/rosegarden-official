
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SUSTAININSERTIONCOMMAND_H_
#define _RG_SUSTAININSERTIONCOMMAND_H_

#include "document/BasicCommand.h"
#include <qstring.h>
#include "base/Event.h"
#include <klocale.h>


class Pedal;


namespace Rosegarden
{

class Segment;
class Event;


class SustainInsertionCommand : public BasicCommand
{
public:
    SustainInsertionCommand(Segment &segment,
			    timeT time,
			    bool down,
			    int controllerNumber);
    virtual ~SustainInsertionCommand();

    static QString getGlobalName(bool down) {
	if (down) {
	    return i18n("Add Pedal &Press");
	} else {
	    return i18n("Add Pedal &Release");
	}
    }

    Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    bool m_down;
    int m_controllerNumber;
    Event *m_lastInsertedEvent;
};



}

#endif
