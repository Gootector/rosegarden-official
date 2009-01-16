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


#include "AddMarkCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "document/CommandRegistry.h"
#include "base/BaseProperties.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;

QString
AddMarkCommand::getGlobalName(Mark markType)
{
    QString m = strtoqstr(markType);

    // Gosh, lots of collisions
    if (markType == Marks::Sforzando)
        m = QObject::tr("S&forzando");
    else if (markType == Marks::Staccato)
        m = QObject::tr("Sta&ccato");
    else if (markType == Marks::Rinforzando)
        m = QObject::tr("R&inforzando");
    else if (markType == Marks::Tenuto)
        m = QObject::tr("T&enuto");
    else if (markType == Marks::Trill)
        m = QObject::tr("Tri&ll");
    else if (markType == Marks::LongTrill)
        m = QObject::tr("Trill &with Line");
    else if (markType == Marks::TrillLine)
        m = QObject::tr("Trill Line");
    else if (markType == Marks::Turn)
        m = QObject::tr("&Turn");
    else if (markType == Marks::Accent)
        m = QObject::tr("&Accent");
    else if (markType == Marks::Staccatissimo)
        m = QObject::tr("&Staccatissimo");
    else if (markType == Marks::Marcato)
        m = QObject::tr("&Marcato");
    else if (markType == Marks::Pause)
        m = QObject::tr("&Pause");
    else if (markType == Marks::UpBow)
        m = QObject::tr("&Up-Bow");
    else if (markType == Marks::DownBow)
        m = QObject::tr("&Down-Bow");
    else if (markType == Marks::Mordent)
        m = QObject::tr("Mo&rdent");
    else if (markType == Marks::MordentInverted)
        m = QObject::tr("Inverted Mordent");
    else if (markType == Marks::MordentLong)
        m = QObject::tr("Long Mordent");
    else if (markType == Marks::MordentLongInverted)
        m = QObject::tr("Lon&g Inverted Mordent");
    else
        m = QObject::tr("&%1%2").arg(m[0].toUpper()).arg(m.right(m.length() - 1));
    // FIXME: That last i18n has very little chance of working, unless
    // by some miracle the exact same string was translated elsewhere already
    // but we'll leave it as a warning

    m = QObject::tr("Add %1").arg(m);
    return m;
}

void
AddMarkCommand::registerCommand(CommandRegistry *r)
{
    std::vector<Mark> marks(Marks::getStandardMarks());

    for (int i = 0; i < marks.size(); ++i) {
        Mark mark = marks[i];
        r->registerCommand
            (getGlobalName(mark), getIconName(mark),
             getShortcut(mark), getActionName(mark),
             new ArgumentAndSelectionCommandBuilder<AddMarkCommand>());
    }
}

QString
AddMarkCommand::getActionName(Mark mark)
{
    return QString("add_%1").arg(strtoqstr(mark));
}

QString
AddMarkCommand::getShortcut(Mark mark)
{
    return "";
}    

QString
AddMarkCommand::getIconName(Mark mark)
{
    return strtoqstr(mark);
}    

Mark
AddMarkCommand::getArgument(QString actionName, CommandArgumentQuerier &)
{
    QString pfx = "add_";
    if (actionName.startsWith(pfx)) {
        QString remainder = actionName.right(actionName.length() - pfx.length());
        return qstrtostr(remainder);
    }
    return "";
}

void
AddMarkCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
         i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;
        (*i)->get<Int>(MARK_COUNT, n);
        (*i)->set<Int>(MARK_COUNT, n + 1);
        (*i)->set<String>(getMarkPropertyName(n), m_mark);
    }
}

}
