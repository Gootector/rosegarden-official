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


#include "MultiViewCommandHistory.h"

#include <klocale.h>
#include "misc/Debug.h"
#include <kactioncollection.h>
#include <kaction.h>
#include <kcommand.h>
#include <kstdaction.h>
#include <qobject.h>
#include <qpopupmenu.h>
#include <qregexp.h>
#include <qstring.h>
#include <kpopupmenu.h>


namespace Rosegarden
{

MultiViewCommandHistory::MultiViewCommandHistory() :
        m_undoLimit(50),
        m_redoLimit(50),
        m_savedAt(0)
{
    // nothing
}

MultiViewCommandHistory::~MultiViewCommandHistory()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
}

void
MultiViewCommandHistory::clear()
{
    m_savedAt = -1;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
}

void
MultiViewCommandHistory::attachView(KActionCollection *collection)
{
    if (m_views.find(collection) != m_views.end())
        return ;

    RG_DEBUG << "MultiViewCommandHistory::attachView() : setting up undo/redo actions\n";

    KToolBarPopupAction *undo = dynamic_cast<KToolBarPopupAction*>(collection->action(KStdAction::stdName(KStdAction::Undo)));

    if (undo) {
        connect(undo, SIGNAL(activated()),
                this, SLOT(slotUndo()));

        connect
        (undo->popupMenu(),
         SIGNAL(aboutToShow()),
         this,
         SLOT(slotUndoAboutToShow()));

        connect
        (undo->popupMenu(),
         SIGNAL(activated(int)),
         this,
         SLOT(slotUndoActivated(int)));
    }

    KToolBarPopupAction *redo = dynamic_cast<KToolBarPopupAction*>(collection->action(KStdAction::stdName(KStdAction::Redo)));

    if (redo) {

        connect(redo, SIGNAL(activated()),
                this, SLOT(slotRedo()));

        connect
        (redo->popupMenu(),
         SIGNAL(aboutToShow()),
         this,
         SLOT(slotRedoAboutToShow()));

        connect
        (redo->popupMenu(),
         SIGNAL(activated(int)),
         this,
         SLOT(slotRedoActivated(int)));
    }

    m_views.insert(collection);
    updateButtons();

}

void
MultiViewCommandHistory::detachView(KActionCollection *collection)
{
    ViewSet::iterator i = m_views.find(collection);
    if (i != m_views.end())
        m_views.erase(collection);
}

void
MultiViewCommandHistory::addCommand(KCommand *command, bool execute)
{
    if (!command)
        return ;

    RG_DEBUG << "MultiViewCommandHistory::addCommand: " << command->name() << endl;

    // We can't redo after adding a command
    clearStack(m_redoStack);

    // can we reach savedAt?
    if ((int)m_undoStack.size() < m_savedAt)
        m_savedAt = -1; // nope

    m_undoStack.push(command);
    clipCommands();

    if (execute) {
        command->execute();
        emit commandExecuted();
        emit commandExecuted(command);
    }

    updateButtons();
}

void
MultiViewCommandHistory::slotUndo()
{
    if (m_undoStack.empty())
        return ;

    KCommand *command = m_undoStack.top();
    command->unexecute();
    emit commandExecuted();
    emit commandExecuted(command);

    m_redoStack.push(command);
    m_undoStack.pop();

    clipCommands();
    updateButtons();

    if ((int)m_undoStack.size() == m_savedAt)
        emit documentRestored();
}

void
MultiViewCommandHistory::slotRedo()
{
    if (m_redoStack.empty())
        return ;

    KCommand *command = m_redoStack.top();
    command->execute();
    emit commandExecuted();
    emit commandExecuted(command);

    m_undoStack.push(command);
    m_redoStack.pop();
    // no need to clip
    updateButtons();
}

void
MultiViewCommandHistory::setUndoLimit(int limit)
{
    if (limit > 0 && limit != m_undoLimit) {
        m_undoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::setRedoLimit(int limit)
{
    if (limit > 0 && limit != m_redoLimit) {
        m_redoLimit = limit;
        clipCommands();
    }
}

void
MultiViewCommandHistory::documentSaved()
{
    m_savedAt = m_undoStack.size();
}

void
MultiViewCommandHistory::clipCommands()
{
    if ((int)m_undoStack.size() > m_undoLimit) {
        m_savedAt -= (m_undoStack.size() - m_undoLimit);
    }

    clipStack(m_undoStack, m_undoLimit);
    clipStack(m_redoStack, m_redoLimit);
}

void
MultiViewCommandHistory::clipStack(CommandStack &stack, int limit)
{
    int i;

    if ((int)stack.size() > limit) {

        CommandStack tempStack;
        for (i = 0; i < limit; ++i) {
            KCommand *togo = stack.top();
            KNamedCommand *named = dynamic_cast<KNamedCommand *>(togo);
            if (named) {
                RG_DEBUG << "MVCH::clipStack: Saving recent command: " << named->name() << " at " << togo << endl;
            } else {
                RG_DEBUG << "MVCH::clipStack: Saving recent unnamed command" << " at " << togo << endl;
            }
            tempStack.push(stack.top());
            stack.pop();
        }
        clearStack(stack);
        for (i = 0; i < m_undoLimit; ++i) {
            stack.push(tempStack.top());
            tempStack.pop();
        }
    }
}

void
MultiViewCommandHistory::clearStack(CommandStack &stack)
{
    while (!stack.empty()) {
        KCommand *togo = stack.top();
        KNamedCommand *named = dynamic_cast<KNamedCommand *>(togo);
        if (named) {
            RG_DEBUG << "MVCH::clearStack: About to delete command: " << named->name() << " at " << togo << endl;
        } else {
            RG_DEBUG << "MVCH::clearStack: About to delete unnamed command" << " at " << togo << endl;
        }
        delete togo;
        stack.pop();
    }
}

void
MultiViewCommandHistory::slotUndoActivated(int pos)
{
    for (int i = 0 ; i <= pos; ++i)
        slotUndo();
}

void
MultiViewCommandHistory::slotRedoActivated(int pos)
{
    for (int i = 0 ; i <= pos; ++i)
        slotRedo();
}

void
MultiViewCommandHistory::slotUndoAboutToShow()
{
    updateMenu(true, KStdAction::stdName(KStdAction::Undo), m_undoStack);
}

void
MultiViewCommandHistory::slotRedoAboutToShow()
{
    updateMenu(false, KStdAction::stdName(KStdAction::Redo), m_redoStack);
}

void
MultiViewCommandHistory::updateButtons()
{
    updateButton(true, KStdAction::stdName(KStdAction::Undo), m_undoStack);
    updateButton(false, KStdAction::stdName(KStdAction::Redo), m_redoStack);
}

void
MultiViewCommandHistory::updateButton(bool undo,
                                      const QString &name,
                                      CommandStack &stack)
{
    for (ViewSet::iterator i = m_views.begin(); i != m_views.end(); ++i) {

        KAction *action = (*i)->action(name);
        if (!action)
            continue;
        QString text;

        if (stack.empty()) {
            action->setEnabled(false);
            if (undo)
                text = i18n("Nothing to undo");
            else
                text = i18n("Nothing to redo");
            action->setText(text);
        } else {
            action->setEnabled(true);
            QString commandName = stack.top()->name();
            commandName.replace(QRegExp("&"), "");
            commandName.replace(QRegExp("\\.\\.\\.$"), "");
            if (undo)
                text = i18n("Und&o %1").arg(commandName);
            else
                text = i18n("Re&do %1").arg(commandName);
            action->setText(text);
        }
    }
}

void
MultiViewCommandHistory::updateMenu(bool undo,
                                    const QString &name,
                                    CommandStack &stack)
{
    for (ViewSet::iterator i = m_views.begin(); i != m_views.end(); ++i) {

        KAction *action = (*i)->action(name);
        if (!action)
            continue;

        KToolBarPopupAction *popupAction =
            dynamic_cast<KToolBarPopupAction *>(action);
        if (!popupAction)
            continue;

        QPopupMenu *menu = popupAction->popupMenu();
        if (!menu)
            continue;
        menu->clear();

        CommandStack tempStack;
        int j = 0;

        while (j < 10 && !stack.empty()) {

            KCommand *command = stack.top();
            tempStack.push(command);
            stack.pop();

            QString commandName = command->name();
            commandName.replace(QRegExp("&"), "");
            commandName.replace(QRegExp("\\.\\.\\.$"), "");

            QString text;
            if (undo)
                text = i18n("Und&o %1").arg(commandName);
            else
                text = i18n("Re&do %1").arg(commandName);
            menu->insertItem(text, j++);
        }

        while (!tempStack.empty()) {
            stack.push(tempStack.top());
            tempStack.pop();
        }
    }
}

}
#include "MultiViewCommandHistory.moc"
