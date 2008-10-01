/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TextInserter.h"

#include <klocale.h>
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/ViewElement.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/TextInsertionCommand.h"
#include "gui/dialogs/TextEventDialog.h"
#include "gui/general/EditTool.h"
#include "gui/general/LinedStaff.h"
#include "NotationTool.h"
#include "NotationView.h"
#include "NotePixmapFactory.h"
#include "NotationElement.h"
#include <QAction>
#include <QDialog>
#include <QIcon>
#include <QString>


namespace Rosegarden
{

TextInserter::TextInserter(NotationView* view)
        : NotationTool("TextInserter", view),
        m_text("", Text::Dynamic)
{
    QIcon icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::
                             makeToolbarPixmap("select")));
    QAction *qa_select = new QAction( "Switch to Select Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_select->setIcon(icon); 
			connect( qa_select, SIGNAL(triggered()), this, SLOT(slotSelectSelected())  );

    QAction *qa_erase = new QAction( "Switch to Erase Tool", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_erase->setIconText("eraser"); 
			connect( qa_erase, SIGNAL(triggered()), this, SLOT(slotEraseSelected())  );

    icon = QIcon
           (NotePixmapFactory::toQPixmap(NotePixmapFactory::
                                         makeToolbarPixmap("crotchet")));
    QAction *qa_notes = new QAction( "Switch to Inserting Notes", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_notes->setIcon(icon); 
			connect( qa_notes, SIGNAL(triggered()), this, SLOT(slotNotesSelected())  );

    createMenu("textinserter.rc");
}

void TextInserter::slotNotesSelected()
{
    m_nParentView->slotLastNoteAction();
}

void TextInserter::slotEraseSelected()
{
//     m_parentView->actionCollection()->action("erase")->activate();
	QAction *tac = findChild<QAction*>( "erase" );
	tac->setEnabled(true);
}

void TextInserter::slotSelectSelected()
{
//     m_parentView->actionCollection()->action("select")->activate();
	QAction *tac = findChild<QAction*>( "select" );
	tac->setEnabled(true);
}

void TextInserter::ready()
{
    m_nParentView->setCanvasCursor(Qt::crossCursor);
    m_nParentView->setHeightTracking(false);
}

void TextInserter::handleLeftButtonPress(timeT,
        int,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    if (staffNo < 0)
        return ;
    LinedStaff *staff = m_nParentView->getLinedStaff(staffNo);

    Text defaultText(m_text);
    timeT insertionTime;
    Event *eraseEvent = 0;

    if (element && element->event()->isa(Text::EventType)) {

        // edit an existing text, if that's what we clicked on

        try {
            defaultText = Text(*element->event());
        } catch (Exception e) {}

        insertionTime = element->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

        eraseEvent = element->event();

    } else {

        Event *clef = 0, *key = 0;

        NotationElementList::iterator closestElement =
            staff->getClosestElementToCanvasCoords(e->x(), (int)e->y(),
                                                   clef, key, false, -1);

        if (closestElement == staff->getViewElementList()->end())
            return ;

        insertionTime = (*closestElement)->event()->getAbsoluteTime(); // not getViewAbsoluteTime()

    }

    TextEventDialog *dialog = new TextEventDialog
                              (m_nParentView, m_nParentView->getNotePixmapFactory(), defaultText);

    if (dialog->exec() == QDialog::Accepted) {

        m_text = dialog->getText();

        TextInsertionCommand *command =
            new TextInsertionCommand
            (staff->getSegment(), insertionTime, m_text);

        if (eraseEvent) {
// 			MacroCommand *macroCommand = new MacroCommand(command->objectName());
			MacroCommand *macroCommand = new MacroCommand( "macro_command_29533" );	//@@@
			
			macroCommand->addCommand(new EraseEventCommand(staff->getSegment(),
                                     eraseEvent, false));
            macroCommand->addCommand(command);
            m_nParentView->addCommandToHistory(macroCommand);
        } else {
            m_nParentView->addCommandToHistory(command);
        }

        Event *event = command->getLastInsertedEvent();
        if (event)
            m_nParentView->setSingleSelectedEvent(staffNo, event);
    }

    delete dialog;
}

const QString TextInserter::ToolName     = "textinserter";

}
#include "TextInserter.moc"
