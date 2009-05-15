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

#include "NotationToolBox.h"

#include "gui/general/BaseTool.h"
#include "NotationWidget.h"
#include "NotationTool.h"
#include "NoteInserter.h"
#include "RestInserter.h"
#include "ClefInserter.h"
#include "TextInserter.h"
#include "GuitarChordInserter.h"
#include "NotationEraser.h"
#include "NotationSelector.h"
#include "NotationScene.h"

#include <QString>
#include <QMessageBox>

namespace Rosegarden
{

NotationToolBox::NotationToolBox(NotationWidget *parent) :
    BaseToolBox(parent),
    m_widget(parent),
    m_scene(0)
{
}

BaseTool *
NotationToolBox::createTool(QString toolName)
{
    NotationTool *tool = 0;

    QString toolNamelc = toolName.toLower();
    
    if (toolNamelc == NoteInserter::ToolName)

        tool = new NoteInserter(m_widget);
/*!!!
    else if (toolNamelc == RestInserter::ToolName)

        tool = new RestInserter(m_nParentView);

    else if (toolNamelc == ClefInserter::ToolName)

        tool = new ClefInserter(m_nParentView);

    else if (toolNamelc == TextInserter::ToolName)

        tool = new TextInserter(m_nParentView);

    else if (toolNamelc == GuitarChordInserter::ToolName)

        tool = new GuitarChordInserter(m_nParentView);

//    else if (toolNamelc == LilyPondDirectiveInserter::ToolName)

//        tool = new LilyPondDirectiveInserter(m_nParentView);

    else if (toolNamelc == NotationEraser::ToolName)

        tool = new NotationEraser(m_nParentView);
*/
    else if (toolNamelc == NotationSelector::ToolName)

        tool = new NotationSelector(m_widget);

    else {
        QMessageBox::critical(0, "", QString("NotationToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    if (m_scene) {
        tool->setScene(m_scene);
        connect(m_scene, SIGNAL(eventRemoved(Event *)),
                tool, SLOT(handleEventRemoved(Event *)));
    }

    return tool;
}

void
NotationToolBox::setScene(NotationScene *scene)
{
    m_scene = scene;

    for (QHash<QString, BaseTool *>::iterator i = m_tools.begin();
         i != m_tools.end(); ++i) {
        NotationTool *nt = dynamic_cast<NotationTool *>(*i);
        if (nt) {
            nt->setScene(scene);
            connect(scene, SIGNAL(eventRemoved(Event *)),
                    nt, SLOT(handleEventRemoved(Event *)));
        }
    }
}

}

#include "NotationToolBox.moc"

