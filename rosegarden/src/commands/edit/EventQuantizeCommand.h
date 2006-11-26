
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

#ifndef _RG_EVENTQUANTIZECOMMAND_H_
#define _RG_EVENTQUANTIZECOMMAND_H_

#include "document/BasicCommand.h"
#include <qobject.h>
#include <qstring.h>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;
class Quantizer;
class EventSelection;


class EventQuantizeCommand : public QObject, public BasicCommand
{
    Q_OBJECT

public:
    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         Quantizer *);
    
    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(EventSelection &selection,
                         Quantizer *);

    /// Constructs own quantizer based on KConfig data in given group
    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         QString configGroup,
                         bool notationDefault);
    
    /// Constructs own quantizer based on KConfig data in given group
    EventQuantizeCommand(EventSelection &selection,
                         QString configGroup,
                         bool notationDefault);

    ~EventQuantizeCommand();
    
    static QString getGlobalName(Quantizer *quantizer = 0);
    void setProgressTotal(int total) { m_progressTotal = total; }

signals:
    void incrementProgress(int);

protected:
    virtual void modifySegment();

private:
    Quantizer *m_quantizer; // I own this
    EventSelection *m_selection;
    QString m_configGroup;
    int m_progressTotal;

    /// Sets to m_quantizer as well as returning value
    Quantizer *makeQuantizer(QString, bool);
};

// Collapse equal-pitch notes into one event
//

}

#endif
