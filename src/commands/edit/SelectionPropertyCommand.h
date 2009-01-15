
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

#ifndef _RG_SELECTIONPROPERTYCOMMAND_H_
#define _RG_SELECTIONPROPERTYCOMMAND_H_

#include "base/PropertyName.h"
#include "document/BasicSelectionCommand.h"
#include <QString>
#include <QObject>


class Set;


namespace Rosegarden
{

class EventSelection;

// Patterns of properties
//
typedef enum 
{
    FlatPattern,          // set selection to velocity 1.
    AlternatingPattern,   // alternate between velocity 1 and 2 on
    // subsequent events.
    CrescendoPattern,     // increasing from velocity 1 to velocity 2.
    DecrescendoPattern,   // decreasing from velocity 1 to velocity 2.
    RingingPattern        // between velocity 1 and 2, dying away.
} PropertyPattern;


class SelectionPropertyCommand : public BasicSelectionCommand
{
public:

    SelectionPropertyCommand(EventSelection *selection,
                             const PropertyName &property,
                             PropertyPattern pattern,
                             int value1,
                             int value2);

    static QString getGlobalName() { return QObject::tr("Set &Property"); }

    virtual void modifySegment();

private:
    EventSelection *m_selection;
    PropertyName    m_property;
    PropertyPattern m_pattern;
    int                         m_value1;
    int                         m_value2;

};


}

#endif
