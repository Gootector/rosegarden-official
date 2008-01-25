/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "EditViewBase.h"
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include <kstddirs.h>
#include "misc/Debug.h"
#include "base/Clipboard.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "EditToolBox.h"
#include "EditTool.h"
#include "EditView.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/general/EditViewTimeSigNotifier.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include <kaction.h>
#include <kcommand.h>
#include <kconfig.h>
#include <kdockwidget.h>
#include <kedittoolbar.h>
#include <kglobal.h>
#include <kkeydialog.h>
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kxmlguiclient.h>
#include <qaccel.h>
#include <qcanvas.h>
#include <qdialog.h>
#include <qframe.h>
#include <qiconset.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

bool EditViewBase::m_inPaintEvent = false;
const unsigned int EditViewBase::ID_STATUS_MSG = 1;
const unsigned int EditViewBase::NbLayoutRows = 6;

EditViewBase::EditViewBase(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           unsigned int cols,
                           QWidget *parent, const char *name) :
        KDockMainWindow(parent, name),
        m_viewNumber( -1),
        m_viewLocalPropertyPrefix(makeViewLocalPropertyPrefix()),
        m_config(kapp->config()),
        m_doc(doc),
        m_segments(segments),
        m_tool(0),
        m_toolBox(0),
        m_mainDockWidget(0),
        m_centralFrame(0),
        m_grid(0),
        m_mainCol(cols - 1),
        m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
        m_needUpdate(false),
        m_pendingPaintEvent(0),
        m_havePendingPaintEvent(false),
        m_accelerators(0),
        m_configDialogPageIndex(0),
        m_inCtor(true),
        m_timeSigNotifier(new EditViewTimeSigNotifier(doc))
{

    QPixmap dummyPixmap; // any icon will do
    m_mainDockWidget = createDockWidget("Rosegarden EditView DockWidget", dummyPixmap,
                                        0L, "editview_dock_widget");
    // allow others to dock to the left and right sides only
    m_mainDockWidget->setDockSite(KDockWidget::DockLeft | KDockWidget::DockRight);
    // forbit docking abilities of m_mainDockWidget itself
    m_mainDockWidget->setEnableDocking(KDockWidget::DockNone);
    setView(m_mainDockWidget); // central widget in a KDE mainwindow
    setMainDockWidget(m_mainDockWidget); // master dockwidget

    m_centralFrame = new QFrame(m_mainDockWidget, "centralframe");
    m_grid = new QGridLayout(m_centralFrame, NbLayoutRows, cols);

    m_mainDockWidget->setWidget(m_centralFrame);

    initSegmentRefreshStatusIds();

    m_doc->attachEditView(this);

    QObject::connect
    (getCommandHistory(), SIGNAL(commandExecuted()),
     this, SLOT(update()));

    QObject::connect
    (getCommandHistory(), SIGNAL(commandExecuted()),
     this, SLOT(slotTestClipboard()));

    // create accelerators
    //
    m_accelerators = new QAccel(this);
}

EditViewBase::~EditViewBase()
{
    delete m_timeSigNotifier;

    m_doc->detachEditView(this);

    getCommandHistory()->detachView(actionCollection());
    m_viewNumberPool.erase(m_viewNumber);
    slotSaveOptions();
}

void EditViewBase::slotSaveOptions()
{}

void EditViewBase::readOptions()
{
    getToggleAction("options_show_statusbar")->setChecked(!statusBar()->isHidden());
    getToggleAction("options_show_toolbar")->setChecked(!toolBar()->isHidden());
}

void EditViewBase::setupActions(QString rcFileName, bool haveClipboard)
{
    setRCFileName(rcFileName);

    // Actions all edit views will have

    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()),
                            actionCollection(), "options_show_toolbar");

    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
                              actionCollection(), "options_show_statusbar");

    KStdAction::preferences(this,
                            SLOT(slotConfigure()),
                            actionCollection());

    KStdAction::keyBindings(this,
                            SLOT(slotEditKeys()),
                            actionCollection());

    KStdAction::configureToolbars(this,
                                  SLOT(slotEditToolbars()),
                                  actionCollection());


    // File menu
    KStdAction::save (this, SIGNAL(saveFile()), actionCollection());
    KStdAction::close(this, SLOT(slotCloseWindow()), actionCollection());

    if (haveClipboard) {
        KStdAction::cut (this, SLOT(slotEditCut()), actionCollection());
        KStdAction::copy (this, SLOT(slotEditCopy()), actionCollection());
        KStdAction::paste (this, SLOT(slotEditPaste()), actionCollection());
    }

    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/matrix.png");
    QIconSet icon = QIconSet(pixmap);
    new KAction(i18n("Open in Matri&x Editor"), icon, 0, this,
                SLOT(slotOpenInMatrix()), actionCollection(),
                "open_in_matrix");

    pixmap.load(pixmapDir + "/toolbar/matrix-percussion.png");
    icon = QIconSet(pixmap);
    new KAction(i18n("Open in &Percussion Matrix Editor"), icon, 0, this,
                SLOT(slotOpenInPercussionMatrix()), actionCollection(),
                "open_in_percussion_matrix");

    pixmap.load(pixmapDir + "/toolbar/notation.png");
    icon = QIconSet(pixmap);
    new KAction(i18n("Open in &Notation Editor"), icon, 0, this,
                SLOT(slotOpenInNotation()), actionCollection(),
                "open_in_notation");

    pixmap.load(pixmapDir + "/toolbar/eventlist.png");
    icon = QIconSet(pixmap);
    new KAction(i18n("Open in &Event List Editor"), icon, 0, this,
                SLOT(slotOpenInEventList()), actionCollection(),
                "open_in_event_list");

    new KAction(i18n("Set Segment Start Time..."), 0, this,
                SLOT(slotSetSegmentStartTime()), actionCollection(),
                "set_segment_start");

    new KAction(i18n("Set Segment Duration..."), 0, this,
                SLOT(slotSetSegmentDuration()), actionCollection(),
                "set_segment_duration");

    // add undo and redo to edit menu and toolbar
    getCommandHistory()->attachView(actionCollection());

}

void EditViewBase::slotConfigure()
{
    ConfigureDialog *configDlg =
        new ConfigureDialog(getDocument(), m_config, this);

    configDlg->showPage(getConfigDialogPageIndex());
    configDlg->show();
}

void EditViewBase::slotEditKeys()
{
    KKeyDialog::configure(actionCollection());
}

void EditViewBase::slotEditToolbars()
{
    KEditToolbar dlg(actionCollection(), getRCFileName());

    connect(&dlg, SIGNAL(newToolbarConfig()),
            SLOT(slotUpdateToolbars()));

    dlg.exec();
}

void EditViewBase::slotUpdateToolbars()
{
    createGUI(getRCFileName());
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

std::set<int> EditViewBase::m_viewNumberPool;

std::string
EditViewBase::makeViewLocalPropertyPrefix()
{
    static char buffer[100];
    int i = 0;
    while (m_viewNumberPool.find(i) != m_viewNumberPool.end())
        ++i;
    m_viewNumber = i;
    m_viewNumberPool.insert(i);
    sprintf(buffer, "View%d::", i);
    return buffer;
}

void EditViewBase::paintEvent(QPaintEvent* e)
{
    // It is possible for this function to be called re-entrantly,
    // because a re-layout procedure may deliberately ask the event
    // loop to process some more events so as to keep the GUI looking
    // responsive.  If that happens, we remember the events that came
    // in in the middle of one paintEvent call and process their union
    // again at the end of the call.
    /*
        if (m_inPaintEvent) {
    	NOTATION_DEBUG << "EditViewBase::paintEvent: in paint event already" << endl;
    	if (e) {
    	    if (m_havePendingPaintEvent) {
    		if (m_pendingPaintEvent) {
    		    QRect r = m_pendingPaintEvent->rect().unite(e->rect());
    		    *m_pendingPaintEvent = QPaintEvent(r);
    		} else {
    		    m_pendingPaintEvent = new QPaintEvent(*e);
    		}
    	    } else {
    		m_pendingPaintEvent = new QPaintEvent(*e);
    	    }
    	}
    	m_havePendingPaintEvent = true;
    	return;
        }
    */ 
    //!!!    m_inPaintEvent = true;

    if (isCompositionModified()) {

        // Check if one of the segments we display has been removed
        // from the composition.
        //
        // For the moment we'll have to close the view if any of the
        // segments we handle has been deleted.

        for (unsigned int i = 0; i < m_segments.size(); ++i) {

            if (!m_segments[i]->getComposition()) {
                // oops, I think we've been deleted
                close();
                return ;
            }
        }
    }


    m_needUpdate = false;

    // Scan all segments and check if they've been modified.
    //
    // If we have more than one segment modified, we need to update
    // them all at once with the same time range, otherwise we can run
    // into problems when the layout of one depends on the others.  So
    // we use updateStart/End to calculate a bounding range for all
    // modifications.

    timeT updateStart = 0, updateEnd = 0;
    int segmentsToUpdate = 0;
    Segment *singleSegment = 0;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        Segment* segment = m_segments[i];
        unsigned int refreshStatusId = m_segmentsRefreshStatusIds[i];
        SegmentRefreshStatus &refreshStatus =
            segment->getRefreshStatus(refreshStatusId);

        if (refreshStatus.needsRefresh() && isCompositionModified()) {

            // if composition is also modified, relayout everything
            refreshSegment(0);
            segmentsToUpdate = 0;
            break;

        } else if (m_timeSigNotifier->hasTimeSigChanged()) {

            // not exactly optimal!
            refreshSegment(0);
            segmentsToUpdate = 0;
            m_timeSigNotifier->reset();
            break;

        } else if (refreshStatus.needsRefresh()) {

            timeT startTime = refreshStatus.from(),
                              endTime = refreshStatus.to();

            if (segmentsToUpdate == 0 || startTime < updateStart) {
                updateStart = startTime;
            }
            if (segmentsToUpdate == 0 || endTime > updateEnd) {
                updateEnd = endTime;
            }
            singleSegment = segment;
            ++segmentsToUpdate;

            refreshStatus.setNeedsRefresh(false);
            m_needUpdate = true;
        }
    }

    if (segmentsToUpdate > 1) {
        refreshSegment(0, updateStart, updateEnd);
    } else if (segmentsToUpdate > 0) {
        refreshSegment(singleSegment, updateStart, updateEnd);
    }

    if (e)
        KMainWindow::paintEvent(e);

    // moved this to the end of the method so that things called
    // from this method can still test whether the composition had
    // been modified (it's sometimes useful to know whether e.g.
    // any time signatures have changed)
    setCompositionModified(false);

    //!!!    m_inPaintEvent = false;
    /*
        if (m_havePendingPaintEvent) {
    	e = m_pendingPaintEvent;
    	m_havePendingPaintEvent = false;
    	m_pendingPaintEvent = 0;
    	paintEvent(e);
    	delete e;
        }
    */
}

void EditViewBase::closeEvent(QCloseEvent* e)
{
    RG_DEBUG << "EditViewBase::closeEvent()\n";

    if (isInCtor()) {
        RG_DEBUG << "EditViewBase::closeEvent() : is in ctor, ignoring close event\n";
        e->ignore();
    } else {
        KMainWindow::closeEvent(e);
    }
}

void EditViewBase::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}

void EditViewBase::setTool(EditTool* tool)
{
    if (m_tool)
        m_tool->stow();

    m_tool = tool;

    if (m_tool)
        m_tool->ready();

}

void EditViewBase::slotCloseWindow()
{
    close();
}

void EditViewBase::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), this);

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();
}

void EditViewBase::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), this);

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
    statusBar()->changeItem(text, ID_STATUS_MSG);
}

void EditViewBase::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void EditViewBase::initSegmentRefreshStatusIds()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        unsigned int rid = m_segments[i]->getNewRefreshStatusId();
        m_segmentsRefreshStatusIds.push_back(rid);
    }
}

bool EditViewBase::isCompositionModified()
{
    return getDocument()->getComposition().getRefreshStatus
           (m_compositionRefreshStatusId).needsRefresh();
}

void EditViewBase::setCompositionModified(bool c)
{
    getDocument()->getComposition().getRefreshStatus
    (m_compositionRefreshStatusId).setNeedsRefresh(c);
}

bool EditViewBase::getSegmentsOnlyRestsAndClefs()
{
    using Rosegarden::Segment;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        Segment* segment = m_segments[i];

        for (Segment::iterator iter = segment->begin();
                iter != segment->end(); ++iter) {

            if (((*iter)->getType() != Note::EventRestType)
                    && ((*iter)->getType() != Clef::EventType))
                return false;
        }

    }

    return true;

}

void EditViewBase::toggleWidget(QWidget* widget,
                                const QString& toggleActionName)
{
    KToggleAction* toggleAction = getToggleAction(toggleActionName);

    if (!toggleAction) {
        RG_DEBUG << "!!! Unknown toggle action : " << toggleActionName << endl;
        return ;
    }

    widget->setShown(toggleAction->isChecked());
}

void
EditViewBase::slotTestClipboard()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): empty" << endl;

        stateChanged("have_clipboard", KXMLGUIClient::StateReverse);
        stateChanged("have_clipboard_single_segment",
                     KXMLGUIClient::StateReverse);
    } else {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): not empty" << endl;

        stateChanged("have_clipboard", KXMLGUIClient::StateNoReverse);
        stateChanged("have_clipboard_single_segment",
                     (getDocument()->getClipboard()->isSingleSegment() ?
                      KXMLGUIClient::StateNoReverse :
                      KXMLGUIClient::StateReverse));
    }
}

void
EditViewBase::slotToggleSolo()
{
    KToggleAction* toggleSoloAction = getToggleAction("toggle_solo");
    if (!toggleSoloAction)
        return ;

    bool newSoloState = toggleSoloAction->isChecked();

    RG_DEBUG << "EditViewBase::slotToggleSolo() : solo  = " << newSoloState << endl;
    emit toggleSolo(newSoloState);

    if (newSoloState) {
        emit selectTrack(getCurrentSegment()->getTrack());
    }

}

void
EditViewBase::slotStateChanged(const QString& s,
                               bool noReverse)
{
    RG_DEBUG << "EditViewBase::slotStateChanged " << s << ", " << noReverse << endl;
    stateChanged(s, noReverse ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
}

void
EditViewBase::slotSetSegmentStartTime()
{
    Segment *s = getCurrentSegment();
    if (!s)
        return ;

    TimeDialog dialog(this, i18n("Segment Start Time"),
                      &getDocument()->getComposition(),
                      s->getStartTime(), false);

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(i18n("Set Segment Start Time"));

        command->addSegment
        (s, dialog.getTime(),
         s->getEndMarkerTime() - s->getStartTime() + dialog.getTime(),
         s->getTrack());

        addCommandToHistory(command);
    }
}

void
EditViewBase::slotSetSegmentDuration()
{
    Segment *s = getCurrentSegment();
    if (!s)
        return ;

    TimeDialog dialog(this, i18n("Segment Duration"),
                      &getDocument()->getComposition(),
                      s->getStartTime(),
                      s->getEndMarkerTime() - s->getStartTime(), false);

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(i18n("Set Segment Duration"));

        command->addSegment
        (s, s->getStartTime(),
         s->getStartTime() + dialog.getTime(),
         s->getTrack());

        addCommandToHistory(command);
    }
}

void EditViewBase::slotCompositionStateUpdate()
{
    // update state of 'solo' toggle
    //
    KToggleAction* toggleSolo = getToggleAction("toggle_solo");
    if (!toggleSolo)
        return ;

    if (getDocument()->getComposition().isSolo()) {
        bool s = m_segments[0]->getTrack() == getDocument()->getComposition().getSelectedTrack();
        RG_DEBUG << "EditViewBase::slotCompositionStateUpdate() : set solo to " << s << endl;
        toggleSolo->setChecked(s);
    } else {
        toggleSolo->setChecked(false);
        RG_DEBUG << "EditViewBase::slotCompositionStateUpdate() : set solo to false\n";
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
    if (m_tool)
        m_tool->handleEventRemoved(event);
}

MultiViewCommandHistory* EditViewBase::getCommandHistory()
{
    return getDocument()->getCommandHistory();
}

KToggleAction* EditViewBase::getToggleAction(const QString& actionName)
{
    return dynamic_cast<KToggleAction*>(actionCollection()->action(actionName));
}

}
#include "EditViewBase.moc"
