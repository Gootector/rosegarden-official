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


#include <QShortcut>
#include "RosegardenGUIApp.h"
#include <QApplication>

#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/TrackButtons.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/application/TransportStatus.h"
#include "base/AnalysisTypes.h"
#include "base/AudioPluginInstance.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/PasteSegmentsCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/edit/AddMarkerCommand.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "commands/edit/RemoveMarkerCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/AudioSegmentAutoSplitCommand.h"
#include "commands/segment/AudioSegmentRescaleCommand.h"
#include "commands/segment/AudioSegmentSplitCommand.h"
#include "commands/segment/ChangeCompositionLengthCommand.h"
#include "commands/segment/CreateTempoMapFromSegmentCommand.h"
#include "commands/segment/CutRangeCommand.h"
#include "commands/segment/DeleteRangeCommand.h"
#include "commands/segment/InsertRangeCommand.h"
#include "commands/segment/ModifyDefaultTempoCommand.h"
#include "commands/segment/MoveTracksCommand.h"
#include "commands/segment/PasteRangeCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/SegmentAutoSplitCommand.h"
#include "commands/segment/SegmentChangeTransposeCommand.h"
#include "commands/segment/SegmentJoinCommand.h"
#include "commands/segment/SegmentLabelCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "commands/segment/SegmentRescaleCommand.h"
#include "commands/segment/SegmentSplitByPitchCommand.h"
#include "commands/segment/SegmentSplitByRecordingSrcCommand.h"
#include "commands/segment/SegmentSplitCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/studio/CreateOrDeleteDeviceCommand.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/io/CsoundExporter.h"
#include "document/io/HydrogenLoader.h"
#include "document/io/LilyPondExporter.h"
#include "document/CommandHistory.h"
#include "document/io/RG21Loader.h"
#include "document/io/MupExporter.h"
#include "document/io/MusicXmlExporter.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/dialogs/AddTracksDialog.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/AudioPluginDialog.h"
#include "gui/dialogs/AudioSplitDialog.h"
#include "gui/dialogs/BeatsBarsDialog.h"
#include "gui/dialogs/CompositionLengthDialog.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/CountdownDialog.h"
#include "gui/dialogs/DocumentConfigureDialog.h"
#include "gui/dialogs/FileMergeDialog.h"
#include "gui/dialogs/IdentifyTextCodecDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/ManageMetronomeDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/SplitByPitchDialog.h"
#include "gui/dialogs/SplitByRecordingSrcDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/TransportDialog.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/editors/parameters/SegmentParameterBox.h"
#include "gui/editors/parameters/TrackParameterBox.h"
#include "gui/editors/segment/segmentcanvas/CompositionView.h"
#include "gui/editors/segment/ControlEditorDialog.h"
#include "gui/editors/segment/MarkerEditor.h"
#include "gui/editors/segment/PlayListDialog.h"
#include "gui/editors/segment/PlayList.h"
#include "gui/editors/segment/segmentcanvas/SegmentEraser.h"
#include "gui/editors/segment/segmentcanvas/SegmentJoiner.h"
#include "gui/editors/segment/segmentcanvas/SegmentMover.h"
#include "gui/editors/segment/segmentcanvas/SegmentPencil.h"
#include "gui/editors/segment/segmentcanvas/SegmentResizer.h"
#include "gui/editors/segment/segmentcanvas/SegmentSelector.h"
#include "gui/editors/segment/segmentcanvas/SegmentSplitter.h"
#include "gui/editors/segment/segmentcanvas/SegmentToolBox.h"
#include "gui/editors/segment/TrackLabel.h"
#include "gui/editors/segment/TriggerSegmentManager.h"
#include "gui/editors/tempo/TempoView.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/IconLoader.h"
#include "gui/general/FileSource.h"
#include "gui/kdeext/KStartupLogo.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/seqmanager/MidiFilterDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/seqmanager/SequencerMapper.h"
#include "gui/studio/AudioMixerWindow.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPluginOSCGUIManager.h"
#include "gui/studio/BankEditorDialog.h"
#include "gui/studio/DeviceManagerDialog.h"
#include "gui/studio/MidiMixerWindow.h"
#include "gui/studio/RemapInstrumentDialog.h"
#include "gui/studio/StudioControl.h"
#include "gui/studio/SynthPluginManagerDialog.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/ProgressBar.h"
#include "gui/widgets/ProgressDialog.h"
#include "LircClient.h"
#include "LircCommander.h"
#include "RosegardenGUIView.h"
#include "SetWaitCursor.h"
#include "sequencer/RosegardenSequencer.h"
#include "sequencer/SequencerThread.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/MappedCommon.h"
#include "sound/MappedComposition.h"
#include "sound/MappedEvent.h"
#include "sound/MappedStudio.h"
#include "sound/MidiFile.h"
#include "sound/PluginIdentifier.h"
#include "sound/SoundDriver.h"
#include "StartupTester.h"

#include <Q3Canvas>

#include <QSettings>
#include <QDockWidget>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QToolTip>
#include <QByteArray>
#include <QCursor>
#include <QDataStream>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QObject>
#include <QObjectList>
#include <QPixmap>
#include <QToolTip>
#include <QPushButton>
#include <QRegExp>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QUrl>

#include <QDialog>
#include <QFileDialog>
#include <QInputDialog>
#include <QPrintDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QPageSetupDialog>

#include <gui/kdeext/KTmpStatusMsg.h>

//#include <kmimetype.h>
//#include <kedittoolbar.h> //
//#include <QFileDialog>
//#include <kglobal.h>
//#include <kinputdialog.h>
//#include <kkeydialog.h>	//&&& disabled kkaydialog.h
//#include <kmainwindow.h>
//#include <ktip.h>
//#include <ktoolbar.h>
//#include <kurl.h>
//#include <kstandardshortcut.h>
//#include <kstandardaction.h>
//#include <kstandarddirs.h>

// remaining kde headers:
#include <kxmlguiclient.h>
#include <klocale.h>


#ifdef HAVE_LIBJACK
#include <jack/jack.h>
#endif


namespace Rosegarden
{

RosegardenGUIApp::RosegardenGUIApp(bool useSequencer,
                                   QObject *startupStatusMessageReceiver) :
    QMainWindow(0),
    m_actionsSetup(false),
    m_view(0),
    m_swapView(0),
    m_mainDockWidget(0),
    m_dockLeft(0),
    m_doc(0),
    m_sequencerThread(0),
    m_sequencerCheckedIn(false),
#ifdef HAVE_LIBJACK
    m_jackProcess(0),
#endif
    m_zoomSlider(0),
    m_seqManager(0),
    m_transport(0),
    m_audioManagerDialog(0),
    m_originatingJump(false),
    m_storedLoopStart(0),
    m_storedLoopEnd(0),
    m_useSequencer(useSequencer),
    m_dockVisible(true),
    m_autoSaveTimer(new QTimer( static_cast<QObject *>(this) )),
    m_clipboard(new Clipboard),
    m_playList(0),
    m_deviceManager(0),
    m_synthManager(0),
    m_audioMixer(0),
    m_midiMixer(0),
    m_bankEditor(0),
    m_markerEditor(0),
    m_tempoView(0),
    m_triggerSegmentManager(0),
#ifdef HAVE_LIBLO
    m_pluginGUIManager(new AudioPluginOSCGUIManager(this)),
#endif
    m_playTimer(new QTimer( static_cast<QObject *>(this) )),
    m_stopTimer(new QTimer( static_cast<QObject *>(this) )),
    m_startupTester(0),
#ifdef HAVE_LIRC
    m_lircClient(0),
    m_lircCommander(0),
#endif
    m_firstRun(false),
    m_haveAudioImporter(false),
    m_parameterArea(0)
{
    m_myself = this;

    if (startupStatusMessageReceiver) {
        QObject::connect(this, SIGNAL(startupStatusMessage(QString)),
                         startupStatusMessageReceiver,
                         SLOT(slotShowStatusMessage(QString)));
    }

    // Try to start the sequencer
    //
    if (m_useSequencer) {

#ifdef HAVE_LIBJACK
#define OFFER_JACK_START_OPTION 1
#ifdef OFFER_JACK_START_OPTION
        // First we check if jackd is running allready

        std::string jackClientName = "rosegarden";

        // attempt connection to JACK server
        //
        jack_client_t* testJackClient;
        testJackClient = jack_client_new(jackClientName.c_str());
        if (testJackClient == 0 ) {

            // If no connection to JACK
            // try to launch JACK - if the configuration wants us to.
            if (!launchJack()) {
                KStartupLogo::hideIfStillThere();
                QMessageBox::critical( dynamic_cast<QWidget*>(this), "", i18n("Attempted to launch JACK audio daemon failed.  Audio will be disabled.\nPlease check configuration (Settings -> Configure Rosegarden -> Audio -> Startup)\n and restart."));
            }
        } else {
            //this client was just for testing
            jack_client_close(testJackClient);
        }
#endif // OFFER_JACK_START_OPTION
#endif // HAVE_LIBJACK

        emit startupStatusMessage(i18n("Starting sequencer..."));
        launchSequencer();

    } else
        RG_DEBUG << "RosegardenGUIApp : don't use sequencer\n";

    // Plugin manager
    //
    emit startupStatusMessage(i18n("Initializing plugin manager..."));
    m_pluginManager = new AudioPluginManager();

    // call inits to invoke all other construction parts
    //
    emit startupStatusMessage(i18n("Initializing view..."));
    initStatusBar();
    setupActions();
    initZoomToolbar();

    QPixmap dummyPixmap; // any icon will do
	
	
	
	// start of docking code 
	this->setDockOptions( // QMainWindow::AllowNestedDocks // not required with ::ForceTabbedDocks
							QMainWindow::AnimatedDocks
							|QMainWindow::ForceTabbedDocks
							//|QMainWindow::VerticalTabs
						);
	
	
	/*
	//&&& m_mainDockWidget may not be required: QMainWindow serves the mainDock
	// 
	// 
	// create mainDockWidget
	// old: m_mainDockWidget = createDockWidget("Rosegarden MainDockWidget", dummyPixmap, 0L, "main_dock_widget");
	// 
	m_mainDockWidget = new QDockWidget("Rosegarden MainDockWidget", dynamic_cast<QWidget*>(this), Qt::Tool );
	//### FIX: deallocate m_mainDockWidget !
	m_mainDockWidget->setMinimumSize( 60, 60 );	//### fix arbitrary value for min-size
	
	// allow others to dock to the left and right sides only
    // old: m_mainDockWidget->setDockSite(QDockWidget::DockLeft | QDockWidget::DockRight);
	m_mainDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	
    // forbit docking abilities of m_mainDockWidget itself
    // old: m_mainDockWidget->setEnableDocking(QDockWidget::DockNone);  //&&& setEnableDocking() not required ?
	
	m_mainDockWidget->setFeatures( QDockWidget::DockWidgetMovable
			| QDockWidget::DockWidgetMovable
			| QDockWidget::DockWidgetFloatable
			| QDockWidget::DockWidgetVerticalTitleBar
			//| QDockWidget::DockWidgetClosable
			);
	
	//setView(m_mainDockWidget); // central widget in a KDE mainwindow
    //setMainDockWidget(m_mainDockWidget); // master dockwidget
	
	// add to QMainWindow, left side
	this->addDockWidget(Qt::LeftDockWidgetArea, m_mainDockWidget);
	
	// end disabled m_mainDockWidget
	*/
	
	
	//old: m_dockLeft = createDockWidget("params dock", dummyPixmap, 0L,
	//							  i18n("Special Parameters"));
	m_dockLeft = new QDockWidget(i18n("Special Parameters"),  dynamic_cast<QWidget*>(this), Qt::Tool );
	m_dockLeft->setMinimumSize( 180, 200 );	//### fix arbitrary value for min-size
	//### FIX: deallocate m_DockLeft !
	
	m_dockLeft->setFeatures( QDockWidget::DockWidgetMovable
			| QDockWidget::DockWidgetMovable
			| QDockWidget::DockWidgetFloatable
			| QDockWidget::DockWidgetVerticalTitleBar
			//| QDockWidget::DockWidgetClosable
								 );
	
	// add dockLeft to mainDockWidget, left side 
	//old: m_dockLeft->manualDock(m_mainDockWidget,             // dock target
	 //                     QDockWidget::DockLeft,  // dock site
	   //                  20);                   // relation target/this (in percent)
	//m_mainDockWidget->addDockWidget( Qt::LeftDockWidgetArea, m_dockLeft );
	this->addDockWidget( Qt::LeftDockWidgetArea, m_dockLeft );
	
	// add to m_mainDockWidget, left side
	//this->addDockWidget(Qt::LeftDockWidgetArea, m_mainDockWidget);
	
	
	
	
	
    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
	
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
	//
	//&&& disabled mainDockWidget connection, may not be required
//    connect(m_mainDockWidget, SIGNAL(docking(QDockWidget*, QDockWidget::DockPosition)),
//            this, SLOT(slotParametersDockedBack(QDockWidget*, QDockWidget::DockPosition)));
	
	
    leaveActionState("parametersbox_closed"); //@@@JAS orig. KXMLGUIClient::StateReverse

    RosegardenGUIDoc* doc = new RosegardenGUIDoc(this, m_pluginManager);

    m_parameterArea = new RosegardenParameterArea(m_dockLeft);
    m_dockLeft->setWidget(m_parameterArea);

    // Populate the parameter-box area with the respective
    // parameter box widgets.

    m_segmentParameterBox = new SegmentParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_segmentParameterBox);
    m_trackParameterBox = new TrackParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_trackParameterBox);
    m_instrumentParameterBox = new InstrumentParameterBox(doc, m_parameterArea);
    m_parameterArea->addRosegardenParameterBox(m_instrumentParameterBox);

    // Lookup the configuration parameter that specifies the default
    // arrangement, and instantiate it.
	
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    m_parameterArea->setArrangement((RosegardenParameterArea::Arrangement)
                                    settings.value("sidebarstyle",
                                    RosegardenParameterArea::CLASSIC_STYLE).toUInt());

    m_dockLeft->update();

    connect(m_instrumentParameterBox,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            this,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(m_instrumentParameterBox,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    // relay this through our own signal so that others can use it too
    connect(m_instrumentParameterBox,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_instrumentParameterBox,
            SLOT(slotInstrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_instrumentParameterBox,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(this,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_instrumentParameterBox,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

    // Load the initial document (this includes doc's own autoload)
    //
    setDocument(doc);

    emit startupStatusMessage(i18n("Starting sequence manager..."));

    // transport is created by setupActions()
    m_seqManager = new SequenceManager(m_doc, getTransport());

    if (m_useSequencer) {
        // Check the sound driver status and warn the user of any
        // problems.  This warning has to happen early, in case it
        // affects the ability to load plugins etc from a file on the
        // command line.
        m_seqManager->checkSoundDriverStatus(true);
    }

    if (m_view) {
        connect(m_seqManager, SIGNAL(controllerDeviceEventReceived(MappedEvent *)),
                m_view, SLOT(slotControllerDeviceEventReceived(MappedEvent *)));
                
                
		
		MIDIInstrumentParameterPanel *mipp;
		mipp = m_instrumentParameterBox->getMIDIInstrumentParameterPanel();
		if(! mipp){
			RG_DEBUG << "Error: m_instrumentParameterBox->getMIDIInstrumentParameterPanel() is NULL in RosegardenGUIApp.cpp 445 \n";
		}
		connect(m_seqManager, SIGNAL(signalSelectProgramNoSend(int,int,int)), (QObject*)mipp, SLOT(slotSelectProgramNoSend(int,int,int)) );
                
    }

    if (m_seqManager->getSoundDriverStatus() & AUDIO_OK) {
        slotStateChanged("got_audio", true);
    } else {
        slotStateChanged("got_audio", false);
    }

    // If we're restarting the gui then make sure any transient
    // studio objects are cleared away.
    emit startupStatusMessage(i18n("Clearing studio data..."));
    m_seqManager->reinitialiseSequencerStudio();

    // Send the transport control statuses for MMC and JACK
    //
    m_seqManager->sendTransportControlStatuses();

    // Now autoload
    //
	enterActionState("new_file"); //@@@ JAS orig. 0
	leaveActionState("have_segments"); //@@@ JAS orig. KXMLGUIClient::StateReverse
	leaveActionState("have_selection"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    slotTestClipboard();

    // Check for lack of MIDI devices and disable Studio options accordingly
    //
    if (!m_doc->getStudio().haveMidiDevices())
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse

    emit startupStatusMessage(i18n("Starting..."));

    setupFileDialogSpeedbar();
    readOptions();

    // All toolbars should be created before this is called
    //### implement or find alternative : rgTempQtIV->setAutoSaveSettings(MainWindowConfigGroup, true);

#ifdef HAVE_LIRC

    try {
        m_lircClient = new LircClient();
    } catch (Exception e) {
        RG_DEBUG << e.getMessage().c_str() << endl;
        // continue without
        m_lircClient = 0;
    }
    if (m_lircClient) {
        m_lircCommander = new LircCommander(m_lircClient, this);
    }
#endif

    leaveActionState("have_project_packager"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    leaveActionState("have_lilypondview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    QTimer::singleShot(1000, this, SLOT(slotTestStartupTester()));

    settings.endGroup();
}

RosegardenGUIApp::~RosegardenGUIApp()
{
    RG_DEBUG << "~RosegardenGUIApp()\n";

    if (getView() &&
            getView()->getTrackEditor() &&
            getView()->getTrackEditor()->getSegmentCanvas()) {
        getView()->getTrackEditor()->getSegmentCanvas()->endAudioPreviewGeneration();
    }

#ifdef HAVE_LIBLO
    delete m_pluginGUIManager;
#endif

    if (isSequencerRunning()) {
        RosegardenSequencer::getInstance()->quit();
        usleep(300000);
        delete m_sequencerThread;
    }

    delete m_jumpToQuickMarkerAction;
    delete m_setQuickMarkerAction;
	
	// delete QMenus
	delete m_menuFile;
	delete m_menuEdit;
	delete m_menuComposition;
	delete m_menuStudio;
	delete m_menuSettings;
	
	// delete QMenuBar
	delete m_menuBarMain;
	
	// delete QAction pointers
	delete m_playTransport;	//@@@ emru: should these be free'd, if GUIApp quits? Assumed so!
	delete m_stopTransport;
	delete m_rewindTransport;
	delete m_ffwdTransport; 
	delete m_recordTransport;
	delete m_rewindEndTransport;
	delete m_ffwdEndTransport;
	delete m_panic;
	
    delete m_transport;

    delete m_seqManager;

#ifdef HAVE_LIRC

    delete m_lircCommander;
    delete m_lircClient;
#endif

    delete m_doc;
    Profiles::getInstance()->dump();
}

void RosegardenGUIApp::setupActions()
{
	
	/*
	// old:
	KStandardAction::open (this, SLOT(slotFileOpen()), actionCollection());
    m_fileRecent = KStandardAction::openRecent(this,
                                          SLOT(slotFileOpenRecent(const KURL&)),
                                          actionCollection());
    KStandardAction::save (this, SLOT(slotFileSave()), actionCollection());
    KStandardAction::saveAs(this, SLOT(slotFileSaveAs()), actionCollection());
    KStandardAction::revert(this, SLOT(slotRevertToSaved()), actionCollection());
    KStandardAction::close (this, SLOT(slotFileClose()), actionCollection());
    KStandardAction::print (this, SLOT(slotFilePrint()), actionCollection());
    KStandardAction::printPreview (this, SLOT(slotFilePrintPreview()), actionCollection());
	KStandardAction::quit (this, SLOT(slotQuit()), actionCollection());
	*/
	/*
	// old:
    KStandardAction::cut (this, SLOT(slotEditCut()), actionCollection());
    KStandardAction::copy (this, SLOT(slotEditCopy()), actionCollection());
    KStandardAction::paste (this, SLOT(slotEditPaste()), actionCollection());
	*/
	/*
	// old:
    KStandardAction::saveOptions(this,
                            SLOT(slotSaveOptions()),
                            actionCollection());

    KStandardAction::preferences(this,
                            SLOT(slotConfigure()),
                            actionCollection());

    KStandardAction::keyBindings(this,
                            SLOT(slotEditKeys()),
                            actionCollection());

    KStandardAction::configureToolbars(this,
                                  SLOT(slotEditToolbars()),
                                  actionCollection());
	*/

    createAction("file_open", SLOT(slotFileOpen()));
    createAction("file_save", SLOT(slotFileSave()));
    createAction("file_save_as", SLOT(slotFileSaveAs()));
    createAction("file_revert", SLOT(slotRevertToSaved()));
    createAction("file_close", SLOT(slotFileClose()));
    createAction("file_print", SLOT(slotFilePrint()));
    createAction("file_print_preview", SLOT(slotFilePrintPreview()));
    createAction("file_quit", SLOT(slotQuit()));

    createAction("edit_cut", SLOT(slotEditCut()));
    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));

    //!!! I don't like Save Settings -- I think we should lose it, and
    // just always save them
    createAction("options_save_options", SLOT(slotSaveOptions()));

    createAction("options_configure", SLOT(slotConfigure()));

    //&&&
//    createAction("options_configure_toolbars", SLOT(slotEditToolbars()));
//    createAction("options_configure_keybinding", SLOT(slotEditKeys()));

    // Recent files menu -- see here and below.  The KDE standard action name is/was file_open_recent.
    //&&& Restore this when it is clear how we'll be enquiring for a
    //menu by name
//    m_fileRecent = KStandardAction::openRecent(this,
//                                          SLOT(slotFileOpenRecent(const KURL&)),
//                                          actionCollection());
    m_menuRecent = 0;
    
    createAction("file_import_project", SLOT(slotImportProject()));
    createAction("file_import_midi", SLOT(slotImportMIDI()));
    createAction("file_import_rg21", SLOT(slotImportRG21()));
    createAction("file_import_hydrogen", SLOT(slotImportHydrogen()));
    createAction("file_merge", SLOT(slotMerge()));
    createAction("file_merge_midi", SLOT(slotMergeMIDI()));
    createAction("file_merge_rg21", SLOT(slotMergeRG21()));
    createAction("file_merge_hydrogen", SLOT(slotMergeHydrogen()));
    createAction("file_export_project", SLOT(slotExportProject()));
    createAction("file_export_midi", SLOT(slotExportMIDI()));
    createAction("file_export_lilypond", SLOT(slotExportLilyPond()));
    createAction("file_export_musicxml", SLOT(slotExportMusicXml()));
    createAction("file_export_csound", SLOT(slotExportCsound()));
    createAction("file_export_mup", SLOT(slotExportMup()));
    createAction("file_print_lilypond", SLOT(slotPrintLilyPond()));
    createAction("file_preview_lilypond", SLOT(slotPreviewLilyPond()));
    createAction("file_show_playlist", SLOT(slotPlayList()));
    createAction("tutorial", SLOT(slotTutorial()));
    createAction("guidelines", SLOT(slotBugGuidelines()));

    //&&& Connect CommandHistory to the edit menu and toolbar
    // (looking up the menu & toolbar by name)
//    new KToolBarPopupAction(i18n("Und&o"), "undo", KStdAccel::shortcut(KStdAccel::Undo), actionCollection(), KStdAction::stdName(KStdAction::Undo));
//    new KToolBarPopupAction(i18n("Re&do"), "redo", KStdAccel::shortcut(KStdAccel::Redo), actionCollection(), KStdAction::stdName(KStdAction::Redo));

    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_tracks_toolbar", SLOT(slotToggleTracksToolBar()));
    createAction("show_editors_toolbar", SLOT(slotToggleEditorsToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_zoom_toolbar", SLOT(slotToggleZoomToolBar()));
    createAction("show_transport", SLOT(slotToggleTransport()));
    createAction("show_tracklabels", SLOT(slotToggleTrackLabels()));
    createAction("show_rulers", SLOT(slotToggleRulers()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
    createAction("show_chord_name_ruler", SLOT(slotToggleChordNameRuler()));
    createAction("show_previews", SLOT(slotTogglePreviews()));
    createAction("show_inst_segment_parameters", SLOT(slotDockParametersBack()));
    createAction("select", SLOT(slotPointerSelected()));
    createAction("draw", SLOT(slotDrawSelected()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("move", SLOT(slotMoveSelected()));
    createAction("resize", SLOT(slotResizeSelected()));
    createAction("split", SLOT(slotSplitSelected()));
    createAction("join", SLOT(slotJoinSelected()));
    createAction("harmonize_selection", SLOT(slotHarmonizeSelection()));
    createAction("add_time_signature", SLOT(slotEditTimeSignature()));
    createAction("edit_tempos", SLOT(slotEditTempos()));
    createAction("cut_range", SLOT(slotCutRange()));
    createAction("copy_range", SLOT(slotCopyRange()));
    createAction("paste_range", SLOT(slotPasteRange()));
    createAction("delete_range", SLOT(slotDeleteRange()));
    createAction("insert_range", SLOT(slotInsertRange()));
    createAction("delete", SLOT(slotDeleteSelectedSegments()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("add_tempo", SLOT(slotEditTempo()));
    createAction("change_composition_length", SLOT(slotChangeCompositionLength()));
    createAction("edit_markers", SLOT(slotEditMarkers()));
    createAction("edit_doc_properties", SLOT(slotEditDocumentProperties()));
    createAction("edit_default", SLOT(slotEdit()));
    createAction("edit_matrix", SLOT(slotEditInMatrix()));
    createAction("edit_percussion_matrix", SLOT(slotEditInPercussionMatrix()));
    createAction("edit_notation", SLOT(slotEditAsNotation()));
    createAction("edit_event_list", SLOT(slotEditInEventList()));
    createAction("quantize_selection", SLOT(slotQuantizeSelection()));
    createAction("relabel_segment", SLOT(slotRelabelSegments()));
    createAction("transpose", SLOT(slotTransposeSegments()));
    createAction("repeat_quantize", SLOT(slotRepeatQuantizeSelection()));
    createAction("rescale", SLOT(slotRescaleSelection()));
    createAction("auto_split", SLOT(slotAutoSplitSelection()));
    createAction("split_by_pitch", SLOT(slotSplitSelectionByPitch()));
    createAction("split_by_recording", SLOT(slotSplitSelectionByRecordedSrc()));
    createAction("split_at_time", SLOT(slotSplitSelectionAtTime()));
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));
    createAction("set_segment_start", SLOT(slotSetSegmentStartTimes()));
    createAction("set_segment_duration", SLOT(slotSetSegmentDurations()));
    createAction("join_segments", SLOT(slotJoinSegments()));
    createAction("repeats_to_real_copies", SLOT(slotRepeatingSegments()));
    createAction("manage_trigger_segments", SLOT(slotManageTriggerSegments()));
    createAction("groove_quantize", SLOT(slotGrooveQuantize()));
    createAction("set_tempo_to_segment_length", SLOT(slotTempoToSegmentLength()));
    createAction("audio_manager", SLOT(slotAudioManager()));
    createAction("show_segment_labels", SLOT(slotToggleSegmentLabels()));
    createAction("add_track", SLOT(slotAddTrack()));
    createAction("add_tracks", SLOT(slotAddTracks()));
    createAction("delete_track", SLOT(slotDeleteTrack()));
    createAction("move_track_down", SLOT(slotMoveTrackDown()));
    createAction("move_track_up", SLOT(slotMoveTrackUp()));
    createAction("select_next_track", SLOT(slotTrackDown()));
    createAction("select_previous_track", SLOT(slotTrackUp()));
    createAction("toggle_mute_track", SLOT(slotToggleMutedCurrentTrack()));
    createAction("toggle_arm_track", SLOT(slotToggleRecordCurrentTrack()));
    createAction("mute_all_tracks", SLOT(slotMuteAllTracks()));
    createAction("unmute_all_tracks", SLOT(slotUnmuteAllTracks()));
    createAction("remap_instruments", SLOT(slotRemapInstruments()));
    createAction("audio_mixer", SLOT(slotOpenAudioMixer()));
    createAction("midi_mixer", SLOT(slotOpenMidiMixer()));
    createAction("manage_devices", SLOT(slotManageMIDIDevices()));
    createAction("manage_synths", SLOT(slotManageSynths()));
    createAction("modify_midi_filters", SLOT(slotModifyMIDIFilters()));
    createAction("enable_midi_routing", SLOT(slotEnableMIDIThruRouting()));
    createAction("manage_metronome", SLOT(slotManageMetronome()));
    createAction("save_default_studio", SLOT(slotSaveDefaultStudio()));
    createAction("load_default_studio", SLOT(slotImportDefaultStudio()));
    createAction("load_studio", SLOT(slotImportStudio()));
    createAction("reset_midi_network", SLOT(slotResetMidiNetwork()));
    createAction("set_quick_marker", SLOT(slotSetQuickMarker()));
    createAction("jump_to_quick_marker", SLOT(slotJumpToQuickMarker()));

// These were commented out in the old KDE3 code as well
//    createAction("insert_marker_here", SLOT(slotInsertMarkerHere()));
//    createAction("insert_marker_at_pointer", SLOT(slotInsertMarkerAtPointer()));
//    createAction("delete_marker", SLOT(slotDeleteMarker()));

    createAction("play", SLOT(slotPlay()));
    createAction("stop", SLOT(slotStop()));
    createAction("fast_forward", SLOT(slotFastforward()));
    createAction("rewind", SLOT(slotRewind()));
    createAction("recordtoggle", SLOT(slotToggleRecord()));
    createAction("record", SLOT(slotRecord()));
    createAction("rewindtobeginning", SLOT(slotRewindToBeginning()));
    createAction("fastforwardtoend", SLOT(slotFastForwardToEnd()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SLOT(slotPanic()));
    createAction("debug_dump_segments", SLOT(slotDebugDump()));

    createGUI("rosegardenui.rc");

    createAndSetupTransport();

    // transport toolbar is hidden by default - TODO : this should be in options
    //
    //toolBar("Transport Toolbar")->hide();

    // was QPopupMenu
    QMenu* setTrackInstrumentMenu = this->findChild<QMenu*>("set_track_instrument");

    if (setTrackInstrumentMenu) {
        connect(setTrackInstrumentMenu, SIGNAL(aboutToShow()),
                this, SLOT(slotPopulateTrackInstrumentPopup()));
    } else {
        RG_DEBUG << "RosegardenGUIApp::setupActions() : couldn't find set_track_instrument menu - check rosegardenui.rcn\n";
    }

    setRewFFwdToAutoRepeat();

/*
    // setup File menu
    // New Window ?

    //&&& Most of the menus are currently created nowhere -- pull
    //across KXMLGUI or equivalent, or hard-code them -- a decision to
    //be made!
    
    QAction* qa_open = new QAction(i18n("&Open"), this );
    connect( qa_open, SIGNAL(toggled()), this, SLOT(slotFileOpen()) ); ;
    //qa_open->setCheckable( true );	//
    qa_open->setAutoRepeat( false );	//
    //qa_open->setObjectName("");
    //qa_open->setActionGroup( 0 );	// QActionGroup*

    m_menuRecent = m_menuFile->addMenu(i18n("Open &Recent"));
    setupRecentFilesMenu();
    connect(&m_recentFiles, SIGNAL(recentChanged()),
            this, SLOT(setupRecentFilesMenu()));

	
	QAction* qa_save = new QAction(i18n("&Save"), this );
	connect( qa_save, SIGNAL(toggled()), this, SLOT(slotFileSave()) ); ;
	//qa_save->setCheckable( true );	//
	qa_save->setAutoRepeat( false );	//
	//qa_save->setObjectName("");
	//qa_save->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_saveAs = new QAction(i18n("Save &as... "), this );
	connect( qa_saveAs, SIGNAL(toggled()), this, SLOT(slotFileSaveAs()) ); ;
	//qa_saveAs->setCheckable( true );	//
	qa_saveAs->setAutoRepeat( false );	//
	//qa_saveAs->setObjectName("");
	//qa_saveAs->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_revert = new QAction(i18n("&Revert to last saved Version"), this );
	connect( qa_revert, SIGNAL(toggled()), this, SLOT(slotRevertToSaved()) ); ;
	//qa_revert->setCheckable( true );	//
	qa_revert->setAutoRepeat( false );	//
	//qa_revert->setObjectName("");
	//qa_revert->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_close = new QAction(i18n("&Close File"), this );
	connect( qa_close, SIGNAL(toggled()), this, SLOT(slotFileClose()) ); ;
	//qa_close->setCheckable( true );	//
	qa_close->setAutoRepeat( false );	//
	//qa_close->setObjectName("");
	//qa_close->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_print = new QAction(i18n("&Print"), this );
	connect( qa_print, SIGNAL(toggled()), this, SLOT(slotFilePrint()) ); ;
	//qa_print->setCheckable( true );	//
	qa_print->setAutoRepeat( false );	//
	//qa_print->setObjectName("");
	//qa_print->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_printPreview = new QAction(i18n("Show Print Pre&view"), this );
	connect( qa_printPreview, SIGNAL(toggled()), this, SLOT(slotFilePrintPreview()) ); ;
	//qa_printPreview->setCheckable( true );	//
	qa_printPreview->setAutoRepeat( false );	//
	//qa_printPreview->setObjectName("");
	//qa_printPreview->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_quit = new QAction(i18n("&Quit"), this );
	connect( qa_quit, SIGNAL(toggled()), this, SLOT(slotQuit()) ); ;
	//qa_quit->setCheckable( true );	//
	qa_quit->setAutoRepeat( false );	//
	//qa_quit->setObjectName("");
	//qa_quit->setActionGroup( 0 );	// QActionGroup*
	
	m_menuBarMain = this->menuBar(); // create and return main menu bar
	
	// create sub-menus 
	m_menuFile = m_menuBarMain->addMenu(i18n("&File"));
	m_menuEdit = m_menuBarMain->addMenu(i18n("&Edit"));
	m_menuComposition = m_menuBarMain->addMenu(i18n("&Composition"));
	m_menuStudio = m_menuBarMain->addMenu(i18n("&Studio"));
	m_menuSettings = m_menuBarMain->addMenu(i18n("Se&ttings"));
	
	// add actions
	m_menuFile->addAction(qa_open);
	m_menuFile->addAction(qa_save);
	m_menuFile->addAction(qa_saveAs);
	m_menuFile->addAction(qa_revert);
	m_menuFile->addAction(qa_close);
	m_menuFile->addAction(qa_print);
	m_menuFile->addAction(qa_printPreview);
	m_menuFile->addAction(qa_quit);
	
	
    QAction *qa_file_import_project = new QAction(i18n("Import Rosegarden &Project file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_import_project->setIconText(0); 
			connect( qa_file_import_project, SIGNAL(triggered()), this, SLOT(slotImportProject())  );

    QAction *qa_file_import_midi = new QAction(i18n("Import &MIDI file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_import_midi->setIconText(0); 
			connect( qa_file_import_midi, SIGNAL(triggered()), this, SLOT(slotImportMIDI())  );

    QAction *qa_file_import_rg21 = new QAction(i18n("Import &Rosegarden 2.1 file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_import_rg21->setIconText(0); 
			connect( qa_file_import_rg21, SIGNAL(triggered()), this, SLOT(slotImportRG21())  );

    QAction *qa_file_import_hydrogen = new QAction(i18n("Import &Hydrogen file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_import_hydrogen->setIconText(0); 
			connect( qa_file_import_hydrogen, SIGNAL(triggered()), this, SLOT(slotImportHydrogen())  );

    QAction *qa_file_merge = new QAction(i18n("Merge &File..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_merge->setIconText(0); 
			connect( qa_file_merge, SIGNAL(triggered()), this, SLOT(slotMerge())  );

    QAction *qa_file_merge_midi = new QAction(i18n("Merge &MIDI file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_merge_midi->setIconText(0); 
			connect( qa_file_merge_midi, SIGNAL(triggered()), this, SLOT(slotMergeMIDI())  );

    QAction *qa_file_merge_rg21 = new QAction(i18n("Merge &Rosegarden 2.1 file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_merge_rg21->setIconText(0); 
			connect( qa_file_merge_rg21, SIGNAL(triggered()), this, SLOT(slotMergeRG21())  );

    QAction *qa_file_merge_hydrogen = new QAction(i18n("Merge &Hydrogen file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_merge_hydrogen->setIconText(0); 
			connect( qa_file_merge_hydrogen, SIGNAL(triggered()), this, SLOT(slotMergeHydrogen())  );

    QAction *qa_file_export_project = new QAction(i18n("Export Rosegarden &Project file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_project->setIconText(0); 
			connect( qa_file_export_project, SIGNAL(triggered()), this, SLOT(slotExportProject())  );

    QAction *qa_file_export_midi = new QAction(i18n("Export &MIDI file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_midi->setIconText(0); 
			connect( qa_file_export_midi, SIGNAL(triggered()), this, SLOT(slotExportMIDI())  );

    QAction *qa_file_export_lilypond = new QAction(i18n("Export &LilyPond file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_lilypond->setIconText(0); 
			connect( qa_file_export_lilypond, SIGNAL(triggered()), this, SLOT(slotExportLilyPond())  );

    QAction *qa_file_export_musicxml = new QAction(i18n("Export Music&XML file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_musicxml->setIconText(0); 
			connect( qa_file_export_musicxml, SIGNAL(triggered()), this, SLOT(slotExportMusicXml())  );

    QAction *qa_file_export_csound = new QAction(i18n("Export &Csound score file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_csound->setIconText(0); 
			connect( qa_file_export_csound, SIGNAL(triggered()), this, SLOT(slotExportCsound())  );

    QAction *qa_file_export_mup = new QAction(i18n("Export M&up file..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_export_mup->setIconText(0); 
			connect( qa_file_export_mup, SIGNAL(triggered()), this, SLOT(slotExportMup())  );

    QAction *qa_file_print_lilypond = new QAction(i18n("Print &with LilyPond..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_print_lilypond->setIconText(0); 
			connect( qa_file_print_lilypond, SIGNAL(triggered()), this, SLOT(slotPrintLilyPond())  );

    QAction *qa_file_preview_lilypond = new QAction(i18n("Preview with Lil&yPond..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_preview_lilypond->setIconText(0); 
			connect( qa_file_preview_lilypond, SIGNAL(triggered()), this, SLOT(slotPreviewLilyPond())  );

    QAction *qa_file_show_playlist = new QAction(i18n("Play&list"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_file_show_playlist->setIconText(0); 
			connect( qa_file_show_playlist, SIGNAL(triggered()), this, SLOT(slotPlayList())  );


    // help menu
    QAction *qa_tutorial = new QAction(i18n("Rosegarden &Tutorial"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_tutorial->setIconText(0); 
			connect( qa_tutorial, SIGNAL(triggered()), this, SLOT(slotTutorial())  );

    QAction *qa_guidelines = new QAction(i18n("&Bug Reporting Guidelines"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_guidelines->setIconText(0); 
			connect( qa_guidelines, SIGNAL(triggered()), this, SLOT(slotBugGuidelines())  );

    
			
	// setup edit menu
	
	QAction* qa_cut = new QAction(i18n("Cu&t"), this );
	connect( qa_cut, SIGNAL(toggled()), this, SLOT(slotEditCut()) ); ;
	//qa_cut->setCheckable( true );	//
	qa_cut->setAutoRepeat( false );	//
	//qa_cut->setObjectName("");
	//qa_cut->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_copy = new QAction(i18n("&Copy"), this );
	connect( qa_copy, SIGNAL(toggled()), this, SLOT(slotEditCopy()) ); ;
	//qa_copy->setCheckable( true );	//
	qa_copy->setAutoRepeat( false );	//
	//qa_copy->setObjectName("");
	//qa_copy->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_paste = new QAction(i18n("&Paste"), this );
	connect( qa_paste, SIGNAL(toggled()), this, SLOT(slotEditPaste()) ); ;
	//qa_paste->setCheckable( true );	//
	qa_paste->setAutoRepeat( false );	//
	//qa_paste->setObjectName("");
	//qa_paste->setActionGroup( 0 );	// QActionGroup*
	
	m_menuEdit->addAction(qa_cut);
	m_menuEdit->addAction(qa_copy);
	m_menuEdit->addAction(qa_paste);
    //
    // undo/redo actions are special in that they are connected to
    // slots later on, when the current document is set up - see
    // CommandHistory::attachView
    //
	QString tmp_name;
	//
    QAction* qa_undo = new QAction( i18n("Und&o"), dynamic_cast<QObject*>(this) );
	//### FIX: deallocate QAction ptr
	qa_undo->setShortcut( QKeySequence::Undo );
	qa_undo->setObjectName("undo");
	//tmp_name = QKeySequence( QKeySequence::Undo ).toString( QKeySequence::NativeText, )

	QAction* qa_redo = new QAction( i18n("Re&do"), dynamic_cast<QObject*>(this) );
	//### FIX: deallocate QAction ptr
	qa_redo->setShortcut( QKeySequence::Redo );
	qa_redo->setObjectName("redo");
	
    /////
	


    // setup Settings menu
    //
    //  old: m_viewToolBar = KStandardAction::showToolbar (this, SLOT(slotToggleToolBar()), actionCollection(),
   //                 "show_stock_toolbar");

	QAction* qa_showToolbar = new QAction(i18n("Show &Toolbar"), this );
	connect( qa_showToolbar, SIGNAL(toggled()), this, SLOT(slotToggleToolBar()) ); ;
	qa_quit->setCheckable( true );	//
	qa_showToolbar->setAutoRepeat( false );	//
	//qa_showToolbar->setObjectName("");
	//qa_showToolbar->setActionGroup( 0 );	// QActionGroup*
	
	m_menuSettings->addAction(qa_showToolbar);
	
	m_viewToolsToolBar = new QAction( i18n("Show T&ools Toolbar"), dynamic_cast<QObject*>(this) );
;

    	connect( m_viewToolsToolBar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleToolsToolBar()) ); ;
    	m_viewToolsToolBar->setObjectName("show_tools_toolbar");
    	m_viewToolsToolBar->setCheckable( true );	//
    	m_viewToolsToolBar->setAutoRepeat( false );	//
    	//m_viewToolsToolBar->setActionGroup( 0 );	// QActionGroup*
	
	
	//QIcon null_icon;
	
	m_viewTracksToolBar = new QAction(  i18n("Show Trac&ks Toolbar"), dynamic_cast<QObject*>(this) );
    	connect( m_viewTracksToolBar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTracksToolBar()) );     
    	m_viewTracksToolBar->setObjectName("show_tracks_toolbar");
    	m_viewTracksToolBar->setCheckable( true );	//
    	m_viewTracksToolBar->setAutoRepeat( false );	//
    	//m_viewTracksToolBar->setActionGroup( 0 );	// QActionGroup*

		m_viewEditorsToolBar = new QAction(  i18n("Show &Editors Toolbar"), dynamic_cast<QObject*>(this) );
    	connect( m_viewEditorsToolBar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleEditorsToolBar()) );     
    	m_viewEditorsToolBar->setObjectName("show_editors_toolbar");
    	m_viewEditorsToolBar->setCheckable( true );	//
    	m_viewEditorsToolBar->setAutoRepeat( false );	//
    	//m_viewEditorsToolBar->setActionGroup( 0 );	// QActionGroup*

		m_viewTransportToolBar = new QAction(  i18n("Show Trans&port Toolbar"), dynamic_cast<QObject*>(this) );
    	connect( m_viewTransportToolBar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTransportToolBar()) );     
    	m_viewTransportToolBar->setObjectName("show_transport_toolbar");
    	m_viewTransportToolBar->setCheckable( true );	//
    	m_viewTransportToolBar->setAutoRepeat( false );	//
    	//m_viewTransportToolBar->setActionGroup( 0 );	// QActionGroup*

		m_viewZoomToolBar = new QAction(  i18n("Show &Zoom Toolbar"), dynamic_cast<QObject*>(this) );
    	connect( m_viewZoomToolBar, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleZoomToolBar()) );     
    	m_viewZoomToolBar->setObjectName("show_zoom_toolbar");
    	m_viewZoomToolBar->setCheckable( true );	//
    	m_viewZoomToolBar->setAutoRepeat( false );	//
    	//m_viewZoomToolBar->setActionGroup( 0 );	// QActionGroup*

// old:    m_viewStatusBar = KStandardAction::showStatusbar(this, SLOT(slotToggleStatusBar()),
//                      actionCollection(), "show_status_bar");

	QAction* qa_showStatusbar = new QAction(i18n("Show the &Statusbar"), this );
	connect( qa_showStatusbar, SIGNAL(toggled()), this, SLOT(slotToggleStatusBar()) ); ;
	qa_showStatusbar->setCheckable( true );	//
	qa_showStatusbar->setAutoRepeat( false );	//
	//qa_showStatusbar->setObjectName("");
	//qa_showStatusbar->setActionGroup( 0 );	// QActionGroup*
	
	m_menuSettings->addAction(qa_showStatusbar);
	
	
	m_viewTransport = new QAction( i18n("Show Tra&nsport"), dynamic_cast<QObject*>(this) );
    	connect( m_viewTransport, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTransport()) );
		m_viewTransport->setShortcut( Qt::Key_T );
    	m_viewTransport->setObjectName("show_transport");
    	m_viewTransport->setCheckable( true );	//
    	m_viewTransport->setAutoRepeat( false );	//
    	//m_viewTransport->setActionGroup( 0 );	// QActionGroup*

    m_viewTrackLabels = new QAction(  i18n("Show Track &Labels"), dynamic_cast<QObject*>(this) );
    	connect( m_viewTrackLabels, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTrackLabels()) );     
    	m_viewTrackLabels->setObjectName("show_tracklabels");
    	m_viewTrackLabels->setCheckable( true );	//
    	m_viewTrackLabels->setAutoRepeat( false );	//
    	//m_viewTrackLabels->setActionGroup( 0 );	// QActionGroup*

    m_viewRulers = new QAction(  i18n("Show Playback Position R&uler"), dynamic_cast<QObject*>(this) );
    	connect( m_viewRulers, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleRulers()) );     
    	m_viewRulers->setObjectName("show_rulers");
    	m_viewRulers->setCheckable( true );	//
    	m_viewRulers->setAutoRepeat( false );	//
    	//m_viewRulers->setActionGroup( 0 );	// QActionGroup*

    m_viewTempoRuler = new QAction(  i18n("Show Te&mpo Ruler"), dynamic_cast<QObject*>(this) );
    	connect( m_viewTempoRuler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTempoRuler()) );     
    	m_viewTempoRuler->setObjectName("show_tempo_ruler");
    	m_viewTempoRuler->setCheckable( true );	//
    	m_viewTempoRuler->setAutoRepeat( false );	//
    	//m_viewTempoRuler->setActionGroup( 0 );	// QActionGroup*

    m_viewChordNameRuler = new QAction(  i18n("Show Cho&rd Name Ruler"), dynamic_cast<QObject*>(this) );
    	connect( m_viewChordNameRuler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleChordNameRuler()) );     
    	m_viewChordNameRuler->setObjectName("show_chord_name_ruler");
    	m_viewChordNameRuler->setCheckable( true );	//
    	m_viewChordNameRuler->setAutoRepeat( false );	//
    	//m_viewChordNameRuler->setActionGroup( 0 );	// QActionGroup*


    m_viewPreviews = new QAction(  i18n("Show Segment Pre&views"), dynamic_cast<QObject*>(this) );
    	connect( m_viewPreviews, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTogglePreviews()) );
    	m_viewPreviews->setObjectName("show_previews");
    	m_viewPreviews->setCheckable( true );	//
    	m_viewPreviews->setAutoRepeat( false );	//
    	//m_viewPreviews->setActionGroup( 0 );	// QActionGroup*

		QAction* qa_show_inst_segment_parameters = new QAction(  i18n("Show Special &Parameters"), dynamic_cast<QObject*>(this) );
		qa_show_inst_segment_parameters->setShortcut( Qt::Key_P );
		connect( qa_show_inst_segment_parameters, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDockParametersBack()) );
		qa_show_inst_segment_parameters->setObjectName("show_inst_segment_parameters");
		qa_show_inst_segment_parameters->setCheckable( true );	//
		qa_show_inst_segment_parameters->setAutoRepeat( false );	//
    	//qa_show_inst_segment_parameters->setActionGroup( 0 );	// QActionGroup*

// old:    KStandardAction::tipOfDay( this, SLOT( slotShowTip() ), actionCollection() );

	QAction* qa_tipOfDay = new QAction(i18n("Show the tip of &day"), this );
	connect( qa_tipOfDay, SIGNAL(toggled()), this, SLOT(slotShowTip()) ); ;
	//qa_tipOfDay->setCheckable( true );	//
	qa_tipOfDay->setAutoRepeat( false );	//
	//qa_tipOfDay->setObjectName("");
	//qa_tipOfDay->setActionGroup( 0 );	// QActionGroup*
	
	m_menuSettings->addAction(qa_tipOfDay);
	
    // Standard Actions
    //
	
	QAction* qa_saveOptions = new QAction(i18n("&Save current Options"), this );
	connect( qa_saveOptions, SIGNAL(toggled()), this, SLOT(slotSaveOptions()) ); ;
	//qa_saveOptions->setCheckable( true );	//
	qa_saveOptions->setAutoRepeat( false );	//
	//qa_saveOptions->setObjectName("");
	//qa_saveOptions->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_configureRg = new QAction(i18n("&Configure Rosegarden"), this );
	connect( qa_configureRg, SIGNAL(toggled()), this, SLOT(slotConfigure()) ); ;
	//qa_configureRg->setCheckable( true );	//
	qa_configureRg->setAutoRepeat( false );	//
	//qa_configureRg->setObjectName("");
	//qa_configureRg->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_editShortcuts = new QAction(i18n("Edit Key-&Shortcuts"), this );
	connect( qa_editShortcuts, SIGNAL(toggled()), this, SLOT(slotEditKeys()) ); ;
	//qa_editShortcuts->setCheckable( true );	//
	qa_editShortcuts->setAutoRepeat( false );	//
	//qa_editShortcuts->setObjectName("");
	//qa_editShortcuts->setActionGroup( 0 );	// QActionGroup*
	
	QAction* qa_editToolbars = new QAction(i18n("Configure &Toolbars"), this );
	connect( qa_editToolbars, SIGNAL(toggled()), this, SLOT(slotEditToolbars()) ); ;
	//qa_editToolbars->setCheckable( true );	//
	qa_editToolbars->setAutoRepeat( false );	//
	//qa_editToolbars->setObjectName("");
	//qa_editToolbars->setActionGroup( 0 );	// QActionGroup*
	
	m_menuSettings->addAction(qa_saveOptions);
	m_menuSettings->addAction(qa_configureRg);
	m_menuSettings->addAction(qa_editShortcuts);
	m_menuSettings->addAction(qa_editToolbars);
	
	
	
    QAction *action = 0;	// was KRadioAction
	
    IconLoader il;

    // Create the select icon
    //
    QIcon icon = il.load("select");
	
	QObject* qa_parent = dynamic_cast<QObject*>(this);
	
	QActionGroup* qag_segmenttools = new QActionGroup( qa_parent );
	
    // TODO : add some shortcuts here
	QAction* qa_select = new QAction( icon, i18n("&Select and Edit"), qa_parent );
			connect( qa_select, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotPointerSelected()) );
			qa_select->setObjectName( "select" );
			qa_select->setCheckable( true );		//
			qa_select->setChecked( false );			//
			qa_select->setAutoRepeat( false );		//
			qa_select->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr


	QAction* qa_draw = new QAction( i18n("&Draw"), qa_parent );
			connect( qa_draw, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDrawSelected()) );
			qa_draw->setObjectName( "draw" );
			// qa_draw->setIcon( "pencil" );	//### FIX icon
			qa_draw->setCheckable( true );		//
			qa_draw->setChecked( false );			//
			qa_draw->setAutoRepeat( false );		//
			qa_draw->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr


	QAction* qa_erase = new QAction( i18n("&Erase"), qa_parent );
			connect( qa_erase, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEraseSelected()) );
			qa_erase->setObjectName( "erase" );
			// qa_erase->setIcon( "eraser" );	//### FIX icon
			qa_erase->setCheckable( true );		//
			qa_erase->setChecked( false );			//
			qa_erase->setAutoRepeat( false );		//
			qa_erase->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr


	QAction* qa_move = new QAction( i18n("&Move"), qa_parent );
			connect( qa_move, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMoveSelected()) );
			qa_move->setObjectName( "move" );
			// qa_move->setIcon( "move" );	//### FIX icon
			qa_move->setCheckable( true );		//
			qa_move->setChecked( false );			//
			qa_move->setAutoRepeat( false );		//
			qa_move->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr


    icon = il.load("resize");
	QAction* qa_resize = new QAction( icon, i18n("&Resize"), qa_parent );
			connect( qa_resize, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotResizeSelected()) );
			qa_resize->setObjectName( "resize" );
			qa_resize->setCheckable( true );		//
			qa_resize->setChecked( false );			//
			qa_resize->setAutoRepeat( false );		//
			qa_resize->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    icon = il.load("split");
	QAction* qa_split = new QAction( icon, i18n("&Split"), qa_parent );
			connect( qa_split, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSplitSelected()) );
			qa_split->setObjectName( "split" );
			qa_split->setCheckable( true );		//
			qa_split->setChecked( false );			//
			qa_split->setAutoRepeat( false );		//
			qa_split->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr


    icon = il.load("join");
	QAction* qa_join = new QAction( icon, i18n("&Join"), qa_parent );
			connect( qa_join, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJoinSelected()) );
			qa_join->setObjectName( "join" );
			qa_join->setCheckable( true );		//
			qa_join->setChecked( false );			//
			qa_join->setAutoRepeat( false );		//
			qa_join->setActionGroup( qag_segmenttools );	// QActionGroup*
			//### FIX: deallocate QAction ptr



    QAction* qa_harmonize_selection = new QAction(  i18n("&Harmonize"), dynamic_cast<QObject*>(this) );
			connect( qa_harmonize_selection, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotHarmonizeSelection()) );
			qa_harmonize_selection->setObjectName( "harmonize_selection" );		//
			//qa_harmonize_selection->setCheckable( true );		//
			qa_harmonize_selection->setAutoRepeat( false );	//
			//qa_harmonize_selection->setActionGroup( 0 );		// QActionGroup*
			//qa_harmonize_selection->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("event-insert-timesig");

    QAction* qa_add_time_signature = new QAction(  AddTimeSignatureCommand::getGlobalName(), dynamic_cast<QObject*>(this) );	// note: this was 0
			connect( qa_add_time_signature, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditTimeSignature()) );
			qa_add_time_signature->setObjectName( "add_time_signature" );		//
			//qa_add_time_signature->setCheckable( true );		//
			qa_add_time_signature->setAutoRepeat( false );	//
			//qa_add_time_signature->setActionGroup( 0 );		// QActionGroup*
			//qa_add_time_signature->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_edit_tempos = new QAction(  i18n("Open Tempo and Time Signature Editor"), dynamic_cast<QObject*>(this) );
			connect( qa_edit_tempos, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditTempos()) );
			qa_edit_tempos->setObjectName( "edit_tempos" );		//
			//qa_edit_tempos->setCheckable( true );		//
			qa_edit_tempos->setAutoRepeat( false );	//
			//qa_edit_tempos->setActionGroup( 0 );		// QActionGroup*
			//qa_edit_tempos->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Edit menu
    //
    QAction* qa_cut_range = new QAction(  i18n("Cut Range"), dynamic_cast<QObject*>(this) );
			connect( qa_cut_range, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotCutRange()) );
			qa_cut_range->setObjectName( "cut_range" );		//
			//qa_cut_range->setCheckable( true );		//
			qa_cut_range->setAutoRepeat( false );	//
			//qa_cut_range->setActionGroup( 0 );		// QActionGroup*
			//qa_cut_range->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_copy_range = new QAction(  i18n("Copy Range"), dynamic_cast<QObject*>(this) );
			connect( qa_copy_range, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotCopyRange()) );
			qa_copy_range->setObjectName( "copy_range" );		//
			//qa_copy_range->setCheckable( true );		//
			qa_copy_range->setAutoRepeat( false );	//
			//qa_copy_range->setActionGroup( 0 );		// QActionGroup*
			//qa_copy_range->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_paste_range = new QAction(  i18n("Paste Range"), dynamic_cast<QObject*>(this) );
			connect( qa_paste_range, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotPasteRange()) );
			qa_paste_range->setObjectName( "paste_range" );		//
			//qa_paste_range->setCheckable( true );		//
			qa_paste_range->setAutoRepeat( false );	//
			//qa_paste_range->setActionGroup( 0 );		// QActionGroup*
			//qa_paste_range->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_insert_range = new QAction(  i18n("Insert Range..."), dynamic_cast<QObject*>(this) );
			connect( qa_insert_range, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotInsertRange()) );
			qa_insert_range->setObjectName( "insert_range" );		//
			//qa_insert_range->setCheckable( true );		//
			qa_insert_range->setAutoRepeat( false );	//
			//qa_insert_range->setActionGroup( 0 );		// QActionGroup*
			//qa_insert_range->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_delete = new QAction(  i18n("De&lete"), dynamic_cast<QObject*>(this) );
			connect( qa_delete, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDeleteSelectedSegments()) );
			qa_delete->setObjectName( "delete" );		//
			//qa_delete->setCheckable( true );		//
			qa_delete->setAutoRepeat( false );	//
			//qa_delete->setActionGroup( 0 );		// QActionGroup*
			//qa_delete->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_select_all = new QAction(  i18n("Select &All Segments"), dynamic_cast<QObject*>(this) );
			connect( qa_select_all, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSelectAll()) );
			qa_select_all->setObjectName( "select_all" );		//
			//qa_select_all->setCheckable( true );		//
			qa_select_all->setAutoRepeat( false );	//
			//qa_select_all->setActionGroup( 0 );		// QActionGroup*
			//qa_select_all->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("event-insert-tempo");

	QAction* qa_add_tempo = new QAction(  AddTempoChangeCommand::getGlobalName(), dynamic_cast<QObject*>(this) );	// note: this was 0
			connect( qa_add_tempo, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditTempo()) );
			qa_add_tempo->setObjectName( "add_tempo" );		//
			//qa_add_tempo->setCheckable( true );		//
			qa_add_tempo->setAutoRepeat( false );	//
			//qa_add_tempo->setActionGroup( 0 );		// QActionGroup*
			//qa_add_tempo->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_change_composition_length = new QAction(  ChangeCompositionLengthCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_change_composition_length, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotChangeCompositionLength()) );
			qa_change_composition_length->setObjectName( "change_composition_length" );		//
			//qa_change_composition_length->setCheckable( true );		//
			qa_change_composition_length->setAutoRepeat( false );	//
			//qa_change_composition_length->setActionGroup( 0 );		// QActionGroup*
			//qa_change_composition_length->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_edit_markers = new QAction(  i18n("Edit Mar&kers..."), dynamic_cast<QObject*>(this) );
			connect( qa_edit_markers, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditMarkers()) );
			qa_edit_markers->setObjectName( "edit_markers" );		//
			//qa_edit_markers->setCheckable( true );		//
			qa_edit_markers->setAutoRepeat( false );	//
			//qa_edit_markers->setActionGroup( 0 );		// QActionGroup*
			//qa_edit_markers->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_edit_doc_properties = new QAction(  i18n("Edit Document P&roperties..."), dynamic_cast<QObject*>(this) );
			connect( qa_edit_doc_properties, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEditDocumentProperties()) );
			qa_edit_doc_properties->setObjectName( "edit_doc_properties" );		//
			//qa_edit_doc_properties->setCheckable( true );		//
			qa_edit_doc_properties->setAutoRepeat( false );	//
			//qa_edit_doc_properties->setActionGroup( 0 );		// QActionGroup*
			//qa_edit_doc_properties->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			


    //
    // Segments menu
    //
    QAction* qa_edit_default = new QAction(  i18n("Open in &Default Editor"), dynamic_cast<QObject*>(this) );
			connect( qa_edit_default, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEdit()) );
			qa_edit_default->setObjectName( "edit_default" );		//
			//qa_edit_default->setCheckable( true );		//
			qa_edit_default->setAutoRepeat( false );	//
			//qa_edit_default->setActionGroup( 0 );		// QActionGroup*
			//qa_edit_default->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("matrix");

    QAction *qa_edit_matrix = new QAction(i18n("Open in Matri&x Editor"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_edit_matrix->setIcon(icon); 
			connect( qa_edit_matrix, SIGNAL(triggered()), this, SLOT(slotEditInMatrix())  );

    icon = il.load("matrix-percussion");

    QAction *qa_edit_percussion_matrix = new QAction(i18n("Open in &Percussion Matrix Editor"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_edit_percussion_matrix->setIcon(icon); 
			connect( qa_edit_percussion_matrix, SIGNAL(triggered()), this, SLOT(slotEditInPercussionMatrix())  );

    icon = il.load("notation");

    QAction *qa_edit_notation = new QAction(i18n("Open in &Notation Editor"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_edit_notation->setIcon(icon); 
			connect( qa_edit_notation, SIGNAL(triggered()), this, SLOT(slotEditAsNotation())  );

    icon = il.load("eventlist");

    QAction *qa_edit_event_list = new QAction(i18n("Open in &Event List Editor"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_edit_event_list->setIcon(icon); 
			connect( qa_edit_event_list, SIGNAL(triggered()), this, SLOT(slotEditInEventList())  );

    icon = il.load("quantize");

    QAction *qa_quantize_selection = new QAction(i18n("&Quantize..."), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_quantize_selection->setIcon(icon); 
			connect( qa_quantize_selection, SIGNAL(triggered()), this, SLOT(slotQuantizeSelection())  );

    QAction* qa_relabel_segment = new QAction(  SegmentLabelCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_relabel_segment, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRelabelSegments()) );
			qa_relabel_segment->setObjectName( "relabel_segment" );		//
			//qa_relabel_segment->setCheckable( true );		//
			qa_relabel_segment->setAutoRepeat( false );	//
			//qa_relabel_segment->setActionGroup( 0 );		// QActionGroup*
			//qa_relabel_segment->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose = new QAction(  SegmentTransposeCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_transpose, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTransposeSegments()) );
			qa_transpose->setObjectName( "transpose" );		//
			//qa_transpose->setCheckable( true );		//
			qa_transpose->setAutoRepeat( false );	//
			//qa_transpose->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_repeat_quantize = new QAction(  i18n("Repeat Last Quantize"), dynamic_cast<QObject*>(this) );
			connect( qa_repeat_quantize, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRepeatQuantizeSelection()) );
			qa_repeat_quantize->setObjectName( "repeat_quantize" );		//
			//qa_repeat_quantize->setCheckable( true );		//
			qa_repeat_quantize->setAutoRepeat( false );	//
			//qa_repeat_quantize->setActionGroup( 0 );		// QActionGroup*
			//qa_repeat_quantize->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_rescale = new QAction(  SegmentRescaleCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_rescale, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRescaleSelection()) );
			qa_rescale->setObjectName( "rescale" );		//
			//qa_rescale->setCheckable( true );		//
			qa_rescale->setAutoRepeat( false );	//
			//qa_rescale->setActionGroup( 0 );		// QActionGroup*
			//qa_rescale->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_auto_split = new QAction(  SegmentAutoSplitCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_auto_split, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotAutoSplitSelection()) );
			qa_auto_split->setObjectName( "auto_split" );		//
			//qa_auto_split->setCheckable( true );		//
			qa_auto_split->setAutoRepeat( false );	//
			//qa_auto_split->setActionGroup( 0 );		// QActionGroup*
			//qa_auto_split->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_split_by_pitch = new QAction(  SegmentSplitByPitchCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_split_by_pitch, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSplitSelectionByPitch()) );
			qa_split_by_pitch->setObjectName( "split_by_pitch" );		//
			//qa_split_by_pitch->setCheckable( true );		//
			qa_split_by_pitch->setAutoRepeat( false );	//
			//qa_split_by_pitch->setActionGroup( 0 );		// QActionGroup*
			//qa_split_by_pitch->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_split_by_recording = new QAction(  SegmentSplitByRecordingSrcCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_split_by_recording, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSplitSelectionByRecordedSrc()) );
			qa_split_by_recording->setObjectName( "split_by_recording" );		//
			//qa_split_by_recording->setCheckable( true );		//
			qa_split_by_recording->setAutoRepeat( false );	//
			//qa_split_by_recording->setActionGroup( 0 );		// QActionGroup*
			//qa_split_by_recording->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_split_at_time = new QAction(  i18n("Split at Time..."), dynamic_cast<QObject*>(this) );
			connect( qa_split_at_time, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSplitSelectionAtTime()) );
			qa_split_at_time->setObjectName( "split_at_time" );		//
			//qa_split_at_time->setCheckable( true );		//
			qa_split_at_time->setAutoRepeat( false );	//
			//qa_split_at_time->setActionGroup( 0 );		// QActionGroup*
			//qa_split_at_time->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_jog_left = new QAction(  i18n("Jog &Left"), dynamic_cast<QObject*>(this) );
			connect( qa_jog_left, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJogLeft()) );
			qa_jog_left->setObjectName( "jog_left" );		//
			//qa_jog_left->setCheckable( true );		//
			qa_jog_left->setAutoRepeat( false );	//
			//qa_jog_left->setActionGroup( 0 );		// QActionGroup*
			//qa_jog_left->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_jog_right = new QAction(  i18n("Jog &Right"), dynamic_cast<QObject*>(this) );
			connect( qa_jog_right, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJogRight()) );
			qa_jog_right->setObjectName( "jog_right" );		//
			//qa_jog_right->setCheckable( true );		//
			qa_jog_right->setAutoRepeat( false );	//
			//qa_jog_right->setActionGroup( 0 );		// QActionGroup*
			//qa_jog_right->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_set_segment_start = new QAction(  i18n("Set Start Time..."), dynamic_cast<QObject*>(this) );
			connect( qa_set_segment_start, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSetSegmentStartTimes()) );
			qa_set_segment_start->setObjectName( "set_segment_start" );		//
			//qa_set_segment_start->setCheckable( true );		//
			qa_set_segment_start->setAutoRepeat( false );	//
			//qa_set_segment_start->setActionGroup( 0 );		// QActionGroup*
			//qa_set_segment_start->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_set_segment_duration = new QAction(  i18n("Set Duration..."), dynamic_cast<QObject*>(this) );
			connect( qa_set_segment_duration, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSetSegmentDurations()) );
			qa_set_segment_duration->setObjectName( "set_segment_duration" );		//
			//qa_set_segment_duration->setCheckable( true );		//
			qa_set_segment_duration->setAutoRepeat( false );	//
			//qa_set_segment_duration->setActionGroup( 0 );		// QActionGroup*
			//qa_set_segment_duration->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_join_segments = new QAction(  SegmentJoinCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_join_segments, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJoinSegments()) );
			qa_join_segments->setObjectName( "join_segments" );		//
			//qa_join_segments->setCheckable( true );		//
			qa_join_segments->setAutoRepeat( false );	//
			//qa_join_segments->setActionGroup( 0 );		// QActionGroup*
			//qa_join_segments->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_repeats_to_real_copies = new QAction(  i18n("Turn Re&peats into Copies"), dynamic_cast<QObject*>(this) );
			connect( qa_repeats_to_real_copies, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRepeatingSegments()) );
			qa_repeats_to_real_copies->setObjectName( "repeats_to_real_copies" );		//
			//qa_repeats_to_real_copies->setCheckable( true );		//
			qa_repeats_to_real_copies->setAutoRepeat( false );	//
			//qa_repeats_to_real_copies->setActionGroup( 0 );		// QActionGroup*
			//qa_repeats_to_real_copies->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_manage_trigger_segments = new QAction(  i18n("Manage Tri&ggered Segments"), dynamic_cast<QObject*>(this) );
			connect( qa_manage_trigger_segments, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotManageTriggerSegments()) );
			qa_manage_trigger_segments->setObjectName( "manage_trigger_segments" );		//
			//qa_manage_trigger_segments->setCheckable( true );		//
			qa_manage_trigger_segments->setAutoRepeat( false );	//
			//qa_manage_trigger_segments->setActionGroup( 0 );		// QActionGroup*
			//qa_manage_trigger_segments->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_groove_quantize = new QAction(  i18n("Set Tempos from &Beat Segment"), dynamic_cast<QObject*>(this) );
			connect( qa_groove_quantize, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotGrooveQuantize()) );
			qa_groove_quantize->setObjectName( "groove_quantize" );		//
			//qa_groove_quantize->setCheckable( true );		//
			qa_groove_quantize->setAutoRepeat( false );	//
			//qa_groove_quantize->setActionGroup( 0 );		// QActionGroup*
			//qa_groove_quantize->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_set_tempo_to_segment_length = new QAction(  i18n("Set &Tempo to Audio Segment Duration"), dynamic_cast<QObject*>(this) );
			connect( qa_set_tempo_to_segment_length, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTempoToSegmentLength()) );
			qa_set_tempo_to_segment_length->setObjectName( "set_tempo_to_segment_length" );		//
			//qa_set_tempo_to_segment_length->setCheckable( true );		//
			qa_set_tempo_to_segment_length->setAutoRepeat( false );	//
			//qa_set_tempo_to_segment_length->setActionGroup( 0 );		// QActionGroup*
			//qa_set_tempo_to_segment_length->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("manage-audio-segments");
    QAction *qa_audio_manager = new QAction(i18n("Manage A&udio Files"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_audio_manager->setIcon(icon); 
			connect( qa_audio_manager, SIGNAL(triggered()), this, SLOT(slotAudioManager())  );

    m_viewSegmentLabels = new QAction(  i18n("Show Segment Labels"), dynamic_cast<QObject*>(this) );
;

    	connect( m_viewSegmentLabels, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleSegmentLabels()) ); ;

    
;

    	m_viewSegmentLabels->setObjectName("show_segment_labels");
;

    	m_viewSegmentLabels->setCheckable( true );	//
;

    	m_viewSegmentLabels->setAutoRepeat( false );	//
;

    	//m_viewSegmentLabels->setActionGroup( 0 );	// QActionGroup*

    //
    // Tracks menu
    //
    icon = il.load("add_tracks");

    QAction *qa_add_track = new QAction(i18n("Add &Track"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_add_track->setIcon(icon); 
			connect( qa_add_track, SIGNAL(triggered()), this, SLOT(slotAddTrack())  );

    QAction* qa_add_tracks = new QAction(  i18n("&Add Tracks..."), dynamic_cast<QObject*>(this) );
			connect( qa_add_tracks, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotAddTracks()) );
			qa_add_tracks->setObjectName( "add_tracks" );		//
			//qa_add_tracks->setCheckable( true );		//
			qa_add_tracks->setAutoRepeat( false );	//
			//qa_add_tracks->setActionGroup( 0 );		// QActionGroup*
			//qa_add_tracks->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("delete_track");

    QAction *qa_delete_track = new QAction(i18n("D&elete Track"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_delete_track->setIcon(icon); 
			connect( qa_delete_track, SIGNAL(triggered()), this, SLOT(slotDeleteTrack())  );

    icon = il.load("move_track_down");

    QAction *qa_move_track_down = new QAction(i18n("Move Track &Down"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_move_track_down->setIcon(icon); 
			connect( qa_move_track_down, SIGNAL(triggered()), this, SLOT(slotMoveTrackDown())  );

    icon = il.load("move_track_up");

    QAction *qa_move_track_up = new QAction(i18n("Move Track &Up"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_move_track_up->setIcon(icon); 
			connect( qa_move_track_up, SIGNAL(triggered()), this, SLOT(slotMoveTrackUp())  );

    QAction* qa_select_next_track = new QAction(  i18n("Select &Next Track"), dynamic_cast<QObject*>(this) );
			connect( qa_select_next_track, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTrackDown()) );
			qa_select_next_track->setObjectName( "select_next_track" );		//
			//qa_select_next_track->setCheckable( true );		//
			qa_select_next_track->setAutoRepeat( false );	//
			//qa_select_next_track->setActionGroup( 0 );		// QActionGroup*
			//qa_select_next_track->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_select_previous_track = new QAction(  i18n("Select &Previous Track"), dynamic_cast<QObject*>(this) );
			connect( qa_select_previous_track, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTrackUp()) );
			qa_select_previous_track->setObjectName( "select_previous_track" );		//
			//qa_select_previous_track->setCheckable( true );		//
			qa_select_previous_track->setAutoRepeat( false );	//
			//qa_select_previous_track->setActionGroup( 0 );		// QActionGroup*
			//qa_select_previous_track->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_toggle_mute_track = new QAction(  i18n("Mute or Unmute Track"), dynamic_cast<QObject*>(this) );
			connect( qa_toggle_mute_track, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleMutedCurrentTrack()) );
			qa_toggle_mute_track->setObjectName( "toggle_mute_track" );		//
			//qa_toggle_mute_track->setCheckable( true );		//
			qa_toggle_mute_track->setAutoRepeat( false );	//
			//qa_toggle_mute_track->setActionGroup( 0 );		// QActionGroup*
			//qa_toggle_mute_track->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_toggle_arm_track = new QAction(  i18n("Arm or Un-arm Track for Record"), dynamic_cast<QObject*>(this) );
			connect( qa_toggle_arm_track, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleRecordCurrentTrack()) );
			qa_toggle_arm_track->setObjectName( "toggle_arm_track" );		//
			//qa_toggle_arm_track->setCheckable( true );		//
			qa_toggle_arm_track->setAutoRepeat( false );	//
			//qa_toggle_arm_track->setActionGroup( 0 );		// QActionGroup*
			//qa_toggle_arm_track->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    icon = il.load("mute-all");

    QAction *qa_mute_all_tracks = new QAction(i18n("&Mute all Tracks"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_mute_all_tracks->setIcon(icon); 
			connect( qa_mute_all_tracks, SIGNAL(triggered()), this, SLOT(slotMuteAllTracks())  );

    icon = il.load("un-mute-all");

    QAction *qa_unmute_all_tracks = new QAction(i18n("&Unmute all Tracks"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_unmute_all_tracks->setIcon(icon); 
			connect( qa_unmute_all_tracks, SIGNAL(triggered()), this, SLOT(slotUnmuteAllTracks())  );

    QAction* qa_remap_instruments = new QAction(  i18n("&Remap Instruments..."), dynamic_cast<QObject*>(this) );
			connect( qa_remap_instruments, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRemapInstruments()) );
			qa_remap_instruments->setObjectName( "remap_instruments" );		//
			//qa_remap_instruments->setCheckable( true );		//
			qa_remap_instruments->setAutoRepeat( false );	//
			//qa_remap_instruments->setActionGroup( 0 );		// QActionGroup*
			//qa_remap_instruments->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Studio menu
    //
    icon = il.load("mixer");

    QAction *qa_audio_mixer = new QAction(i18n("&Audio Mixer"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_audio_mixer->setIcon(icon); 
			connect( qa_audio_mixer, SIGNAL(triggered()), this, SLOT(slotOpenAudioMixer())  );

    icon = il.load("midimixer");

    QAction *qa_midi_mixer = new QAction(i18n("Midi Mi&xer"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_midi_mixer->setIcon(icon); 
			connect( qa_midi_mixer, SIGNAL(triggered()), this, SLOT(slotOpenMidiMixer())  );

    icon = il.load("manage-midi-devices");

    QAction *qa_manage_devices = new QAction(i18n("Manage MIDI &Devices"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_manage_devices->setIcon(icon); 
			connect( qa_manage_devices, SIGNAL(triggered()), this, SLOT(slotManageMIDIDevices())  );

    icon = il.load("manage-synth-plugins");

    QAction *qa_manage_synths = new QAction(i18n("Manage S&ynth Plugins"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_manage_synths->setIcon(icon); 
			connect( qa_manage_synths, SIGNAL(triggered()), this, SLOT(slotManageSynths())  );

    QAction *qa_modify_midi_filters = new QAction(i18n("Modify MIDI &Filters"), dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_modify_midi_filters->setIconText("filter"); 
			connect( qa_modify_midi_filters, SIGNAL(triggered()), this, SLOT(slotModifyMIDIFilters())  );

    m_enableMIDIrouting = new QAction(  i18n("MIDI Thru Routing"), dynamic_cast<QObject*>(this) );
;

    	connect( m_enableMIDIrouting, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEnableMIDIThruRouting()) ); ;

    
;

    	m_enableMIDIrouting->setObjectName("enable_midi_routing");
;

    	m_enableMIDIrouting->setCheckable( true );	//
;

    	m_enableMIDIrouting->setAutoRepeat( false );	//
;

    	//m_enableMIDIrouting->setActionGroup( 0 );	// QActionGroup*

    icon = il.load("time-musical");

    QAction* qa_manage_metronome = new QAction(  i18n("Manage &Metronome"), dynamic_cast<QObject*>(this) );
			connect( qa_manage_metronome, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotManageMetronome()) );
			qa_manage_metronome->setObjectName( "manage_metronome" );		//
			//qa_manage_metronome->setCheckable( true );		//
			qa_manage_metronome->setAutoRepeat( false );	//
			//qa_manage_metronome->setActionGroup( 0 );		// QActionGroup*
			//qa_manage_metronome->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_save_default_studio = new QAction(  i18n("&Save Current Document as Default Studio"), dynamic_cast<QObject*>(this) );
			connect( qa_save_default_studio, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSaveDefaultStudio()) );
			qa_save_default_studio->setObjectName( "save_default_studio" );		//
			//qa_save_default_studio->setCheckable( true );		//
			qa_save_default_studio->setAutoRepeat( false );	//
			//qa_save_default_studio->setActionGroup( 0 );		// QActionGroup*
			//qa_save_default_studio->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_load_default_studio = new QAction(  i18n("&Import Default Studio"), dynamic_cast<QObject*>(this) );
			connect( qa_load_default_studio, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotImportDefaultStudio()) );
			qa_load_default_studio->setObjectName( "load_default_studio" );		//
			//qa_load_default_studio->setCheckable( true );		//
			qa_load_default_studio->setAutoRepeat( false );	//
			//qa_load_default_studio->setActionGroup( 0 );		// QActionGroup*
			//qa_load_default_studio->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_load_studio = new QAction(  i18n("Im&port Studio from File..."), dynamic_cast<QObject*>(this) );
			connect( qa_load_studio, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotImportStudio()) );
			qa_load_studio->setObjectName( "load_studio" );		//
			//qa_load_studio->setCheckable( true );		//
			qa_load_studio->setAutoRepeat( false );	//
			//qa_load_studio->setActionGroup( 0 );		// QActionGroup*
			//qa_load_studio->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_reset_midi_network = new QAction(  i18n("&Reset MIDI Network"), dynamic_cast<QObject*>(this) );
			connect( qa_reset_midi_network, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotResetMidiNetwork()) );
			qa_reset_midi_network->setObjectName( "reset_midi_network" );		//
			//qa_reset_midi_network->setCheckable( true );		//
			qa_reset_midi_network->setAutoRepeat( false );	//
			//qa_reset_midi_network->setActionGroup( 0 );		// QActionGroup*
			//qa_reset_midi_network->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //@@@ JAS Check here first for errors and pointer deallocation
    m_setQuickMarkerAction = new QAction(i18n("Set Quick Marker at Playback Position"), dynamic_cast<QObject*>(this));
    m_setQuickMarkerAction->setIconText(0); 
    connect(m_setQuickMarkerAction, SIGNAL(triggered()), this, SLOT(slotSetQuickMarker()));

    //@@@ JAS Check here first for errors and pointer deallocation
    m_jumpToQuickMarkerAction = new QAction(i18n("Jump to Quick Marker"), dynamic_cast<QObject*>(this)); 
    m_jumpToQuickMarkerAction->setIconText(0); 
    connect( m_jumpToQuickMarkerAction, SIGNAL(triggered()), this, SLOT(slotJumpToQuickMarker()));

    //
    // Marker Ruler popup menu
    //
//    new KAction(i18n("Insert Marker"), 0, 0, this,
//             SLOT(slotInsertMarkerHere()), actionCollection(),
//             "insert_marker_here");
//    
//    new KAction(i18n("Insert Marker at Playback Position"), 0, 0, this,
//             SLOT(slotInsertMarkerAtPointer()), actionCollection(),
//             "insert_marker_at_pointer");
//
//    new KAction(i18n("Delete Marker"), 0, 0, this,
//             SLOT(slotDeleteMarker()), actionCollection(),
//             "delete_marker");



    //
    // Transport menu
    //

    // Transport controls [rwb]
    //
    // We set some default key bindings - with numlock off
    // use 1 (End) and 3 (Page Down) for Rwd and Ffwd and
    // 0 (insert) and keypad Enter for Play and Stop
    //
	QActionGroup* qag_TransportDialogConfigGroup = new QActionGroup( dynamic_cast<QObject*>(this) );
	//### FIX: deallocate QActionGroup
	//
    icon = il.load("transport-play");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_playTransport = new QAction(i18n("&Play"), dynamic_cast<QObject*>(this));
    m_playTransport->setIcon(icon); 
    connect(m_playTransport, SIGNAL(triggered()), this, SLOT(slotPlay()));

    // Alternative shortcut for Play
	//### use pointer instead ?:
	//QShortcut playShortcut = QShortcut( m_playTransport->shortcut(), dynamic_cast<QWidgett*>(this) );
	QList<QKeySequence> playShortcuts;
	//QKeySequence playShortcut = m_playTransport->shortcut();
	playShortcuts.append( m_playTransport->shortcut() );
	playShortcuts.append( QKeySequence(Qt::Key_Return + Qt::CTRL) );
	
    m_playTransport->setShortcuts(playShortcuts);
	m_playTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-stop");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_stopTransport = new QAction(i18n("&Stop"), dynamic_cast<QObject*>(this));
    m_stopTransport->setIcon(icon); 
    connect(m_stopTransport, SIGNAL(triggered()), this, SLOT(slotStop()));
	m_stopTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-ffwd");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_ffwdTransport = new QAction(i18n("&Fast Forward"), dynamic_cast<QObject*>(this));
    m_ffwdTransport->setIcon(icon); 
    connect(m_ffwdTransport, SIGNAL(triggered()), this, SLOT(slotFastforward()));
	m_ffwdTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-rewind");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_rewindTransport = new QAction(i18n("Re&wind"), dynamic_cast<QObject*>(this));
    m_rewindTransport->setIcon(icon); 
    connect(m_rewindTransport, SIGNAL(triggered()), this, SLOT(slotRewind()));
	m_rewindTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-record");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_recordTransport = new QAction(i18n("P&unch in Record"), dynamic_cast<QObject*>(this) );
    m_recordTransport->setIcon(icon); 
    connect( m_recordTransport, SIGNAL(triggered()), this, SLOT(slotToggleRecord()));
	m_recordTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-record");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_recordTransport = new QAction(i18n("&Record"), dynamic_cast<QObject*>(this) );
    m_recordTransport->setIcon(icon); 
    connect(m_recordTransport, SIGNAL(triggered()), this, SLOT(slotRecord()));
	m_recordTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-rewind-end");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_rewindEndTransport = new QAction(i18n("Rewind to &Beginning"), dynamic_cast<QObject*>(this));
    m_rewindEndTransport->setIcon(icon); 
    connect(m_rewindEndTransport, SIGNAL(triggered()), this, SLOT(slotRewindToBeginning()));
	m_rewindEndTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-ffwd-end");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_ffwdEndTransport = new QAction(i18n("Fast Forward to &End"), dynamic_cast<QObject*>(this));
    m_ffwdEndTransport->setIcon(icon); 
    connect(m_ffwdEndTransport, SIGNAL(triggered()), this, SLOT(slotFastForwardToEnd()));
	m_ffwdEndTransport->setActionGroup( qag_TransportDialogConfigGroup );

    icon = il.load("transport-tracking");

    QAction* qa_toggle_tracking = new QAction( icon, i18n("Scro&ll to Follow Playback"), dynamic_cast<QObject*>(this) );
    connect( qa_toggle_tracking, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotToggleTracking()) );
	qa_toggle_tracking->setObjectName( "toggle_tracking" );	//### FIX: deallocate QAction ptr
	qa_toggle_tracking->setCheckable( true );	//
	qa_toggle_tracking->setShortcut( Qt::Key_Pause );
	qa_toggle_tracking->setAutoRepeat( false );	//
	//qa_toggle_tracking->setActionGroup( 0 );	// QActionGroup*
	qa_toggle_tracking->setChecked( true );	//

    icon = il.load("transport-panic");

    //@@@ JAS Check here first for errors and pointer deallocation
    m_panic = new QAction(i18n("Panic"), dynamic_cast<QObject*>(this)); //@@@ JAS Check to make certain pointer is deallocated
    m_panic->setIcon(icon); 
    connect(m_panic, SIGNAL(triggered()), this, SLOT(slotPanic()));

    // DEBUG FACILITY
    QAction* qa_debug_dump_segments = new QAction(  i18n("Segment Debug Dump "), dynamic_cast<QObject*>(this) );
    connect( qa_debug_dump_segments, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDebugDump()) );
    qa_debug_dump_segments->setObjectName( "debug_dump_segments" );		//
    //qa_debug_dump_segments->setCheckable( true );		//
    qa_debug_dump_segments->setAutoRepeat( false );	//
    //qa_debug_dump_segments->setActionGroup( 0 );		// QActionGroup*
    //qa_debug_dump_segments->setChecked( false );		//
    //### FIX: deallocate QAction ptr
    

    // create main gui
    //
    rgTempQtIV->createGUI("rosegardenui.rc", false);

    createAndSetupTransport();

    // transport toolbar is hidden by default - TODO : this should be in options
    //
    //toolBar("Transport Toolbar")->hide();

	// was QPopupMenu
	QMenu* setTrackInstrumentMenu = this->findChild<QMenu*>("set_track_instrument");

    if (setTrackInstrumentMenu) {
        connect(setTrackInstrumentMenu, SIGNAL(aboutToShow()),
                this, SLOT(slotPopulateTrackInstrumentPopup()));
    } else {
        RG_DEBUG << "RosegardenGUIApp::setupActions() : couldn't find set_track_instrument menu - check rosegardenui.rcn\n";
    }

    setRewFFwdToAutoRepeat();
*/
}


void
RosegardenGUIApp::setupRecentFilesMenu()
{
    if (!m_menuRecent) {
        std::cerr << "ERROR: RosegardenGUIApp::setupRecentFilesMenu: No recent files menu!" << std::endl;
        return;
    }
    m_menuRecent->clear();
    std::vector<QString> files = m_recentFiles.getRecent();
    for (size_t i = 0; i < files.size(); ++i) {
	QAction *action = new QAction(files[i], this);
	connect(action, SIGNAL(triggered()), this, SLOT(slotFileOpenRecent()));
	m_menuRecent->addAction(action);
    }
}


void RosegardenGUIApp::setRewFFwdToAutoRepeat()
{
	//QWidget* transportToolbar = factory()->container("Transport Toolbar", this);
	QWidget* transportToolbar = this->toolBar("Transport Toolbar");

    if (transportToolbar) {
		//QList<QPushButton *> allPButtons = parentWidget.findChildren<QPushButton *>();
		QPushButton* rew = transportToolbar->findChild<QPushButton*>( "rewind" );
		QPushButton* ffw = transportToolbar->findChild<QPushButton*>( "fast_forward" );
		//rew->setAutoRepeat(true);
		//ffw->setAutoRepeat(true);
		
		/*
		//&&& //### reimplement all this:
		//
		//
		//### changed var *l from <QObjectList*> to <QListWidget*>
		QListWidget *l = transportToolbar->queryList();		//###################### FIX next
		QListWidgetItem it(*l); // iterate over the buttons
		QObject *obj;
		
        while ( (obj = it.current()) != 0 ) {
            // for each found object...
            ++it;
            // RG_DEBUG << "obj name : " << obj->objectName() << endl;
            QString objName = obj->objectName();

            if (objName.endsWith("rewind") || objName.endsWith("fast_forward")) {
                QPushButton* btn = dynamic_cast<QPushButton*>(obj);
                if (!btn) {
                    RG_DEBUG << "Very strange - found widgets in transport_toolbar which aren't buttons\n";

                    continue;
                }
                btn->setAutoRepeat(true);
            }


        }
        delete l;
		*/
    } else {
        RG_DEBUG << "transportToolbar == 0\n";
    }

}

void RosegardenGUIApp::initZoomToolbar()
{
    QToolBar *zoomToolbar = this->addToolBar("Zoom Toolbar");
    if (!zoomToolbar) {
        RG_DEBUG << "RosegardenGUIApp::initZoomToolbar() : "
        << "zoom toolbar not found" << endl;
        return ;
    }

    new QLabel(i18n("  Zoom:  "), zoomToolbar, "kde toolbar widget");

    std::vector<double> zoomSizes; // in units-per-pixel
    double defaultBarWidth44 = 100.0;
    double duration44 = TimeSignature(4, 4).getBarDuration();
    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0 , 20.0 };

    for (unsigned int i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
        zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));
    }

    // zoom labels
    QString minZoom = QString("%1%").arg(factors[0] * 100.0);
    QString maxZoom = QString("%1%").arg(factors[(sizeof(factors) / sizeof(factors[0])) - 1] * 100.0);

    m_zoomSlider = new ZoomSlider<double>
                   (zoomSizes, -1, Qt::Horizontal, zoomToolbar, "kde toolbar widget");
    m_zoomSlider->setTracking(true);
    m_zoomSlider->setFocusPolicy(Qt::NoFocus);
    m_zoomLabel = new QLabel(minZoom, zoomToolbar, "kde toolbar widget");
    m_zoomLabel->setIndent(10);

    connect(m_zoomSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeZoom(int)));

    // set initial zoom - we might want to make this a settings option
    //    m_zoomSlider->setToDefault();

}

void RosegardenGUIApp::initStatusBar()
{
    KTmpStatusMsg::setDefaultMsg("");
	// QStatusBar::addPermanentWidget ( QWidget * widget, int stretch = 0 )
//	statusBar()->addPermanentWidget( KTmpStatusMsg::getDefaultMsg(),
//			  KTmpStatusMsg::getDefaultId(), 1);
//	statusBar()->setItemAlignment(KTmpStatusMsg::getDefaultId(),
//			  AlignLeft | AlignVCenter);
	m_statusBarLabel1 = new QLabel( "status", this );
	
	statusBar()->addPermanentWidget( m_statusBarLabel1 );
//	statusBar()->setItemAlignment(KTmpStatusMsg::getDefaultId(),
//			  AlignLeft | AlignVCenter);

    m_progressBar = new ProgressBar(100, true, statusBar());
    //    m_progressBar()->setMinimumWidth(100);
    m_progressBar->setFixedWidth(60);
    m_progressBar->setFixedHeight(18);
//    m_progressBar->setTextEnabled(false);  //&&&
    statusBar()->addPermanentWidget( m_progressBar );
	
	
	/* init toolbar */
	/****************/
	
	m_toolBarMain = new QToolBar( "Main Toolbar", this );
	this->addToolBar( Qt::TopToolBarArea, m_toolBarMain );
}


QToolBar* RosegardenGUIApp::toolBar( const char* name ){
	/* convenience method qt3 -> qt4 */
        if( QString(name) == "" ){
		return m_toolBarMain;
	}
	return this->findChild<QToolBar*>( name );
}

void RosegardenGUIApp::initView()
{
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    RG_DEBUG << "RosegardenGUIApp::initView()" << endl;

    Composition &comp = m_doc->getComposition();

    // Ensure that the start and end markers for the piece are set
    // to something reasonable
    //
    if (comp.getStartMarker() == 0 &&
            comp.getEndMarker() == 0) {
        int endMarker = comp.getBarRange(100 + comp.getNbBars()).second;
        comp.setEndMarker(endMarker);
    }

    m_swapView = new RosegardenGUIView(m_viewTrackLabels->isChecked(),
                                       m_segmentParameterBox,
                                       m_instrumentParameterBox,
                                       m_trackParameterBox, this);

    // Connect up this signal so that we can force tool mode
    // changes from the view
    connect(m_swapView, SIGNAL(activateTool(QString)),
            this, SLOT(slotActivateTool(QString)));

    connect(m_swapView,
            SIGNAL(segmentsSelected(const SegmentSelection &)),
            SIGNAL(segmentsSelected(const SegmentSelection &)));

    connect(m_swapView,
            SIGNAL(addAudioFile(AudioFileId)),
            SLOT(slotAddAudioFile(AudioFileId)));

    connect(m_swapView, SIGNAL(toggleSolo(bool)), SLOT(slotToggleSolo(bool)));

    m_doc->attachView(m_swapView);
	
    // m_mainDockWidget->setWidget(m_swapView);	//&&& later: check dockWidget code
	
    //     setCentralWidget(m_swapView);
    setCaption(m_doc->getTitle());


    // Transport setup
    //
    std::string transportMode = m_doc->getConfiguration().
                       get
                        <String>
                        (DocumentConfiguration::TransportMode);
 

    slotEnableTransport(true);

    // and the time signature
    //
    getTransport()->setTimeSignature(comp.getTimeSignatureAt(comp.getPosition()));

    // set the tempo in the transport
    //
    getTransport()->setTempo(comp.getCurrentTempo());

    // bring the transport to the front
    //
    getTransport()->raise();

    // set the play metronome button
    getTransport()->MetronomeButton()->setOn(comp.usePlayMetronome());

    // Set the solo button
    getTransport()->SoloButton()->setOn(comp.isSolo());

    // set the transport mode found in the configuration
    getTransport()->setNewMode(transportMode);

    // set the pointer position
    //
    slotSetPointerPosition(m_doc->getComposition().getPosition());

    // make sure we show
    //
    RosegardenGUIView *oldView = m_view;
    m_view = m_swapView;

    connect(m_view, SIGNAL(stateChange(QString, bool)),
            this, SLOT (slotStateChanged(QString, bool)));

    connect(m_view, SIGNAL(instrumentParametersChanged(InstrumentId)),
            this, SIGNAL(instrumentParametersChanged(InstrumentId)));

    // We only check for the SequenceManager to make sure
    // we're not on the first pass though - we don't want
    // to send these toggles twice on initialisation.
    //
    // Clunky but we just about get away with it for the
    // moment.
    //
    if (m_seqManager != 0) {
        slotToggleChordNameRuler();
        slotToggleRulers();
        slotToggleTempoRuler();
        slotTogglePreviews();
        slotToggleSegmentLabels();

        // Reset any loop on the sequencer
        //
        try {
            if (isUsingSequencer())
                m_seqManager->setLoop(0, 0);
		leaveActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        } catch (QString s) {
            KStartupLogo::hideIfStillThere();
            CurrentProgressDialog::freeze();
			QMessageBox::critical( dynamic_cast<QWidget*>(this), "", s, QMessageBox::Ok, QMessageBox::Ok);
            CurrentProgressDialog::thaw();
        }

        connect(m_seqManager, SIGNAL(controllerDeviceEventReceived(MappedEvent *)),
                m_view, SLOT(slotControllerDeviceEventReceived(MappedEvent *)));
    }

    //    delete m_playList;
    //    m_playList = 0;

    delete m_deviceManager;
    m_deviceManager = 0;

    delete m_synthManager;
    m_synthManager = 0;

    delete m_audioMixer;
    m_audioMixer = 0;

    delete m_bankEditor;
    m_bankEditor = 0;

    delete m_markerEditor;
    m_markerEditor = 0;

    delete m_tempoView;
    m_tempoView = 0;

    delete m_triggerSegmentManager;
    m_triggerSegmentManager = 0;

    delete oldView;

    // set the highlighted track
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

    // play tracking on in the editor by default: turn off if need be
    /* was toggle */ 
	// old. QAction *trackingAction = dynamic_cast<QAction*>
	//                                (actionCollection()->action("toggle_tracking"));
	QAction *trackingAction = findChild<QAction*>( QString("toggle_tracking") );
	// note: findChild not avail. in MSVC_6 (ms-windows), use qFindChild() instead 
	
	
	if (trackingAction && !trackingAction->isChecked()) {
        m_view->getTrackEditor()->slotToggleTracking();
    }

    m_view->show();

    connect(m_view->getTrackEditor()->getSegmentCanvas(),
            SIGNAL(showContextHelp(const QString &)),
            this,
            SLOT(slotShowToolHelp(const QString &)));

    // We have to do this to make sure that the 2nd call ("select")
    // actually has any effect. Activating the same radio action
    // doesn't work the 2nd time (like pressing down the same radio
    // button twice - it doesn't have any effect), so if you load two
    // files in a row, on the 2nd file a new SegmentCanvas will be
    // created but its tool won't be set, even though it will appear
    // to be selected.
    //
    // old: actionCollection()->action("move")->activate();
	QAction *actionx = this->findChild<QAction*>( QString("move") );
	actionx->setEnabled(true);
	
	if (m_doc->getComposition().getNbSegments() > 0){
		//actionCollection()->action("select")->activate();
		QAction *actionx = this->findChild<QAction*>( QString("select") );
		actionx->setEnabled(true);
	}else {
		//actionCollection()->action("draw")->activate();
		QAction *actionx = this->findChild<QAction*>( QString("draw") );
		actionx->setEnabled(true);
	}
	
	
/*    int zoomLevel = m_doc->getConfiguration().
                    get
                        <Int>
                        (DocumentConfiguration::ZoomLevel);
	*/
	//QSettings settings;
	int zoomLevel = m_doc->getConfiguration().get<Int>(DocumentConfiguration::ZoomLevel);

    m_zoomSlider->setSize(double(zoomLevel) / 1000.0);
    slotChangeZoom(zoomLevel);

    //slotChangeZoom(int(m_zoomSlider->getCurrentSize()));

    enterActionState("new_file"); //@@@ JAS orig. 0

    ProgressDialog::processEvents();

    if (m_viewChordNameRuler->isChecked()) {
        SetWaitCursor swc;
        m_view->initChordNameRuler();
    } else {
        m_view->initChordNameRuler();
    }
}

void RosegardenGUIApp::setDocument(RosegardenGUIDoc* newDocument)
{
    if (m_doc == newDocument)
        return ;

    emit documentAboutToChange();
    qApp->processEvents(); // to make sure all opened dialogs (mixer, midi devices...) are closed

    // Take care of all subparts which depend on the document

    // Caption
    //
    //QString caption = qApp->caption();
	//QString caption = qApp->windowTitle();
	QString caption = qApp->applicationName();
	
    setCaption(caption + ": " + newDocument->getTitle());
	

    //     // reset AudioManagerDialog
    //     //
    //     delete m_audioManagerDialog; // TODO : replace this with a connection to documentAboutToChange() sig.
    //     m_audioManagerDialog = 0;

    RosegardenGUIDoc* oldDoc = m_doc;

    m_doc = newDocument;

    if (m_seqManager) // when we're called at startup, the seq. man. isn't created yet
        m_seqManager->setDocument(m_doc);

    if (m_markerEditor)
        m_markerEditor->setDocument(m_doc);
    if (m_tempoView) {
        delete m_tempoView;
        m_tempoView = 0;
    }
    if (m_triggerSegmentManager)
        m_triggerSegmentManager->setDocument(m_doc);

    m_trackParameterBox->setDocument(m_doc);
    m_segmentParameterBox->setDocument(m_doc);
    m_instrumentParameterBox->setDocument(m_doc);

#ifdef HAVE_LIBLO

    if (m_pluginGUIManager) {
        m_pluginGUIManager->stopAllGUIs();
        m_pluginGUIManager->setStudio(&m_doc->getStudio());
    }
#endif

    if (getView() &&
        getView()->getTrackEditor() &&
        getView()->getTrackEditor()->getSegmentCanvas()) {
        getView()->getTrackEditor()->getSegmentCanvas()->endAudioPreviewGeneration();
    }

    // this will delete all edit views
    //
    delete oldDoc;

    // connect needed signals
    //
    connect(m_segmentParameterBox, SIGNAL(documentModified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_doc, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    connect(m_doc, SIGNAL(documentModified(bool)),
            this, SLOT(slotDocumentModified(bool)));

    connect(m_doc, SIGNAL(loopChanged(timeT, timeT)),
            this, SLOT(slotSetLoop(timeT, timeT)));

//    CommandHistory::getInstance()->attachView( actionCollection() );		//&&& needed ? how to ?

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            SLOT(update()));
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            SLOT(slotTestClipboard()));

    // connect and start the autosave timer
    connect(m_autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));
    m_autoSaveTimer->start(m_doc->getAutoSavePeriod() * 1000);

    // Connect the playback timer
    //
    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotUpdatePlaybackPosition()));
    connect(m_stopTimer, SIGNAL(timeout()), this, SLOT(slotUpdateMonitoring()));

    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotCheckTransportStatus()));
    connect(m_stopTimer, SIGNAL(timeout()), this, SLOT(slotCheckTransportStatus()));

    // finally recreate the main view
    //
    initView();

    if (getView() && getView()->getTrackEditor()) {
        connect(m_doc, SIGNAL(makeTrackVisible(int)),
                getView()->getTrackEditor(), SLOT(slotScrollToTrack(int)));
    }

    connect(m_doc, SIGNAL(devicesResyncd()),
            this, SLOT(slotDocumentDevicesResyncd()));

    m_doc->syncDevices();
    m_doc->clearModifiedStatus();

    if (newDocument->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig KXMLGUIClient::StateReverse
    }

    // Ensure the sequencer knows about any audio files
    // we've loaded as part of the new Composition
    //
    m_doc->prepareAudio();

    // Do not reset instrument prog. changes after all.
    //     if (m_seqManager)
    //         m_seqManager->preparePlayback(true);

    Composition &comp = m_doc->getComposition();

    // Set any loaded loop at the Composition and
    // on the marker on SegmentCanvas and clients
    //
    if (m_seqManager)
        m_doc->setLoop(comp.getLoopStart(), comp.getLoopEnd());

    emit documentChanged(m_doc);

    m_doc->clearModifiedStatus(); // because it's set as modified by the various
    // init operations
    // TODO: this sucks, have to sort it out somehow.

    // Readjust canvas size
    //
    m_view->getTrackEditor()->slotReadjustCanvasSize();

    m_stopTimer->start(100);
}

void
RosegardenGUIApp::openFile(QString filePath, ImportType type)
{
    RG_DEBUG << "RosegardenGUIApp::openFile " << filePath << endl;

    if (type == ImportCheckType && filePath.endsWith(".rgp")) {
        importProject(filePath);
        return ;
    }

    RosegardenGUIDoc *doc = createDocument(filePath, type);
    if (doc) {
        setDocument(doc);

        // fix # 1235755, "SPB combo not updating after document swap"
        RG_DEBUG << "RosegardenGUIApp::openFile(): calling slotDocColoursChanged() in doc" << endl;
        doc->slotDocColoursChanged();

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        if ( qStrToBool( settings.value("alwaysusedefaultstudio", "false" ) ) ) {

            QString autoloadFile =
                KGlobal::dirs()->findResource("appdata", "autoload.rg");

            QFileInfo autoloadFileInfo(autoloadFile);
            if (autoloadFileInfo.isReadable()) {

                RG_DEBUG << "Importing default studio from " << autoloadFile << endl;

                slotImportStudioFromFile(autoloadFile);
            }
        }

        QFileInfo fInfo(filePath);
        QString tmp ( fInfo.absFilePath() );
        m_recentFiles.add(tmp);
		
        settings.endGroup();
    }
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocument(QString filePath, ImportType importType)
{
    QFileInfo info(filePath);
    RosegardenGUIDoc *doc = 0;

    if (!info.exists()) {
        // can happen with command-line arg, so...
//        KStartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
        QMessageBox::warning(dynamic_cast<QWidget*>(this), filePath, i18n("File \"%1\" does not exist", QMessageBox::Ok, QMessageBox::Ok ));
        return 0;
    }
	
    if (info.isDir()) {
//		KStartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
		QMessageBox::warning(dynamic_cast<QWidget*>(this), filePath, i18n("File \"%1\" is actually a directory"), QMessageBox::Ok, QMessageBox::Ok );
		return 0;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
//		KStartupLogo::hideIfStillThere();  //&&& TODO: use QSplashScreen instead
        QString errStr =
            i18n("You do not have read permission for \"%1\"", filePath);

        QMessageBox::warning(this, "", errStr, QMessageBox::Ok, QMessageBox::Ok );
        return 0;
    }

    // Stop if playing
    //
    if (m_seqManager && m_seqManager->getTransportStatus() == PLAYING)
        slotStop();

    slotEnableTransport(false);

    if (importType == ImportCheckType) {
        //KMimeType::Ptr fileMimeType = KMimeType::findByPath(filePath); //&&& disabled mime, used file-ext. instead
		/*
		if (fileMimeType->objectName() == "audio/x-midi")
			importType = ImportMIDI;
		else if (fileMimeType->objectName() == "audio/x-rosegarden")
			*/
		
		if( filePath.endsWith(".mid") || filePath.endsWith(".midi") )
				importType = ImportMIDI;
		else if ( filePath.endsWith(".rg") )
			importType = ImportRG4;
        else if (filePath.endsWith(".rose"))
            importType = ImportRG21;
        else if (filePath.endsWith(".h2song"))
            importType = ImportHydrogen;
    }


    switch (importType) {
    case ImportMIDI:
        doc = createDocumentFromMIDIFile(filePath);
        break;
    case ImportRG21:
        doc = createDocumentFromRG21File(filePath);
        break;
    case ImportHydrogen:
        doc = createDocumentFromHydrogenFile(filePath);
        break;
    default:
        doc = createDocumentFromRGFile(filePath);
    }

    slotEnableTransport(true);

    return doc;
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromRGFile(QString filePath)
{
    // Check for an autosaved file to recover
    QString effectiveFilePath = filePath;
    bool canRecover = false;

    //### implement this! (but not in rgTempQtIV) -- was in KApplication
    QString autoSaveFileName = rgTempQtIV->checkRecoverFile(filePath, canRecover);

    if (canRecover) {
        // First check if the auto-save file is more recent than the doc
        QFileInfo docFileInfo(filePath), autoSaveFileInfo(autoSaveFileName);

        if (docFileInfo.lastModified() < autoSaveFileInfo.lastModified()) {

            RG_DEBUG << "RosegardenGUIApp::openFile : "
            << "found a more recent autosave file\n";

            // At this point the splash screen may still be there, hide it if
            // it's the case
            KStartupLogo::hideIfStillThere();

            // It is, so ask the user if he wants to use the autosave file
            int reply = QMessageBox::question(this, "",
											  i18n("An auto-save file for this document has been found\nDo you want to open it instead ?"), QMessageBox::Yes | QMessageBox::No );

            if (reply == QMessageBox::Yes)
                // open the autosave file instead
                effectiveFilePath = autoSaveFileName;
            else {
                // user doesn't want the autosave, so delete it
                // so it won't bother us again if we reload
                canRecover = false;
                QFile::remove
                    (autoSaveFileName);
            }

        } else
            canRecover = false;
    }

    // Create a new blank document
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager,
                               true); // skipAutoload

    // ignore return thingy
    //
    if (newDoc->openDocument(effectiveFilePath)) {
        if (canRecover) {
            // Mark the document as modified,
            // set the "regular" filepath and name (not those of
            // the autosaved doc)
            //
            newDoc->slotDocumentModified();
            QFileInfo info(filePath);
            newDoc->setAbsFilePath(info.absFilePath());
            newDoc->setTitle(info.fileName());
        } else {
            newDoc->clearModifiedStatus();
        }
    } else {
        delete newDoc;
        return 0;
    }

    return newDoc;
}

void RosegardenGUIApp::slotSaveOptions()
{
    RG_DEBUG << "RosegardenGUIApp::slotSaveOptions()\n";

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 2 : transport flap extended = %1").arg(getTransport()->isExpanded()));
    _settingLog(QString("SETTING 2 : show track labels = %1").arg(m_viewTrackLabels->isChecked()));
#endif

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("Show Transport", m_viewTransport->isChecked());
    settings.setValue("Expanded Transport", m_transport ? getTransport()->isExpanded() : true);
    settings.setValue("Show Track labels", m_viewTrackLabels->isChecked());
    settings.setValue("Show Rulers", m_viewRulers->isChecked());
    settings.setValue("Show Tempo Ruler", m_viewTempoRuler->isChecked());
    settings.setValue("Show Chord Name Ruler", m_viewChordNameRuler->isChecked());
    settings.setValue("Show Previews", m_viewPreviews->isChecked());
    settings.setValue("Show Segment Labels", m_viewSegmentLabels->isChecked());
    settings.setValue("Show Parameters", m_dockVisible);
    settings.setValue("MIDI Thru Routing", m_enableMIDIrouting->isChecked());

#ifdef SETTING_LOG_DEBUG

    RG_DEBUG << "SHOW PARAMETERS = " << m_dockVisible << endl;
#endif

    //     saveMainWindowSettings(RosegardenGUIApp::MainWindowConfigGroup); - no need to, done by KMainWindow
    settings.sync();

    settings.endGroup();
}

void RosegardenGUIApp::setupFileDialogSpeedbar()
{
    QSettings settings;
    settings.beginGroup( "QFileDialog Speedbar" );

    RG_DEBUG << "RosegardenGUIApp::setupFileDialogSpeedbar" << endl;

    bool hasSetExamplesItem = qStrToBool( settings.value("Examples Set", "false" ) ) ;

    RG_DEBUG << "RosegardenGUIApp::setupFileDialogSpeedbar: examples set " << hasSetExamplesItem << endl;

    if (!hasSetExamplesItem) {

        unsigned int n = settings.value("Number of Entries", 0).toUInt() ;

        settings.setValue(QString("Description_%1").arg(n), i18n("Example Files"));
        settings.setValue(QString("IconGroup_%1").arg(n), 4);
        settings.setValue(QString("Icon_%1").arg(n), "folder");
        settings.setValue(QString("URL_%1").arg(n),
                           KGlobal::dirs()->findResource("appdata", "examples/"));

        RG_DEBUG << "wrote url " << settings.value(QString("URL_%1").arg(n)).toString() << endl;

        settings.setValue("Examples Set", true);
        settings.setValue("Number of Entries", n + 1);
        settings.sync();
    }

    settings.endGroup();
}

void RosegardenGUIApp::readOptions()
{
    QSettings settings;
//    applyMainWindowSettings(MainWindowConfigGroup);	//&&& not required

//    settings.reparseConfiguration();  //&&& may not be required: reparseConfig..()

    // Statusbar and toolbars toggling action status
    //
    m_viewStatusBar ->setChecked(!statusBar() ->isHidden());
    m_viewToolBar ->setChecked(!toolBar() ->isHidden());
    m_viewToolsToolBar ->setChecked(!toolBar("Tools Toolbar") ->isHidden());
    m_viewTracksToolBar ->setChecked(!toolBar("Tracks Toolbar") ->isHidden());
    m_viewEditorsToolBar ->setChecked(!toolBar("Editors Toolbar") ->isHidden());
    m_viewTransportToolBar->setChecked(!toolBar("Transport Toolbar")->isHidden());
    m_viewZoomToolBar ->setChecked(!toolBar("Zoom Toolbar") ->isHidden());

    bool opt;

    settings.beginGroup( GeneralOptionsConfigGroup );

    opt = qStrToBool( settings.value("Show Transport", "true" ) ) ;
    m_viewTransport->setChecked(opt);
    slotToggleTransport();

    opt = qStrToBool( settings.value("Expanded Transport", "true" ) ) ;

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 3 : transport flap extended = %1").arg(opt));
#endif

    if (opt)
        getTransport()->slotPanelOpenButtonClicked();
    else
        getTransport()->slotPanelCloseButtonClicked();

    opt = qStrToBool( settings.value("Show Track labels", "true" ) ) ;

#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 3 : show track labels = %1").arg(opt));
#endif

    m_viewTrackLabels->setChecked(opt);
    slotToggleTrackLabels();

    opt = qStrToBool( settings.value("Show Rulers", "true" ) ) ;
    m_viewRulers->setChecked(opt);
    slotToggleRulers();

    opt = qStrToBool( settings.value("Show Tempo Ruler", "true" ) ) ;
    m_viewTempoRuler->setChecked(opt);
    slotToggleTempoRuler();

    opt = qStrToBool( settings.value("Show Chord Name Ruler", "false" ) ) ;
    m_viewChordNameRuler->setChecked(opt);
    slotToggleChordNameRuler();

    opt = qStrToBool( settings.value("Show Previews", "true" ) ) ;
    m_viewPreviews->setChecked(opt);
    slotTogglePreviews();

    opt = qStrToBool( settings.value("Show Segment Labels", "true" ) ) ;
    m_viewSegmentLabels->setChecked(opt);
    slotToggleSegmentLabels();

    opt = qStrToBool( settings.value("Show Parameters", "true" ) ) ;
    if (!opt) {
        //m_dockLeft->undock();
    m_dockLeft->setFloating( true );
        //m_dockLeft->hide();
    m_dockLeft->setVisible( false );
    enterActionState("parametersbox_closed"); //@@@ JAS KXMLGUIClient::StateNoReverse);
    m_dockVisible = false;
    }

    // MIDI Thru routing
    opt = qStrToBool( settings.value("MIDI Thru Routing", "true" ) ) ;
    m_enableMIDIrouting->setChecked(opt);
    slotEnableMIDIThruRouting();

    settings.endGroup();

    m_actionsSetup = true;
}

void RosegardenGUIApp::saveGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    if (m_doc->getTitle() != i18n("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename = m_doc->getAbsFilePath();
        settings.setValue("filename", filename);
        settings.setValue("modified", m_doc->isModified());

        //### implement (was in KApplication?)
        QString tempname = rgTempQtIV->tempSaveName(filename);
        QString errMsg;
        bool res = m_doc->saveDocument(tempname, errMsg);
        if (!res) {
            if ( !errMsg.isEmpty() )
                QMessageBox::critical(this, "", i18n(qStrToCharPtrUtf8( QString("Could not save document at %1\nError was : %2").arg(tempname).arg(errMsg))) );
            else
                QMessageBox::critical(this, "", i18n( qStrToCharPtrUtf8( QString("Could not save document at %1").arg(tempname)))  );
        }
    }
}

void RosegardenGUIApp::readGlobalProperties()
{
    QSettings settings;
    //@@@ JAS Do we need a settings.startGroup() here?

    QString filename = settings.value("filename", "").toString();
    bool modified = qStrToBool( settings.value("modified", "false" ) ) ;

    if (modified) {
        bool canRecover;
        //### implement (was in KApplication)
        QString tempname = rgTempQtIV->checkRecoverFile(filename, canRecover);

        if (canRecover) {
            slotEnableTransport(false);
            m_doc->openDocument(tempname);
            slotEnableTransport(true);
            m_doc->slotDocumentModified();
            QFileInfo info(filename);
            m_doc->setAbsFilePath(info.absFilePath());
            m_doc->setTitle(info.fileName());
        }
    } else {
        if (!filename.isEmpty()) {
            slotEnableTransport(false);
            m_doc->openDocument(filename);
            slotEnableTransport(true);
        }
    }

	//QString caption = qApp->caption();
	QString caption = qApp->organizationName();
	setCaption(caption + ": " + m_doc->getTitle());
}

void RosegardenGUIApp::showEvent(QShowEvent* e)
{
    RG_DEBUG << "RosegardenGUIApp::showEvent()\n";

    getTransport()->raise();
    
	//KMainWindow::showEvent(e);  //&&& disabled. a debug function ?
	//QMainWindow::showEvent( e );
}

bool RosegardenGUIApp::queryClose()
{
    RG_DEBUG << "RosegardenGUIApp::queryClose" << endl;
#ifdef SETTING_LOG_DEBUG

    _settingLog(QString("SETTING 1 : transport flap extended = %1").arg(getTransport()->isExpanded()));
    _settingLog(QString("SETTING 1 : show track labels = %1").arg(m_viewTrackLabels->isChecked()));
#endif

    QString errMsg;

    bool canClose = m_doc->saveIfModified();

    /*
    if (canClose && m_transport) {

        // or else the closing of the transport will toggle off the 
        // 'view transport' action, and its state will be saved as 
        // 'off'
        //

        disconnect(m_transport, SIGNAL(closed()),
                   this, SLOT(slotCloseTransport()));
    }
    */

    return canClose;

}

bool RosegardenGUIApp::queryExit()
{
    RG_DEBUG << "RosegardenGUIApp::queryExit" << endl;
    if (m_actionsSetup)
        slotSaveOptions();

    return true;
}

void RosegardenGUIApp::slotFileNewWindow()
{
    KTmpStatusMsg msg(i18n("Opening a new application window..."), this);

    RosegardenGUIApp *new_window = new RosegardenGUIApp();
    new_window->show();
}

void RosegardenGUIApp::slotFileNew()
{
    RG_DEBUG << "RosegardenGUIApp::slotFileNew()\n";

    KTmpStatusMsg msg(i18n("Creating new document..."), this);

    bool makeNew = false;

    if (!m_doc->isModified()) {
        makeNew = true;
        // m_doc->closeDocument();
    } else if (m_doc->saveIfModified()) {
        makeNew = true;
    }

    if (makeNew) {

        setDocument(new RosegardenGUIDoc(this, m_pluginManager));
    }
}

void RosegardenGUIApp::slotOpenDroppedURL(QString url)
{
    ProgressDialog::processEvents(); // or else we get a crash because the
    // track editor is erased too soon - it is the originator of the signal
    // this slot is connected to.

    if (!m_doc->saveIfModified())
        return ;

    openURL(QUrl(url));
}

void RosegardenGUIApp::openURL(QString url)
{
    RG_DEBUG << "RosegardenGUIApp::openURL: QString " << url << endl;
    openURL(QUrl(url));
}

void RosegardenGUIApp::openURL(const QUrl& url)
{
    SetWaitCursor waitCursor;
	
    // related: http://doc.trolltech.com/4.3/qurl.html#FormattingOption-enum
    QString netFile = url.toString( QUrl::None );
	
    RG_DEBUG << "RosegardenGUIApp::openURL: QUrl " << netFile << endl;

    if (!url.isValid()) {
        QString string;
        string = i18n( "Malformed URL\n%1", netFile);

        /* was sorry */ QMessageBox::warning(this, "", string);
        return ;
    }

    QString target;
    QString caption( url.path() );

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    target = source.getLocalFilename();
    
    RG_DEBUG << "RosegardenGUIApp::openURL: target : " << target << endl;

    if (!m_doc->saveIfModified())
        return ;

    source.waitForData();
    openFile(target);

    setCaption(caption);
}

void RosegardenGUIApp::slotFileOpen()
{
    slotStatusHelpMsg(i18n("Opening file..."));

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    QString lastOpenedVersion = settings.value("Last File Opened Version", "none").toString();
    settings.endGroup();

    if (lastOpenedVersion != VERSION) {

        // We haven't opened any files with this version of the
        // program before.  Default to the examples directory.

        QString examplesDir = KGlobal::dirs()->findResource("appdata", "examples/");
        settings.beginGroup( "Recent Dirs" );

        QString recentString = settings.value("ROSEGARDEN", "").toString() ;
        settings.setValue
        ("ROSEGARDEN", QString("file:%1,%2").arg(examplesDir).arg(recentString));
    }

// old:   QUrl url = QFileDialog::getOpenURL
//                (":ROSEGARDEN",
//                 "audio/x-rosegarden audio/x-midi audio/x-rosegarden21", this,
//                 i18n("Open File"));
	// getOpenFileName ( QWidget * parent = 0, const QString & caption = QString(), const QString & dir = QString(), const QString & filter = QString(), QString * selectedFilter = 0, Options options = 0 )
		
		//@@@ check disabled and following code:
		QString fname = QFileDialog::getOpenFileName( this, i18n("Open File"), QDir::currentPath(), "*", 0, 0 ); 
		// last params: QString filter="*", QString* selectedFilter = 0, Options options = 0 )
		// options info: http://doc.trolltech.com/4.3/qfiledialog.html#Option-enum
	
		QUrl url( fname );
	
    if ( url.isEmpty() ) {
        return ;
    }

    if (m_doc && !m_doc->saveIfModified())
        return ;

    settings.beginGroup( GeneralOptionsConfigGroup );
    settings.setValue("Last File Opened Version", VERSION);

    openURL(url);

    settings.endGroup();
}

void RosegardenGUIApp::slotMerge()
{
    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open File"), QDir::currentPath(), "*", 0, 0 );
    if ( url.isEmpty() ) {
        return ;
    }


    QString target;

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()));
        return;
    }

    source.waitForData();
    target = source.getLocalFilename();

    mergeFile(target);
}

void RosegardenGUIApp::slotFileOpenRecent()
{
    QObject *obj = sender();
    QAction *action = dynamic_cast<QAction *>(obj);
    
    if (!action) {
	std::cerr << "WARNING: RosegardenGUIApp::slotFileOpenRecent: sender is not an action"
		  << std::endl;
	return;
    }

    QString path = action->text();
    if (path == "") return;

    KTmpStatusMsg msg(i18n("Opening file..."), this);

    if (m_doc) {
        if (!m_doc->saveIfModified()) {
            return ;
        }
    }

    openURL(path);
}

void RosegardenGUIApp::slotFileSave()
{
    if (!m_doc /*|| !m_doc->isModified()*/)
        return ; // ALWAYS save, even if doc is not modified.

    KTmpStatusMsg msg(i18n("Saving file..."), this);

    // if it's a new file (no file path), or an imported file
    // (file path doesn't end with .rg), call saveAs
    //
    if (!m_doc->isRegularDotRGFile()) {

        slotFileSaveAs();

    } else {

        SetWaitCursor waitCursor;
        QString errMsg, docFilePath = m_doc->getAbsFilePath();

        bool res = m_doc->saveDocument(docFilePath, errMsg);
        if (!res) {
            if ( ! errMsg.isEmpty() )
                QMessageBox::critical(this, "", i18n( qStrToCharPtrUtf8(QString("Could not save document at %1\nError was : %2")
                                              .arg(docFilePath).arg(errMsg))) );
            else
                QMessageBox::critical(this, "", i18n( qStrToCharPtrUtf8(QString("Could not save document at %1")
                                              .arg(docFilePath))) );
        }
    }
}

QString
RosegardenGUIApp::getValidWriteFile(QString descriptiveExtension,
                                    QString label)
{
    // extract first extension listed in descriptiveExtension, for instance,
    // ".rg" from "*.rg|Rosegarden files", or ".mid" from "*.mid *.midi|MIDI Files"
    //
    QString extension = descriptiveExtension.left(descriptiveExtension.find('|')).mid(1).section(' ', 0, 0);

    RG_DEBUG << "RosegardenGUIApp::getValidWriteFile() : extension = " << extension << endl;

    // It's too bad there isn't this functionality within
    // QFileDialog::getSaveFileName
	
// old:
	/*
//    QFileDialog saveFileDialog(":ROSEGARDEN", descriptiveExtension, this, label, true);
//    saveFileDialog.setOperationMode(QFileDialog::Saving);
	
	// old:
    if (m_doc) {
        QString saveFileName = m_doc->getAbsFilePath();
        // Show filename without the old extension
        int dotLoc = saveFileName.findRev('.');
        if (dotLoc >= int(saveFileName.length() - 4)) {
            saveFileName = saveFileName.left(dotLoc);
        }
        saveFileDialog.setSelection(saveFileName);
    }
    saveFileDialog.exec();
    QString name = saveFileDialog.selectedFile();
	*/
	// qt4:
	
	QString name = QFileDialog::getSaveFileName( this, i18n("Save File"), QDir::currentPath(), "*", 0, 0 ); 
	
    //     RG_DEBUG << "RosegardenGUIApp::getValidWriteFile() : QFileDialog::getSaveFileName returned "
    //              << name << endl;


    if (name.isEmpty())
        return name;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty()) {
        static QRegExp rgFile("\\..{1,4}$");
		if (rgFile.indexIn(name) == -1) {
            name += extension;
        }
    }

    QUrl *u = new QUrl(name);

    if (!u->isValid()) {
        /* was sorry */ QMessageBox::warning(this, "", i18n("This is not a valid filename.\n"));
        return "";
    }

    //if (!u->isLocalFile()) {
	if ( ! QFile::exists( u->toLocalFile() )){
        /* was sorry */ QMessageBox::warning(this, "",  i18n("This is not a local file.\n"));
        return "";
    }

    QFileInfo info(name);

    if (info.isDir()) {
        /* was sorry */ QMessageBox::warning(this, "", i18n("You have specified a directory"));
        return "";
    }

    if (info.exists()) {
        int overwrite = QMessageBox::question
				(this, "", i18n("The specified file exists.  Overwrite?"), 
				 QMessageBox::Yes | QMessageBox::No,
				 QMessageBox::No );

        if (overwrite != QMessageBox::Yes)
            return "";
    }

    return name;
}

bool RosegardenGUIApp::slotFileSaveAs()
{
    if (!m_doc)
        return false;

    KTmpStatusMsg msg(i18n("Saving file with a new filename..."), this);

    QString newName = getValidWriteFile("*.rg|" + i18n("Rosegarden files") +
                                        "\n*|" + i18n("All files"),
                                        i18n("Save as..."));
    if (newName.isEmpty())
        return false;

    SetWaitCursor waitCursor;
    QFileInfo saveAsInfo(newName);
    m_doc->setTitle(saveAsInfo.fileName());
    m_doc->setAbsFilePath(saveAsInfo.absFilePath());
    QString errMsg;
    bool res = m_doc->saveDocument(newName, errMsg);
    if (!res) {
        if ( !errMsg.isEmpty() )
            QMessageBox::critical(this, "", i18n(qStrToCharPtrUtf8(QString("Could not save document at %1\nError was : %2")
                                          .arg(newName).arg(errMsg))) );
        else
            QMessageBox::critical(this, "", i18n(qStrToCharPtrUtf8(QString("Could not save document at %1")
                                          .arg(newName))) );

    } else {

        m_recentFiles.add(newName);

        QString caption = qApp->applicationName();
        setCaption(caption + ": " + m_doc->getTitle());
        // update the edit view's captions too
        emit compositionStateUpdate();
    }

    return res;
}

void RosegardenGUIApp::slotFileClose()
{
    RG_DEBUG << "RosegardenGUIApp::slotFileClose()" << endl;

    if (!m_doc)
        return ;

    KTmpStatusMsg msg(i18n("Closing file..."), this);

    if (m_doc->saveIfModified()) {
        setDocument(new RosegardenGUIDoc(this, m_pluginManager));
    }

    // Don't close the whole view (i.e. Quit), just close the doc.
    //    close();
}

void RosegardenGUIApp::slotFilePrint()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        /* was sorry */ QMessageBox::warning(this, "", "Please create some tracks first (until we implement menu state management)");
        return ;
    }

    KTmpStatusMsg msg(i18n("Printing..."), this);

    m_view->print(&m_doc->getComposition());
}

void RosegardenGUIApp::slotFilePrintPreview()
{
    if (m_doc->getComposition().getNbSegments() == 0) {
        /* was sorry */ QMessageBox::warning(this, "", "Please create some tracks first (until we implement menu state management)");
        return ;
    }

    KTmpStatusMsg msg(i18n("Previewing..."), this);

    m_view->print(&m_doc->getComposition(), true);
}

void RosegardenGUIApp::slotQuit()
{
    slotStatusMsg(i18n("Exiting..."));

    Profiles::getInstance()->dump();

    // close the first window, the list makes the next one the first again.
    // This ensures that queryClose() is called on each window to ask for closing
	QMainWindow* w;
	QList<QMainWindow*> wl = this->findChildren<QMainWindow*>();
	for (int i=0; i < wl.size(); ++i) {
            // only close the window if the closeEvent is accepted. If
            // the user presses Cancel on the saveIfModified() dialog,
            // the window and the application stay open.
		w = wl.at(i);
		if ( ! w->close() )
			break;
	}
	//this->close();  //@@@ call this close() here too? 
	
	/*
	// old:
    QMainWindow* w;
    if (memberList) {

        for (w = memberList->first(); w != 0; w = memberList->next()) {
            // only close the window if the closeEvent is accepted. If
            // the user presses Cancel on the saveIfModified() dialog,
            // the window and the application stay open.
            if (!w->close())
                break;
        }
    }
	*/
}

void RosegardenGUIApp::slotEditCut()
{
    if (!m_view->haveSelection())
        return ;
    KTmpStatusMsg msg(i18n("Cutting selection..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand
    (new CutCommand(selection, m_clipboard));
}

void RosegardenGUIApp::slotEditCopy()
{
    if (!m_view->haveSelection())
        return ;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    SegmentSelection selection(m_view->getSelection());
    CommandHistory::getInstance()->addCommand
    (new CopyCommand(selection, m_clipboard));
}

void RosegardenGUIApp::slotEditPaste()
{
    if (m_clipboard->isEmpty()) {
        KTmpStatusMsg msg(i18n("Clipboard is empty"), this);
        return ;
    }
    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    // for now, but we could paste at the time of the first copied
    // segment and then do ghosting drag or something
    timeT insertionTime = m_doc->getComposition().getPosition();
    CommandHistory::getInstance()->addCommand
    (new PasteSegmentsCommand(&m_doc->getComposition(),
                              m_clipboard, insertionTime,
                              m_doc->getComposition().getSelectedTrack(),
                              false));

    // User preference? Update song pointer position on paste
    m_doc->slotSetPointerPosition(m_doc->getComposition().getPosition());
}

void RosegardenGUIApp::slotCutRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new CutRangeCommand(&m_doc->getComposition(), t0, t1, m_clipboard));
}

void RosegardenGUIApp::slotCopyRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new CopyCommand(&m_doc->getComposition(), t0, t1, m_clipboard));
}

void RosegardenGUIApp::slotPasteRange()
{
    if (m_clipboard->isEmpty())
        return ;

    CommandHistory::getInstance()->addCommand
    (new PasteRangeCommand(&m_doc->getComposition(), m_clipboard,
                           m_doc->getComposition().getPosition()));

    m_doc->setLoop(0, 0);
}

void RosegardenGUIApp::slotDeleteRange()
{
    timeT t0 = m_doc->getComposition().getLoopStart();
    timeT t1 = m_doc->getComposition().getLoopEnd();

    if (t0 == t1)
        return ;

    CommandHistory::getInstance()->addCommand
    (new DeleteRangeCommand(&m_doc->getComposition(), t0, t1));

    m_doc->setLoop(0, 0);
}

void RosegardenGUIApp::slotInsertRange()
{
    timeT t0 = m_doc->getComposition().getPosition();
    std::pair<timeT, timeT> r = m_doc->getComposition().getBarRangeForTime(t0);
    TimeDialog dialog(m_view, i18n("Duration of empty range to insert"),
                      &m_doc->getComposition(), t0, r.second - r.first, false);
    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new InsertRangeCommand(&m_doc->getComposition(), t0, dialog.getTime()));
        m_doc->setLoop(0, 0);
    }
}

void RosegardenGUIApp::slotSelectAll()
{
    m_view->slotSelectAllSegments();
}

void RosegardenGUIApp::slotDeleteSelectedSegments()
{
    m_view->getTrackEditor()->slotDeleteSelectedSegments();
}

void RosegardenGUIApp::slotQuantizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview

    QuantizeDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (EventQuantizeCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
                             dialog.getQuantizer()));
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotRepeatQuantizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (EventQuantizeCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        command->addCommand(new EventQuantizeCommand
                            (**i, (*i)->getStartTime(), (*i)->getEndTime(),
                             "Quantize Dialog Grid", false)); // no i18n (config group name)
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotGrooveQuantize()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    if (selection.size() != 1) {
        /* was sorry */ QMessageBox::warning(this, "", i18n("This function needs no more than one segment to be selected."));
        return ;
    }

    Segment *s = *selection.begin();
    m_view->slotAddCommandToHistory(new CreateTempoMapFromSegmentCommand(s));
}

void RosegardenGUIApp::slotJoinSegments()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    SegmentSelection selection = m_view->getSelection();
    if (selection.size() == 0)
        return ;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((*i)->getType() != Segment::Internal) {
            /* was sorry */ QMessageBox::warning(this, "", i18n("Can't join Audio segments"));
            return ;
        }
    }

    m_view->slotAddCommandToHistory(new SegmentJoinCommand(selection));
    m_view->updateSelectionContents();
}

void RosegardenGUIApp::slotRescaleSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! should it?

    SegmentSelection selection = m_view->getSelection();

    timeT startTime = 0, endTime = 0;
    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((i == selection.begin()) || ((*i)->getStartTime() < startTime)) {
            startTime = (*i)->getStartTime();
        }
        if ((i == selection.begin()) || ((*i)->getEndMarkerTime() > endTime)) {
            endTime = (*i)->getEndMarkerTime();
        }
    }

    RescaleDialog dialog(m_view, &m_doc->getComposition(),
                         startTime, endTime - startTime,
                         false, false);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    std::vector<AudioSegmentRescaleCommand *> asrcs;

    int mult = dialog.getNewDuration();
    int div = endTime - startTime;
    float ratio = float(mult) / float(div);

    std::cerr << "slotRescaleSelection: mult = " << mult << ", div = " << div << ", ratio = " << ratio << std::endl;

    MacroCommand *command = new MacroCommand
                             (SegmentRescaleCommand::getGlobalName());

    bool pathTested = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if ((*i)->getType() == Segment::Audio) {
            if (!pathTested) {
	        testAudioPath(i18n("rescaling an audio file"));
                pathTested = true;
            }
            AudioSegmentRescaleCommand *asrc = new AudioSegmentRescaleCommand
                (m_doc, *i, ratio);
            command->addCommand(asrc);
            asrcs.push_back(asrc);
        } else {
            command->addCommand(new SegmentRescaleCommand(*i, mult, div));
        }
    }
	
	
	ProgressDialog *progressDlg = 0; //@@@ should this be QProgressDialog ??

    if (!asrcs.empty()) {
        progressDlg = new ProgressDialog
            (i18n("Rescaling audio file..."), 100, this);
        progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(false);
        progressDlg->show();
        for (size_t i = 0; i < asrcs.size(); ++i) {
            asrcs[i]->connectProgressDialog(progressDlg);
        }
    }

    m_view->slotAddCommandToHistory(command);

    if (!asrcs.empty()) {

        progressDlg->setLabelText(i18n("Generating audio preview..."));

//        for (size_t i = 0; i < asrcs.size(); ++i) {
//            asrcs[i]->disconnectProgressDialog(progressDlg);	//&&& obsolete (?)
//        }

        connect(&m_doc->getAudioFileManager(), SIGNAL(setValue(int)),
				 m_progressBar, SLOT(setValue(int)));	// was progressDlg->progressBar()
        connect(progressDlg, SIGNAL(cancelClicked()),
                &m_doc->getAudioFileManager(), SLOT(slotStopPreview()));

        for (size_t i = 0; i < asrcs.size(); ++i) {
            int fid = asrcs[i]->getNewAudioFileId();
            if (fid >= 0) {
                slotAddAudioFile(fid);
                m_doc->getAudioFileManager().generatePreview(fid);
            }
        }
    }

    if (progressDlg) delete progressDlg;
}

bool
RosegardenGUIApp::testAudioPath(QString op)
{
    try {
        m_doc->getAudioFileManager().testAudioPath();
    } catch (AudioFileManager::BadAudioPathException) {
        if ( QMessageBox::warning
				(this, i18n("Warning"),
	     i18n("The audio file path does not exist or is not writable.\nYou must set the audio file path to a valid directory in Document Properties before %1.\nWould you like to set it now?", op ),
				QMessageBox::Ok | QMessageBox::Cancel,
	 			QMessageBox::Cancel 
				) == QMessageBox::Ok 
		   ){
            slotOpenAudioPathSettings();
        }
	return false;
    }
    return true;
}

void RosegardenGUIApp::slotAutoSplitSelection()
{
    if (!m_view->haveSelection())
        return ;

    //!!! this should all be in rosegardenguiview
    //!!! or should it?

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentAutoSplitCommand::getGlobalName());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
            AudioSplitDialog aSD(this, (*i), m_doc);

            if (aSD.exec() == QDialog::Accepted) {
                // split to threshold
                //
                command->addCommand(
                    new AudioSegmentAutoSplitCommand(m_doc,
                                                     *i,
                                                     aSD.getThreshold()));
                // dmm - verifying that widget->value() accessors *can* work without crashing
                //		std::cout << "SILVAN: getThreshold() = " << aSD.getThreshold() << std::endl;
            }
        } else {
            command->addCommand(new SegmentAutoSplitCommand(*i));
        }
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::slotJogLeft()
{
    RG_DEBUG << "RosegardenGUIApp::slotJogLeft" << endl;
    jogSelection( -Note(Note::Demisemiquaver).getDuration());
}

void RosegardenGUIApp::slotJogRight()
{
    RG_DEBUG << "RosegardenGUIApp::slotJogRight" << endl;
    jogSelection(Note(Note::Demisemiquaver).getDuration());
}

void RosegardenGUIApp::jogSelection(timeT amount)
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    SegmentReconfigureCommand *command = new SegmentReconfigureCommand(i18n("Jog Selection"));

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        command->addSegment((*i),
                            (*i)->getStartTime() + amount,
                            (*i)->getEndMarkerTime() + amount,
                            (*i)->getTrack());
    }

    m_view->slotAddCommandToHistory(command);
}

void RosegardenGUIApp::createAndSetupTransport()
{
    // create the Transport GUI and add the callbacks to the
    // buttons and keyboard shortcuts
    //
    m_transport = new TransportDialog(this);
	
//
//    plugShortcuts(m_transport, m_transport->getShortcuts());
/*
    m_transport->getShortcuts()->connectItem
        (m_transport->getShortcuts()->addItem(Qt::Key_T),
         this,
         SLOT(slotHideTransport()));
*/	
	
	// new qt4: 
	QWidget* sc_parent = this;
	QShortcut* sc_tmp;
	
	// types:Qt::WidgetShortcut, Qt::ApplicationShortcut, Qt::WindowShortcut
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_T), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(activated()), this, SLOT(slotHideTransport()) );
	
	
    // Ensure that the checkbox is unchecked if the dialog
    // is closed
    connect(m_transport, SIGNAL(closed()),
            SLOT(slotCloseTransport()));

    // Handle loop setting and unsetting from the transport loop button
    //

    connect(m_transport, SIGNAL(setLoop()), SLOT(slotSetLoop()));
    connect(m_transport, SIGNAL(unsetLoop()), SLOT(slotUnsetLoop()));
    connect(m_transport, SIGNAL(panic()), SLOT(slotPanic()));

    connect(m_transport, SIGNAL(editTempo(QWidget*)),
            SLOT(slotEditTempo(QWidget*)));

    connect(m_transport, SIGNAL(editTimeSignature(QWidget*)),
            SLOT(slotEditTimeSignature(QWidget*)));

    connect(m_transport, SIGNAL(editTransportTime(QWidget*)),
            SLOT(slotEditTransportTime(QWidget*)));

    // Handle set loop start/stop time buttons.
    //
    connect(m_transport, SIGNAL(setLoopStartTime()), SLOT(slotSetLoopStart()));
    connect(m_transport, SIGNAL(setLoopStopTime()), SLOT(slotSetLoopStop()));

    if (m_seqManager != 0)
        m_seqManager->setTransport(m_transport);

}

void RosegardenGUIApp::slotSplitSelectionByPitch()
{
    if (!m_view->haveSelection())
        return ;

    SplitByPitchDialog dialog(m_view);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentSplitByPitchCommand::getGlobalName());

    bool haveSomething = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
            // nothing
        } else {
            command->addCommand
            (new SegmentSplitByPitchCommand
             (*i,
              dialog.getPitch(),
              dialog.getShouldRange(),
              dialog.getShouldDuplicateNonNoteEvents(),
              (SegmentSplitByPitchCommand::ClefHandling)
              dialog.getClefHandling()));
            haveSomething = true;
        }
    }

    if (haveSomething)
        m_view->slotAddCommandToHistory(command);
    //!!! else complain
}

void
RosegardenGUIApp::slotSplitSelectionByRecordedSrc()
{
    if (!m_view->haveSelection())
        return ;

    SplitByRecordingSrcDialog dialog(m_view, m_doc);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    SegmentSelection selection = m_view->getSelection();

    MacroCommand *command = new MacroCommand
                             (SegmentSplitByRecordingSrcCommand::getGlobalName());

    bool haveSomething = false;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {

        if ((*i)->getType() == Segment::Audio) {
            // nothing
        } else {
            command->addCommand
            (new SegmentSplitByRecordingSrcCommand(*i,
                                                   dialog.getChannel(),
                                                   dialog.getDevice()));
            haveSomething = true;
        }
    }
    if (haveSomething)
        m_view->slotAddCommandToHistory(command);
}

void
RosegardenGUIApp::slotSplitSelectionAtTime()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT now = m_doc->getComposition().getPosition();

    QString title = i18np("Split Segment at Time",
                         "Split %1 Segments at Time",
                         selection.size());

    TimeDialog dialog(m_view, title,
                      &m_doc->getComposition(),
                      now, true);

    MacroCommand *command = new MacroCommand( title );

    if (dialog.exec() == QDialog::Accepted) {
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            if ((*i)->getType() == Segment::Audio) {
                command->addCommand(new AudioSegmentSplitCommand(*i, dialog.getTime()));
            } else {
                command->addCommand(new SegmentSplitCommand(*i, dialog.getTime()));
            }
        }
        m_view->slotAddCommandToHistory(command);
    }
}

void
RosegardenGUIApp::slotSetSegmentStartTimes()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT someTime = (*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, i18n("Segment Start Time"),
                      &m_doc->getComposition(),
                      someTime, false);

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                          i18n("Set Segment Start Times") :
                                          i18n("Set Segment Start Time"));

        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            command->addSegment
            (*i, dialog.getTime(),
             (*i)->getEndMarkerTime() - (*i)->getStartTime() + dialog.getTime(),
             (*i)->getTrack());
        }

        m_view->slotAddCommandToHistory(command);
    }
}

void
RosegardenGUIApp::slotSetSegmentDurations()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    if (selection.empty())
        return ;

    timeT someTime =
        (*selection.begin())->getStartTime();

    timeT someDuration =
        (*selection.begin())->getEndMarkerTime() -
        (*selection.begin())->getStartTime();

    TimeDialog dialog(m_view, i18n("Segment Duration"),
                      &m_doc->getComposition(),
                      someTime,
                      someDuration,
                      false);

    if (dialog.exec() == QDialog::Accepted) {

        bool plural = (selection.size() > 1);

        SegmentReconfigureCommand *command =
            new SegmentReconfigureCommand(plural ?
                                          i18n("Set Segment Durations") :
                                          i18n("Set Segment Duration"));

        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {

            command->addSegment
            (*i, (*i)->getStartTime(),
             (*i)->getStartTime() + dialog.getTime(),
             (*i)->getTrack());
        }

        m_view->slotAddCommandToHistory(command);
    }
}

void RosegardenGUIApp::slotHarmonizeSelection()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();
    //!!! This should be somewhere else too

    CompositionTimeSliceAdapter adapter(&m_doc->getComposition(),
                                        &selection);

    AnalysisHelper helper;
    Segment *segment = new Segment;
    helper.guessHarmonies(adapter, *segment);

    //!!! do nothing with the results yet
    delete segment;
}

void RosegardenGUIApp::slotTempoToSegmentLength()
{
    slotTempoToSegmentLength(this);
}

void RosegardenGUIApp::slotTempoToSegmentLength(QWidget* parent)
{
    RG_DEBUG << "RosegardenGUIApp::slotTempoToSegmentLength" << endl;

    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection = m_view->getSelection();

    // Only set for a single selection
    //
    if (selection.size() == 1 &&
            (*selection.begin())->getType() == Segment::Audio) {
        Composition &comp = m_doc->getComposition();
        Segment *seg = *selection.begin();

        TimeSignature timeSig =
            comp.getTimeSignatureAt( seg->getStartTime());

        timeT endTime = seg->getEndTime();

        if (seg->getRawEndMarkerTime())
            endTime = seg->getEndMarkerTime();

        RealTime segDuration =
            seg->getAudioEndTime() - seg->getAudioStartTime();

        int beats = 0;

        // Get user to tell us how many beats or bars the segment contains
        BeatsBarsDialog dialog(parent);
        if (dialog.exec() == QDialog::Accepted) {
            beats = dialog.getQuantity(); // beats (or bars)
            if (dialog.getMode() == 1)    // bars  (multiply by time sig)
                beats *= timeSig.getBeatsPerBar();
#ifdef DEBUG_TEMPO_FROM_AUDIO

            RG_DEBUG << "RosegardenGUIApp::slotTempoToSegmentLength - beats = " << beats
            << " mode = " << ((dialog.getMode() == 0) ? "bars" : "beats") << endl
            << " beats per bar = " << timeSig.getBeatsPerBar()
            << " user quantity = " << dialog.getQuantity()
            << " user mode = " << dialog.getMode() << endl;
#endif

        } else {
            RG_DEBUG << "RosegardenGUIApp::slotTempoToSegmentLength - BeatsBarsDialog aborted"
            << endl;
            return ;
        }

        double beatLengthUsec =
            double(segDuration.sec * 1000000 + segDuration.usec()) /
            double(beats);

        // New tempo is a minute divided by time of beat
        // converted up (#1414252) to a sane value via getTempoFoQpm()
        //
        tempoT newTempo =
            comp.getTempoForQpm(60.0 * 1000000.0 / beatLengthUsec);

#ifdef DEBUG_TEMPO_FROM_AUDIO

        RG_DEBUG << "RosegardenGUIApp::slotTempoToSegmentLength info: " << endl
        << " beatLengthUsec   = " << beatLengthUsec << endl
        << " segDuration.usec = " << segDuration.usec() << endl
        << " newTempo         = " << newTempo << endl;
#endif

        MacroCommand *macro = new MacroCommand(i18n("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < comp.getTempoChangeCount(); i++)
            macro->addCommand(new RemoveTempoChangeCommand(&comp,
                              (comp.getTempoChangeCount() - 1 - i)));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(&comp, 0, newTempo));

        // execute
        CommandHistory::getInstance()->addCommand(macro);
    }
}

void RosegardenGUIApp::slotToggleSegmentLabels()
{
    /* was toggle */ 
	//QAction* act = dynamic_cast<QAction*>(actionCollection()->action("show_segment_labels"));
	QAction* act = this->findChild<QAction*>("show_segment_labels");
	
	if (act) {
        m_view->slotShowSegmentLabels(act->isChecked());
    }
}

void RosegardenGUIApp::slotEdit()
{
    m_view->slotEditSegment(0);
}

void RosegardenGUIApp::slotEditAsNotation()
{
    m_view->slotEditSegmentNotation(0);
}

void RosegardenGUIApp::slotEditInMatrix()
{
    m_view->slotEditSegmentMatrix(0);
}

void RosegardenGUIApp::slotEditInPercussionMatrix()
{
    m_view->slotEditSegmentPercussionMatrix(0);
}

void RosegardenGUIApp::slotEditInEventList()
{
    m_view->slotEditSegmentEventList(0);
}

void RosegardenGUIApp::slotEditTempos()
{
    slotEditTempos(m_doc->getComposition().getPosition());
}

void RosegardenGUIApp::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), this);

    if (m_viewToolBar->isChecked())
        toolBar("mainToolBar")->show();
    else
        toolBar("mainToolBar")->hide();
}

void RosegardenGUIApp::slotToggleToolsToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tools toolbar..."), this);

    if (m_viewToolsToolBar->isChecked())
        toolBar("Tools Toolbar")->show();
    else
        toolBar("Tools Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTracksToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the tracks toolbar..."), this);

    if (m_viewTracksToolBar->isChecked())
        toolBar("Tracks Toolbar")->show();
    else
        toolBar("Tracks Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleEditorsToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the editor toolbar..."), this);

    if (m_viewEditorsToolBar->isChecked())
        toolBar("Editors Toolbar")->show();
    else
        toolBar("Editors Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTransportToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the transport toolbar..."), this);

    if (m_viewTransportToolBar->isChecked())
        toolBar("Transport Toolbar")->show();
    else
        toolBar("Transport Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleZoomToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the zoom toolbar..."), this);

    if (m_viewZoomToolBar->isChecked())
        toolBar("Zoom Toolbar")->show();
    else
        toolBar("Zoom Toolbar")->hide();
}

void RosegardenGUIApp::slotToggleTransport()
{
    KTmpStatusMsg msg(i18n("Toggle the Transport"), this);

    if (m_viewTransport->isChecked()) {
        getTransport()->show();
        getTransport()->raise();
        getTransport()->blockSignals(false);
    } else {
        getTransport()->hide();
        getTransport()->blockSignals(true);
    }
}

void RosegardenGUIApp::slotHideTransport()
{
    if (m_viewTransport->isChecked()) {
        m_viewTransport->blockSignals(true);
        m_viewTransport->setChecked(false);
        m_viewTransport->blockSignals(false);
    }
    getTransport()->hide();
    getTransport()->blockSignals(true);
}        

void RosegardenGUIApp::slotToggleTrackLabels()
{
    if (m_viewTrackLabels->isChecked()) {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels on");
#endif

        m_view->getTrackEditor()->getTrackButtons()->
        changeTrackInstrumentLabels(TrackLabel::ShowTrack);
    } else {
#ifdef SETTING_LOG_DEBUG
        _settingLog("toggle track labels off");
#endif

        m_view->getTrackEditor()->getTrackButtons()->
        changeTrackInstrumentLabels(TrackLabel::ShowInstrument);
    }
}

void RosegardenGUIApp::slotToggleRulers()
{
    m_view->slotShowRulers(m_viewRulers->isChecked());
}

void RosegardenGUIApp::slotToggleTempoRuler()
{
    m_view->slotShowTempoRuler(m_viewTempoRuler->isChecked());
}

void RosegardenGUIApp::slotToggleChordNameRuler()
{
    m_view->slotShowChordNameRuler(m_viewChordNameRuler->isChecked());
}

void RosegardenGUIApp::slotTogglePreviews()
{
    m_view->slotShowPreviews(m_viewPreviews->isChecked());
}

void RosegardenGUIApp::slotDockParametersBack()
{
//    m_dockLeft->dockBack();
	m_dockLeft->setFloating(false);
	m_dockLeft->setVisible(true);
}

void RosegardenGUIApp::slotParametersClosed()
{
    enterActionState("parametersbox_closed"); //@@@ JAS orig. 0
    m_dockVisible = false;
}

void RosegardenGUIApp::slotParametersDockedBack(QDockWidget* dw, int ) //qt4: Qt::DockWidgetAreas //qt3 was: QDockWidget::DockPosition)
{
    if (dw == m_dockLeft) {
	leaveActionState("parametersbox_closed"); //@@@ JAS KXMLGUIClient::StateReverse
        m_dockVisible = true;
    }
}

void RosegardenGUIApp::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), this);

    if (!m_viewStatusBar->isChecked())
        statusBar()->hide();
    else
        statusBar()->show();
}

void RosegardenGUIApp::slotStatusMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clearMessage();
//	statusBar()->changeItem(text, EditViewBase::ID_STATUS_MSG);
//	statusBar()->showMessage(text, EditViewBase::ID_STATUS_MSG );
	statusBar()->showMessage(text, 0 );	// note: last param == timeout
}

void RosegardenGUIApp::slotStatusHelpMsg(QString text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

void RosegardenGUIApp::slotEnableTransport(bool enable)
{
    if (m_transport)
        getTransport()->setEnabled(enable);
}

void RosegardenGUIApp::slotPointerSelected()
{
    m_view->selectTool(SegmentSelector::ToolName);
}

void RosegardenGUIApp::slotEraseSelected()
{
    m_view->selectTool(SegmentEraser::ToolName);
}

void RosegardenGUIApp::slotDrawSelected()
{
    m_view->selectTool(SegmentPencil::ToolName);
}

void RosegardenGUIApp::slotMoveSelected()
{
    m_view->selectTool(SegmentMover::ToolName);
}

void RosegardenGUIApp::slotResizeSelected()
{
    m_view->selectTool(SegmentResizer::ToolName);
}

void RosegardenGUIApp::slotJoinSelected()
{
    QMessageBox::information(this,
                             i18n("The join tool isn't implemented yet.  Instead please highlight "
                                  "the segments you want to join and then use the menu option:\n\n"
                                  "        Segments->Collapse Segments.\n"),
                             i18n("Join tool not yet implemented"));

    m_view->selectTool(SegmentJoiner::ToolName);
}

void RosegardenGUIApp::slotSplitSelected()
{
    m_view->selectTool(SegmentSplitter::ToolName);
}

void RosegardenGUIApp::slotAddTrack()
{
    if (!m_view)
        return ;

    // default to the base number - might not actually exist though
    //
    InstrumentId id = MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (DeviceList::iterator it = devices->begin();
            it != devices->end() && !have; it++) {

        if ((*it)->getType() != Device::Midi)
            continue;

        InstrumentList instruments = (*it)->getAllInstruments();
        for (InstrumentList::iterator iit = instruments.begin();
                iit != instruments.end(); iit++) {

            if ((*iit)->getId() >= MidiInstrumentBase) {
                id = (*iit)->getId();
                have = true;
                break;
            }
        }
    }

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    int pos = -1;
    if (track) pos = track->getPosition() + 1;

    m_view->slotAddTracks(1, id, pos);
}

void RosegardenGUIApp::slotAddTracks()
{
    if (!m_view)
        return ;

    // default to the base number - might not actually exist though
    //
    InstrumentId id = MidiInstrumentBase;

    // Get the first Internal/MIDI instrument
    //
    DeviceList *devices = m_doc->getStudio().getDevices();
    bool have = false;

    for (DeviceList::iterator it = devices->begin();
            it != devices->end() && !have; it++) {

        if ((*it)->getType() != Device::Midi)
            continue;

        InstrumentList instruments = (*it)->getAllInstruments();
        for (InstrumentList::iterator iit = instruments.begin();
                iit != instruments.end(); iit++) {

            if ((*iit)->getId() >= MidiInstrumentBase) {
                id = (*iit)->getId();
                have = true;
                break;
            }
        }
    }

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    int pos = 0;
    if (track) pos = track->getPosition();

    bool ok = false;

    AddTracksDialog dialog(this, pos);

    if (dialog.exec() == QDialog::Accepted) {
        m_view->slotAddTracks(dialog.getTracks(), id, 
                              dialog.getInsertPosition());
    }
}

void RosegardenGUIApp::slotDeleteTrack()
{
    if (!m_view)
        return ;

    Composition &comp = m_doc->getComposition();
    TrackId trackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(trackId);

    RG_DEBUG << "RosegardenGUIApp::slotDeleteTrack() : about to delete track id "
    << trackId << endl;

    if (track == 0)
        return ;

    // Always have at least one track in a composition
    //
    if (comp.getNbTracks() == 1)
        return ;

    // VLADA
    if (m_view->haveSelection()) {

        SegmentSelection selection = m_view->getSelection();
        m_view->slotSelectTrackSegments(trackId);
        m_view->getTrackEditor()->slotDeleteSelectedSegments();
        m_view->slotPropagateSegmentSelection(selection);

    } else {

        m_view->slotSelectTrackSegments(trackId);
        m_view->getTrackEditor()->slotDeleteSelectedSegments();
    }
    //VLADA

    int position = track->getPosition();

    // Delete the track
    //
    std::vector<TrackId> tracks;
    tracks.push_back(trackId);

    m_view->slotDeleteTracks(tracks);

    // Select a new valid track
    //
    if (comp.getTrackByPosition(position))
        trackId = comp.getTrackByPosition(position)->getId();
    else if (comp.getTrackByPosition(position - 1))
        trackId = comp.getTrackByPosition(position - 1)->getId();
    else {
        RG_DEBUG << "RosegardenGUIApp::slotDeleteTrack - "
        << "can't select a highlighted track after delete"
        << endl;
    }

    comp.setSelectedTrack(trackId);

    Instrument *inst = m_doc->getStudio().
                       getInstrumentById(comp.getTrackById(trackId)->getInstrument());

    //VLADA
    //    m_view->slotSelectTrackSegments(trackId);
    //VLADA
}

void RosegardenGUIApp::slotMoveTrackDown()
{
    RG_DEBUG << "RosegardenGUIApp::slotMoveTrackDown" << endl;

    Composition &comp = m_doc->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() + 1);

    if (destTrack == 0)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());

}

void RosegardenGUIApp::slotMoveTrackUp()
{
    RG_DEBUG << "RosegardenGUIApp::slotMoveTrackUp" << endl;

    Composition &comp = m_doc->getComposition();
    Track *srcTrack = comp.getTrackById(comp.getSelectedTrack());

    // Check for track object
    //
    if (srcTrack == 0)
        return ;

    // Check we're not at the top already
    //
    if (srcTrack->getPosition() == 0)
        return ;

    // Check destination track exists
    //
    Track *destTrack =
        comp.getTrackByPosition(srcTrack->getPosition() - 1);

    if (destTrack == 0)
        return ;

    MoveTracksCommand *command =
        new MoveTracksCommand(&comp, srcTrack->getId(), destTrack->getId());

    CommandHistory::getInstance()->addCommand(command);

    // make sure we're showing the right selection
    m_view->slotSelectTrackSegments(comp.getSelectedTrack());
}

void RosegardenGUIApp::slotRevertToSaved()
{
    RG_DEBUG << "RosegardenGUIApp::slotRevertToSaved" << endl;

    if (m_doc->isModified()) {
        int revert =
            QMessageBox::question(this, "", 
                                       i18n("Revert modified document to previous saved version?"));

        if (revert == QMessageBox::No)
            return ;

        openFile(m_doc->getAbsFilePath());
    }
}

void RosegardenGUIApp::slotImportProject()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName( this, i18n("Import Rosegarden Project File"), QDir::currentPath(), "*", 0, 0 );

    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }
    source.waitForData();
    tmpfile = source.getLocalFilename();

    importProject(tmpfile);
}

void RosegardenGUIApp::importProject(QString filePath)
{
    //setup "rosegarden-project-package" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--unpack";
    procArgs << filePath;

    KStartupLogo::hideIfStillThere();
    proc->execute("rosegarden-project-package", procArgs);

    if ((proc->exitStatus() != QProcess::NormalExit) || proc->exitCode()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, filePath, i18n("Failed to import project file \"%1\""));
        CurrentProgressDialog::thaw();
        delete proc;
        return ;
    }

    delete proc;

    QString rgFile = filePath;
    rgFile.replace(QRegExp(".rg.rgp$"), ".rg");
    rgFile.replace(QRegExp(".rgp$"), ".rg");
    openURL(rgFile);
}

void RosegardenGUIApp::slotImportMIDI()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open MIDI File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    source.waitForData();
    tmpfile = source.getLocalFilename();

    openFile(tmpfile, ImportMIDI); // does everything including setting the document
}

void RosegardenGUIApp::slotMergeMIDI()
{
    QUrl url = QFileDialog::getOpenFileName( this, i18n("Merge MIDI File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportMIDI);
}

QTextCodec *
RosegardenGUIApp::guessTextCodec(std::string text)
{
    QTextCodec *codec = 0;

    for (int c = 0; c < text.length(); ++c) {
        if (text[c] & 0x80) {

            CurrentProgressDialog::freeze();
            KStartupLogo::hideIfStillThere();

            IdentifyTextCodecDialog dialog(0, text);
            dialog.exec();

            std::string codecName = dialog.getCodec();

            CurrentProgressDialog::thaw();

            if (codecName != "") {
                codec = QTextCodec::codecForName(codecName.c_str());
            }
            break;
        }
    }

    return codec;
}

void
RosegardenGUIApp::fixTextEncodings(Composition *c)

{
    QTextCodec *codec = 0;

    for (Composition::iterator i = c->begin();
            i != c->end(); ++i) {

        for (Segment::iterator j = (*i)->begin();
                j != (*i)->end(); ++j) {

            if ((*j)->isa(Text::EventType)) {

                std::string text;

                if ((*j)->get
                        <String>
                        (Text::TextPropertyName, text)) {

                    if (!codec)
                        codec = guessTextCodec(text);

                    if (codec) {
                        (*j)->set
                        <String>
                        (Text::TextPropertyName,
                         convertFromCodec(text, codec));
                    }
                }
            }
        }
    }

    if (!codec)
        codec = guessTextCodec(c->getCopyrightNote());
    if (codec)
        c->setCopyrightNote(convertFromCodec(c->getCopyrightNote(), codec));

    for (Composition::trackcontainer::iterator i =
                c->getTracks().begin(); i != c->getTracks().end(); ++i) {
        if (!codec)
            codec = guessTextCodec(i->second->getLabel());
        if (codec)
            i->second->setLabel(convertFromCodec(i->second->getLabel(), codec));
    }

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
        if (!codec)
            codec = guessTextCodec((*i)->getLabel());
        if (codec)
            (*i)->setLabel(convertFromCodec((*i)->getLabel(), codec));
    }
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromMIDIFile(QString file)
{
    //if (!merge && !m_doc->saveIfModified()) return;

    // Create new document (autoload is inherent)
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    std::string fname(QFile::encodeName(file));

    MidiFile midiFile(fname,
                      &newDoc->getStudio());

    KStartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(i18n("Importing MIDI file..."),
                               200,
                               this);

    CurrentProgressDialog::set
        (&progressDlg);

    connect(&midiFile, SIGNAL(setValue(int)),
            m_progressBar, SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
			 m_progressBar, SLOT(advance(int)));

    if (!midiFile.open()) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "", strtoqstr(midiFile.getError()) ); //!!! i18n
        delete newDoc;
        return 0;
    }

    midiFile.convertToRosegarden(newDoc->getComposition(),
                                 MidiFile::CONVERT_REPLACE);

    fixTextEncodings(&newDoc->getComposition());

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    // Clean up for notation purposes (after reinitialise, because that
    // sets the composition's end marker time which is needed here)

    progressDlg.slotSetOperationName(i18n("Calculating notation..."));
    ProgressDialog::processEvents();

    Composition *comp = &newDoc->getComposition();

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        Segment &segment = **i;
        SegmentNotationHelper helper(segment);
        segment.insert(helper.guessClef(segment.begin(),
                                        segment.getEndMarker()).getAsEvent
                       (segment.getStartTime()));
    }

    //was: progressDlg.progressBar()
	m_progressBar->setValue(100);

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        // find first key event in each segment (we'd have done the
        // same for clefs, except there is no MIDI clef event)

        Segment &segment = **i;
        timeT firstKeyTime = segment.getEndMarkerTime();

        for (Segment::iterator si = segment.begin();
                segment.isBeforeEndMarker(si); ++si) {
            if ((*si)->isa(Rosegarden::Key::EventType)) {
                firstKeyTime = (*si)->getAbsoluteTime();
                break;
            }
        }

        if (firstKeyTime > segment.getStartTime()) {
            CompositionTimeSliceAdapter adapter
            (comp, timeT(0), firstKeyTime);
            AnalysisHelper helper;
            segment.insert(helper.guessKey(adapter).getAsEvent
                           (segment.getStartTime()));
        }
    }

    int progressPer = 100;
    if (comp->getNbSegments() > 0)
        progressPer = (int)(100.0 / double(comp->getNbSegments()));

    MacroCommand *command = new MacroCommand(i18n("Calculate Notation"));

    for (Composition::iterator i = comp->begin();
            i != comp->end(); ++i) {

        Segment &segment = **i;
        timeT startTime(segment.getStartTime());
        timeT endTime(segment.getEndMarkerTime());

//        std::cerr << "segment: start time " << segment.getStartTime() << ", end time " << segment.getEndTime() << ", end marker time " << segment.getEndMarkerTime() << ", events " << segment.size() << std::endl;

        EventQuantizeCommand *subCommand = new EventQuantizeCommand
                                           (segment, startTime, endTime, "Notation Options", true);

        subCommand->setProgressTotal(progressPer + 1);
        QObject::connect(subCommand, SIGNAL(incrementProgress(int)),
						 m_progressBar, SLOT(advance(int)));

        command->addCommand(subCommand);
    }

    CommandHistory::getInstance()->addCommand(command);

    if (comp->getTimeSignatureCount() == 0) {
        CompositionTimeSliceAdapter adapter(comp);
        AnalysisHelper analysisHelper;
        TimeSignature timeSig =
            analysisHelper.guessTimeSignature(adapter);
        comp->addTimeSignature(0, timeSig);
    }

    return newDoc;
}

void RosegardenGUIApp::slotImportRG21()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open Rosegarden 2.1 File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    openFile(tmpfile, ImportRG21);
}

void RosegardenGUIApp::slotMergeRG21()
{
    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open Rosegarden 2.1 File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportRG21);
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromRG21File(QString file)
{
    KStartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(
        i18n("Importing Rosegarden 2.1 file..."), 100, this);

    CurrentProgressDialog::set
        (&progressDlg);

    // Inherent autoload
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    RG21Loader rg21Loader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&rg21Loader, SIGNAL(setValue(int)),
            m_progressBar, SLOT(setValue(int)));

    connect(&rg21Loader, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
    //progressDlg.progressBar()->advance(40);
	//m_progressBar->advance(40);
	m_progressBar->setValue(40);

    if (!rg21Loader.load(file, newDoc->getComposition())) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "", 
                           i18n("Can't load Rosegarden 2.1 file.  It appears to be corrupted."));
        delete newDoc;
        return 0;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    return newDoc;

}

void
RosegardenGUIApp::slotImportHydrogen()
{
    if (m_doc && !m_doc->saveIfModified())
        return ;

    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open Hydrogen File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    openFile(tmpfile, ImportHydrogen);
}

void RosegardenGUIApp::slotMergeHydrogen()
{
    QUrl url = QFileDialog::getOpenFileName( this, i18n("Open Hydrogen File"), QDir::currentPath(), "*", 0, 0 );
    if (url.isEmpty()) {
        return ;
    }

    //&&& KIO used to show a progress dialog of its own; we need to
    //replicate that

    QString tmpfile;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    tmpfile = source.getLocalFilename();
    source.waitForData();

    mergeFile(tmpfile, ImportHydrogen);
}

RosegardenGUIDoc*
RosegardenGUIApp::createDocumentFromHydrogenFile(QString file)
{
    KStartupLogo::hideIfStillThere();
    ProgressDialog progressDlg(
        i18n("Importing Hydrogen file..."), 100, this);

    CurrentProgressDialog::set
        (&progressDlg);

    // Inherent autoload
    //
    RosegardenGUIDoc *newDoc = new RosegardenGUIDoc(this, m_pluginManager);

    HydrogenLoader hydrogenLoader(&newDoc->getStudio());

    // TODO: make RG21Loader to actually emit these signals
    //
    connect(&hydrogenLoader, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));

    connect(&hydrogenLoader, SIGNAL(incrementProgress(int)),
			 m_progressBar, SLOT(advance(int)));

    // "your starter for 40%" - helps the "freeze" work
    //
	m_progressBar->setValue(40);

    if (!hydrogenLoader.load(file, newDoc->getComposition())) {
        CurrentProgressDialog::freeze();
        QMessageBox::critical(this, "",
                           i18n("Can't load Hydrogen file.  It appears to be corrupted."));
        delete newDoc;
        return 0;
    }

    // Set modification flag
    //
    newDoc->slotDocumentModified();

    // Set the caption and add recent
    //
    newDoc->setTitle(QFileInfo(file).fileName());
    newDoc->setAbsFilePath(QFileInfo(file).absFilePath());

    return newDoc;

}

void
RosegardenGUIApp::mergeFile(QString filePath, ImportType type)
{
    RosegardenGUIDoc *doc = createDocument(filePath, type);

    if (doc) {
        if (m_doc) {

            bool timingsDiffer = false;
            Composition &c1 = m_doc->getComposition();
            Composition &c2 = doc->getComposition();

            // compare tempos and time sigs in the two -- rather laborious

            if (c1.getTimeSignatureCount() != c2.getTimeSignatureCount()) {
                timingsDiffer = true;
            } else {
                for (int i = 0; i < c1.getTimeSignatureCount(); ++i) {
                    std::pair<timeT, TimeSignature> t1 =
                        c1.getTimeSignatureChange(i);
                    std::pair<timeT, TimeSignature> t2 =
                        c2.getTimeSignatureChange(i);
                    if (t1.first != t2.first || t1.second != t2.second) {
                        timingsDiffer = true;
                        break;
                    }
                }
            }

            if (c1.getTempoChangeCount() != c2.getTempoChangeCount()) {
                timingsDiffer = true;
            } else {
                for (int i = 0; i < c1.getTempoChangeCount(); ++i) {
                    std::pair<timeT, tempoT> t1 = c1.getTempoChange(i);
                    std::pair<timeT, tempoT> t2 = c2.getTempoChange(i);
                    if (t1.first != t2.first || t1.second != t2.second) {
                        timingsDiffer = true;
                        break;
                    }
                }
            }

            FileMergeDialog dialog(this, filePath, timingsDiffer);
            if (dialog.exec() == QDialog::Accepted) {
                m_doc->mergeDocument(doc, dialog.getMergeOptions());
            }

            delete doc;

        } else {
            setDocument(doc);
        }
    }
}

void
RosegardenGUIApp::slotCheckTransportStatus()
{
    ExternalTransport::TransportRequest req;
    RealTime rt;
    bool have = RosegardenSequencer::getInstance()->
        getNextTransportRequest(req, rt);

    if (have) {
        switch (req) {
        case ExternalTransport::TransportNoChange:    break;
        case ExternalTransport::TransportStop:        stop(); break;
        case ExternalTransport::TransportStart:       play(); break;
        case ExternalTransport::TransportPlay:        play(); break;
        case ExternalTransport::TransportRecord:      record(); break;
        case ExternalTransport::TransportJumpToTime:  jumpToTime(rt); break;
        case ExternalTransport::TransportStartAtTime: startAtTime(rt); break;
        case ExternalTransport::TransportStopAtTime:  stop(); jumpToTime(rt); break;
        }
    }

    TransportStatus status = RosegardenSequencer::getInstance()->
        getStatus();
    
    if (status == PLAYING || status == RECORDING) { //@@@ JAS orig ? KXMLGUIClient::StateReverse : KXMLGUIClient::StateNoReverse
        leaveActionState("not_playing");
    } else {
        enterActionState("not_playing");
    }

    if (m_seqManager) {

        m_seqManager->setTransportStatus(status);

        MappedComposition asynchronousQueue =
            RosegardenSequencer::getInstance()->pullAsynchronousMidiQueue();

        if (!asynchronousQueue.empty()) {
            
            m_seqManager->processAsynchronousMidi(asynchronousQueue, 0);

            if (m_view && m_seqManager->getSequencerMapper()) {
                m_view->updateMeters(m_seqManager->getSequencerMapper());
            }
        }
    }
    
    return;
}

void
RosegardenGUIApp::slotUpdatePlaybackPosition()
{
    static int callbackCount = 0;

    // Either sequencer mappper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager || !m_seqManager->getSequencerMapper())
        return ;

    SequencerMapper *mapper = m_seqManager->getSequencerMapper();

    MappedEvent ev;
    bool haveEvent = mapper->getVisual(ev);
    if (haveEvent)
        getTransport()->setMidiOutLabel(&ev);

    RealTime position = mapper->getPositionPointer();

    //    std::cerr << "RosegardenGUIApp::slotUpdatePlaybackPosition: mapper pos = " << position << std::endl;

    Composition &comp = m_doc->getComposition();
    timeT elapsedTime = comp.getElapsedTimeForRealTime(position);

    //    std::cerr << "RosegardenGUIApp::slotUpdatePlaybackPosition: mapper timeT = " << elapsedTime << std::endl;

    if (m_seqManager->getTransportStatus() == RECORDING) {

        MappedComposition mC;
        if (mapper->getRecordedEvents(mC) > 0) {
            m_seqManager->processAsynchronousMidi(mC, 0);
            m_doc->insertRecordedMidi(mC);
        }

        m_doc->updateRecordingMIDISegment();
        m_doc->updateRecordingAudioSegments();
    }

    m_originatingJump = true;
    m_doc->slotSetPointerPosition(elapsedTime);
    m_originatingJump = false;

    if (m_audioMixer && m_audioMixer->isVisible())
        m_audioMixer->updateMeters(mapper);

    if (m_midiMixer && m_midiMixer->isVisible())
        m_midiMixer->updateMeters(mapper);

    m_view->updateMeters(mapper);

    if (++callbackCount == 60) {
        slotUpdateCPUMeter(true);
        callbackCount = 0;
    }

    //     if (elapsedTime >= comp.getEndMarker())
    //         slotStop();
}

void
RosegardenGUIApp::slotUpdateCPUMeter(bool playing)
{
    static std::ifstream *statstream = 0;
    static bool modified = false;
    static unsigned long lastBusy = 0, lastIdle = 0;

    if (playing) {

        if (!statstream) {
            statstream = new std::ifstream("/proc/stat", std::ios::in);
        }

        if (!statstream || !*statstream)
            return ;
        statstream->seekg(0, std::ios::beg);

        std::string cpu;
        unsigned long user, nice, sys, idle;
        *statstream >> cpu;
        *statstream >> user;
        *statstream >> nice;
        *statstream >> sys;
        *statstream >> idle;

        unsigned long busy = user + nice + sys;
        unsigned long count = 0;

        if (lastBusy > 0) {
            unsigned long bd = busy - lastBusy;
            unsigned long id = idle - lastIdle;
            if (bd + id > 0)
                count = bd * 100 / (bd + id);
            if (count > 100)
                count = 100;
        }

        lastBusy = busy;
        lastIdle = idle;

        if (m_progressBar) {
            if (!modified) {
                //m_progressBar->setTextEnabled(true);
				m_progressBar->setTextVisible(true);
				m_progressBar->setFormat("CPU%p%");
				
            }
            m_progressBar->setValue(count);
        }

        modified = true;

    } else if (modified) {
        if (m_progressBar) {
            m_progressBar->setTextVisible(false);
            m_progressBar->setFormat("%p%");
            m_progressBar->setValue(0);
        }
        modified = false;
    }
}

void
RosegardenGUIApp::slotUpdateMonitoring()
{
    // Either sequencer mappper or the sequence manager could be missing at
    // this point.
    //
    if (!m_seqManager || !m_seqManager->getSequencerMapper())
        return ;

    SequencerMapper *mapper = m_seqManager->getSequencerMapper();

    if (m_audioMixer && m_audioMixer->isVisible())
        m_audioMixer->updateMonitorMeters(mapper);

    if (m_midiMixer && m_midiMixer->isVisible())
        m_midiMixer->updateMonitorMeter(mapper);

    m_view->updateMonitorMeters(mapper);

    slotUpdateCPUMeter(false);
}

void RosegardenGUIApp::slotSetPointerPosition(timeT t)
{
    Composition &comp = m_doc->getComposition();

    //    std::cerr << "RosegardenGUIApp::slotSetPointerPosition: t = " << t << std::endl;

    if (m_seqManager) {
	    if ( m_seqManager->getTransportStatus() == PLAYING ||
	            m_seqManager->getTransportStatus() == RECORDING ) {
	        if (t > comp.getEndMarker()) {
	            if (m_seqManager->getTransportStatus() == PLAYING) {
	
	                slotStop();
	                t = comp.getEndMarker();
	                m_doc->slotSetPointerPosition(t); //causes this method to be re-invoked
	                return ;
	
	            } else { // if recording, increase composition duration
	                std::pair<timeT, timeT> timeRange = comp.getBarRangeForTime(t);
	                timeT barDuration = timeRange.second - timeRange.first;
	                timeT newEndMarker = t + 10 * barDuration;
	                comp.setEndMarker(newEndMarker);
	                getView()->getTrackEditor()->slotReadjustCanvasSize();
	                getView()->getTrackEditor()->updateRulers();
	            }
	        }
	    }
	
	    // cc 20050520 - jump at the sequencer even if we're not playing,
	    // because we might be a transport master of some kind
	    try {
	        if (!m_originatingJump) {
	            m_seqManager->sendSequencerJump(comp.getElapsedRealTime(t));
	        }
	    } catch (QString s) {
	        QMessageBox::critical(this, "", s);
	    }
    }

    // set the time sig
    getTransport()->setTimeSignature(comp.getTimeSignatureAt(t));

    // and the tempo
    getTransport()->setTempo(comp.getTempoAtTime(t));

    // and the time
    //
    TransportDialog::TimeDisplayMode mode =
        getTransport()->getCurrentMode();

    if (mode == TransportDialog::BarMode ||
            mode == TransportDialog::BarMetronomeMode) {

        slotDisplayBarTime(t);

    } else {

        RealTime rT(comp.getElapsedRealTime(t));

        if (getTransport()->isShowingTimeToEnd()) {
            rT = rT - comp.getElapsedRealTime(comp.getDuration());
        }

        if (mode == TransportDialog::RealMode) {

            getTransport()->displayRealTime(rT);

        } else if (mode == TransportDialog::SMPTEMode) {

            getTransport()->displaySMPTETime(rT);

        } else {

            getTransport()->displayFrameTime(rT);
        }
    }
    
    // handle transport mode configuration changes
    std::string modeAsString = getTransport()->getCurrentModeAsString();
	
    if (m_doc->getConfiguration().get<String>
            (DocumentConfiguration::TransportMode) != modeAsString) {

        m_doc->getConfiguration().set<String>
            (DocumentConfiguration::TransportMode, modeAsString);

        //m_doc->slotDocumentModified(); to avoid being prompted for a file change when merely changing the transport display
    }

    // Update position on the marker editor if it's available
    //
    if (m_markerEditor)
        m_markerEditor->updatePosition();
}

void RosegardenGUIApp::slotDisplayBarTime(timeT t)
{
    Composition &comp = m_doc->getComposition();

    int barNo = comp.getBarNumber(t);
    timeT barStart = comp.getBarStart(barNo);

    TimeSignature timeSig = comp.getTimeSignatureAt(t);
    timeT beatDuration = timeSig.getBeatDuration();

    int beatNo = (t - barStart) / beatDuration;
    int unitNo = (t - barStart) - (beatNo * beatDuration);

    if (getTransport()->isShowingTimeToEnd()) {
        barNo = barNo + 1 - comp.getNbBars();
        beatNo = timeSig.getBeatsPerBar() - 1 - beatNo;
        unitNo = timeSig.getBeatDuration() - 1 - unitNo;
    } else {
        // convert to 1-based display bar numbers
        barNo += 1;
        beatNo += 1;
    }

    // show units in hemidemis (or whatever), not in raw time ticks
    unitNo /= Note(Note::Shortest).getDuration();

    getTransport()->displayBarTime(barNo, beatNo, unitNo);
}

void RosegardenGUIApp::slotRefreshTimeDisplay()
{
    if ( m_seqManager->getTransportStatus() == PLAYING ||
            m_seqManager->getTransportStatus() == RECORDING ) {
        return ; // it'll be refreshed in a moment anyway
    }
    slotSetPointerPosition(m_doc->getComposition().getPosition());
}

bool
RosegardenGUIApp::isTrackEditorPlayTracking() const
{
    return m_view->getTrackEditor()->isTracking();
}

void RosegardenGUIApp::slotToggleTracking()
{
    m_view->getTrackEditor()->slotToggleTracking();
}

void RosegardenGUIApp::slotTestStartupTester()
{
    RG_DEBUG << "RosegardenGUIApp::slotTestStartupTester" << endl;

    if (!m_startupTester) {
        m_startupTester = new StartupTester();
        connect(m_startupTester, SIGNAL(newerVersionAvailable(QString)),
                this, SLOT(slotNewerVersionAvailable(QString)));
        m_startupTester->start();
        QTimer::singleShot(100, this, SLOT(slotTestStartupTester()));
        return ;
    }

    if (!m_startupTester->isReady()) {
        QTimer::singleShot(100, this, SLOT(slotTestStartupTester()));
        return ;
    }

    QStringList missingFeatures;
    QStringList allMissing;

    QStringList missing;
    bool have = m_startupTester->haveProjectPackager(&missing);

    if (have) { //@@@ JAS orig ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
        enterActionState("have_project_packager");
    } else {
        leaveActionState("have_project_packager");

        missingFeatures.push_back(i18n("Export and import of Rosegarden Project files"));
        if (missing.count() == 0) {
            allMissing.push_back(i18n("The Rosegarden Project Packager helper script"));
        } else {
            for (int i = 0; i < missing.count(); ++i) {
//                if (missingFeatures.count() > 1) {
                    allMissing.push_back(i18n("%1 - for project file support", missing[i]));
//                } else {
//                    allMissing.push_back(missing[i]);
//                }
            }
        }
    }

    have = m_startupTester->haveLilyPondView(&missing);

    if (have) { //@@@ JAS orig ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
        enterActionState("have_lilypondview");
    } else {
        leaveActionState("have_lilypondview");

        missingFeatures.push_back(i18n("Notation previews through LilyPond"));
        if (missing.count() == 0) {
            allMissing.push_back(i18n("The Rosegarden LilyPondView helper script"));
        } else {
            for (int i = 0; i < missing.count(); ++i) {
                if (missingFeatures.count() > 1) {
                    allMissing.push_back(i18n("%1 - for LilyPond preview support", missing[i]));
                } else {
                    allMissing.push_back(missing[i]);
                }
            }
        }
    }

#ifdef HAVE_LIBJACK
    if (m_seqManager && (m_seqManager->getSoundDriverStatus() & AUDIO_OK)) {

        m_haveAudioImporter = m_startupTester->haveAudioFileImporter(&missing);

        if (!m_haveAudioImporter) {
            missingFeatures.push_back(i18n("General audio file import and conversion"));
            if (missing.count() == 0) {
                allMissing.push_back(i18n("The Rosegarden Audio File Importer helper script"));
            } else {
                for (int i = 0; i < missing.count(); ++i) {
                    if (missingFeatures.count() > 1) {
                        allMissing.push_back(i18n("%1 - for audio file import", missing[i]));
                    } else {
                        allMissing.push_back(missing[i]);
                    }
                }
            }
        }
    }
#endif

    if (missingFeatures.count() > 0) {
        QString message = i18n("<h3>Helper programs not found</h3><p>Rosegarden could not find one or more helper programs which it needs to provide some features.  The following features will not be available:</p>");
        message += i18n("<ul>");
        for (int i = 0; i < missingFeatures.count(); ++i) {
            message += i18n("<li>%1</li>", missingFeatures[i]);
        }
        message += i18n("</ul>");
        message += i18n("<p>To fix this, you should install the following additional programs:</p>");
        message += i18n("<ul>");
        for (int i = 0; i < allMissing.count(); ++i) {
            message += i18n("<li>%1</li>", allMissing[i]);
        }
        message += i18n("</ul>");

        awaitDialogClearance();
        
        QMessageBox::information
            (m_view,
             message,
             i18n("Helper programs not found"),
             "startup-helpers-missing");
    }

    delete m_startupTester;
    m_startupTester = 0;
}

void RosegardenGUIApp::slotDebugDump()
{
    Composition &comp = m_doc->getComposition();
    comp.dump(std::cerr);
}

bool RosegardenGUIApp::launchSequencer()
{
    if (!isUsingSequencer()) {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - not using seq. - returning\n";
        return false; // no need to launch anything
    }

    if (isSequencerRunning()) {
        RG_DEBUG << "RosegardenGUIApp::launchSequencer() - sequencer already running - returning\n";
        if (m_seqManager) m_seqManager->checkSoundDriverStatus(false);
        return true;
    }

    m_sequencerThread = new SequencerThread();
    connect(m_sequencerThread, SIGNAL(finished()), this, SLOT(slotSequencerExited()));
    m_sequencerThread->start();

    // Sync current devices with the sequencer
    //
    if (m_doc) m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }

    return true;
}

#ifdef HAVE_LIBJACK
bool RosegardenGUIApp::launchJack()
{
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    bool startJack = qStrToBool( settings.value("jackstart", "false" ) ) ;
    settings.endGroup();

    if (!startJack)
        return true; // we don't do anything

    settings.beginGroup( SequencerOptionsConfigGroup );
    QString jackPath = settings.value("jackcommand", "").toString();

    emit startupStatusMessage(i18n("Clearing down jackd..."));

    //setup "/usr/bin/killall" process
    QProcess *proc = new QProcess; // TODO: do it in a less clumsy way
    QStringList procArgs;

    procArgs << "-9";
    procArgs << "jackd";

    proc->execute("/usr/bin/killall", procArgs);

    if (proc->exitCode())
        RG_DEBUG << "couldn't kill any jackd processes" << endl;
    else
        RG_DEBUG << "killed old jackd processes" << endl;

    emit startupStatusMessage(i18n("Starting jackd..."));

    if (jackPath != "") {

        RG_DEBUG << "starting jack \"" << jackPath << "\"" << endl;

        QStringList splitCommand;
        splitCommand = jackPath.split(" ", QString::SkipEmptyParts);

        RG_DEBUG << "RosegardenGUIApp::launchJack() : splitCommand length : "
        << splitCommand.size() << endl;

        // setup "jack" process
        m_jackProcess = new QProcess;

        m_jackProcess->start(splitCommand.takeFirst(), splitCommand);
    }
    settings.endGroup();

    return m_jackProcess != 0 ? m_jackProcess->state() == QProcess::Running : true;
}
#endif

void RosegardenGUIApp::slotDocumentDevicesResyncd()
{
    m_sequencerCheckedIn = true;
    m_trackParameterBox->populateDeviceLists();
}

void RosegardenGUIApp::slotSequencerExited(QProcess*)
{
    RG_DEBUG << "RosegardenGUIApp::slotSequencerExited Sequencer exited\n";

    KStartupLogo::hideIfStillThere();

    if (m_sequencerCheckedIn) {

        QMessageBox::critical( this, "", i18n("The Rosegarden sequencer process has exited unexpectedly.  Sound and recording will no longer be available for this session.\nPlease exit and restart Rosegarden to restore sound capability."));

    } else {

		QMessageBox::critical( this, "", i18n("The Rosegarden sequencer could not be started, so sound and recording will be unavailable for this session.\nFor assistance with correct audio and MIDI configuration, go to http://rosegardenmusic.com."));
    }

    delete m_sequencerThread;
    m_sequencerThread = 0; // isSequencerRunning() will return false
    // but isUsingSequencer() will keep returning true
    // so pressing the play button may attempt to restart the sequencer
}

void RosegardenGUIApp::slotExportProject()
{
    KTmpStatusMsg msg(i18n("Exporting Rosegarden Project file..."), this);

    QString fileName = getValidWriteFile
                       ("*.rgp|" + i18n("Rosegarden Project files\n") +
                        "\n*|" + i18n("All files"),
                        i18n("Export as..."));

    if (fileName.isEmpty())
        return ;

    QString rgFile = fileName;
    rgFile.replace(QRegExp(".rg.rgp$"), ".rg");
    rgFile.replace(QRegExp(".rgp$"), ".rg");

    CurrentProgressDialog::freeze();

    QString errMsg;
    if (!m_doc->saveDocument(rgFile, errMsg,
                             true)) { // pretend it's autosave
        /* was sorry */ QMessageBox::warning(this, "", i18n("Saving Rosegarden file to package failed: %1", errMsg));
        CurrentProgressDialog::thaw();
        return ;
    }

    //setup "rosegarden-project-package" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--pack";
    procArgs << rgFile;
    procArgs << fileName;

    proc->execute("rosegarden-project-package", procArgs);

    if ((proc->exitStatus() != QProcess::NormalExit) || proc->exitCode()) {
        /* was sorry */ QMessageBox::warning(this, "", i18n("Failed to export to project file \"%1\"", fileName));
        CurrentProgressDialog::thaw();
        delete proc;
        return ;
    }

    delete proc;
}

void RosegardenGUIApp::slotExportMIDI()
{
    KTmpStatusMsg msg(i18n("Exporting MIDI file..."), this);

    QString fileName = getValidWriteFile
                       ("*.mid *.midi|" + i18n("Standard MIDI files\n") +
                        "\n*|" + i18n("All files"),
                        i18n("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportMIDIFile(fileName);
}

void RosegardenGUIApp::exportMIDIFile(QString file)
{
    ProgressDialog progressDlg(i18n("Exporting MIDI file..."),
                               100,
                               this);

    std::string fname(QFile::encodeName(file));

    MidiFile midiFile(fname,
                      &m_doc->getStudio());

    connect(&midiFile, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));

    connect(&midiFile, SIGNAL(incrementProgress(int)),
			 m_progressBar, SLOT(advance(int)));

    midiFile.convertToMidi(m_doc->getComposition());

    if (!midiFile.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportCsound()
{
    KTmpStatusMsg msg(i18n("Exporting Csound score file..."), this);

    QString fileName = getValidWriteFile(QString("*|") + i18n("All files"),
                                         i18n("Export as..."));
    if (fileName.isEmpty())
        return ;

    exportCsoundFile(fileName);
}

void RosegardenGUIApp::exportCsoundFile(QString file)
{
    ProgressDialog progressDlg(i18n("Exporting Csound score file..."),
                               100,
                               this);

    CsoundExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
			 m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportMup()
{
    KTmpStatusMsg msg(i18n("Exporting Mup file..."), this);

    QString fileName = getValidWriteFile
                       ("*.mup|" + i18n("Mup files\n") + "\n*|" + i18n("All files"),
                        i18n("Export as..."));
    if (fileName.isEmpty())
        return ;

    exportMupFile(fileName);
}

void RosegardenGUIApp::exportMupFile(QString file)
{
    ProgressDialog progressDlg(i18n("Exporting Mup file..."),
                               100,
                               this);

    MupExporter e(this, &m_doc->getComposition(), std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
			 m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Export failed.  The file could not be opened for writing."));
    }
}

void RosegardenGUIApp::slotExportLilyPond()
{
    KTmpStatusMsg msg(i18n("Exporting LilyPond file..."), this);

    QString fileName = getValidWriteFile
                       (QString("*.ly|") + i18n("LilyPond files") +
                        "\n*|" + i18n("All files"),
                        i18n("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportLilyPondFile(fileName);
}

std::map<QProcess *, QTemporaryFile *> RosegardenGUIApp::m_lilyTempFileMap;

void RosegardenGUIApp::slotPrintLilyPond()
{
    KTmpStatusMsg msg(i18n("Printing LilyPond file..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Failed to open a temporary file for LilyPond export."));
        delete file;
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }
    //setup "rosegarden-lilypondview" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--graphical";
    procArgs << "--print";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->start("rosegarden-lilypondview" ,procArgs); //@@@JAS KProcess::NotifyOnExit
}

void RosegardenGUIApp::slotPreviewLilyPond()
{
    KTmpStatusMsg msg(i18n("Previewing LilyPond file..."), this);
    QTemporaryFile *file = new QTemporaryFile("XXXXXX.ly");
    file->setAutoRemove(true);
    if (!file->open()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Failed to open a temporary file for LilyPond export."));
        delete file;
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename
    if (!exportLilyPondFile(filename, true)) {
        return ;
    }
    //setup "rosegarden-lilypondview" process
    QProcess *proc = new QProcess;
    QStringList procArgs;
    procArgs << "--graphical";
    procArgs << "--pdf";
    procArgs << filename;
    connect(proc, SIGNAL(processExited(QProcess *)),
            this, SLOT(slotLilyPondViewProcessExited(QProcess *)));
    m_lilyTempFileMap[proc] = file;
    proc->execute("rosegarden-lilypondview", procArgs);
}

void RosegardenGUIApp::slotLilyPondViewProcessExited(QProcess *p)
{
    delete m_lilyTempFileMap[p];
    m_lilyTempFileMap.erase(p);
    delete p;
}

bool RosegardenGUIApp::exportLilyPondFile(QString file, bool forPreview)
{
    QString caption = "", heading = "";
    if (forPreview) {
        caption = i18n("LilyPond Preview Options");
        heading = i18n("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, m_doc, caption, heading);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    ProgressDialog progressDlg(i18n("Exporting LilyPond file..."),
                               100,
                               this);

    LilyPondExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));
	//progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Export failed.  The file could not be opened for writing."));
        return false;
    }

    return true;
}

void RosegardenGUIApp::slotExportMusicXml()
{
    KTmpStatusMsg msg(i18n("Exporting MusicXML file..."), this);

    QString fileName = getValidWriteFile
                       (QString("*.xml|") + i18n("XML files") +
                        "\n*|" + i18n("All files"), i18n("Export as..."));

    if (fileName.isEmpty())
        return ;

    exportMusicXmlFile(fileName);
}

void RosegardenGUIApp::exportMusicXmlFile(QString file)
{
    ProgressDialog progressDlg(i18n("Exporting MusicXML file..."),
                               100,
                               this);

    MusicXmlExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    connect(&e, SIGNAL(setValue(int)),
			 m_progressBar, SLOT(setValue(int)));
//	progressDlg.progressBar(), SLOT(setValue(int)));

    connect(&e, SIGNAL(incrementProgress(int)),
            m_progressBar, SLOT(advance(int)));

    if (!e.write()) {
        CurrentProgressDialog::freeze();
        /* was sorry */ QMessageBox::warning(this, "", i18n("Export failed.  The file could not be opened for writing."));
    }
}

void
RosegardenGUIApp::slotCloseTransport()
{
    m_viewTransport->setChecked(false);
    slotToggleTransport(); // hides the transport
}

void
RosegardenGUIApp::slotDeleteTransport()
{
    delete m_transport;
    m_transport = 0;
}

void
RosegardenGUIApp::slotActivateTool(QString toolName)
{
    if (toolName == SegmentSelector::ToolName) {
		//actionCollection()->action("select")->activate();
		(this->findChild<QAction*>("select"))->setEnabled( true );
	}
}

void
RosegardenGUIApp::slotToggleMetronome()
{
    Composition &comp = m_doc->getComposition();

    if (m_seqManager->getTransportStatus() == STARTING_TO_RECORD ||
            m_seqManager->getTransportStatus() == RECORDING ||
            m_seqManager->getTransportStatus() == RECORDING_ARMED) {
        if (comp.useRecordMetronome())
            comp.setRecordMetronome(false);
        else
            comp.setRecordMetronome(true);

        getTransport()->MetronomeButton()->setOn(comp.useRecordMetronome());
    } else {
        if (comp.usePlayMetronome())
            comp.setPlayMetronome(false);
        else
            comp.setPlayMetronome(true);

        getTransport()->MetronomeButton()->setOn(comp.usePlayMetronome());
    }
}

void
RosegardenGUIApp::slotRewindToBeginning()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_seqManager->rewindToBeginning();
}

void
RosegardenGUIApp::slotFastForwardToEnd()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_seqManager->fastForwardToEnd();
}

void
RosegardenGUIApp::slotSetPlayPosition(timeT time)
{
    RG_DEBUG << "RosegardenGUIApp::slotSetPlayPosition(" << time << ")" << endl;
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    m_doc->slotSetPointerPosition(time);

    if (m_seqManager->getTransportStatus() == PLAYING)
        return ;

    slotPlay();
}

void
RosegardenGUIApp::slotRecord()
{
    if (!isUsingSequencer())
        return ;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if we fail
        //
        if (!launchSequencer()) return;
    }

    if (m_seqManager->getTransportStatus() == RECORDING) {
        slotStop();
        return ;
    } else if (m_seqManager->getTransportStatus() == PLAYING) {
        slotToggleRecord();
        return ;
    }

    // Attempt to start recording
    //
    try {
        m_seqManager->record(false);
    } catch (QString s) {
        // We should already be stopped by this point so just unset
        // the buttons after clicking the dialog.
        //
        QMessageBox::critical(this, "", s);

        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    } catch (AudioFileManager::BadAudioPathException e) {
	        if (QMessageBox::warning
					(this, i18n("Warning"),
                 i18n("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before recording audio.\nWould you like to set it now?"),
					  QMessageBox::Yes | QMessageBox::Cancel, 
	   						QMessageBox::Cancel ) // default button
                 //i18n("Set audio file path")) == QMessageBox::Continue) {
				== QMessageBox::Yes )
			{
				slotOpenAudioPathSettings();
			}//end if
        
        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()));

        getTransport()->MetronomeButton()->setOn(false);
        getTransport()->RecordButton()->setOn(false);
        getTransport()->PlayButton()->setOn(false);
        return ;
    }

    // plugin the keyboard shortcuts for focus on this dialog
    plugShortcuts(m_seqManager->getCountdownDialog(),
                     m_seqManager->getCountdownDialog()->getShortcuts());

    connect(m_seqManager->getCountdownDialog(), SIGNAL(stopped()),
            this, SLOT(slotStop()));

    // Start the playback timer - this fetches the current sequencer position &c
    //
    m_stopTimer->stop();
    m_playTimer->start(23); // avoid multiples of 10 just so as
    // to avoid always having the same digit
    // in one place on the transport.  How
    // shallow.)
}

void
RosegardenGUIApp::slotToggleRecord()
{
    if (!isUsingSequencer() || (!isSequencerRunning() && !launchSequencer()))
        return;

    try {
        m_seqManager->record(true);
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
    } catch (AudioFileManager::BadAudioPathException e) {
        if (QMessageBox::warning
			(this, i18n("Error"),
                 i18n("The audio file path does not exist or is not writable.\nPlease set the audio file path to a valid directory in Document Properties before you start to record audio.\nWould you like to set it now?"),
					  QMessageBox::Ok | QMessageBox::Cancel, 
					   QMessageBox::Cancel
				) == QMessageBox::Ok 
		){
			//i18n("Set audio file path")) == QMessageBox::Continue) {
		slotOpenAudioPathSettings();
		}
    } catch (Exception e) {
        QMessageBox::critical(this, "",  strtoqstr(e.getMessage()));
    }

}

void
RosegardenGUIApp::slotSetLoop(timeT lhs, timeT rhs)
{
    try {
        m_doc->slotDocumentModified();

        m_seqManager->setLoop(lhs, rhs);

        // toggle the loop button
        if (lhs != rhs) {
            getTransport()->LoopButton()->setOn(true);
            enterActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            getTransport()->LoopButton()->setOn(false);
            leaveActionState("have_range"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
    }
}

void RosegardenGUIApp::alive()
{
    if (m_doc)
        m_doc->syncDevices();

    if (m_doc && m_doc->getStudio().haveMidiDevices()) {
        enterActionState("got_midi_devices"); //@@@ JAS orig. 0
    } else {
        leaveActionState("got_midi_devices"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }
}

void RosegardenGUIApp::slotPlay()
{
    if (!isUsingSequencer())
        return ;

    if (!isSequencerRunning()) {

        // Try to launch sequencer and return if it fails
        //
        if (!launchSequencer()) return;
    }

    if (!m_seqManager) return;

    // If we're armed and ready to record then do this instead (calling
    // slotRecord ensures we don't toggle the recording state in
    // SequenceManager)
    //
    if (m_seqManager->getTransportStatus() == RECORDING_ARMED) {
        slotRecord();
        return ;
    }

    // Send the controllers at start of playback if required
    //
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

    bool sendControllers = qStrToBool( settings.value("alwayssendcontrollers", "false" ) ) ;

    if (sendControllers)
        m_doc->initialiseControllers();

    bool pausedPlayback = false;

    try {
        pausedPlayback = m_seqManager->play(); // this will stop playback (pause) if it's already running
        // Check the new state of the transport and start or stop timer
        // accordingly
        //
        if (!pausedPlayback) {

            // Start the playback timer - this fetches the current sequencer position &c
            //
            m_stopTimer->stop();
            m_playTimer->start(23);
        } else {
            m_playTimer->stop();
            m_stopTimer->start(100);
        }
    } catch (QString s) {
        QMessageBox::critical(this, "", s);
        m_playTimer->stop();
        m_stopTimer->start(100);
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()) );
        m_playTimer->stop();
        m_stopTimer->start(100);
    }

    settings.endGroup();
}

void RosegardenGUIApp::slotJumpToTime(RealTime rt)
{
    Composition *comp = &m_doc->getComposition();
    timeT t = comp->getElapsedTimeForRealTime(rt);
    m_doc->slotSetPointerPosition(t);
}

void RosegardenGUIApp::slotStartAtTime(RealTime rt)
{
    slotJumpToTime(rt);
    slotPlay();
}

void RosegardenGUIApp::slotStop()
{
    if (m_seqManager &&
        m_seqManager->getCountdownDialog()) {
        disconnect(m_seqManager->getCountdownDialog(), SIGNAL(stopped()),
                   this, SLOT(slotStop()));
        disconnect(m_seqManager->getCountdownDialog(), SIGNAL(completed()),
                   this, SLOT(slotStop()));
    }

    try {
        if (m_seqManager)
            m_seqManager->stopping();
    } catch (Exception e) {
        QMessageBox::critical(this, "", strtoqstr(e.getMessage()));
    }

    // stop the playback timer
    m_playTimer->stop();
    m_stopTimer->start(100);
}

void RosegardenGUIApp::slotRewind()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;
    if (m_seqManager)
        m_seqManager->rewind();
}

void RosegardenGUIApp::slotFastforward()
{
    // ignore requests if recording
    //
    if (m_seqManager->getTransportStatus() == RECORDING)
        return ;

    if (m_seqManager)
        m_seqManager->fastforward();
}

void
RosegardenGUIApp::slotSetLoop()
{
    // restore loop
    m_doc->setLoop(m_storedLoopStart, m_storedLoopEnd);
}

void
RosegardenGUIApp::slotUnsetLoop()
{
    Composition &comp = m_doc->getComposition();

    // store the loop
    m_storedLoopStart = comp.getLoopStart();
    m_storedLoopEnd = comp.getLoopEnd();

    // clear the loop at the composition and propagate to the rest
    // of the display items
    m_doc->setLoop(0, 0);
}

void
RosegardenGUIApp::slotSetLoopStart()
{
    // Check so that start time is before endtime, othervise move upp the
    // endtime to that same pos.
    if ( m_doc->getComposition().getPosition() < m_doc->getComposition().getLoopEnd() ) {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getLoopEnd());
    } else {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getPosition());
    }
}

void
RosegardenGUIApp::slotSetLoopStop()
{
    // Check so that end time is after start time, othervise move upp the
    // start time to that same pos.
    if ( m_doc->getComposition().getLoopStart() < m_doc->getComposition().getPosition() ) {
        m_doc->setLoop(m_doc->getComposition().getLoopStart(), m_doc->getComposition().getPosition());
    } else {
        m_doc->setLoop(m_doc->getComposition().getPosition(), m_doc->getComposition().getPosition());
    }
}

void RosegardenGUIApp::slotToggleSolo(bool value)
{
    RG_DEBUG << "RosegardenGUIApp::slotToggleSolo value = " << value << endl;

    m_doc->getComposition().setSolo(value);
    getTransport()->SoloButton()->setOn(value);

    m_doc->slotDocumentModified();

    emit compositionStateUpdate();
}

void RosegardenGUIApp::slotTrackUp()
{
    Composition &comp = m_doc->getComposition();

    TrackId tid = comp.getSelectedTrack();
    TrackId pos = comp.getTrackById(tid)->getPosition();

    // If at top already
    if (pos == 0)
        return ;

    Track *track = comp.getTrackByPosition(pos - 1);

    // If the track exists
    if (track) {
        comp.setSelectedTrack(track->getId());
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotTrackDown()
{
    Composition &comp = m_doc->getComposition();

    TrackId tid = comp.getSelectedTrack();
    TrackId pos = comp.getTrackById(tid)->getPosition();

    Track *track = comp.getTrackByPosition(pos + 1);

    // If the track exists
    if (track) {
        comp.setSelectedTrack(track->getId());
        m_view->slotSelectTrackSegments(comp.getSelectedTrack());
    }

}

void RosegardenGUIApp::slotMuteAllTracks()
{
    RG_DEBUG << "RosegardenGUIApp::slotMuteAllTracks" << endl;

    Composition &comp = m_doc->getComposition();

    Composition::trackcontainer tracks = comp.getTracks();
    Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), true);
}

void RosegardenGUIApp::slotUnmuteAllTracks()
{
    RG_DEBUG << "RosegardenGUIApp::slotUnmuteAllTracks" << endl;

    Composition &comp = m_doc->getComposition();

    Composition::trackcontainer tracks = comp.getTracks();
    Composition::trackiterator tit;
    for (tit = tracks.begin(); tit != tracks.end(); ++tit)
        m_view->slotSetMute((*tit).second->getInstrument(), false);
}

void RosegardenGUIApp::slotToggleMutedCurrentTrack()
{
    Composition &comp = m_doc->getComposition();
    TrackId tid = comp.getSelectedTrack();
    Track *track = comp.getTrackById(tid);
    // If the track exists
    if (track) {
    	bool isMuted = track->isMuted();
    	m_view->slotSetMuteButton(tid, !isMuted);
    }
}

void RosegardenGUIApp::slotToggleRecordCurrentTrack()
{
    Composition &comp = m_doc->getComposition();
    TrackId tid = comp.getSelectedTrack();
    int pos = comp.getTrackPositionById(tid);
    m_view->getTrackEditor()->getTrackButtons()->slotToggleRecordTrack(pos);
}


void RosegardenGUIApp::slotConfigure()
{
    RG_DEBUG << "RosegardenGUIApp::slotConfigure\n";

    ConfigureDialog *configDlg =
        new ConfigureDialog(m_doc, this);

    connect(configDlg, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SLOT(slotUpdateAutoSaveInterval(unsigned int)));
    connect(configDlg, SIGNAL(updateSidebarStyle(unsigned int)),
            this, SLOT(slotUpdateSidebarStyle(unsigned int)));

    configDlg->show();
}

void RosegardenGUIApp::slotEditDocumentProperties()
{
    RG_DEBUG << "RosegardenGUIApp::slotEditDocumentProperties\n";

    DocumentConfigureDialog *configDlg =
        new DocumentConfigureDialog(m_doc, this);

    configDlg->show();
}

void RosegardenGUIApp::slotOpenAudioPathSettings()
{
    RG_DEBUG << "RosegardenGUIApp::slotOpenAudioPathSettings\n";

    DocumentConfigureDialog *configDlg =
        new DocumentConfigureDialog(m_doc, this);

    configDlg->showAudioPage();
    configDlg->show();
}

void RosegardenGUIApp::slotEditKeys()
{
    //&&&
 //   KKeyDialog::configure(actionCollection());	//&&& disabled KKeyDialog for now
}

void RosegardenGUIApp::slotEditToolbars()
{
    //&&&
// KEditToolbar dlg(actionCollection(), "rosegardenui.rc");	//&&& disabled Toolbar config. no qt4 equival.
	/*
	QToolBar dlg;

    connect(&dlg, SIGNAL(newToolbarConfig()),
            SLOT(slotUpdateToolbars()));

    dlg.exec();
	*/
}

void RosegardenGUIApp::slotUpdateToolbars()
{
    createGUI("rosegardenui.rc");
    m_viewToolBar->setChecked(!toolBar()->isHidden());
}

void RosegardenGUIApp::slotEditTempo()
{
    slotEditTempo(this);
}

void RosegardenGUIApp::slotEditTempo(timeT atTime)
{
    slotEditTempo(this, atTime);
}

void RosegardenGUIApp::slotEditTempo(QWidget *parent)
{
    slotEditTempo(parent, m_doc->getComposition().getPosition());
}

void RosegardenGUIApp::slotEditTempo(QWidget *parent, timeT atTime)
{
    RG_DEBUG << "RosegardenGUIApp::slotEditTempo\n";

    TempoDialog tempoDialog(parent, m_doc);

    connect(&tempoDialog,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            SLOT(slotChangeTempo(timeT,
                                 tempoT,
                                 tempoT,
                                 TempoDialog::TempoDialogAction)));

    tempoDialog.setTempoPosition(atTime);
    tempoDialog.exec();
}

void RosegardenGUIApp::slotEditTimeSignature()
{
    slotEditTimeSignature(this);
}

void RosegardenGUIApp::slotEditTimeSignature(timeT atTime)
{
    slotEditTimeSignature(this, atTime);
}

void RosegardenGUIApp::slotEditTimeSignature(QWidget *parent)
{
    slotEditTimeSignature(parent, m_doc->getComposition().getPosition());
}

void RosegardenGUIApp::slotEditTimeSignature(QWidget *parent,
        timeT time)
{
    Composition &composition(m_doc->getComposition());

    TimeSignature sig = composition.getTimeSignatureAt(time);

    TimeSignatureDialog dialog(parent, &composition, time, sig);

    if (dialog.exec() == QDialog::Accepted) {

        time = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            CommandHistory::getInstance()->addCommand
            (new AddTimeSignatureAndNormalizeCommand
             (&composition, time, dialog.getTimeSignature()));
        } else {
            CommandHistory::getInstance()->addCommand
            (new AddTimeSignatureCommand
             (&composition, time, dialog.getTimeSignature()));
        }
    }
}

void RosegardenGUIApp::slotEditTransportTime()
{
    slotEditTransportTime(this);
}

void RosegardenGUIApp::slotEditTransportTime(QWidget *parent)
{
    TimeDialog dialog(parent, i18n("Move playback pointer to time"),
                      &m_doc->getComposition(),
                      m_doc->getComposition().getPosition(),
                      true);
    if (dialog.exec() == QDialog::Accepted) {
        m_doc->slotSetPointerPosition(dialog.getTime());
    }
}

void RosegardenGUIApp::slotChangeZoom(int)
{
    double duration44 = TimeSignature(4, 4).getBarDuration();
    double value = double(m_zoomSlider->getCurrentSize());
    m_zoomLabel->setText(i18n("%1%", duration44 / value));

    RG_DEBUG << "RosegardenGUIApp::slotChangeZoom : zoom size = "
    << m_zoomSlider->getCurrentSize() << endl;

    // initZoomToolbar sets the zoom value. With some old versions of
    // Qt3.0, this can cause slotChangeZoom() to be called while the
    // view hasn't been initialized yet, so we need to check it's not
    // null
    //
    if (m_view)
        m_view->setZoomSize(m_zoomSlider->getCurrentSize());

    long newZoom = int(m_zoomSlider->getCurrentSize() * 1000.0);

    if (m_doc->getConfiguration().get<Int>
            (DocumentConfiguration::ZoomLevel) != newZoom) {

        m_doc->getConfiguration().set<Int>
        (DocumentConfiguration::ZoomLevel, newZoom);

        m_doc->slotDocumentModified();
    }
}

void
RosegardenGUIApp::slotZoomIn()
{
    m_zoomSlider->increment();
}

void
RosegardenGUIApp::slotZoomOut()
{
    m_zoomSlider->decrement();
}

void
RosegardenGUIApp::slotChangeTempo(timeT time,
                                  tempoT value,
                                  tempoT target,
                                  TempoDialog::TempoDialogAction action)
{
    //!!! handle target

    Composition &comp = m_doc->getComposition();

    // We define a macro command here and build up the command
    // label as we add commands on.
    //
    if (action == TempoDialog::AddTempo) {
        CommandHistory::getInstance()->addCommand
        (new AddTempoChangeCommand(&comp, time, value, target));
    } else if (action == TempoDialog::ReplaceTempo) {
        int index = comp.getTempoChangeNumberAt(time);

        // if there's no previous tempo change then just set globally
        //
        if (index == -1) {
            CommandHistory::getInstance()->addCommand
            (new AddTempoChangeCommand(&comp, 0, value, target));
            return ;
        }

        // get time of previous tempo change
        timeT prevTime = comp.getTempoChange(index).first;

        MacroCommand *macro =
            new MacroCommand(i18n("Replace Tempo Change at %1", time));

        macro->addCommand(new RemoveTempoChangeCommand(&comp, index));
        macro->addCommand(new AddTempoChangeCommand(&comp, prevTime, value,
                          target));

        CommandHistory::getInstance()->addCommand(macro);

    } else if (action == TempoDialog::AddTempoAtBarStart) {
        CommandHistory::getInstance()->addCommand(new
                                               AddTempoChangeCommand(&comp, comp.getBarStartForTime(time),
                                                                     value, target));
    } else if (action == TempoDialog::GlobalTempo ||
               action == TempoDialog::GlobalTempoWithDefault) {
        MacroCommand *macro = new MacroCommand(i18n("Set Global Tempo"));

        // Remove all tempo changes in reverse order so as the index numbers
        // don't becoming meaningless as the command gets unwound.
        //
        for (int i = 0; i < comp.getTempoChangeCount(); i++)
            macro->addCommand(new RemoveTempoChangeCommand(&comp,
                              (comp.getTempoChangeCount() - 1 - i)));

        // add tempo change at time zero
        //
        macro->addCommand(new AddTempoChangeCommand(&comp, 0, value, target));

        // are we setting default too?
        //
        if (action == TempoDialog::GlobalTempoWithDefault) {
            macro->setName(i18n("Set Global and Default Tempo"));
            macro->addCommand(new ModifyDefaultTempoCommand(&comp, value));
        }

        CommandHistory::getInstance()->addCommand(macro);

    } else {
        RG_DEBUG << "RosegardenGUIApp::slotChangeTempo() - "
        << "unrecognised tempo command" << endl;
    }
}

void
RosegardenGUIApp::slotMoveTempo(timeT oldTime,
                                timeT newTime)
{
    Composition &comp = m_doc->getComposition();
    int index = comp.getTempoChangeNumberAt(oldTime);

    if (index < 0)
        return ;

    MacroCommand *macro =
        new MacroCommand(i18n("Move Tempo Change"));

    std::pair<timeT, tempoT> tc =
        comp.getTempoChange(index);
    std::pair<bool, tempoT> tr =
        comp.getTempoRamping(index, false);

    macro->addCommand(new RemoveTempoChangeCommand(&comp, index));
    macro->addCommand(new AddTempoChangeCommand(&comp,
                      newTime,
                      tc.second,
                      tr.first ? tr.second : -1));

    CommandHistory::getInstance()->addCommand(macro);
}

void
RosegardenGUIApp::slotDeleteTempo(timeT t)
{
    Composition &comp = m_doc->getComposition();
    int index = comp.getTempoChangeNumberAt(t);

    if (index < 0)
        return ;

    CommandHistory::getInstance()->addCommand(new RemoveTempoChangeCommand
                                           (&comp, index));
}

void
RosegardenGUIApp::slotAddMarker(timeT time)
{
    AddMarkerCommand *command =
        new AddMarkerCommand(&m_doc->getComposition(),
                             time,
                            qStrToStrUtf8( i18n("new marker")),
                            qStrToStrUtf8( i18n("no description"))  );

    CommandHistory::getInstance()->addCommand(command);    
}

void
RosegardenGUIApp::slotDeleteMarker(int id, timeT time, QString name, QString description)
{
    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&m_doc->getComposition(),
                                id,
                                time,
                                qstrtostr(name),
                                qstrtostr(description));

    CommandHistory::getInstance()->addCommand(command);
}

void
RosegardenGUIApp::slotDocumentModified(bool m)
{
    RG_DEBUG << "RosegardenGUIApp::slotDocumentModified(" << m << ") - doc path = "
    << m_doc->getAbsFilePath() << endl;

    if (!m_doc->getAbsFilePath().isEmpty()) {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // slotStateChanged( qStrToCharPtrUtf8(QString("saved_file_modified")), m);
        slotStateChanged("saved_file_modified", m);
    } else {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // slotStateChanged( qStrToCharPtrUtf8(QString("new_file_modified")), m);
        slotStateChanged("new_file_modified", m);
    }

}

void
RosegardenGUIApp::slotStateChanged(QString s,
                                   bool noReverse)
{
    //     RG_DEBUG << "RosegardenGUIApp::slotStateChanged " << s << "," << noReverse << endl;

//	stateChanged(s, noReverse ? KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse); 
//@@@ JAS 	rgTempQtIV->stateChanged( qStrToCharPtrUtf8(s), 0);  //### check later
//@@@ JAS Rewriting code as follows.
    if (noReverse) {
        //### JAS the qStrToCharPtrUtf8(s) is probably unnecessary.
        // enterActionState(qStrToCharPtrUtf8(s));
        enterActionState(qStrToCharPtrUtf8(s));
    } else {
        // leaveActionState(s);
        leaveActionState(s);
    }
}

void
RosegardenGUIApp::slotTestClipboard()
{
	// use qt4:
	//QClipboard *clipboard = QApplication::clipboard();
	//QString originalText = clipboard->text();

    if (m_clipboard->isEmpty()) {
        leaveActionState("have_clipboard"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_clipboard_single_segment"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    } else {
        enterActionState("have_clipboard");
        if (m_clipboard->isSingleSegment()) {  //@@@ JAS orig. KXMLGUIClient::StateNoReverse : KXMLGUIClient::StateReverse
            enterActionState("have_clipboard_single_segment");
        } else {
            leaveActionState("have_clipboard_single_segment");
        }
    }
}

void
RosegardenGUIApp::plugShortcuts(QWidget *widget, QShortcut *acc)
{
	// new qt4: 
	QWidget* sc_parent = this;
	QShortcut* sc_tmp;
	
	// types:Qt::WidgetShortcut, Qt::ApplicationShortcut, Qt::WindowShortcut
//	sc_tmp = new QShortcut( QKeySequence(Qt::Key_T), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
//	connect( sc_tmp, SIGNAL(activated(), this, SLOT(slotHideTransport()) )
	
	
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_Enter), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotPlay()), this, SLOT(slotHideTransport()) );
	
	// alternative Shortcut for Play command
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_Return + Qt::CTRL), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotPlay()), this, SLOT(slotHideTransport()) );
	
	
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_Insert), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotStop()), this, SLOT(slotHideTransport()) );
	
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_PageUp), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotFastforward()), this, SLOT(slotHideTransport()) );
	
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_PageDown), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotRewind()), this, SLOT(slotHideTransport()) );
	
	sc_tmp = new QShortcut( QKeySequence(Qt::Key_Space), sc_parent, 0, 0, Qt::ApplicationShortcut ); 
	connect( sc_tmp, SIGNAL(slotToggleRecord()), this, SLOT(slotHideTransport()) );
	


    TransportDialog *transport =
        dynamic_cast<TransportDialog*>(widget);

    if (transport) {
		
		//### check shortcuts:
		sc_tmp = new QShortcut( QKeySequence(Qt::Key_M), sc_parent, 0, 0, Qt::ApplicationShortcut );
		//sc_tmp = m_jumpToQuickMarkerAction->shortcut();
		connect( sc_tmp, SIGNAL(slotJumpToQuickMarker()), this, SLOT(slotHideTransport()) );
		
		sc_tmp = new QShortcut( QKeySequence(Qt::CTRL + Qt::Key_M), sc_parent, 0, 0, Qt::ApplicationShortcut );
		//sc_tmp = m_setQuickMarkerAction->shortcut();
		connect( sc_tmp, SIGNAL(slotSetQuickMarker()), this, SLOT(slotHideTransport()) );
		

        connect(transport->PlayButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotPlay()));

        connect(transport->StopButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotStop()));

        connect(transport->FfwdButton(),
                SIGNAL(clicked()),
                SLOT(slotFastforward()));

        connect(transport->RewindButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRewind()));

        connect(transport->RecordButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRecord()));

        connect(transport->RewindEndButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRewindToBeginning()));

        connect(transport->FfwdEndButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotFastForwardToEnd()));

        connect(transport->MetronomeButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotToggleMetronome()));

        connect(transport->SoloButton(),
                SIGNAL(toggled(bool)),
                this,
                SLOT(slotToggleSolo(bool)));

        connect(transport->TimeDisplayButton(),
                SIGNAL(clicked()),
                this,
                SLOT(slotRefreshTimeDisplay()));

        connect(transport->ToEndButton(),
                SIGNAL(clicked()),
                SLOT(slotRefreshTimeDisplay()));
    }
}

void
RosegardenGUIApp::setCursor(const QCursor& cursor)
{
	//KDockMainWindow::setCursor(cursor);
	this->setCursor(cursor);	// note. this (qmainwindow) now contains main dock

    // play it safe, so we can use this class at anytime even very early in the app init
    if ((getView() &&
            getView()->getTrackEditor() &&
            getView()->getTrackEditor()->getSegmentCanvas() &&
            getView()->getTrackEditor()->getSegmentCanvas()->viewport())) {

        getView()->getTrackEditor()->getSegmentCanvas()->viewport()->setCursor(cursor);
    }

    // view, main window...
    //
    getView()->setCursor(cursor);

    // toolbars...
    //
	/*
	//&&& what does this do? - disabled
    QPtrListIterator<KToolBar> tbIter = toolBarIterator();
    QToolBar* tb = 0;
    while ((tb = tbIter.current()) != 0) {
        tb->setCursor(cursor);
        ++tbIter;
    }
	*/

    m_dockLeft->setCursor(cursor);
}

QVector<QString>
RosegardenGUIApp::createRecordAudioFiles(const QVector<InstrumentId> &recordInstruments)
{
    QVector<QString> qv;
    for (unsigned int i = 0; i < recordInstruments.size(); ++i) {
        AudioFile *aF = 0;
        try {
            aF = m_doc->getAudioFileManager().createRecordingAudioFile();
            if (aF) {
                // createRecordingAudioFile doesn't actually write to the disk,
                // and in principle it shouldn't fail
                qv.push_back(aF->getFilename().c_str());
                m_doc->addRecordAudioSegment(recordInstruments[i],
                                             aF->getId());
            } else {
                std::cerr << "ERROR: RosegardenGUIApp::createRecordAudioFiles: Failed to create recording audio file" << std::endl;
                return qv;
            }
        } catch (AudioFileManager::BadAudioPathException e) {
            delete aF;
            std::cerr << "ERROR: RosegardenGUIApp::createRecordAudioFiles: Failed to create recording audio file: " << e.getMessage() << std::endl;
            return qv;
        }
    }
    return qv;
}

QString
RosegardenGUIApp::getAudioFilePath()
{
    return QString(m_doc->getAudioFileManager().getAudioPath().c_str());
}

QVector<InstrumentId>
RosegardenGUIApp::getArmedInstruments()
{
    std::set
        <InstrumentId> iid;

    const Composition::recordtrackcontainer &tr =
        m_doc->getComposition().getRecordTracks();

    for (Composition::recordtrackcontainer::const_iterator i =
                tr.begin(); i != tr.end(); ++i) {
        TrackId tid = (*i);
        Track *track = m_doc->getComposition().getTrackById(tid);
        if (track) {
            iid.insert(track->getInstrument());
        } else {
            std::cerr << "Warning: RosegardenGUIApp::getArmedInstruments: Armed track " << tid << " not found in Composition" << std::endl;
        }
    }

    QVector<InstrumentId> iv;
    for (std::set
                <InstrumentId>::iterator ii = iid.begin();
                ii != iid.end(); ++ii) {
            iv.push_back(*ii);
        }
    return iv;
}

void
RosegardenGUIApp::showError(QString error)
{
    KStartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();

    // This is principally used for return values from DSSI plugin
    // configure() calls.  It seems some plugins return a string
    // telling you when everything's OK, as well as error strings, but
    // dssi.h does make it reasonably clear that configure() should
    // only return a string when there is actually a problem, so we're
    // going to stick with a sorry dialog here rather than an
    // information one

    /* was sorry */ QMessageBox::warning(0, "", error);

    CurrentProgressDialog::thaw();
}

void
RosegardenGUIApp::slotAudioManager()
{
    if (m_audioManagerDialog) {
        m_audioManagerDialog->show();
        m_audioManagerDialog->raise();
        m_audioManagerDialog->setActiveWindow();
        return ;
    }

    m_audioManagerDialog =
        new AudioManagerDialog(this, m_doc);

    connect(m_audioManagerDialog,
            SIGNAL(playAudioFile(AudioFileId,
                                 const RealTime &,
                                 const RealTime&)),
            SLOT(slotPlayAudioFile(AudioFileId,
                                   const RealTime &,
                                   const RealTime &)));

    connect(m_audioManagerDialog,
            SIGNAL(addAudioFile(AudioFileId)),
            SLOT(slotAddAudioFile(AudioFileId)));

    connect(m_audioManagerDialog,
            SIGNAL(deleteAudioFile(AudioFileId)),
            SLOT(slotDeleteAudioFile(AudioFileId)));

    //
    // Sync segment selection between audio man. dialog and main window
    //

    // from dialog to us...
    connect(m_audioManagerDialog,
            SIGNAL(segmentsSelected(const SegmentSelection&)),
            m_view,
            SLOT(slotPropagateSegmentSelection(const SegmentSelection&)));

    // and from us to dialog
    connect(this, SIGNAL(segmentsSelected(const SegmentSelection&)),
            m_audioManagerDialog,
            SLOT(slotSegmentSelection(const SegmentSelection&)));


    connect(m_audioManagerDialog,
            SIGNAL(deleteSegments(const SegmentSelection&)),
            SLOT(slotDeleteSegments(const SegmentSelection&)));

    connect(m_audioManagerDialog,
            SIGNAL(insertAudioSegment(AudioFileId,
                                      const RealTime&,
                                      const RealTime&)),
            m_view,
            SLOT(slotAddAudioSegmentDefaultPosition(AudioFileId,
                                                    const RealTime&,
                                                    const RealTime&)));
    connect(m_audioManagerDialog,
            SIGNAL(cancelPlayingAudioFile(AudioFileId)),
            SLOT(slotCancelAudioPlayingFile(AudioFileId)));

    connect(m_audioManagerDialog,
            SIGNAL(deleteAllAudioFiles()),
            SLOT(slotDeleteAllAudioFiles()));

    // Make sure we know when the audio man. dialog is closing
    //
    connect(m_audioManagerDialog,
            SIGNAL(closing()),
            SLOT(slotAudioManagerClosed()));

    // And that it goes away when the current document is changing
    //
    connect(this, SIGNAL(documentAboutToChange()),
            m_audioManagerDialog, SLOT(close()));

    m_audioManagerDialog->setAudioSubsystemStatus(
        m_seqManager->getSoundDriverStatus() & AUDIO_OK);

    plugShortcuts(m_audioManagerDialog,
                     m_audioManagerDialog->getShortcuts());

    m_audioManagerDialog->show();
}

void
RosegardenGUIApp::slotPlayAudioFile(unsigned int id,
                                    const RealTime &startTime,
                                    const RealTime &duration)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return ;

    MappedEvent mE(m_doc->getStudio().
                   getAudioPreviewInstrument(),
                   id,
                   RealTime( -120, 0),
                   duration,                   // duration
                   startTime);                // start index

    StudioControl::sendMappedEvent(mE);

}

void
RosegardenGUIApp::slotAddAudioFile(unsigned int id)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0) return;

    int result = RosegardenSequencer::getInstance()->
        addAudioFile(strtoqstr(aF->getFilename()), aF->getId());

    if (!result) {
        QMessageBox::critical(this, "", i18n("Sequencer failed to add audio file %1", aF->getFilename().c_str()) );
    }
}

void
RosegardenGUIApp::slotDeleteAudioFile(unsigned int id)
{
    if (m_doc->getAudioFileManager().removeFile(id) == false)
        return ;

    int result = RosegardenSequencer::getInstance()->removeAudioFile(id);

    if (!result) {
        QMessageBox::critical(this, "", i18n("Sequencer failed to remove audio file id %1", id));
    }
}

void
RosegardenGUIApp::slotDeleteSegments(const SegmentSelection &selection)
{
    m_view->slotPropagateSegmentSelection(selection);
    slotDeleteSelectedSegments();
}

void
RosegardenGUIApp::slotCancelAudioPlayingFile(AudioFileId id)
{
    AudioFile *aF = m_doc->getAudioFileManager().getAudioFile(id);

    if (aF == 0)
        return ;

    MappedEvent mE(m_doc->getStudio().
                   getAudioPreviewInstrument(),
                   MappedEvent::AudioCancel,
                   id);

    StudioControl::sendMappedEvent(mE);
}

void
RosegardenGUIApp::slotDeleteAllAudioFiles()
{
    m_doc->getAudioFileManager().clear();

    // Clear at the sequencer
    //
    RosegardenSequencer::getInstance()->clearAllAudioFiles();
}

void
RosegardenGUIApp::slotRepeatingSegments()
{
    m_view->getTrackEditor()->slotTurnRepeatingSegmentToRealCopies();
}

void
RosegardenGUIApp::slotRelabelSegments()
{
    if (!m_view->haveSelection())
        return ;

    SegmentSelection selection(m_view->getSelection());
    QString editLabel;

    if (selection.size() == 0)
        return ;
    else if (selection.size() == 1)
        editLabel = i18n("Modify Segment label");
    else
        editLabel = i18n("Modify Segments label");

    KTmpStatusMsg msg(i18n("Relabelling selection..."), this);

    // Generate label
    QString label = strtoqstr((*selection.begin())->getLabel());

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        if (strtoqstr((*i)->getLabel()) != label)
            label = "";
    }

    bool ok = false;

	// getText ( QWidget * parent, const QString & title, const QString & label, QLineEdit::EchoMode mode = QLineEdit::Normal, const QString & text = QString(), bool * ok = 0, Qt::WindowFlags f = 0 )  
	
	//QWidget* _this = ;
	//QString _in("Input");
	QString newLabel = QInputDialog::getText( dynamic_cast<QWidget*>(this), QString("Input"), i18n("Enter new label") );

    if (ok) {
        CommandHistory::getInstance()->addCommand
        (new SegmentLabelCommand(selection, newLabel));
        m_view->getTrackEditor()->getSegmentCanvas()->slotUpdateSegmentsDrawBuffer();
    }
}

void
RosegardenGUIApp::slotTransposeSegments()
{
    if (!m_view->haveSelection())
        return ;

    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();
	
    if (!ok || (semitones == 0 && steps == 0)) return;
    
    CommandHistory::getInstance()->addCommand
        	(new SegmentTransposeCommand(m_view->getSelection(), intervalDialog.getChangeKey(), steps, semitones, intervalDialog.getTransposeSegmentBack()));
}

void
RosegardenGUIApp::slotChangeCompositionLength()
{
    CompositionLengthDialog dialog(this, &m_doc->getComposition());

    if (dialog.exec() == QDialog::Accepted) {
        ChangeCompositionLengthCommand *command
        = new ChangeCompositionLengthCommand(
              &m_doc->getComposition(),
              dialog.getStartMarker(),
              dialog.getEndMarker());

        m_view->getTrackEditor()->getSegmentCanvas()->clearSegmentRectsCache(true);
        CommandHistory::getInstance()->addCommand(command);
    }
}

void
RosegardenGUIApp::slotManageMIDIDevices()
{
    if (m_deviceManager) {
        m_deviceManager->show();
        m_deviceManager->raise();
        m_deviceManager->setActiveWindow();
        return ;
    }

    m_deviceManager = new DeviceManagerDialog(this, m_doc);

    connect(m_deviceManager, SIGNAL(closing()),
            this, SLOT(slotDeviceManagerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_deviceManager, SLOT(close()));

    // Cheating way of updating the track/instrument list
    //
    connect(m_deviceManager, SIGNAL(deviceNamesChanged()),
            m_view, SLOT(slotSynchroniseWithComposition()));

    connect(m_deviceManager, SIGNAL(editBanks(DeviceId)),
            this, SLOT(slotEditBanks(DeviceId)));

    connect(m_deviceManager, SIGNAL(editControllers(DeviceId)),
            this, SLOT(slotEditControlParameters(DeviceId)));

    if (m_midiMixer) {
        connect(m_deviceManager, SIGNAL(deviceNamesChanged()),
                m_midiMixer, SLOT(slotSynchronise()));

    }


    m_deviceManager->show();
}

void
RosegardenGUIApp::slotManageSynths()
{
    if (m_synthManager) {
        m_synthManager->show();
        m_synthManager->raise();
        m_synthManager->setActiveWindow();
        return ;
    }

    m_synthManager = new SynthPluginManagerDialog(this, m_doc
#ifdef HAVE_LIBLO
                     , m_pluginGUIManager
#endif
                                                 );

    connect(m_synthManager, SIGNAL(closing()),
            this, SLOT(slotSynthPluginManagerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_synthManager, SLOT(close()));

    connect(m_synthManager,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            this,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(m_synthManager,
            SIGNAL(showPluginDialog(QWidget *, InstrumentId, int)),
            this,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(m_synthManager,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    m_synthManager->show();
}

void
RosegardenGUIApp::slotOpenAudioMixer()
{
    if (m_audioMixer) {
        m_audioMixer->show();
        m_audioMixer->raise();
        m_audioMixer->setActiveWindow();
        return ;
    }

    m_audioMixer = new AudioMixerWindow(this, m_doc);

    connect(m_audioMixer, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            m_audioMixer, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));

    connect(m_audioMixer, SIGNAL(closing()),
            this, SLOT(slotAudioMixerClosed()));

    connect(m_audioMixer, SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            this, SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));

    connect(this,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_audioMixer,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(this,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_audioMixer,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

    connect(this, SIGNAL(documentAboutToChange()),
            m_audioMixer, SLOT(close()));

    connect(m_view, SIGNAL(checkTrackAssignments()),
            m_audioMixer, SLOT(slotTrackAssignmentsChanged()));

    connect(m_audioMixer, SIGNAL(play()),
            this, SLOT(slotPlay()));
    connect(m_audioMixer, SIGNAL(stop()),
            this, SLOT(slotStop()));
    connect(m_audioMixer, SIGNAL(fastForwardPlayback()),
            this, SLOT(slotFastforward()));
    connect(m_audioMixer, SIGNAL(rewindPlayback()),
            this, SLOT(slotRewind()));
    connect(m_audioMixer, SIGNAL(fastForwardPlaybackToEnd()),
            this, SLOT(slotFastForwardToEnd()));
    connect(m_audioMixer, SIGNAL(rewindPlaybackToBeginning()),
            this, SLOT(slotRewindToBeginning()));
    connect(m_audioMixer, SIGNAL(record()),
            this, SLOT(slotRecord()));
    connect(m_audioMixer, SIGNAL(panic()),
            this, SLOT(slotPanic()));

    connect(m_audioMixer,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_audioMixer,
            SLOT(slotUpdateInstrument(InstrumentId)));

    if (m_synthManager) {
        connect(m_synthManager,
                SIGNAL(pluginSelected(InstrumentId, int, int)),
                m_audioMixer,
                SLOT(slotPluginSelected(InstrumentId, int, int)));
    }

    plugShortcuts(m_audioMixer, m_audioMixer->getShortcuts());

    m_audioMixer->show();
}

void
RosegardenGUIApp::slotOpenMidiMixer()
{
    if (m_midiMixer) {
        m_midiMixer->show();
        m_midiMixer->raise();
        m_midiMixer->setActiveWindow();
        return ;
    }

    m_midiMixer = new MidiMixerWindow(this, m_doc);

    connect(m_midiMixer, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            m_midiMixer, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));

    connect(m_midiMixer, SIGNAL(closing()),
            this, SLOT(slotMidiMixerClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_midiMixer, SLOT(close()));

    connect(m_midiMixer, SIGNAL(play()),
            this, SLOT(slotPlay()));
    connect(m_midiMixer, SIGNAL(stop()),
            this, SLOT(slotStop()));
    connect(m_midiMixer, SIGNAL(fastForwardPlayback()),
            this, SLOT(slotFastforward()));
    connect(m_midiMixer, SIGNAL(rewindPlayback()),
            this, SLOT(slotRewind()));
    connect(m_midiMixer, SIGNAL(fastForwardPlaybackToEnd()),
            this, SLOT(slotFastForwardToEnd()));
    connect(m_midiMixer, SIGNAL(rewindPlaybackToBeginning()),
            this, SLOT(slotRewindToBeginning()));
    connect(m_midiMixer, SIGNAL(record()),
            this, SLOT(slotRecord()));
    connect(m_midiMixer, SIGNAL(panic()),
            this, SLOT(slotPanic()));

    connect(m_midiMixer,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            this,
            SIGNAL(instrumentParametersChanged(InstrumentId)));

    connect(this,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_midiMixer,
            SLOT(slotUpdateInstrument(InstrumentId)));

    plugShortcuts(m_midiMixer, m_midiMixer->getShortcuts());

    m_midiMixer->show();
}

void
RosegardenGUIApp::slotEditControlParameters(DeviceId device)
{
    for (std::set
                <ControlEditorDialog *>::iterator i = m_controlEditors.begin();
                i != m_controlEditors.end(); ++i) {
            if ((*i)->getDevice() == device) {
                (*i)->show();
                (*i)->raise();
                (*i)->setActiveWindow();
                return ;
            }
        }

    ControlEditorDialog *controlEditor = new ControlEditorDialog(this, m_doc,
                                         device);
    m_controlEditors.insert(controlEditor);

    RG_DEBUG << "inserting control editor dialog, have " << m_controlEditors.size() << " now" << endl;

    connect(controlEditor, SIGNAL(closing()),
            SLOT(slotControlEditorClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            controlEditor, SLOT(close()));

    connect(m_doc, SIGNAL(devicesResyncd()),
            controlEditor, SLOT(slotUpdate()));

    controlEditor->show();
}

void
RosegardenGUIApp::slotEditBanks()
{
    slotEditBanks(Device::NO_DEVICE);
}

void
RosegardenGUIApp::slotEditBanks(DeviceId device)
{
    if (m_bankEditor) {
        if (device != Device::NO_DEVICE)
            m_bankEditor->setCurrentDevice(device);
        m_bankEditor->show();
        m_bankEditor->raise();
        m_bankEditor->setActiveWindow();
        return ;
    }

    m_bankEditor = new BankEditorDialog(this, m_doc, device);

    connect(m_bankEditor, SIGNAL(closing()),
            this, SLOT(slotBankEditorClosed()));

    connect(this, SIGNAL(documentAboutToChange()),
            m_bankEditor, SLOT(slotFileClose()));

    // Cheating way of updating the track/instrument list
    //
    connect(m_bankEditor, SIGNAL(deviceNamesChanged()),
            m_view, SLOT(slotSynchroniseWithComposition()));

    m_bankEditor->show();
}

void
RosegardenGUIApp::slotManageTriggerSegments()
{
    if (m_triggerSegmentManager) {
        m_triggerSegmentManager->show();
        m_triggerSegmentManager->raise();
        m_triggerSegmentManager->setActiveWindow();
        return ;
    }

    m_triggerSegmentManager = new TriggerSegmentManager(this, m_doc);

    connect(m_triggerSegmentManager, SIGNAL(closing()),
            SLOT(slotTriggerManagerClosed()));

    connect(m_triggerSegmentManager, SIGNAL(editTriggerSegment(int)),
            m_view, SLOT(slotEditTriggerSegment(int)));

    m_triggerSegmentManager->show();
}

void
RosegardenGUIApp::slotTriggerManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotTriggerManagerClosed" << endl;

    m_triggerSegmentManager = 0;
}

void
RosegardenGUIApp::slotEditMarkers()
{
    if (m_markerEditor) {
        m_markerEditor->show();
        m_markerEditor->raise();
        m_markerEditor->setActiveWindow();
        return ;
    }

    m_markerEditor = new MarkerEditor(this, m_doc);

    connect(m_markerEditor, SIGNAL(closing()),
            SLOT(slotMarkerEditorClosed()));

    connect(m_markerEditor, SIGNAL(jumpToMarker(timeT)),
            m_doc, SLOT(slotSetPointerPosition(timeT)));

    plugShortcuts(m_markerEditor, m_markerEditor->getShortcuts());

    m_markerEditor->show();
}

void
RosegardenGUIApp::slotMarkerEditorClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotMarkerEditorClosed" << endl;

    m_markerEditor = 0;
}

void
RosegardenGUIApp::slotEditTempos(timeT t)
{
    if (m_tempoView) {
        m_tempoView->show();
        m_tempoView->raise();
        m_tempoView->setActiveWindow();
        return ;
    }

    m_tempoView = new TempoView(m_doc, getView(), t);

    connect(m_tempoView, SIGNAL(closing()),
            SLOT(slotTempoViewClosed()));

    connect(m_tempoView, SIGNAL(windowActivated()),
            getView(), SLOT(slotActiveMainWindowChanged()));

    connect(m_tempoView,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            this,
            SLOT(slotChangeTempo(timeT,
                                 tempoT,
                                 tempoT,
                                 TempoDialog::TempoDialogAction)));

    connect(m_tempoView, SIGNAL(saveFile()), this, SLOT(slotFileSave()));

    plugShortcuts(m_tempoView, m_tempoView->getShortcuts());

    m_tempoView->show();
}

void
RosegardenGUIApp::slotTempoViewClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotTempoViewClosed" << endl;

    m_tempoView = 0;
}

void
RosegardenGUIApp::slotControlEditorClosed()
{
    const QObject *s = sender();

    RG_DEBUG << "RosegardenGUIApp::slotControlEditorClosed" << endl;

    for (std::set
                <ControlEditorDialog *>::iterator i = m_controlEditors.begin();
                i != m_controlEditors.end(); ++i) {
            if (*i == s) {
                m_controlEditors.erase(i);
                RG_DEBUG << "removed control editor dialog, have " << m_controlEditors.size() << " left" << endl;
                return ;
            }
        }

    std::cerr << "WARNING: control editor " << s << " closed, but couldn't find it in our control editor list (we have " << m_controlEditors.size() << " editors)" << std::endl;
}

void
RosegardenGUIApp::slotShowPluginDialog(QWidget *parent,
                                       InstrumentId instrumentId,
                                       int index)
{
    if (!parent)
        parent = this;

    int key = (index << 16) + instrumentId;

    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->show();
        m_pluginDialogs[key]->raise();
        m_pluginDialogs[key]->setActiveWindow();
        return ;
    }

    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotShowPluginDialog - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    // only create a dialog if we've got a plugin instance
    AudioPluginInstance *inst =
        container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotShowPluginDialog - "
        << "no AudioPluginInstance found for index "
        << index << endl;
        return ;
    }

    // Create the plugin dialog
    //
    AudioPluginDialog *dialog =
        new AudioPluginDialog(parent,
                              m_doc->getPluginManager(),
#ifdef HAVE_LIBLO
                              m_pluginGUIManager,
#endif
                              container,
                              index);

    connect(dialog, SIGNAL(windowActivated()),
            m_view, SLOT(slotActiveMainWindowChanged()));

/* This feature isn't provided by the plugin dialog
    connect(m_view, SIGNAL(controllerDeviceEventReceived(MappedEvent *, const void *)),
            dialog, SLOT(slotControllerDeviceEventReceived(MappedEvent *, const void *)));
*/

    // Plug the new dialog into the standard keyboard shortcuts so
    // that we can use them still while the plugin has focus.
    //
    plugShortcuts(dialog, dialog->getShortcuts());

    connect(dialog,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            this,
            SLOT(slotPluginSelected(InstrumentId, int, int)));

    connect(dialog,
            SIGNAL(pluginPortChanged(InstrumentId, int, int)),
            this,
            SLOT(slotPluginPortChanged(InstrumentId, int, int)));

    connect(dialog,
            SIGNAL(pluginProgramChanged(InstrumentId, int)),
            this,
            SLOT(slotPluginProgramChanged(InstrumentId, int)));

    connect(dialog,
            SIGNAL(changePluginConfiguration(InstrumentId, int, bool, QString, QString)),
            this,
            SLOT(slotChangePluginConfiguration(InstrumentId, int, bool, QString, QString)));

    connect(dialog,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotShowPluginGUI(InstrumentId, int)));

    connect(dialog,
            SIGNAL(stopPluginGUI(InstrumentId, int)),
            this,
            SLOT(slotStopPluginGUI(InstrumentId, int)));

    connect(dialog,
            SIGNAL(bypassed(InstrumentId, int, bool)),
            this,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));

    connect(dialog,
            SIGNAL(destroyed(InstrumentId, int)),
            this,
            SLOT(slotPluginDialogDestroyed(InstrumentId, int)));

    connect(this, SIGNAL(documentAboutToChange()), dialog, SLOT(close()));

    m_pluginDialogs[key] = dialog;
    m_pluginDialogs[key]->show();

    // Set modified
    m_doc->slotDocumentModified();
}

void
RosegardenGUIApp::slotPluginSelected(InstrumentId instrumentId,
                                     int index, int plugin)
{
    const QObject *s = sender();

    bool fromSynthMgr = (s == m_synthManager);

    // It's assumed that ports etc will already have been set up on
    // the AudioPluginInstance before this is invoked.

    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst =
        container->getPlugin(index);

    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
        << "got index of unknown plugin!" << endl;
        return ;
    }

    if (plugin == -1) {
        // Destroy plugin instance
        //!!! seems iffy -- why can't we just unassign it?

        if (StudioControl::
                destroyStudioObject(inst->getMappedId())) {
            RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
            << "cannot destroy Studio object "
            << inst->getMappedId() << endl;
        }

        inst->setAssigned(false);
    } else {
        // If unassigned then create a sequencer instance of this
        // AudioPluginInstance.
        //
        if (inst->isAssigned()) {
            RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
            << " setting identifier for mapper id " << inst->getMappedId()
            << " to " << inst->getIdentifier() << endl;

            StudioControl::setStudioObjectProperty
            (inst->getMappedId(),
             MappedPluginSlot::Identifier,
             strtoqstr(inst->getIdentifier()));
        } else {
            // create a studio object at the sequencer
            MappedObjectId newId =
                StudioControl::createStudioObject
                (MappedObject::PluginSlot);

            RG_DEBUG << "RosegardenGUIApp::slotPluginSelected - "
            << " new MappedObjectId = " << newId << endl;

            // set the new Mapped ID and that this instance
            // is assigned
            inst->setMappedId(newId);
            inst->setAssigned(true);

            // set the instrument id
            StudioControl::setStudioObjectProperty
            (newId,
             MappedObject::Instrument,
             MappedObjectValue(instrumentId));

            // set the position
            StudioControl::setStudioObjectProperty
            (newId,
             MappedObject::Position,
             MappedObjectValue(index));

            // set the plugin id
            StudioControl::setStudioObjectProperty
            (newId,
             MappedPluginSlot::Identifier,
             strtoqstr(inst->getIdentifier()));
        }
    }

    int pluginMappedId = inst->getMappedId();

    //!!! much code duplicated here from RosegardenGUIDoc::initialiseStudio

    inst->setConfigurationValue
    (qstrtostr(PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY),
     m_doc->getAudioFileManager().getAudioPath());

    // Set opaque string configuration data (e.g. for DSSI plugin)
    //
    MappedObjectPropertyList config;
    for (AudioPluginInstance::ConfigMap::const_iterator
            i = inst->getConfiguration().begin();
            i != inst->getConfiguration().end(); ++i) {
        config.push_back(strtoqstr(i->first));
        config.push_back(strtoqstr(i->second));
    }

    QString error = StudioControl::setStudioObjectPropertyList
        (pluginMappedId,
         MappedPluginSlot::Configuration,
         config);

    if (error != "") showError(error);

    // Set the bypass
    //
    StudioControl::setStudioObjectProperty
    (pluginMappedId,
     MappedPluginSlot::Bypassed,
     MappedObjectValue(inst->isBypassed()));

    // Set the program
    //
    if (inst->getProgram() != "") {
        StudioControl::setStudioObjectProperty
        (pluginMappedId,
         MappedPluginSlot::Program,
         strtoqstr(inst->getProgram()));
    }

    // Set all the port values
    //
    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        StudioControl::setStudioPluginPort
        (pluginMappedId,
         (*portIt)->number,
         (*portIt)->value);
    }

    if (fromSynthMgr) {
        int key = (index << 16) + instrumentId;
        if (m_pluginDialogs[key]) {
            m_pluginDialogs[key]->updatePlugin(plugin);
        }
    } else if (m_synthManager) {
        m_synthManager->updatePlugin(instrumentId, plugin);
    }

    emit pluginSelected(instrumentId, index, plugin);

    // Set modified
    m_doc->slotDocumentModified();
}

void
RosegardenGUIApp::slotChangePluginPort(InstrumentId instrumentId,
                                       int pluginIndex,
                                       int portIndex,
                                       float value)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginPort - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginPort - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginPort - no port "
        << portIndex << endl;
        return ;
    }

    RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - "
    << "setting plugin port (" << inst->getMappedId()
    << ", " << portIndex << ") from " << port->value
    << " to " << value << endl;

    port->setValue(value);

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    m_doc->slotDocumentModified();

    // This modification came from The Outside!
    int key = (pluginIndex << 16) + instrumentId;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->updatePluginPortControl(portIndex);
    }
}

void
RosegardenGUIApp::slotPluginPortChanged(InstrumentId instrumentId,
                                        int pluginIndex,
                                        int portIndex)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    PluginPortInstance *port = inst->getPort(portIndex);
    if (!port) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - no port "
        << portIndex << endl;
        return ;
    }

    RG_DEBUG << "RosegardenGUIApp::slotPluginPortChanged - "
    << "setting plugin port (" << inst->getMappedId()
    << ", " << portIndex << ") to " << port->value << endl;

    StudioControl::setStudioPluginPort(inst->getMappedId(),
                                       portIndex, port->value);

    m_doc->slotDocumentModified();

#ifdef HAVE_LIBLO
    // This modification came from our own plugin dialog, so update
    // any external GUIs
    if (m_pluginGUIManager) {
        m_pluginGUIManager->updatePort(instrumentId,
                                       pluginIndex,
                                       portIndex);
    }
#endif
}

void
RosegardenGUIApp::slotChangePluginProgram(InstrumentId instrumentId,
        int pluginIndex,
        QString program)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginProgram - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginProgram - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    RG_DEBUG << "RosegardenGUIApp::slotChangePluginProgram - "
    << "setting plugin program ("
    << inst->getMappedId() << ") from " << inst->getProgram()
    << " to " << program << endl;

    inst->setProgram(qstrtostr(program));

    StudioControl::
    setStudioObjectProperty(inst->getMappedId(),
                            MappedPluginSlot::Program,
                            program);

    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        float value = StudioControl::getStudioPluginPort
                      (inst->getMappedId(),
                       (*portIt)->number);
        (*portIt)->value = value;
    }

    // Set modified
    m_doc->slotDocumentModified();

    int key = (pluginIndex << 16) + instrumentId;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->updatePluginProgramControl();
    }
}

void
RosegardenGUIApp::slotPluginProgramChanged(InstrumentId instrumentId,
        int pluginIndex)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginProgramChanged - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);
    if (!inst) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginProgramChanged - "
        << "no plugin at index " << pluginIndex << " on " << instrumentId << endl;
        return ;
    }

    QString program = strtoqstr(inst->getProgram());

    RG_DEBUG << "RosegardenGUIApp::slotPluginProgramChanged - "
    << "setting plugin program ("
    << inst->getMappedId() << ") to " << program << endl;

    StudioControl::
    setStudioObjectProperty(inst->getMappedId(),
                            MappedPluginSlot::Program,
                            program);

    PortInstanceIterator portIt;

    for (portIt = inst->begin();
            portIt != inst->end(); ++portIt) {
        float value = StudioControl::getStudioPluginPort
                      (inst->getMappedId(),
                       (*portIt)->number);
        (*portIt)->value = value;
    }

    // Set modified
    m_doc->slotDocumentModified();

#ifdef HAVE_LIBLO

    if (m_pluginGUIManager)
        m_pluginGUIManager->updateProgram(instrumentId,
                                          pluginIndex);
#endif
}

void
RosegardenGUIApp::slotChangePluginConfiguration(InstrumentId instrumentId,
        int index,
        bool global,
        QString key,
        QString value)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotChangePluginConfiguration - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(index);

    if (global && inst) {

        // Set the same configuration on other plugins in the same
        // instance group

        AudioPlugin *pl =
            m_pluginManager->getPluginByIdentifier(strtoqstr(inst->getIdentifier()));

        if (pl && pl->isGrouped()) {

            InstrumentList il =
                m_doc->getStudio().getAllInstruments();

            for (InstrumentList::iterator i = il.begin();
                    i != il.end(); ++i) {

                for (PluginInstanceIterator pli =
                            (*i)->beginPlugins();
                        pli != (*i)->endPlugins(); ++pli) {

                    if (*pli && (*pli)->isAssigned() &&
                            (*pli)->getIdentifier() == inst->getIdentifier() &&
                            (*pli) != inst) {

                        slotChangePluginConfiguration
                        ((*i)->getId(), (*pli)->getPosition(),
                         false, key, value);

#ifdef HAVE_LIBLO

                        m_pluginGUIManager->updateConfiguration
                        ((*i)->getId(), (*pli)->getPosition(), key);
#endif

                    }
                }
            }
        }
    }

    if (inst) {

        inst->setConfigurationValue(qstrtostr(key), qstrtostr(value));

        MappedObjectPropertyList config;
        for (AudioPluginInstance::ConfigMap::const_iterator
                i = inst->getConfiguration().begin();
                i != inst->getConfiguration().end(); ++i) {
            config.push_back(strtoqstr(i->first));
            config.push_back(strtoqstr(i->second));
        }

        RG_DEBUG << "RosegardenGUIApp::slotChangePluginConfiguration: setting new config on mapped id " << inst->getMappedId() << endl;

        QString error = StudioControl::setStudioObjectPropertyList
        (inst->getMappedId(),
         MappedPluginSlot::Configuration,
         config);

        if (error != "") showError(error);

        // Set modified
        m_doc->slotDocumentModified();

        int key = (index << 16) + instrumentId;
        if (m_pluginDialogs[key]) {
            m_pluginDialogs[key]->updatePluginProgramList();
        }
    }
}

void
RosegardenGUIApp::slotPluginDialogDestroyed(InstrumentId instrumentId,
        int index)
{
    int key = (index << 16) + instrumentId;
    m_pluginDialogs[key] = 0;
}

void
RosegardenGUIApp::slotPluginBypassed(InstrumentId instrumentId,
                                     int pluginIndex, bool bp)
{
    PluginContainer *container = 0;

    container = m_doc->getStudio().getContainerById(instrumentId);
    if (!container) {
        RG_DEBUG << "RosegardenGUIApp::slotPluginBypassed - "
        << "no instrument or buss of id " << instrumentId << endl;
        return ;
    }

    AudioPluginInstance *inst = container->getPlugin(pluginIndex);

    if (inst) {
        StudioControl::setStudioObjectProperty
        (inst->getMappedId(),
         MappedPluginSlot::Bypassed,
         MappedObjectValue(bp));

        // Set the bypass on the instance
        //
        inst->setBypass(bp);

        // Set modified
        m_doc->slotDocumentModified();
    }

    emit pluginBypassed(instrumentId, pluginIndex, bp);
}

void
RosegardenGUIApp::slotShowPluginGUI(InstrumentId instrument,
                                    int index)
{
#ifdef HAVE_LIBLO
    m_pluginGUIManager->showGUI(instrument, index);
#endif
}

void
RosegardenGUIApp::slotStopPluginGUI(InstrumentId instrument,
                                    int index)
{
#ifdef HAVE_LIBLO
    m_pluginGUIManager->stopGUI(instrument, index);
#endif
}

void
RosegardenGUIApp::slotPluginGUIExited(InstrumentId instrument,
                                      int index)
{
    int key = (index << 16) + instrument;
    if (m_pluginDialogs[key]) {
        m_pluginDialogs[key]->guiExited();
    }
}

void
RosegardenGUIApp::slotPlayList()
{
    if (!m_playList) {
        m_playList = new PlayListDialog(i18n("Play List"), this);
        connect(m_playList, SIGNAL(closing()),
                SLOT(slotPlayListClosed()));
        connect(m_playList->getPlayList(), SIGNAL(play(QString)),
                SLOT(slotPlayListPlay(QString)));
    }

    m_playList->show();
}

void
RosegardenGUIApp::slotPlayListPlay(QString url)
{
    slotStop();
    openURL(url);
    slotPlay();
}

void
RosegardenGUIApp::slotPlayListClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotPlayListClosed()\n";
    m_playList = 0;
}

static void
invokeBrowser(QString url)
{
    // This is mostly lifted from Qt Assistant source code

    QProcess *process = new QProcess;
    QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    QStringList args;

#ifdef Q_OS_MAC
    args.append(url);
    process->start("open", args);
#else
#ifdef Q_OS_WIN32

    QString pf(getenv("ProgramFiles"));
    QString command = pf + QString("\\Internet Explorer\\IEXPLORE.EXE");

    args.append(url);
    process->start(command, args);

#else
#ifdef Q_WS_X11
    if (!qgetenv("KDE_FULL_SESSION").isEmpty()) {
        args.append("exec");
        args.append(url);
        process->start("kfmclient", args);
    } else if (!qgetenv("BROWSER").isEmpty()) {
        args.append(url);
        process->start(qgetenv("BROWSER"), args);
    } else {
        args.append(url);
        process->start("firefox", args);
    }
#endif
#endif
#endif
}

void
RosegardenGUIApp::slotTutorial()
{
    QString tutorialURL = i18n("http://rosegarden.sourceforge.net/tutorial/en/chapter-0.html");
    invokeBrowser(tutorialURL);
}

void
RosegardenGUIApp::slotBugGuidelines()
{
    QString glURL = i18n("http://rosegarden.sourceforge.net/tutorial/bug-guidelines.html");
    invokeBrowser(glURL);
}

void
RosegardenGUIApp::slotBankEditorClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotBankEditorClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_bankEditor = 0;
}

void
RosegardenGUIApp::slotDeviceManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotDeviceManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_deviceManager = 0;
}

void
RosegardenGUIApp::slotSynthPluginManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotSynthPluginManagerClosed()\n";

    m_synthManager = 0;
}

void
RosegardenGUIApp::slotAudioMixerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotAudioMixerClosed()\n";

    m_audioMixer = 0;
}

void
RosegardenGUIApp::slotMidiMixerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotMidiMixerClosed()\n";

    m_midiMixer = 0;
}

void
RosegardenGUIApp::slotAudioManagerClosed()
{
    RG_DEBUG << "RosegardenGUIApp::slotAudioManagerClosed()\n";

    if (m_doc->isModified()) {
        if (m_view)
            m_view->slotSelectTrackSegments(m_doc->getComposition().getSelectedTrack());
    }

    m_audioManagerDialog = 0;
}

void
RosegardenGUIApp::slotPanic()
{
    if (m_seqManager) {
        // Stop the transport before we send a panic as the
        // playback goes all to hell anyway.
        //
        slotStop();

        ProgressDialog progressDlg(i18n("Queueing MIDI panic events for tranmission..."),
                                   100,
                                   this);
        CurrentProgressDialog::set
            (&progressDlg);
        ProgressDialog::processEvents();

        connect(m_seqManager, SIGNAL(setValue(int)),
                m_progressBar, SLOT(setValue(int)));
        connect(m_seqManager, SIGNAL(incrementProgress(int)),
                m_progressBar, SLOT(advance(int)));

        m_seqManager->panic();

    }
}

void
RosegardenGUIApp::slotPopulateTrackInstrumentPopup()
{
    RG_DEBUG << "RosegardenGUIApp::slotSetTrackInstrument\n";
    Composition &comp = m_doc->getComposition();
    Track *track = comp.getTrackById(comp.getSelectedTrack());

    if (!track) {
        RG_DEBUG << "Weird: no track available for instrument popup!" << endl;
        return ;
    }

    Instrument* instrument = m_doc->getStudio().getInstrumentById(track->getInstrument());

//	QPopupMenu* popup = dynamic_cast<QPopupMenu*>(factory()->container("set_track_instrument", this));
	QMenu* popup = this->findChild<QMenu*>("set_track_instrument");

    m_view->getTrackEditor()->getTrackButtons()->populateInstrumentPopup(instrument, popup);
}

void
RosegardenGUIApp::slotRemapInstruments()
{
    RG_DEBUG << "RosegardenGUIApp::slotRemapInstruments\n";
    RemapInstrumentDialog dialog(this, m_doc);

    connect(&dialog, SIGNAL(applyClicked()),
            m_view->getTrackEditor()->getTrackButtons(),
            SLOT(slotSynchroniseWithComposition()));

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotRemapInstruments - accepted\n";
    }

}

void
RosegardenGUIApp::slotSaveDefaultStudio()
{
    RG_DEBUG << "RosegardenGUIApp::slotSaveDefaultStudio\n";

    int reply = QMessageBox::warning
                (this, "", i18n("Are you sure you want to save this as your default studio?"), 
				 QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

    if (reply != QMessageBox::Yes)
        return ;

    KTmpStatusMsg msg(i18n("Saving current document as default studio..."), this);

//    QString autoloadFile = locateLocal("appdata", "autoload.rg"); //@@@ FIX //&&&
	QString autoloadFile ("");
	
    RG_DEBUG << "RosegardenGUIApp::slotSaveDefaultStudio : saving studio in "
    << autoloadFile << endl;

    SetWaitCursor waitCursor;
    QString errMsg;
    bool res = m_doc->saveDocument(autoloadFile, errMsg);
    if (!res) {
        if (!errMsg.isEmpty() )
            QMessageBox::critical(this, "", i18n(qStrToCharPtrUtf8(QString("Could not auto-save document at %1\nError was : %2")
                                          .arg(autoloadFile).arg(errMsg))) );
        else
			QMessageBox::critical(this, "", i18n(qStrToCharPtrUtf8(QString("Could not auto-save document at %1")
                                          .arg(autoloadFile))) );

    }
}

void
RosegardenGUIApp::slotImportDefaultStudio()
{
    int reply = QMessageBox::warning
			(this, "", i18n("Are you sure you want to import your default studio and lose the current one?"), QMessageBox::Yes | QMessageBox::No );

    if (reply != QMessageBox::Yes)
        return ;

    QString autoloadFile =
        KGlobal::dirs()->findResource("appdata", "autoload.rg");

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable()) {
        RG_DEBUG << "RosegardenGUIDoc::slotImportDefaultStudio - "
        << "can't find autoload file - defaulting" << endl;
        return ;
    }

    slotImportStudioFromFile(autoloadFile);
}

void
RosegardenGUIApp::slotImportStudio()
{
    RG_DEBUG << "RosegardenGUIApp::slotImportStudio()\n";

    QString studioDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(studioDir);
    if (!dir.exists()) {
        studioDir = ":ROSEGARDENDEVICE";
    } else {
        studioDir = "file://" + studioDir;
    }

    QUrl url = QFileDialog::getOpenFileName( this, i18n("Import Studio from File"), QDir::currentPath(), "*", 0, 0 );

    if (url.isEmpty())
        return ;

    QString target;
    FileSource source(url);
    if (!source.isAvailable()) {
        QMessageBox::critical(this, "", i18n("Cannot download file %1", url.toString()) );
        return ;
    }

    target = source.getLocalFilename();
    source.waitForData();

    slotImportStudioFromFile(target);
}

void
RosegardenGUIApp::slotImportStudioFromFile(const QString &file)
{
    RosegardenGUIDoc *doc = new RosegardenGUIDoc(this, 0, true); // skipAutoload

    Studio &oldStudio = m_doc->getStudio();
    Studio &newStudio = doc->getStudio();

    // Add some dummy devices for when we open the document.  We guess
    // that the file won't have more than 32 devices.
    //
    //    for (unsigned int i = 0; i < 32; i++) {
    //        newStudio.addDevice("", i, Device::Midi);
    //    }

    if (doc->openDocument(file, true)) { // true because we actually
        // do want to create devices
        // on the sequencer here

        MacroCommand *command = new MacroCommand(i18n("Import Studio"));
        doc->syncDevices();

        // We actually only copy across MIDI play devices... for now
        std::vector<DeviceId> midiPlayDevices;

        for (DeviceList::const_iterator i =
                    oldStudio.begin(); i != oldStudio.end(); ++i) {

            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                midiPlayDevices.push_back((*i)->getId());
            }
        }

        std::vector<DeviceId>::iterator di(midiPlayDevices.begin());

        for (DeviceList::const_iterator i =
                    newStudio.begin(); i != newStudio.end(); ++i) {

            MidiDevice *md =
                dynamic_cast<MidiDevice *>(*i);

            if (md && (md->getDirection() == MidiDevice::Play)) {
                if (di != midiPlayDevices.end()) {
                    MidiDevice::VariationType variation
                    (md->getVariationType());
                    BankList bl(md->getBanks());
                    ProgramList pl(md->getPrograms());
                    ControlList cl(md->getControlParameters());

                    ModifyDeviceCommand* mdCommand = new ModifyDeviceCommand(&oldStudio,
                                                     *di,
                                                     md->getName(),
                                                     md->getLibrarianName(),
                                                     md->getLibrarianEmail());
                    mdCommand->setVariation(variation);
                    mdCommand->setBankList(bl);
                    mdCommand->setProgramList(pl);
                    mdCommand->setControlList(cl);
                    mdCommand->setOverwrite(true);
                    mdCommand->setRename(md->getName() != "");

                    command->addCommand(mdCommand);
                    ++di;
                }
            }
        }

        while (di != midiPlayDevices.end()) {
            command->addCommand(new CreateOrDeleteDeviceCommand
                                (&oldStudio,
                                 *di));
        }

        oldStudio.setMIDIThruFilter(newStudio.getMIDIThruFilter());
        oldStudio.setMIDIRecordFilter(newStudio.getMIDIRecordFilter());

        CommandHistory::getInstance()->addCommand(command);
        m_doc->syncDevices();
        m_doc->initialiseStudio(); // The other document will have reset it
    }

    delete doc;
}

void
RosegardenGUIApp::slotResetMidiNetwork()
{
    if (m_seqManager) {

        m_seqManager->preparePlayback(true);

        m_seqManager->resetMidiNetwork();
    }

}

void
RosegardenGUIApp::slotModifyMIDIFilters()
{
    RG_DEBUG << "RosegardenGUIApp::slotModifyMIDIFilters" << endl;

    MidiFilterDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotModifyMIDIFilters - accepted" << endl;
    }
}

void
RosegardenGUIApp::slotManageMetronome()
{
    RG_DEBUG << "RosegardenGUIApp::slotManageMetronome" << endl;

    ManageMetronomeDialog dialog(this, m_doc);

    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotManageMetronome - accepted" << endl;
    }
}

void
RosegardenGUIApp::slotAutoSave()
{
    if (!m_seqManager ||
        m_seqManager->getTransportStatus() == PLAYING ||
        m_seqManager->getTransportStatus() == RECORDING)
        return ;

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("autosave", "true" ) ) ) {
        settings.endGroup();
        return ;
    }
    m_doc->slotAutoSave();

    settings.endGroup();
}

void
RosegardenGUIApp::slotUpdateAutoSaveInterval(unsigned int interval)
{
    RG_DEBUG << "RosegardenGUIApp::slotUpdateAutoSaveInterval - "
    << "changed interval to " << interval << endl;
    m_autoSaveTimer->changeInterval(int(interval) * 1000);
}

void
RosegardenGUIApp::slotUpdateSidebarStyle(unsigned int style)
{
    RG_DEBUG << "RosegardenGUIApp::slotUpdateSidebarStyle - "
    << "changed style to " << style << endl;
    m_parameterArea->setArrangement((RosegardenParameterArea::Arrangement) style);
}

void
RosegardenGUIApp::slotShowTip()
{
    RG_DEBUG << "RosegardenGUIApp::slotShowTip" << endl;
//    KTipDialog::showTip(this, locate("data", "rosegarden/tips"), true); //&&& showTip dialog deactivated.
}

void RosegardenGUIApp::slotShowToolHelp(const QString &s)
{
    QString msg = s;
    if (msg != "") msg = " " + msg;
    slotStatusMsg(msg);
}

void
RosegardenGUIApp::slotEnableMIDIThruRouting()
{
    m_seqManager->enableMIDIThruRouting(m_enableMIDIrouting->isChecked());
}

TransportDialog* RosegardenGUIApp::getTransport() 
{
    if (m_transport == 0)
        createAndSetupTransport();
    
    return m_transport;
}

RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void
RosegardenGUIApp::awaitDialogClearance()
{
    bool haveDialog = true;

    std::cerr << "RosegardenGUIApp::awaitDialogClearance: entering" << std::endl;
    
	QDialog* c;
	QList<QDialog*> cl;
	int i;
    while (haveDialog) {
		//&&& TODO: check reimplemented code here:
		cl = this->findChildren<QDialog*>();
		for( i=0; i < cl.size(); i++ ){
			c = cl.at(i);
			if( c->isVisible() ){
				haveDialog = true;
				break;	
			}
			
		}
		/*
        const QObjectList *c = children();
        if (!c) return;
        haveDialog = false;
        for (QObjectList::const_iterator i = c->begin(); i != c->end(); ++i) {
            QDialog *dialog = dynamic_cast<QDialog *>(*i);
            if (dialog && dialog->isVisible()) {
                haveDialog = true;
                break;
            }
        }
		*/

//        std::cerr << "RosegardenGUIApp::awaitDialogClearance: have dialog = "
//                  << haveDialog << std::endl;
    
        if (haveDialog) qApp->processEvents();
    }

    std::cerr << "RosegardenGUIApp::awaitDialogClearance: exiting" << std::endl;
}

void
RosegardenGUIApp::slotNewerVersionAvailable(QString v)
{
    if (m_firstRun) return;
    KStartupLogo::hideIfStillThere();
    CurrentProgressDialog::freeze();
    awaitDialogClearance();
    QMessageBox::information
			(this, i18n("Newer version available"),
         i18n("<h3>Newer version available</h3><p>A newer version of Rosegarden may be available.<br>Please consult the <a href=\"http://www.rosegardenmusic.com/getting/\">Rosegarden website</a> for more information.</p>").arg("version-%1-available-show").arg(v) );
       //  QMessageBox::AllowLink);
	
    CurrentProgressDialog::thaw();
}

void
RosegardenGUIApp::slotSetQuickMarker()
{
    RG_DEBUG << "RosegardenGUIApp::slotSetQuickMarker" << endl;
    
    m_doc->setQuickMarker();
    getView()->getTrackEditor()->updateRulers();
}

void
RosegardenGUIApp::slotJumpToQuickMarker()
{
    RG_DEBUG << "RosegardenGUIApp::slotJumpToQuickMarker" << endl;

    m_doc->jumpToQuickMarker();
}

RosegardenGUIApp *RosegardenGUIApp::m_myself = 0;

}// end namespace Rosegarden

#include "RosegardenGUIApp.moc"
