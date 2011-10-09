/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "ParameterPattern.h"

#include "AlternatingParameterPattern.h"
#include "FlatParameterPattern.h"
#include "IncreaseParameterPattern.h"
#include "LinearParameterPattern.h"
#include "RingingParameterPattern.h"

#include "base/BaseProperties.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "misc/Strings.h"
#include <limits>

namespace Rosegarden
{

    /*** Nested class Situation  ***/

// Get the property name as a QString
// @author Tom Breton (Tehom)
QString
ParameterPattern::Situation::getPropertyNameQString(void) const
{
    return strtoqstr(m_property);
}
    /*** Nested class Result  ***/


// Get the selection we have.
EventSelection *
ParameterPattern::Result::getSelection(void)
{
    return m_situation->m_selection;
}

// Modify the segment we have.  This is the guts of
// SelectionPropertyCommand.
void
ParameterPattern::Result::modifySegment(void)
{
    typedef EventSelection::eventcontainer::iterator iterator;
    const EventSelection *selection = m_situation->m_selection;

    iterator begin =
        selection->getSegmentEvents().begin();
    iterator end =
        selection->getSegmentEvents().end();

    m_pattern->setEventProperties(begin, end, this);
}

/* ***** Static data ***** */

ParameterPattern *
ParameterPattern::FlatPattern = &FlatParameterPattern::single;

// Making the vector is tricky.  To get past std::vector's lack of
// nice static initialization, we put everything into an array and
// initialize the vector from that.
       
// Internal
ParameterPattern * _VelocityPatterns[] = {
    &FlatParameterPattern::single,
    &AlternatingParameterPattern::single,
    &LinearParameterPattern::crescendo,
    &LinearParameterPattern::diminuendo,
    &RingingParameterPattern::single,
    &IncreaseParameterPattern::decrease,
    &IncreaseParameterPattern::increase,

};
// Internal
ParameterPattern ** _EndVelocityPatterns = 
    _VelocityPatterns + sizeof(_VelocityPatterns)/sizeof(_VelocityPatterns[0]);

// A vector of all the patterns that are useful for setting velocity.
ParameterPattern::ParameterPatternVec
ParameterPattern::VelocityPatterns(_VelocityPatterns, _EndVelocityPatterns);
 
/* ***** Helper functions ***** */
   
// Get the start and duration in timeT of the interval defined by
// begin and end.
ParameterPattern::StartAndDuration
ParameterPattern::getTimes (iterator begin, iterator end) 
{
    // Start with values such that anything we find will supersede
    // them.
    timeT endTime   = std::numeric_limits<int>::min();
    timeT startTime = std::numeric_limits<int>::max();

    for (iterator i = begin; i != end; ++i) {
        if ((*i)->getAbsoluteTime() < startTime) {
            startTime = (*i)->getAbsoluteTime();
        }

        if ((*i)->getAbsoluteTime() > endTime) {
            endTime = (*i)->getAbsoluteTime();
        }
    }

    return StartAndDuration(startTime, endTime - startTime);
}

/* ***** End-to-end functions ***** */

// Set some property, with a dialog
void
ParameterPattern::
setProperties(QMainWindow *parent,
              EventSelection *selection,
              PropertyName property,
              const ParameterPatternVec *patterns,
              int normValue)
{
    if (!selection) { return; }

    if (normValue < 0) {
        normValue = selection->getAverageProperty(property);
    }

    // situation will ultimately be owned by SelectionPropertyCommand via
    // dialog result
    Situation * situation =
        new Situation(property, selection, normValue);

    EventParameterDialog
        dialog(parent,
               EventParameterDialog::tr("Set Event Velocities"),
               situation,
               patterns);

    if (dialog.exec() == QDialog::Accepted) {
        TmpStatusMsg msg(EventParameterDialog::tr("Setting Velocities..."), 
                         parent);
        CommandHistory::getInstance()->addCommand
            (new SelectionPropertyCommand(dialog.getResult()));
    } else { delete situation; }
}

// Set velocities, with a dialog
void
ParameterPattern::
setVelocities(QMainWindow *parent,
              EventSelection *selection,
              int normVelocity)
{
    setProperties(parent, selection, BaseProperties::VELOCITY,
                  &ParameterPattern::VelocityPatterns, normVelocity);
}

// Set some property to targetValue, no dialog. 
void
ParameterPattern::
setPropertyFlat(EventSelection *selection,
                PropertyName property,
                int targetValue)
{
    if (!selection) { return; }

    Result
        result(new Situation(property, selection),
               ParameterPattern::FlatPattern,
               targetValue);

    CommandHistory::getInstance()->addCommand
        (new SelectionPropertyCommand(result));
}

// Set velocity to targetVelocity, no dialog. 
void
ParameterPattern::
setVelocitiesFlat(EventSelection *selection, int targetVelocity)
{
    setPropertyFlat(selection, BaseProperties::VELOCITY,
                    targetVelocity);
}

} // End namespace Rosegarden 
