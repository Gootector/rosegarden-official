// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qlayout.h>

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kstatusbar.h>
#include <kmessagebox.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>

#include "editview.h"
#include "rosestrings.h"
#include "rosegardencanvasview.h"
#include "edittool.h"
#include "qcanvasgroupableitem.h"
#include "basiccommand.h"
#include "rosegardenguidoc.h"
#include "rosegardenconfiguredialog.h"
#include "multiviewcommandhistory.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "Clipboard.h"
#include "sequencemanager.h"

#include "rosedebug.h"

//----------------------------------------------------------------------
const unsigned int EditViewBase::ID_STATUS_MSG = 1;
const unsigned int EditViewBase::NbLayoutRows = 6;

EditViewBase::EditViewBase(RosegardenGUIDoc *doc,
                           std::vector<Rosegarden::Segment *> segments,
                           unsigned int cols,
                           QWidget *parent, const char *name) :
    KMainWindow(parent, name),
    m_viewNumber(-1),
    m_viewLocalPropertyPrefix(makeViewLocalPropertyPrefix()),
    m_config(kapp->config()),
    m_doc(doc),
    m_segments(segments),
    m_tool(0),
    m_toolBox(0),
    m_centralFrame(new QFrame(this)),
    m_grid(new QGridLayout(m_centralFrame, NbLayoutRows, cols)),
    m_mainCol(cols - 1),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_needUpdate(false),
    m_accelerators(0),
    m_configDialogPageIndex(0),
    m_shiftDown(false),
    m_controlDown(false),
    m_inCtor(true)
{
    initSegmentRefreshStatusIds();

    m_doc->attachEditView(this);
    
    setCentralWidget(m_centralFrame);

    m_centralFrame->setMargin(0);

    QObject::connect
        (getCommandHistory(), SIGNAL(commandExecuted()),
         this,                  SLOT(update()));

    QObject::connect
	(getCommandHistory(), SIGNAL(commandExecuted()),
	 this, SLOT(slotTestClipboard()));

    // create accelerators
    //
    m_accelerators = new QAccel(this);
}

EditViewBase::~EditViewBase()
{
    m_doc->detachEditView(this);

    getCommandHistory()->detachView(actionCollection());
    m_viewNumberPool.erase(m_viewNumber);
    slotSaveOptions();
}

void EditViewBase::slotSaveOptions()
{
}

void EditViewBase::readOptions()
{
    getToggleAction("options_show_statusbar")->setChecked(!statusBar()->isHidden());
    getToggleAction("options_show_toolbar")->setChecked(!toolBar()->isHidden());
}

void EditViewBase::setupActions(QString rcFileName)
{
    setRCFileName(rcFileName);

    // Actions all edit views will have

    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());


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
    KStdAction::close (this, SLOT(slotCloseWindow()),      actionCollection());

    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

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


    // add undo and redo to edit menu and toolbar
    getCommandHistory()->attachView(actionCollection());
    
}

void EditViewBase::slotConfigure()
{
    Rosegarden::ConfigureDialog *configDlg = 
        new Rosegarden::ConfigureDialog(getDocument(), m_config, this);

    configDlg->showPage(getConfigDialogPageIndex());
    configDlg->show();
}

void EditViewBase::slotEditKeys()
{
    KKeyDialog::configureKeys(actionCollection(), xmlFile(), true, this);
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


std::set<int>
EditViewBase::m_viewNumberPool;

std::string
EditViewBase::makeViewLocalPropertyPrefix()
{
    static char buffer[100];
    int i = 0;
    while (m_viewNumberPool.find(i) != m_viewNumberPool.end()) ++i;
    m_viewNumber = i;
    m_viewNumberPool.insert(i);
    sprintf(buffer, "View%d::", i);
    return buffer;
}

void EditViewBase::paintEvent(QPaintEvent* e)
{
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
		return;
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

    Rosegarden::timeT updateStart = 0, updateEnd = 0;
    int segmentsToUpdate = 0;
    Rosegarden::Segment *singleSegment = 0;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        Rosegarden::Segment* segment = m_segments[i];
        unsigned int refreshStatusId = m_segmentsRefreshStatusIds[i];
        Rosegarden::SegmentRefreshStatus &refreshStatus =
	    segment->getRefreshStatus(refreshStatusId);
        
        if (refreshStatus.needsRefresh() && isCompositionModified()) {

            // if composition is also modified, relayout everything
            refreshSegment(0);
	    segmentsToUpdate = 0;
	    break;
            
        } else if (refreshStatus.needsRefresh()) {

            Rosegarden::timeT startTime = refreshStatus.from(),
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

    if (e) KMainWindow::paintEvent(e);

    // moved this to the end of the method so that things called
    // from this method can still test whether the composition had
    // been modified (it's sometimes useful to know whether e.g.
    // any time signatures have changed)
    setCompositionModified(false);
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


MultiViewCommandHistory* EditViewBase::getCommandHistory()
{
    return getDocument()->getCommandHistory();
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

//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void EditViewBase::slotCloseWindow()
{
    close();
}

//
// Toolbar and statusbar toggling
//
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

//
// Status messages
//
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
    for(unsigned int i = 0; i < m_segments.size(); ++i) {

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

bool EditViewBase::getSegmentsOnlyRests()
{
    using Rosegarden::Segment;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        
        Segment* segment = m_segments[i];
        
        for (Segment::iterator iter = segment->begin();
             iter != segment->end(); ++iter) {

            if ((*iter)->getType() != Rosegarden::Note::EventRestType)
                return false;
        }
        
    }
    
    return true;
    
}


KToggleAction* EditViewBase::getToggleAction(const QString& actionName)
{
    return dynamic_cast<KToggleAction*>(actionCollection()->action(actionName));
}

void EditViewBase::toggleWidget(QWidget* widget,
                                const QString& toggleActionName)
{
    KToggleAction* toggleAction = getToggleAction(toggleActionName);

    if (!toggleAction) {
        RG_DEBUG << "!!! Unknown toggle action : " << toggleActionName << endl;
        return;
    }

    // Qt 3.1 only :
//     widget->setShown(toggleAction->isChecked());

    if (toggleAction->isChecked())
        widget->show();
    else
        widget->hide();
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
    RG_DEBUG << "EditViewBase::slotToggleSolo()\n";

    if(getDocument()->getComposition().isSolo()) {

        getDocument()->getComposition().setSolo(false);

    } else {

        emit selectTrack(m_segments[0]->getTrack());
        getDocument()->getComposition().setSolo(true);

    }
    
}

void
EditViewBase::slotStateChanged(const QString& s,
                               bool noReverse)
{
    stateChanged(s, noReverse ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse);
}


/*
 * Let tools know if their current element has gone
 */
void
EditViewBase::handleEventRemoved(Rosegarden::Event *event)
{
    if (m_tool) m_tool->handleEventRemoved(event);
}

