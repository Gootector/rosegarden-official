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

#include "EditViewBase.h"

#include "document/RosegardenGUIDoc.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "base/Clipboard.h"
#include "commands/segment/SegmentReconfigureCommand.h"

#include "gui/widgets/TmpStatusMsg.h"

#include "misc/Debug.h"

#include <QShortcut>
#include <QTabWidget>
#include <QAction>
#include <QStatusBar>
#include <QTabWidget>

namespace Rosegarden
{

EditViewBase::EditViewBase(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent, const char *name) :
    QMainWindow(parent, name),
    m_doc(doc),
    m_segments(segments),
    m_configDialogPageIndex(0),
    m_shortcuts(0)
{
    m_doc->attachEditView(this);

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotTestClipboard()));

    m_shortcuts = new QShortcut(this);
}

EditViewBase::~EditViewBase()
{
    m_doc->detachEditView(this);
    slotSaveOptions();
}

void EditViewBase::slotSaveOptions()
{
}

void EditViewBase::readOptions()
{
    QAction *a = findAction("options_show_statusbar");
    if (a) a->setChecked( ! statusBar()->isHidden() );

//    a = findAction("options_show_toolbar");
//    if (a) a->setChecked( ! m_toolBar->isHidden());
}

void EditViewBase::setupActions(bool haveClipboard)
{
    // Actions all edit views will have

//    createAction("options_show_toolbar", SLOT(slotToggleToolBar()));
    createAction("options_show_statusbar", SLOT(slotToggleStatusBar()));
    createAction("options_configure", SLOT(slotConfigure()));
//    createAction("options_configure_keybindings", SLOT(slotEditKeys()));
//    createAction("options_configure_toolbars", SLOT(slotEditToolbars()));

    createAction("file_save", SIGNAL(saveFile()));
    createAction("file_close", SLOT(slotCloseWindow()));

    if (haveClipboard) {
        createAction("edit_cut", SLOT(slotEditCut()));
        createAction("edit_copy", SLOT(slotEditCopy()));
        createAction("edit_paste", SLOT(slotEditPaste()));
    }

    createAction("open_in_matrix", SLOT(slotOpenInMatrix()));
    createAction("open_in_percussion_matrix", SLOT(slotOpenInPercussionMatrix()));
    createAction("open_in_notation", SLOT(slotOpenInNotation()));
    createAction("open_in_event_list", SLOT(slotOpenInEventList()));
    createAction("set_segment_start", SLOT(slotSetSegmentStartTime()));
    createAction("set_segment_duration", SLOT(slotSetSegmentDuration()));
}

void EditViewBase::slotConfigure()
{
    ConfigureDialog *configDlg =
        new ConfigureDialog(getDocument(), this);

//     configDlg->showPage(getConfigDialogPageIndex());
	configDlg->m_tabWidget->setCurrentIndex( getConfigDialogPageIndex() );
	
    configDlg->show();
}

void EditViewBase::slotEditKeys()
{
//&&&    KKeyDialog::configure(actionCollection());
}

void EditViewBase::slotEditToolbars()
{
//&&&
//    KEditToolbar dlg(actionCollection(), getRCFileName());

//    connect(&dlg, SIGNAL(newToolbarConfig()),
//            SLOT(slotUpdateToolbars()));

//    dlg.exec();
}

void EditViewBase::slotUpdateToolbars()
{
//!!!    createGUI(getRCFileName());
    //m_viewToolBar->setChecked(!toolBar()->isHidden());
}

void
EditViewBase::slotOpenInNotation()
{
    emit openInNotation(m_segments);
}

void
EditViewBase::slotOpenInMatrix()
{
    emit openInMatrix(m_segments);
}

void
EditViewBase::slotOpenInPercussionMatrix()
{
    emit openInPercussionMatrix(m_segments);
}

void
EditViewBase::slotOpenInEventList()
{
    emit openInEventList(m_segments);
}

void EditViewBase::closeEvent(QCloseEvent* e)
{
    RG_DEBUG << "EditViewBase::closeEvent()\n";
/*!!!
    if (isInCtor()) {
        RG_DEBUG << "EditViewBase::closeEvent() : is in ctor, ignoring close event\n";
        e->ignore();
    } else {
//         KMainWindow::closeEvent(e);
		close(e);
    }
*/
}

void EditViewBase::slotCloseWindow()
{
    close();
}
/*
void EditViewBase::slotToggleToolBar()
{
    TmpStatusMsg msg(tr("Toggle the toolbar..."), this);

    if (m_toolBar->isVisible())
		m_toolBar->hide();
    else
		m_toolBar->show();
}
*/
void EditViewBase::slotToggleStatusBar()
{
    TmpStatusMsg msg(tr("Toggle the statusbar..."), this);

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

void EditViewBase::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->showMessage(text);	//, ID_STATUS_MSG);
}

void EditViewBase::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void
EditViewBase::slotTestClipboard()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): empty" << endl;
        leaveActionState("have_clipboard");
	leaveActionState("have_clipboard_single_segment");
    } else {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): not empty" << endl;
        enterActionState("have_clipboard");
        if (getDocument()->getClipboard()->isSingleSegment()) {
            enterActionState("have_clipboard_single_segment");
        } else {
            leaveActionState("have_clipboard_single_segment");
        }           
    }
}

void
EditViewBase::slotToggleSolo()
{
    QAction *toggleSoloAction = findAction("toggle_solo");
    if (!toggleSoloAction) return;

    bool newSoloState = toggleSoloAction->isChecked();

    RG_DEBUG << "EditViewBase::slotToggleSolo() : solo  = " << newSoloState << endl;
    emit toggleSolo(newSoloState);

    if (newSoloState) {
        emit selectTrack(getCurrentSegment()->getTrack());
    }
}

void
EditViewBase::slotSetSegmentStartTime()
{
    Segment *s = getCurrentSegment();
    if (!s)
        return ;

    TimeDialog dialog(this, tr("Segment Start Time"),
                      &getDocument()->getComposition(),
                      s->getStartTime(), false);

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(tr("Set Segment Start Time"));

        command->addSegment
        (s, dialog.getTime(),
         s->getEndMarkerTime() - s->getStartTime() + dialog.getTime(),
         s->getTrack());

        CommandHistory::getInstance()->addCommand(command);
    }
}

void
EditViewBase::slotSetSegmentDuration()
{
    Segment *s = getCurrentSegment();
    if (!s)
        return ;

    TimeDialog dialog(this, tr("Segment Duration"),
                      &getDocument()->getComposition(),
                      s->getStartTime(),
                      s->getEndMarkerTime() - s->getStartTime(), false);

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(tr("Set Segment Duration"));

        command->addSegment
        (s, s->getStartTime(),
         s->getStartTime() + dialog.getTime(),
         s->getTrack());

        CommandHistory::getInstance()->addCommand(command);
    }
}

void
EditViewBase::slotCompositionStateUpdate()
{
    // update state of 'solo' toggle
    //
    QAction *toggleSolo = findAction("toggle_solo");
    if (!toggleSolo) return;

    if (getDocument()->getComposition().isSolo()) {
        bool s = m_segments[0]->getTrack() == getDocument()->getComposition().getSelectedTrack();
        RG_DEBUG << "ListEditView::slotCompositionStateUpdate() : set solo to " << s << endl;
        toggleSolo->setChecked(s);
    } else {
        toggleSolo->setChecked(false);
        RG_DEBUG << "ListEditView::slotCompositionStateUpdate() : set solo to false\n";
    }

    // update the window caption
    //
    updateViewCaption();
}

void
EditViewBase::windowActivationChange(bool oldState)
{
    if (isActiveWindow()) {
        emit windowActivated();
    }
}

void
EditViewBase::handleEventRemoved(Event *event)
{
}

}
#include "EditViewBase.moc"
