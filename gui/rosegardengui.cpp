// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

// include files for QT
#include <qdragobject.h>

// include files for KDE
#include <kstdaccel.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kprinter.h>
#include <dcopclient.h>
#include <qiconset.h>
#include <kstddirs.h>

#include <kaction.h>
#include <kstdaction.h>

// application specific includes
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "MidiFile.h"
#include "rg21io.h"
#include "rosegardendcop.h"
#include "ktmpstatusmsg.h"
#include "SegmentPerformanceHelper.h"
#include "NotationTypes.h"

#define ID_STATUS_MSG 1

using std::cerr;
using std::endl;
using std::cout;

static Rosegarden::MappedComposition mappComp;

RosegardenGUIApp::RosegardenGUIApp()
    : KMainWindow(0), RosegardenIface(this), DCOPObject("RosegardenIface"),
      m_config(kapp->config()),
      m_fileRecent(0),
      m_view(0),
      m_doc(0),
      m_playLatency(0, 100000), // the sequencer's head start
      m_fetchLatency(0, 50000), // how long we allow to fetch and queue new events
      m_readAhead(0, 40000),    // how many events to fetch (microseconds)
      m_transportStatus(STOPPED),
      m_sequencerProcess(0),
      m_soundSystemStatus(Rosegarden::NO_SEQUENCE_SUBSYS)
{
    // accept dnd
    setAcceptDrops(true);

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    
    initStatusBar();
    initDocument();
    setupActions();

    initView();

    readOptions();

    actionCollection()->action("draw")->activate();

//     ///////////////////////////////////////////////////////////////////
//     // disable menu and toolbar items at startup
//     disableCommand(ID_FILE_SAVE);
//     disableCommand(ID_FILE_SAVE_AS);
//     disableCommand(ID_FILE_PRINT);
 
//     disableCommand(ID_EDIT_CUT);
//     disableCommand(ID_EDIT_COPY);
//     disableCommand(ID_EDIT_PASTE);
}

RosegardenGUIApp::~RosegardenGUIApp()
{
    kdDebug(KDEBUG_AREA) << "~RosegardenGUIApp()\n";

    if (m_sequencerProcess) {
        m_sequencerProcess->blockSignals(true);
        delete m_sequencerProcess;
    }
}


void RosegardenGUIApp::setupActions()
{
    // setup File menu
    // New Window ?
    m_fileNew  = KStdAction::openNew (this, SLOT(fileNew()),     actionCollection());
    m_fileOpen = KStdAction::open    (this, SLOT(fileOpen()),    actionCollection());
    m_fileRecent = KStdAction::openRecent(this,
                                          SLOT(fileOpenRecent(const KURL&)),
                                          actionCollection());
    m_fileSave   = KStdAction::save  (this, SLOT(fileSave()),          actionCollection());
    m_fileSaveAs = KStdAction::saveAs(this, SLOT(fileSaveAs()),        actionCollection());
    m_fileClose  = KStdAction::close (this, SLOT(fileClose()),         actionCollection());
    m_filePrint  = KStdAction::print (this, SLOT(filePrint()),         actionCollection());

    new KAction(i18n("Import &MIDI file..."), 0, 0, this,
                SLOT(importMIDI()), actionCollection(),
                "file_import_midi");

    new KAction(i18n("Import &Rosegarden 2.1 file..."), 0, 0, this,
                SLOT(importRG21()), actionCollection(),
                "file_import_rg21");

    new KAction(i18n("Export as &MIDI file..."), 0, 0, this,
                SLOT(exportMIDI()), actionCollection(),
                "file_export_midi");

    m_fileQuit = KStdAction::quit  (this, SLOT(quit()),              actionCollection());

    // setup edit menu
    m_editCut = KStdAction::cut      (this, SLOT(editCut()),        actionCollection());
    m_editCopy = KStdAction::copy     (this, SLOT(editCopy()),       actionCollection());
    m_editPaste = KStdAction::paste    (this, SLOT(editPaste()),      actionCollection());

    // setup Settings menu
    m_viewToolBar = KStdAction::showToolbar  (this, SLOT(toggleToolBar()),   actionCollection());
    m_viewTracksToolBar = new KToggleAction(i18n("Show Tracks Toolbar..."), 0, this,
                                            SLOT(toggleTracksToolBar()), actionCollection(),
                                            "show_tracks_toolbar");
    m_viewStatusBar = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());

    m_viewTransport = new KToggleAction(i18n("Show Transport"), 0, this,
                                             SLOT(toggleTransport()),
                                             actionCollection(),
                                             "show_transport");

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()),    actionCollection());

    KStdAction::keyBindings      (this, SLOT(editKeys()),     actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    KRadioAction *action = 0;
    
    // Create the select icon
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/select.xpm"));

    // TODO : add some shortcuts here
    action = new KRadioAction(i18n("&Select"), icon,
                              0,
                              this, SLOT(pointerSelected()),
                              actionCollection(), "select");

    action->setExclusiveGroup("segmenttools");
                             
    action = new KRadioAction(i18n("&Erase"), "eraser",
                              0,
                              this, SLOT(eraseSelected()),
                              actionCollection(), "erase");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Draw"), "pencil",
                              0,
                              this, SLOT(drawSelected()),
                              actionCollection(), "draw");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Move"), "move",
                              0,
                              this, SLOT(moveSelected()),
                              actionCollection(), "move");
    action->setExclusiveGroup("segmenttools");

    action = new KRadioAction(i18n("&Resize"), "misc", // TODO : find a better icon
                              0,
                              this, SLOT(resizeSelected()),
                              actionCollection(), "resize");
    action->setExclusiveGroup("segmenttools");

    new KAction(i18n("Change &Time Resolution..."), 
                0,
                this, SLOT(slotChangeTimeResolution()),
                actionCollection(), "change_time_res");

    new KAction(i18n("&Score Editor"),
                0,
                this, SLOT(editAllTracks()),
                actionCollection(), "edit_all_tracks");

    // Transport controls [rwb]
    //
    // We set some default key bindings - with numlock off
    // use 1 (End) and 3 (Page Down) for Rwd and Ffwd and
    // 0 (insert) and Enter for Play and Stop 
    //
    m_playTransport = new KAction(i18n("&Play"), 0, Key_Enter, this,
                                  SLOT(play()), actionCollection(),
                                  "play");
    m_playTransport->setGroup("transportcontrols");

    m_stopTransport = new KAction(i18n("&Stop"), 0, Key_Insert, this,
                                  SLOT(stop()), actionCollection(),
                                  "stop");
    m_stopTransport->setGroup("transportcontrols");

    m_ffwdTransport = new KAction(i18n("&Fast Forward"), 0, Key_PageDown,
                                  this,
                                  SLOT(fastforward()), actionCollection(),
                                  "fast_forward");
    m_ffwdTransport->setGroup("transportcontrols");

    m_rewindTransport = new KAction(i18n("Re&wind"), 0, Key_End, this,
                                  SLOT(rewind()), actionCollection(),
                                  "rewind");
    m_rewindTransport->setGroup("transportcontrols");

    m_recordTransport = new KAction(i18n("&Record"), 0, Key_End, this,
                                  SLOT(record()), actionCollection(),
                                  "record");

    m_recordTransport->setGroup("transportcontrols");

    m_rewindEndTransport = new KAction(i18n("Rewind to Beginning"), 0, 0, this,
                                  SLOT(rewindToBeginning()), actionCollection(),
                                  "rewindtobeginning");

    m_rewindEndTransport->setGroup("transportcontrols");

    m_ffwdEndTransport = new KAction(i18n("Fast Forward to End"), 0, 0, this,
                                  SLOT(fastForwardToEnd()), actionCollection(),
                                   "fastforwardtoend");

    m_ffwdEndTransport->setGroup("transportcontrols");

    // create the Transport GUI and add the callbacks
    //
    m_transport = new Rosegarden::RosegardenTransportDialog(this, "");

    // We should be plugging the actions into the buttons
    // on the transport - but this doesn't seem to be working
    // as expected (i.e. at all) so we're duplicating the
    // accelerators in the dialog - nasty but works for the
    // moment.
    //
/*
    m_playTransport->plug(m_transport->PlayButton);
    m_stopTransport->plug(m_transport->StopButton);
    m_rewindTransport->plug(m_transport->RewindButton);
    m_ffwdTransport->plug(m_transport->FfwdButton);
*/

    QAccel *a = new QAccel(m_transport);

    connect((QObject *) m_transport->PlayButton,
             SIGNAL(released()),
             SLOT(play()));

    a->connectItem(a->insertItem(Key_Enter),
                   this,
                   SLOT(play()));

    connect((QObject *) m_transport->StopButton,
             SIGNAL(released()),
             SLOT(stop()));
             
    a->connectItem(a->insertItem(Key_Insert),
                   this,
                   SLOT(stop()));

    connect((QObject *) m_transport->FfwdButton,
             SIGNAL(released()),
             SLOT(fastforward()));
            
    a->connectItem(a->insertItem(Key_PageDown),
                   this,
                   SLOT(fastforward()));

    connect((QObject *) m_transport->RewindButton,
             SIGNAL(released()),
             SLOT(rewind()));

    a->connectItem(a->insertItem(Key_End),
                   this,
                   SLOT(rewind()));

    connect((QObject *) m_transport->RecordButton,
             SIGNAL(released()),
             SLOT(record()));

    a->connectItem(a->insertItem(Key_Space),
                   this,
                   SLOT(record()));

    connect((QObject *) m_transport->RewindEndButton,
             SIGNAL(released()),
             SLOT(rewindToBeginning()));


    connect((QObject *) m_transport->FfwdEndButton,
             SIGNAL(released()),
             SLOT(fastForwardToEnd()));

    connect((QObject *)m_transport->MetronomeButton,
            SIGNAL(released()),
            SLOT(toggleMetronome()));
            
    connect((QObject *) m_transport->TimeDisplayButton,
             SIGNAL(released()),
             SLOT(refreshTimeDisplay()));

    connect((QObject *) m_transport->ToEndButton,
             SIGNAL(released()),
             SLOT(refreshTimeDisplay()));

    createGUI("rosegardenui.rc");

    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect((QObject *)m_transport, SIGNAL(closed()),
                                    SLOT(closeTransport()));

}


void RosegardenGUIApp::initStatusBar()
{
    statusBar()->insertItem(KTmpStatusMsg::getDefaultMsg(),
			    KTmpStatusMsg::getDefaultId(), 1);
    statusBar()->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
				  AlignLeft | AlignVCenter);
}

void RosegardenGUIApp::initDocument()
{
    m_doc = new RosegardenGUIDoc(this);
    m_doc->newDocument();

    m_doc->getCommandHistory()->attachView(actionCollection());
}

void RosegardenGUIApp::initView()
{ 
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::initView()" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();

    // Ensure that the start and end markers for the piece are set
    // to something reasonable
    //
    if (comp.getStartMarker() == 0 &&
        comp.getEndMarker() == 0)
    {
        int endMarker = comp.getBarRange(10 + comp.getNbBars()).second;
        comp.setEndMarker(endMarker);
    }
    
    m_view = new RosegardenGUIView(this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(m_view, SIGNAL(activateTool(SegmentCanvas::ToolType)),
            this,   SLOT(activateTool(SegmentCanvas::ToolType)));

    connect(m_view, SIGNAL(setGUIPositionPointer(Rosegarden::timeT)),
            this,   SLOT(setPointerPosition(Rosegarden::timeT)));

    connect(m_view, SIGNAL(setGUIPlayPosition(Rosegarden::timeT)),
            this,   SLOT(setPlayPosition(Rosegarden::timeT)));

    connect(m_view, SIGNAL(setGUILoop(Rosegarden::timeT, Rosegarden::timeT)),
            this,   SLOT(setLoop(Rosegarden::timeT, Rosegarden::timeT)));

    m_doc->addView(m_view);
    setCentralWidget(m_view);
    setCaption(m_doc->getTitle());

    // set the pointer position
    //
    setPointerPosition(comp.getElapsedRealTime(
                        m_doc->getComposition().getPosition()));

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // bring the transport to the front 
    //
    m_transport->raise();

    // set the play metronome button
    m_transport->MetronomeButton->setOn(comp.usePlayMetronome());

}

void RosegardenGUIApp::openDocumentFile(const char* _cmdl)
{
    KTmpStatusMsg msg(i18n("Opening file..."), statusBar());
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openDocumentFile("
                         << _cmdl << ")" << endl;

    m_doc->saveIfModified();
    m_doc->closeDocument();

    if (m_doc->openDocument(_cmdl)) {

        initView();

        // See comment in RosegardenGUIApp::openURL()
        // for an explanation on why we have to do this
        actionCollection()->action("draw")->activate();
        actionCollection()->action("move")->activate();
    }

}

void RosegardenGUIApp::openFile(const QString& url)
{

    setCaption(url);
    KURL *u = new KURL( url );

    if (u->isMalformed()) {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return;
    }

    if (!u->isLocalFile()) {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return;
    }

    QFileInfo info(u->path());

    if (!info.exists()) {
        KMessageBox::sorry(this, i18n("The specified file does not exist"));
        return;
    }

    if (info.isDir()) {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    QFile file(u->path());

    if (!file.open(IO_ReadOnly)) {
        KMessageBox::sorry(this, i18n("You do not have read permission to this file."));
        return;
    }

    // Stop if playing
    //
    if (m_transportStatus == PLAYING)
      stop();

    m_doc->closeDocument();
    m_doc->openDocument(u->path());

    initView();

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    m_doc->prepareAudio();

    Rosegarden::Composition &comp = m_doc->getComposition();

    // Set any loaded loop at the Composition and
    // on the marker on SegmentCanvas and clients
    //
    setLoop(comp.getLoopStart(), comp.getLoopEnd());
    m_doc->setLoopMarker(comp.getLoopStart(), comp.getLoopEnd());


}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void RosegardenGUIApp::saveOptions()
{	
    m_config->setGroup("General Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", m_viewToolBar->isChecked());
    m_config->writeEntry("Show Tracks Toolbar", m_viewTracksToolBar->isChecked());
    m_config->writeEntry("Show Transport", m_viewTransport->isChecked());
    m_config->writeEntry("Show Statusbar",m_viewStatusBar->isChecked());
    m_config->writeEntry("ToolBarPos", (int) toolBar("mainToolBar")->barPos());
    m_config->writeEntry("TracksToolBarPos", (int) toolBar("tracksToolBar")->barPos());

    m_fileRecent->saveEntries(m_config, "Recent Files");
}


void RosegardenGUIApp::readOptions()
{
    m_config->setGroup("General Options");

    // status bar settings
    bool viewStatusbar = m_config->readBoolEntry("Show Statusbar", true);
    m_viewStatusBar->setChecked(viewStatusbar);
    toggleStatusBar();

    bool viewToolBar = m_config->readBoolEntry("Show Toolbar", true);
    m_viewToolBar->setChecked(viewToolBar);
    toggleToolBar();

    viewToolBar = m_config->readBoolEntry("Show Tracks Toolbar", true);
    m_viewTracksToolBar->setChecked(viewToolBar);
    toggleTracksToolBar();

    bool viewTransport = m_config->readBoolEntry("Show Transport", true);
    m_viewTransport->setChecked(viewTransport);
    toggleTransport();

    // bar position settings
    KToolBar::BarPosition toolBarPos;
    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
    toolBar("mainToolBar")->setBarPos(toolBarPos);

    toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("TracksToolBarPos", KToolBar::Top);
    toolBar("tracksToolBar")->setBarPos(toolBarPos);
	
    // initialise the recent file list
    //
    m_fileRecent->loadEntries(m_config);

    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void RosegardenGUIApp::saveProperties(KConfig *_cfg)
{
    if (m_doc->getTitle()!=i18n("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename=m_doc->getAbsFilePath();	
        _cfg->writeEntry("filename", filename);
        _cfg->writeEntry("modified", m_doc->isModified());
		
        QString tempname = kapp->tempSaveName(filename);
        m_doc->saveDocument(tempname);
    }
}


void RosegardenGUIApp::readProperties(KConfig* _cfg)
{
    QString filename = _cfg->readEntry("filename", "");
    bool modified = _cfg->readBoolEntry("modified", false);

    if (modified) {
            bool canRecover;
            QString tempname = kapp->checkRecoverFile(filename, canRecover);
  	
            if (canRecover) {
                m_doc->openDocument(tempname);
                m_doc->setModified();
                QFileInfo info(filename);
                m_doc->setAbsFilePath(info.absFilePath());
                m_doc->setTitle(info.fileName());
                QFile::remove(tempname);
            }
        } else {
            if (!filename.isEmpty()) {
                m_doc->openDocument(filename);
            }
        }

    QString caption=kapp->caption();
    setCaption(caption+": "+m_doc->getTitle());
}		

void RosegardenGUIApp::dragEnterEvent(QDragEnterEvent *event)
{
    // accept uri drops only
    event->accept(QUriDrag::canDecode(event));
}

void RosegardenGUIApp::dropEvent(QDropEvent *event)
{
    // this is a very simplistic implementation of a drop event.  we
    // will only accept a dropped URL.  the Qt dnd code can do *much*
    // much more, so please read the docs there
    QStrList uri;

    // see if we can decode a URI.. if not, just ignore it
    if (QUriDrag::decode(event, uri))
    {
        // okay, we have a URI.. process it
        QString url, target;
        url = uri.first();

        // load in the file
        openURL(KURL(url));
    }
}


bool RosegardenGUIApp::queryClose()
{
    return m_doc->saveIfModified();
}

bool RosegardenGUIApp::queryExit()
{
    saveOptions();
    return true;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

// Not connected to anything at the moment
//
void RosegardenGUIApp::fileNewWindow()
{
    KTmpStatusMsg msg(i18n("Opening a new application window..."), statusBar());
	
    RosegardenGUIApp *new_window= new RosegardenGUIApp();
    new_window->show();
}

void RosegardenGUIApp::fileNew()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::fileNew()\n";

    KTmpStatusMsg msg(i18n("Creating new document..."), statusBar());

    bool makeNew = false;
    
    if (!m_doc->isModified()) {
        makeNew = true;
        m_doc->closeDocument();
    } else if (m_doc->saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {

        m_doc->newDocument();

        QString caption=kapp->caption();	
        setCaption(caption+": "+m_doc->getTitle());
        initView();

        // See comment in RosegardenGUIApp::openURL()
        // for an explanation on why we have to do this
        actionCollection()->action("move")->activate();
        actionCollection()->action("draw")->activate();
    }
}

void RosegardenGUIApp::openURL(const KURL& url)
{
    QString netFile = url.url();
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openURL: " << netFile << endl;

    if (url.isMalformed()) {
        QString string;
        string = i18n( "Malformed URL\n%1").arg(netFile);

        KMessageBox::sorry(this, string);
        return;
    }

    QString target;

    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, i18n("Cannot download file!"));
        return;
    }

    static QRegExp midiFile("\\.mid$"), rg21File("\\.rose$");

    if (midiFile.match(url.path()) != -1) {
        importMIDIFile(target);
    } else if (rg21File.match(url.path()) != -1) {
        importRG21File(target);
    } else {
        openFile(target);
    }

    setCaption(url.path());
    m_fileRecent->addURL(url);

    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openURL() : enabling 'move'\n";

    // We have to do this to make sure that the 2nd call ("move")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new SegmentCanvas will be
    // created but its tool won't be set, so clicking on the canvas
    // will crash.
    //
    actionCollection()->action("draw")->activate();
    actionCollection()->action("move")->activate();
}

void RosegardenGUIApp::fileOpen()
{
    statusHelpMsg(i18n("Opening file..."));

    KURL url = KFileDialog::getOpenURL(QString::null, "*.rg", this,
                                       i18n("Open File"));
    if ( url.isEmpty() ) { return; }

    openURL(url);
}

void RosegardenGUIApp::fileOpenRecent(const KURL &url)
{
    KTmpStatusMsg msg(i18n("Opening file..."), statusBar());
	
//     if (!doc->saveIfModified()) {
//         // here saving wasn't successful
//     } else {
//         doc->closeDocument();
//         doc->openDocument(recentFiles.at(id_));
//         QString caption=kapp->caption();	
//         setCaption(caption+": "+doc->getTitle());
//     }

//     initView();

    openURL(url);
}

void RosegardenGUIApp::fileSave()
{
    if (!m_doc || !m_doc->isModified()) return;

    KTmpStatusMsg msg(i18n("Saving file..."), statusBar());

    if (!m_doc->getAbsFilePath())
        fileSaveAs();
    else
        m_doc->saveDocument(m_doc->getAbsFilePath());
}

void RosegardenGUIApp::fileSaveAs()
{
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Saving file with a new filename..."), statusBar());

    QString newName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                 i18n("*.rg"), this, i18n("Save as..."));
    if (!newName.isEmpty()) {
        QFileInfo saveAsInfo(newName);
        m_doc->setTitle(saveAsInfo.fileName());
        m_doc->setAbsFilePath(saveAsInfo.absFilePath());
        m_doc->saveDocument(newName);
        m_fileRecent->addURL(newName);

        QString caption=kapp->caption();	
        setCaption(caption + ": " + m_doc->getTitle());
    }
}

void RosegardenGUIApp::fileClose()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::fileClose()" << endl;
    
    if (!m_doc) return;

    KTmpStatusMsg msg(i18n("Closing file..."), statusBar());
	
    m_doc->saveIfModified();

    m_doc->closeDocument();

    m_doc->newDocument();

    initView();

    // See comment in RosegardenGUIApp::openURL()
    // for an explanation on why we have to do this
    actionCollection()->action("move")->activate();
    actionCollection()->action("draw")->activate();

    //close();
}

void RosegardenGUIApp::filePrint()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
        return;
    }

    KTmpStatusMsg msg(i18n("Printing..."), statusBar());

    KPrinter printer;

    printer.setFullPage(true);

    if (printer.setup(this)) {
        m_view->print(&printer, &m_doc->getComposition());
    }
}

void RosegardenGUIApp::quit()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::quit()" << endl;
    
    statusMsg(i18n("Exiting..."));
    saveOptions();
    // close the first window, the list makes the next one the first again.
    // This ensures that queryClose() is called on each window to ask for closing
    KMainWindow* w;
    if (memberList) {

        for(w=memberList->first(); w!=0; w=memberList->first()) {
            // only close the window if the closeEvent is accepted. If
            // the user presses Cancel on the saveIfModified() dialog,
            // the window and the application stay open.
            if (!w->close())
                break;
        }
    }	
}

void RosegardenGUIApp::editCut()
{
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());
}

void RosegardenGUIApp::editCopy()
{
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());
}

void RosegardenGUIApp::editPaste()
{
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), statusBar());
}

void RosegardenGUIApp::toggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (m_viewToolBar->isChecked())
        toolBar("mainToolBar")->show();
    else
        toolBar("mainToolBar")->hide();
}

void RosegardenGUIApp::toggleTracksToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tracks toolbar..."), statusBar());

    if (m_viewTracksToolBar->isChecked())
        toolBar("tracksToolBar")->show();
    else
        toolBar("tracksToolBar")->hide();
}

void RosegardenGUIApp::toggleTransport()
{
    KTmpStatusMsg msg(i18n("Toggle the Transport"), statusBar());

    if (m_viewTransport->isChecked())
    {
        m_transport->show();
        m_transport->raise();
    }
    else
        m_transport->hide();
}

void RosegardenGUIApp::toggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if(!m_viewStatusBar->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}


void RosegardenGUIApp::statusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void RosegardenGUIApp::statusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void RosegardenGUIApp::pointerSelected()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::pointerSelected\n";
             
    m_view->pointerSelected();
}

void RosegardenGUIApp::eraseSelected()
{
    m_view->eraseSelected();
}

void RosegardenGUIApp::drawSelected()
{
    m_view->drawSelected();
}

void RosegardenGUIApp::moveSelected()
{
    m_view->moveSelected();
}

void RosegardenGUIApp::resizeSelected()
{
    m_view->resizeSelected();
}

#include <qlayout.h>
#include <qspinbox.h>
#include <kdialog.h>

class GetTimeResDialog : public KDialog
{
public:
    GetTimeResDialog(QWidget *parent = 0,
                     const char *name = 0,
                     bool modal = false, WFlags f = 0);

    void setInitialTimeRes(unsigned int);
    
    unsigned int getTimeRes() const;
    
protected:
    QSpinBox *m_spinbox;
};

GetTimeResDialog::GetTimeResDialog(QWidget *parent,
                                   const char *name,
                                   bool modal, WFlags f)
    : KDialog(parent, name, modal, f),
      m_spinbox(0)
{
    QVBoxLayout *box = new QVBoxLayout(this);
    box->setAutoAdd(true);
    
    new QLabel("Enter new time resolution", this);
    m_spinbox = new QSpinBox(this);
}


unsigned int GetTimeResDialog::getTimeRes() const
{
    return m_spinbox->value();
}

void GetTimeResDialog::setInitialTimeRes(unsigned int v)
{
    m_spinbox->setValue(v);
}



void RosegardenGUIApp::changeTimeResolution()
{
    GetTimeResDialog *dialog = new GetTimeResDialog(this);
    
    dialog->setInitialTimeRes(0);
    dialog->show();

    if (dialog->result()) {
        
        unsigned int timeResolution = dialog->getTimeRes();
    }
    
}

// This method is called from the Sequencer when it's playing.
// It's a request to get the next slice of events for the
// Sequencer to play.
//
//
const Rosegarden::MappedComposition&
RosegardenGUIApp::getSequencerSlice(const long &sliceStartSec,
                                    const long &sliceStartUsec,
                                    const long &sliceEndSec,
                                    const long &sliceEndUsec)
{
    // Clear the global return structure
    //
    mappComp.clear();

    // if we're closing then return no events
    //
    if (m_transportStatus == STOPPING)
      return mappComp;

    Rosegarden::Composition &comp = m_doc->getComposition();
  
    mappComp.setStartTime(Rosegarden::RealTime(sliceStartSec, sliceStartUsec));
    mappComp.setEndTime(Rosegarden::RealTime(sliceEndSec, sliceEndUsec));

    timeT sliceStartElapsed =
              comp.getElapsedTimeForRealTime(mappComp.getStartTime());

    timeT sliceEndElapsed =
              comp.getElapsedTimeForRealTime(mappComp.getEndTime());

    // Place metronome clicks in the global MappedCompisition
    // if they're enabled for our current mode
    //
    if ( ( ( m_transportStatus == PLAYING ||
             m_transportStatus == STARTING_TO_PLAY )
                && comp.usePlayMetronome() ) ||
         ( ( m_transportStatus == RECORDING_MIDI ||
             m_transportStatus == RECORDING_AUDIO ||
             m_transportStatus == STARTING_TO_RECORD_MIDI ||
             m_transportStatus == STARTING_TO_RECORD_AUDIO )
                && comp.useRecordMetronome() ) )
        insertMetronomeClicks(sliceStartElapsed, sliceEndElapsed);

    Rosegarden::RealTime eventTime;
    Rosegarden::RealTime duration;
  
    for (Rosegarden::Composition::iterator i = comp.begin();
                             i != comp.end(); i++ )
    {
        // Skip segment if the track is muted
        //
        if (comp.getTrackByIndex((*i)->getTrack())->isMuted())
            continue;

        if ((*i)->getType() == Rosegarden::Segment::Audio)
        {

            // An Audio event has three time parameters associated
            // with it.  The start of the Segment is when the audio
            // event should start.  The StartTime is how far into
            // the sample the playback should commence and the
            // EndTime is how far into the sample playback should
            // stop.
            //
            //
            if ((*i)->getStartTime() < sliceStartElapsed ||
                (*i)->getStartTime() > sliceEndElapsed)
                continue;

            cout << "AUDIO START = " << (*i)->getAudioStartTime() << endl;
            cout << "AUDIO END   = " << (*i)->getAudioEndTime() << endl;
            cout << "SLICE START = " << sliceStartElapsed << endl;
            cout << "SLICE END   = " << sliceEndElapsed << endl << endl;

            eventTime = comp.getElapsedRealTime((*i)->getStartTime());

            Rosegarden::RealTime startTime =
                       comp.getElapsedRealTime (((*i)->getAudioStartTime()));

            duration = comp.getElapsedRealTime
                            (((*i)->getAudioEndTime())) - startTime;

            assert (duration >= Rosegarden::RealTime(0,0));
            
            Rosegarden::TrackId track = (*i)->getTrack();

            Rosegarden::InstrumentId instrument = comp.
                             getTrackByIndex(track)->getInstrument();

            // Insert Audio event
            //
            Rosegarden::MappedEvent *me =
                    new Rosegarden::MappedEvent(eventTime,
                                                startTime,
                                                duration,
                                                instrument,
                                                track,
                                                (*i)->getAudioFileID());
            mappComp.insert(me);

            continue; // next Segment
        }
        else
        {
            // Skip the Segment if it starts too late to be of
            // interest to our slice.
            if ( (*i)->getStartTime() > sliceEndElapsed )
                continue;
        }

        Rosegarden::SegmentPerformanceHelper helper(**i);

        for ( Rosegarden::Segment::iterator j = (*i)->findTime(sliceStartElapsed);
                                          j != (*i)->end(); ++j )
        {
            // Skip this event if it isn't a note
            //
            if (!(*j)->isa(Rosegarden::Note::EventType))
                continue;

            // get the eventTime
            eventTime = comp.getElapsedRealTime((*j)->getAbsoluteTime());

            // As events are stored chronologically we can escape if
            // we're already beyond our event horizon for this slice.
            //
            if ( eventTime > mappComp.getEndTime() )
                break;

            // Eliminate events before our required time
            //
            if ( eventTime >= mappComp.getStartTime() &&
                 eventTime <= mappComp.getEndTime())
            {
		// Find the performance duration, i.e. taking into account any
		// ties etc that this note may have  --cc
		// 
		duration = helper.getRealSoundingDuration(j);
		
		// probably in a tied series, but not as first note
		//
		if (duration == Rosegarden::RealTime(0, 0))
		    continue;

                Rosegarden::TrackId track = (*i)->getTrack();

                Rosegarden::InstrumentId instrument = comp.
                             getTrackByIndex(track)->getInstrument();
                // insert event
                Rosegarden::MappedEvent *me =
                      new Rosegarden::MappedEvent(**j, eventTime, duration,
                                                  instrument, track);

                mappComp.insert(me);
            }
        }
    }

    //std::cout << "GOT SLICE OF " << mappComp.size() << " ELEMENTS" << endl;
    
    // send the MappedEvents to the GUI as well so we can process
    // them for the level meters
    //
    showVisuals(mappComp);

    return mappComp;
}


void RosegardenGUIApp::importMIDI()
{
    KURL url = KFileDialog::getOpenURL(QString::null, "*.mid", this,
                                     i18n("Open MIDI File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);
    importMIDIFile(tmpfile);

    // See comment in RosegardenGUIApp::openURL()
    // for an explanation on why we have to do this
    actionCollection()->action("draw")->activate();
    actionCollection()->action("move")->activate();
  
    KIO::NetAccess::removeTempFile( tmpfile );
}

void RosegardenGUIApp::importMIDIFile(const QString &file)
{
    Rosegarden::MidiFile *midiFile;

    midiFile = new Rosegarden::MidiFile(file.data());

    if (!midiFile->open())
    {
        KMessageBox::error(this,
          i18n("Couldn't understand MIDI file.\nIt might be corrupted."));
        return;
    }

    // Stop if playing
    //
    if (m_transportStatus == PLAYING)
        stop();

    m_doc->closeDocument();

    m_doc->newDocument();

    Rosegarden::Composition *tmpComp = midiFile->convertToRosegarden();

    m_doc->getComposition().swap(*tmpComp);

    delete tmpComp;

    // Set modification flag
    //
    m_doc->setModified();

    initView();
}

void RosegardenGUIApp::importRG21()
{
    KURL url = KFileDialog::getOpenURL(QString::null, "*.rose", this,
                                       i18n("Open Rosegarden 2.1 File"));
    if (url.isEmpty()) { return; }

    QString tmpfile;
    KIO::NetAccess::download(url, tmpfile);

    // Stop if playing
    //
    if (m_transportStatus == PLAYING)
        stop();
    
    importRG21File(tmpfile);

    KIO::NetAccess::removeTempFile(tmpfile);
}

void RosegardenGUIApp::importRG21File(const QString &file)
{
    RG21Loader rg21Loader(file);

    m_doc->closeDocument();
    m_doc->newDocument();
    Rosegarden::Composition *tmpComp = rg21Loader.getComposition();

    m_doc->getComposition().swap(*tmpComp);

    delete tmpComp;

    // Set modification flag
    //
    m_doc->setModified();

    initView();
}

void RosegardenGUIApp::setPointerPosition(const long &posSec,
                                          const long &posUsec)
{
    // We do this the lazily dangerous way of setting Composition
    // time and then gui time - we should probably make this
    // an atomic operation with observers or something to make
    // it nice and encapsulated ... but as long as we only EVER
    // use this modifier method for changing composition time
    // we can probably get away with it.

    // cc -- this seems like a good idea, but is it safe?
    //
    // rwb - it's not safe if you want initView to refresh the
    //       pointer position after a document load, so for the
    //       moment we lose this
    //

    Rosegarden::RealTime rT(posSec, posUsec);
    Rosegarden::Composition &comp = m_doc->getComposition();

    timeT elapsedTime = comp.getElapsedTimeForRealTime(rT);

    if (elapsedTime >= comp.getEndMarker())
    {
        stop();
        rT = Rosegarden::RealTime(0, 0);
        elapsedTime = 0;
    }

    // Set the composition time
    comp.setPosition(elapsedTime);

    // and the gui time
    m_view->setPointerPosition(elapsedTime);

    // and the tempo
    m_transport->setTempo(comp.getTempoAt(elapsedTime));

    // and the time...
    //
    if (m_transport->isShowingBarTime()) {

	displayBarTime(elapsedTime);

    } else {

	if (m_transport->isShowingTimeToEnd()) {
	    rT = rT - comp.getElapsedRealTime(comp.getDuration());
	}

	m_transport->displayTime(rT);
    }
}

void RosegardenGUIApp::setPointerPosition(timeT t)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    if ( m_transportStatus == PLAYING ||
         m_transportStatus == RECORDING_MIDI ||
         m_transportStatus == RECORDING_AUDIO )
    {
        if (t >= comp.getEndMarker())
        {
            stop();
            t = comp.getEndMarker();
        }

        sendSequencerJump(comp.getElapsedRealTime(t));
        return;
    }

    if (t >= comp.getEndMarker())
        t = 0;

    // set the composition time
    comp.setPosition(t);

    // and the gui time
    m_view->setPointerPosition(t);

    // and the tempo
    m_transport->setTempo(comp.getTempoAt(t));

    // and the time
    //
    if (m_transport->isShowingBarTime()) {

	displayBarTime(t);

    } else {

	Rosegarden::RealTime rT(comp.getElapsedRealTime(t));

	if (m_transport->isShowingTimeToEnd()) {
	    rT = rT - comp.getElapsedRealTime(comp.getDuration());
	}

	m_transport->displayTime(rT);
    }
}

void RosegardenGUIApp::displayBarTime(timeT t)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    int barNo = comp.getBarNumber(t);
    timeT barStart = comp.getBarStart(barNo);

    Rosegarden::TimeSignature timeSig = comp.getTimeSignatureAt(t);
    timeT beatDuration = timeSig.getBeatDuration();

    int beatNo = (t - barStart) / beatDuration;
    int unitNo = (t - barStart) - (beatNo * beatDuration);
    
    if (m_transport->isShowingTimeToEnd()) {
	barNo = barNo + 1 - comp.getNbBars();
	beatNo = timeSig.getBeatsPerBar() - 1 - beatNo;
	unitNo = timeSig.getBeatDuration() - 1 - unitNo;
    }
    
    m_transport->displayBarTime(barNo, beatNo, unitNo);
}

void RosegardenGUIApp::play()
{

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
  
    // If already playing or recording then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stop();
        return;
    }

    if (!m_sequencerProcess && !launchSequencer())
        return;

    // For the moment just throw this check in here
    // (see the note on the method for how we should
    // eventually use it)
    //
    if(!getSoundSystemStatus())
        return;

    // make sure we toggle the play button
    // 
    if (m_transport->PlayButton->state() == QButton::Off)
        m_transport->PlayButton->toggle();

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (m_doc->getComposition().getTempo() == 0)
    {
      cout <<
       "RosegardenGUIApp::play() - setting Tempo to Default value of 120.000"
        << endl;
      m_doc->getComposition().setDefaultTempo(120.0);
    }
    else
    {
        cout << "RosegardenGUIApp::play() - starting to play" <<  endl;
    }

    // set the tempo in the transport
    //
    m_transport->setTempo(m_doc->getComposition().getTempo());

    // The arguments for the Sequencer
    //

    Rosegarden::RealTime startPos = m_doc->getComposition().
                getElapsedRealTime(m_doc->getComposition().getPosition());

    // If we're looping then jump to loop start
    //
    if (m_doc->getComposition().isLooping())
        startPos = m_doc->getComposition().getElapsedRealTime(
                            m_doc->getComposition().getLoopStart());

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << m_playLatency.sec;
    streamOut << m_playLatency.usec;

    // fetch latency
    streamOut << m_fetchLatency.sec;
    streamOut << m_fetchLatency.usec;

    // read ahead slice
    streamOut << m_readAhead.sec;
    streamOut << m_readAhead.usec;

    // Send Play to the Sequencer
    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "play(long int, long int, long int, long int, long int, long int, long int, long int)",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        m_transportStatus = STOPPED;
        KMessageBox::error(this,
          i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result)
        {
            // completed successfully 
            m_transportStatus = STARTING_TO_PLAY;
        }
        else
        {
            m_transportStatus = STOPPED;
            KMessageBox::error(this, i18n("Failed to start playback"));
        }
    }

}

// Send stop request to Sequencer if playing, else
// return to start of segment
void RosegardenGUIApp::stop()
{
    // Animate the stop button - this seems to get caught looping
    // at the moment
    //
    // m_transport->StopButton->animateClick();

    if (m_transportStatus == STOPPED)
    {
        setPointerPosition(0);
        return;
    }

    // set the play metronome on
    //
    m_transport->MetronomeButton->setOn(m_doc->getComposition().usePlayMetronome());


    // If we're recording MIDI then tidy up the recording Segment
    //
    if (m_transportStatus == RECORDING_MIDI)
    {
        m_doc->stopRecordingMidi();
    } 


    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "stop()", data))
    {
        // failed - pop up and disable sequencer options
        KMessageBox::error(this,
        i18n("Failed to contact Rosegarden sequencer with \"Stop\" command"));
    }

    // always untoggle the play button at this stage
    //
    if (m_transport->PlayButton->state() == QButton::On)
        m_transport->PlayButton->toggle();

    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        // untoggle the record button
        //
        if (m_transport->RecordButton->state() == QButton::On)
            m_transport->RecordButton->toggle();

        cout << "RosegardenGUIApp::stop() - stopped recording" << endl;
    }
    else
    {
        cout << "RosegardenGUIApp::stop() - stopped playing" << endl;
    }

    // ok, we're stopped
    //
    m_transportStatus = STOPPED;

}

// Jump to previous bar
//
void RosegardenGUIApp::rewind()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition();
    
    // want to cope with bars beyond the actual end of the piece
    timeT newPosition = composition.getBarRangeForTime(position - 1).first;

    setPointerPosition(newPosition);
}


// Jump to next bar
//
void RosegardenGUIApp::fastforward()
{
    Rosegarden::Composition &composition = m_doc->getComposition();

    timeT position = composition.getPosition() + 1;
    timeT newPosition = composition.getBarRangeForTime(position).second;

    // we need to work out where the trackseditor finishes so we
    // don't skip beyond it.  Generally we need extra-Composition
    // non-destructive start and end markers for the piece.
    //
    setPointerPosition(newPosition);

}


void RosegardenGUIApp::refreshTimeDisplay()
{
    setPointerPosition(m_doc->getComposition().getPosition());
}


// This method is a callback from the Sequencer to update the GUI
// with state change information.  The GUI requests the Sequencer
// to start playing or to start recording and enters a pending
// state (see rosegardendcop.h for TransportStatus values).
// The Sequencer replies when ready with it's status.  If anything
// fails then we default (or try to default) to STOPPED at both
// the GUI and the Sequencer.
//
void RosegardenGUIApp::notifySequencerStatus(const int& status)
{
    // for the moment we don't do anything fancy
    m_transportStatus = (TransportStatus) status;
}


void RosegardenGUIApp::sendSequencerJump(const Rosegarden::RealTime &position)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << position.sec;
    streamOut << position.usec;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "jumpTo(long int, long int)",
                                  data))
    {
      // failed - pop up and disable sequencer options
      m_transportStatus = STOPPED;
      KMessageBox::error(this, i18n("Failed to contact Rosegarden sequencer"));
    }

    return;
}

void RosegardenGUIApp::editAllTracks()
{
    m_view->editAllTracks(&m_doc->getComposition());
}


// Sequencer auxiliary process management


bool RosegardenGUIApp::launchSequencer()
{
    if (m_sequencerProcess) return true;

    // If we've already registered a sequencer then don't start another
    // one
    //
    if (kapp->dcopClient()->
        isApplicationRegistered(QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
      return true;

    KTmpStatusMsg msg(i18n("Starting the sequencer..."), statusBar());
    
    m_sequencerProcess = new KProcess;

    (*m_sequencerProcess) << "rosegardensequencer";

    connect(m_sequencerProcess, SIGNAL(processExited(KProcess*)),
            this, SLOT(sequencerExited(KProcess*)));

    bool res = m_sequencerProcess->start();
    
    if (!res) {
        KMessageBox::error(0, i18n("Couldn't start the sequencer"));
        kdDebug(KDEBUG_AREA) << "Couldn't start the sequencer\n";
        m_sequencerProcess = 0;
    }

    return res;
}

void RosegardenGUIApp::sequencerExited(KProcess*)
{
    kdDebug(KDEBUG_AREA) << "Sequencer exited\n";

    KMessageBox::error(0, i18n("Sequencer exited"));

    m_sequencerProcess = 0;
}


void RosegardenGUIApp::exportMIDI()
{
    KTmpStatusMsg msg(i18n("Exporting to MIDI file..."), statusBar());

    QString fileName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
                                                  i18n("*.mid"), this, i18n("Export as..."));

    if (fileName.isEmpty())
      return;

    // Add a .mid extension if we don't already have one
    if (fileName.find(".mid", -4) == -1)
      fileName += ".mid";

    KURL *u = new KURL(fileName);

    if (u->isMalformed())
    {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return;
    }

    if (!u->isLocalFile())
    {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return;
    }

    QFileInfo info(fileName);

    if (info.isDir())
    {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    if (info.exists())
    {
        int overwrite = KMessageBox::questionYesNo(this,
                               i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes)
         return;
    }

    // Go ahead and export the file
    //
    exportMIDIFile(fileName);

}

void RosegardenGUIApp::exportMIDIFile(const QString &file)
{
    Rosegarden::MidiFile midiFile(file.data());

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write())
    {
        KMessageBox::sorry(this, i18n("The MIDI File has not been exported."));
        return;
    }
}

// Uncheck the transport window check box
void
RosegardenGUIApp::closeTransport()
{
     m_viewTransport->setChecked(false);
}

// Called when we want to start recording from the GUI.
// This method tells the sequencer to start recording and
// from then on the sequencer returns MappedCompositions
// to the GUI via the "processRecordedMidi() method -
// also called via DCOP
//
//

void
RosegardenGUIApp::record()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    // if already recording then stop
    //
    if (m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        stop();
        return;
    }

    // ensure these exist
    //
    if (!m_sequencerProcess && !launchSequencer())
        return;

    // For the moment just throw this check in here
    // (see the note on the method for how we should
    // eventually use it)
    //
    if(!getSoundSystemStatus())
        return;

    // toggle the Metronome button if it's in use
    //
    m_transport->MetronomeButton->setOn(comp.useRecordMetronome());

    // Adjust backwards by the bar count in
    //
    int startBar = comp.getBarNumber(comp.getPosition());
    startBar -= comp.getCountInBars();
    setPointerPosition(comp.getBarRange(startBar).first);


    // Some locals
    //
    TransportStatus recordType;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    // get the record track and check its type
    int rID = comp.getRecordTrack();

    switch (comp.getTrackByIndex(rID)->getType())
    {
        case Rosegarden::Track::Midi:
            recordType = STARTING_TO_RECORD_MIDI;
            cout << "RosegardenGUIApp::record() - starting to record MIDI" << endl;
            break;

        case Rosegarden::Track::Audio:
            recordType = STARTING_TO_RECORD_AUDIO;
            cout << "RosegardenGUIApp::record() - starting to record Audio" << endl;
            break;

        default:
            cout << "RosegardenGUIApp::record() - unrecognised track type" << endl;
            return;
            break;
    }

    // make sure we toggle the record button and play button
    // 
    if (m_transport->RecordButton->state() == QButton::Off)
        m_transport->RecordButton->toggle();

    if (m_transport->PlayButton->state() == QButton::Off)
        m_transport->PlayButton->toggle();

    // write the start position argument to the outgoing stream
    //
    QDataStream streamOut(data, IO_WriteOnly);

    if (comp.getTempo() == 0)
    {
        cout <<
         "RosegardenGUIApp::play() - setting Tempo to Default value of 120.000"
          << endl;
        comp.setDefaultTempo(120.0);
    }
    else
    {
        cout << "RosegardenGUIApp::record() - starting to record" << endl;
    }

    // set the tempo in the transport
    //
    m_transport->setTempo(comp.getTempo());

    // The arguments for the Sequencer  - record is similar to playback,
    // we must being playing to record
    //

    Rosegarden::RealTime startPos =comp.getElapsedRealTime(comp.getPosition());

    // playback start position
    streamOut << startPos.sec;
    streamOut << startPos.usec;

    // playback latency
    streamOut << m_playLatency.sec;
    streamOut << m_playLatency.usec;

    // fetch latency
    streamOut << m_fetchLatency.sec;
    streamOut << m_fetchLatency.usec;

    // read ahead slice
    streamOut << m_readAhead.sec;
    streamOut << m_readAhead.usec;

    // record type
    streamOut << (int)recordType;

    // Send Play to the Sequencer
    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "record(long int, long int, long int, long int, long int, long int, long int, long int, int)",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        m_transportStatus = STOPPED;
        KMessageBox::error(this,
          i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
  
        if (result)
        {
            // completed successfully 
            m_transportStatus = recordType;
        }
        else
        {
            m_transportStatus = STOPPED;
            KMessageBox::error(this, i18n("Failed to start recording"));
        }
    }

}



// This method accepts an incoming MappedComposition and goes about
// inserting it into the Composition and updating the display to show
// what has been recorded and where.
//
//
void
RosegardenGUIApp::processRecordedMidi(const Rosegarden::MappedComposition &mC)
{
    if (mC.size() > 0)
    {
/*
        std::cout << "processRecordMidi has returned a composition with " <<
                     mC.size() << " elements" << endl;
*/

        Rosegarden::MappedComposition::iterator i;

        // send all events to the MIDI in label
        //
        for (i = mC.begin(); i != mC.end(); ++i )
        {
            m_transport->setMidiInLabel(*i);
        }

    }

    // send any recorded Events to a Segment for storage and display
    //
    m_doc->insertRecordedMidi(mC);

}



// Process unexpected MIDI events at the GUI - send them to the Transport
// or to a MIDI mixer for display purposes only.  Useful feature to enable
// the musician to prove to herself quickly that the MIDI input is still
// working.
//
//
void
RosegardenGUIApp::processAsynchronousMidi(const Rosegarden::MappedComposition &mC)
{
    if (mC.size())
    {
/*
        std::cout << "RosegardenGUIApp::processAsynchronousMidi - GOT " <<
                      mC.size() << " ASYNC EVENT(S)" << endl;
*/

        Rosegarden::MappedComposition::iterator i;

        // send all events to the MIDI in label
        //
        for (i = mC.begin(); i != mC.end(); ++i )
        {
            m_transport->setMidiInLabel(*i);
        }
    }
}


// Double stop always does the trick
void
RosegardenGUIApp::rewindToBeginning()
{
    cout << "RosegardenGUIApp::rewindToBeginning()" << endl;
    setPointerPosition(0);

}


void
RosegardenGUIApp::fastForwardToEnd()
{
    cout << "RosegardenGUIApp::fastForwardToEnd()" << endl;

    Rosegarden::Composition &composition = m_doc->getComposition();
    Rosegarden::RealTime jumpTo = composition.getElapsedRealTime(composition.getDuration());

    setPointerPosition(jumpTo);
}


// We use this slot to active a tool mode on the GUI
// from a layer below the top level.  For example when
// we select a Track on the trackeditor and want this
// action to go on to select all the Segments on that
// Track we must change the edit mode to "Selector"
//
//
void
RosegardenGUIApp::activateTool(SegmentCanvas::ToolType tt)
{
    switch(tt)
    {
         case SegmentCanvas::Selector:
             actionCollection()->action("select")->activate();
             break;

         default:
             break;
    }
}


void
RosegardenGUIApp::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:  //  select add for segments
            m_view->setShift(true);
            break;

        case Key_Control:   // select copy of segments
            m_view->setControl(true);
            break;

        default:
            event->ignore();
            break;
    }

}


void
RosegardenGUIApp::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:  //  select add for segments
            m_view->setShift(false);
            break;

        case Key_Control:   // select copy of segments
            m_view->setControl(false);
            break;

        default:
            event->ignore();
            break;
    }

}


// Called from the LoopRuler (usually a double click)
// to set position and start playing
//
void
RosegardenGUIApp::setPlayPosition(Rosegarden::timeT position)
{

    // If already playing then stop
    //
    if (m_transportStatus == PLAYING ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO )
    {
        stop();
        return;
    }
    
    // otherwise off we go
    //
    setPointerPosition(position);
    play();
}


void
RosegardenGUIApp::setLoop(Rosegarden::timeT lhs, Rosegarden::timeT rhs)
{
    m_doc->getComposition().setLoopStart(lhs);
    m_doc->getComposition().setLoopEnd(rhs);
/*
    std::cout << "setGUILoop(): START = "
              << m_doc->getComposition().getLoopStart()
              << " END = "
              << m_doc->getComposition().getLoopEnd() << std::endl;
*/


    // Let the sequencer know about the loop markers
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    Rosegarden::RealTime loopStart =
            m_doc->getComposition().getElapsedRealTime(lhs);
    Rosegarden::RealTime loopEnd =
            m_doc->getComposition().getElapsedRealTime(rhs);

    streamOut << loopStart.sec;
    streamOut << loopStart.usec;
    streamOut << loopEnd.sec;
    streamOut << loopEnd.usec;
  
    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setLoop(long int, long int, long int, long int)", data))
    {
        // failed - pop up and disable sequencer options
        KMessageBox::error(this,
        i18n("Failed to contact Rosegarden sequencer with \"setLoop\""));
    }
}

// Process the outgoing sounds into some form of visual
// feedback at the GUI
//
void
RosegardenGUIApp::showVisuals(const Rosegarden::MappedComposition &mC)
{
    Rosegarden::MappedComposition::iterator it;

    for (it = mC.begin(); it != mC.end(); ++it )
    {
        if ((*it)->getType() == Rosegarden::MappedEvent::Internal)
        {
            m_transport->setMidiOutLabel(*it);
            m_view->showVisuals(*it);
        }
    }

}

// Once we have proper initialisation of the Sequencer we can use
// this method properly (i.e. once) - at the moment we just call
// it whenever we try to start the Sequencer
//
bool
RosegardenGUIApp::getSoundSystemStatus()
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getSoundSystemStatus()",
                                  data, replyType, replyData))
    {
        // failed - pop up and disable sequencer options
        KMessageBox::error(this,
        i18n("Failed to contact Rosegarden sequencer"));
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        int result;
        streamIn >> result;
        m_soundSystemStatus = (Rosegarden::SoundSystemStatus) result;

        switch(m_soundSystemStatus)
        {
            case Rosegarden::MIDI_AND_AUDIO_SUBSYS_OK:
                // we're fine
                break;

            case Rosegarden::MIDI_SUBSYS_OK:
                KMessageBox::error(this,
                        i18n("Audio subsystem has failed to initialise"));
                return false;
                break;

            case Rosegarden::AUDIO_SUBSYS_OK:
                KMessageBox::error(this,
                        i18n("MIDI subsystem has failed to initialise"));
                return false;
                break;
                
            case Rosegarden::NO_SEQUENCE_SUBSYS:
            default:
                KMessageBox::error(this,
                   i18n("MIDI and Audio subsystems have failed to initialise"));
                return false;
                break;
        }
    }

    return true;
}


// Insert metronome clicks into the global MappedComposition that
// will be returned as part of the slice fetch from the Sequencer.
//
//
void
RosegardenGUIApp::insertMetronomeClicks(timeT sliceStart, timeT sliceEnd)
{
    // move these outside at some point
    //
    int metronomePitch = 70;
    int metronomeInstrument = 0;
    int metronomeTrack = 0;
    int metronomeBarVely = 110;  // louder on bar start
    int metronomeBeatVely = 80;

    Rosegarden::Composition &comp = m_doc->getComposition();
    std::pair<timeT, timeT> barStart = comp.getBarRange(comp.getBarNumber(sliceStart));
    std::pair<timeT, timeT> barEnd = comp.getBarRange(comp.getBarNumber(sliceEnd));

    // The slice can straddle a bar boundary so check
    // in both bars for the marker
    //
    if (barStart.first >= sliceStart && barStart.first <= sliceEnd)
    {
        Rosegarden::MappedEvent *me =
            new Rosegarden::MappedEvent(metronomePitch,
                                        comp.getElapsedRealTime(barStart.first),
                                        Rosegarden::RealTime(0, 100000),
                                        metronomeBarVely,
                                        metronomeInstrument,
                                        metronomeTrack);


        mappComp.insert(me);
    }
    else if (barEnd.first >= sliceStart && barEnd.first <= sliceEnd)
    {
        Rosegarden::MappedEvent *me =
            new Rosegarden::MappedEvent(metronomePitch,
                                        comp.getElapsedRealTime(barEnd.first),
                                        Rosegarden::RealTime(0, 100000),
                                        metronomeBarVely,
                                        metronomeInstrument,
                                        metronomeTrack);


        mappComp.insert(me);
    }

    // Is this solution for the beats bulletproof?  I'm not so sure.
    //
    //
    bool isNew;
    Rosegarden::TimeSignature timeSig = comp.getTimeSignatureInBar(comp.getBarNumber(sliceStart), isNew);
    for (int i = barStart.first + timeSig.getBeatDuration();
             i < barStart.second;
             i += timeSig.getBeatDuration())
    {
        if (i >= sliceStart && i <= sliceEnd)
        {
            Rosegarden::MappedEvent *me =
                new Rosegarden::MappedEvent(metronomePitch,
                                            comp.getElapsedRealTime(i),
                                            Rosegarden::RealTime(0, 100000),
                                            metronomeBeatVely,
                                            metronomeInstrument,
                                            metronomeTrack);


            mappComp.insert(me);
        }
    }
}

// Sets the play or record Metronome status according to
// the current transport status
//
void
RosegardenGUIApp::toggleMetronome()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    if (m_transportStatus == STARTING_TO_RECORD_MIDI ||
        m_transportStatus == STARTING_TO_RECORD_AUDIO ||
        m_transportStatus == RECORDING_MIDI ||
        m_transportStatus == RECORDING_AUDIO)
    {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        m_transport->MetronomeButton->setOn(comp.useRecordMetronome());
    }
    else
    {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        m_transport->MetronomeButton->setOn(comp.usePlayMetronome());
    }
}

