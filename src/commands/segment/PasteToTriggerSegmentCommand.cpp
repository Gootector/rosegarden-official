/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PasteToTriggerSegmentCommand.h"

#include "base/Event.h"
#include <klocale.h>
#include "misc/Strings.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/TriggerSegment.h"
#include <QString>


namespace Rosegarden
{

PasteToTriggerSegmentCommand::PasteToTriggerSegmentCommand(Composition *composition,
        Clipboard *clipboard,
        QString label,
        int basePitch,
        int baseVelocity) :
        NamedCommand(i18n("Paste as New Triggered Segment")),
        m_composition(composition),
        m_clipboard(new Clipboard(*clipboard)),
        m_label(label),
        m_basePitch(basePitch),
        m_baseVelocity(baseVelocity),
        m_segment(0),
        m_detached(false)
{
    // nothing else
}

PasteToTriggerSegmentCommand::~PasteToTriggerSegmentCommand()
{
    if (m_detached)
        delete m_segment;
    delete m_clipboard;
}

void
PasteToTriggerSegmentCommand::execute()
{
    if (m_segment) {

        m_composition->addTriggerSegment(m_segment, m_id, m_basePitch, m_baseVelocity);

    } else {

        if (m_clipboard->isEmpty())
            return ;

        m_segment = new Segment();

        timeT earliestStartTime = 0;
        timeT latestEndTime = 0;

        for (Clipboard::iterator i = m_clipboard->begin();
                i != m_clipboard->end(); ++i) {

            if (i == m_clipboard->begin() ||
                    (*i)->getStartTime() < earliestStartTime) {
                earliestStartTime = (*i)->getStartTime();
            }

            if ((*i)->getEndMarkerTime() > latestEndTime)
                latestEndTime = (*i)->getEndMarkerTime();
        }

        for (Clipboard::iterator i = m_clipboard->begin();
                i != m_clipboard->end(); ++i) {

            for (Segment::iterator si = (*i)->begin();
                    (*i)->isBeforeEndMarker(si); ++si) {
                if (!(*si)->isa(Note::EventRestType)) {
                    m_segment->insert
                    (new Event(**si,
                               (*si)->getAbsoluteTime() - earliestStartTime));
                }
            }
        }

        if (m_label == "" && m_clipboard->isSingleSegment()) {
            m_segment->setLabel(m_clipboard->getSingleSegment()->getLabel());
        }
        else {
            m_segment->setLabel(qstrtostr(m_label));
        }

        TriggerSegmentRec *rec =
            m_composition->addTriggerSegment(m_segment, m_basePitch, m_baseVelocity);
        if (rec)
            m_id = rec->getId();
    }

    m_composition->getTriggerSegmentRec(m_id)->updateReferences();

    m_detached = false;
}

void
PasteToTriggerSegmentCommand::unexecute()
{
    if (m_segment)
        m_composition->detachTriggerSegment(m_id);
    m_detached = true;
}

}
