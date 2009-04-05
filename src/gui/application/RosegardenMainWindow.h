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

#ifndef _RG_ROSEGARDENGUIAPP_H_
#define _RG_ROSEGARDENGUIAPP_H_

#include "base/MidiProgram.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/widgets/ZoomSlider.h"
#include "gui/general/RecentFiles.h"
#include "base/Event.h"
#include "sound/AudioFile.h"
#include "sound/Midi.h"
#include "gui/general/ActionFileClient.h"
#include "gui/ui/DevicesManagerNew.h"

#include <QDockWidget>
#include <QString>
#include <QVector>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>

#include <map>
#include <set>

class QWidget;
class QTimer;
class QTextCodec;
class QShowEvent;
class QObject;
class QLabel;
class QCursor;
class QShortcut;
class QTemporaryFile;
class QProcess;
class QAction;


namespace Rosegarden
{

class TriggerSegmentManager;
class TransportDialog;
class TrackParameterBox;
class TempoView;
class SynthPluginManagerDialog;
class StartupTester;
class SequenceManager;
class SegmentSelection;
class SegmentParameterBox;
class RosegardenParameterArea;
class RosegardenMainViewWidget;
class RosegardenDocument;
class RealTime;
class ProgressBar;
class PlayListDialog;
class MidiMixerWindow;
class MarkerEditor;
class MappedComposition;
class LircCommander;
class LircClient;
class InstrumentParameterBox;
class DeviceManagerDialog;
class ControlEditorDialog;
class Composition;
class Clipboard;
class BankEditorDialog;
class AudioPluginOSCGUIManager;
class AudioPluginManager;
class AudioPluginDialog;
class AudioMixerWindow;
class AudioManagerDialog;
class SequencerThread;

/**
  * The base class for the main Rosegarden application window.  This
  * sets up the main window and reads the config file as well as
  * providing a menubar, toolbar and statusbar.  The central widget
  * is a RosegardenMainViewWidget, connected to the window's document.
  */
class RosegardenMainWindow : public QMainWindow, public ActionFileClient
{
    Q_OBJECT
    
    friend class RosegardenMainViewWidget;

public:

    /**
     * construtor of RosegardenMainWindow, calls all init functions to
     * create the application.
     * \arg useSequencer : if true, the sequencer is launched
     * @see initMenuBar initToolBar
     */
    RosegardenMainWindow(bool useSequencer = true,
                     QObject *startupStatusMessageReceiver = 0);

    virtual ~RosegardenMainWindow();

    /*
     * Get the current copy of the app object
     */
    static RosegardenMainWindow *self() { return m_myself; }
    
    /**
     * Return current Main Toolbar
     */
    QToolBar* toolBar( const char* name="\0" );
    
    /**
     * returns a pointer to the current document connected to the
     * KTMainWindow instance and is used by * the View class to access
     * the document object's methods
     */ 
    RosegardenDocument *getDocument() const;      

    RosegardenMainViewWidget* getView() { return m_view; }

    TransportDialog* getTransport();

    enum ImportType { ImportRG4, ImportMIDI, ImportRG21, ImportHydrogen, ImportCheckType };

    /**
     * open a Rosegarden file
     */
    virtual void openFile(QString filePath) { openFile(filePath, ImportCheckType); }

    /**
     * open a file, explicitly specifying its type
     */
    void openFile(QString filePath, ImportType type);

    /**
     * decode and open a project file
     */
    void importProject(QString filePath);

    /**
     * open a URL
     */
    virtual void openURL(QString url);

    /**
     * merge a file with the existing document
     */ 
    virtual void mergeFile(QString filePath) { mergeFile(filePath, ImportCheckType); }
    
    /**
     * merge a file, explicitly specifying its type
     */
    void mergeFile(QString filePath, ImportType type);

    /**
     * open a URL
     */
    void openURL(const QUrl &url);

    /**
     * export a MIDI file
     */
    void exportMIDIFile(QString url);

    /**
     * export a Csound scorefile
     */
    void exportCsoundFile(QString url);

    /**
     * export a Mup file
     */
    void exportMupFile(QString url);

    /**
     * export a LilyPond file
     */
    bool exportLilyPondFile(QString url, bool forPreview = false);

    /**
     * export a MusicXml file
     */
    void exportMusicXmlFile(QString url);

    /**
     * Get the sequence manager object
     */
    SequenceManager* getSequenceManager() { return m_seqManager; }

    /**
     * Get a value() bar
     */
    ProgressBar *getProgressBar() { return m_progressBar; }

    /**
     * Equivalents of the GUI slots, for DCOP use
     */
    virtual void fileNew()    { slotFileNew(); }
    virtual void fileSave()   { slotFileSave(); }
    virtual void fileClose()  { slotFileClose(); }
    virtual void quit()       { slotQuit(); }

    virtual void play()               { slotPlay(); }
    virtual void stop()               { slotStop(); }
    virtual void rewind()             { slotRewind(); }
    virtual void fastForward()        { slotFastforward(); }
    virtual void record()             { slotRecord(); }
    virtual void rewindToBeginning()  { slotRewindToBeginning(); }
    virtual void fastForwardToEnd()   { slotFastForwardToEnd(); }
    virtual void jumpToTime(RealTime rt) { slotJumpToTime(rt); }
    virtual void startAtTime(RealTime rt) { slotStartAtTime(rt); }
    
    virtual void trackUp()            { slotTrackUp(); }
    virtual void trackDown()          { slotTrackDown(); }
    virtual void toggleMutedCurrentTrack() { slotToggleMutedCurrentTrack(); }
    virtual void toggleRecordCurrentTrack() { slotToggleRecordCurrentTrack(); }
       
    /**
     * Create some new audio files for the sequencer and return the
     * paths for them as QStrings.
     */
    QVector<QString> createRecordAudioFiles(const QVector<InstrumentId> &);

    QVector<InstrumentId> getArmedInstruments();
 
    /**
     * Start the sequencer auxiliary process
     * (built in the 'sequencer' directory)
     *
     * @see slotSequencerExited()
     */
    bool launchSequencer();

#ifdef HAVE_LIBJACK
    /*
     * Launch and control JACK if required to by configuration 
     */
    bool launchJack();

#endif // HAVE_LIBJACK


    /**
     * Returns whether we're using a sequencer.
     * false if the '--nosequencer' option was given
     * true otherwise.
     * This doesn't give the state of the sequencer
     * @see #isSequencerRunning
     */
    bool isUsingSequencer() { return m_useSequencer; }

    /**
     * Returns whether there's a sequencer running.
     * The result is dynamically updated depending on the sequencer's
     * status.
     */
    bool isSequencerRunning() { return m_useSequencer && (m_sequencerThread != 0); }

    /*
     * The sequencer calls this method when it's running to
     * allow us to sync data with it.
     *
     */
    virtual void alive();
    
    /*
     * Tell the application whether this is the first time this
     * version of RG has been run
     */
    void setIsFirstRun(bool first) { m_firstRun = first; }

    /*
     * Wait in a sub-event-loop until all modal dialogs from the main
     * window have been cleared
     */
    void awaitDialogClearance();

    /*
     * Return the clipboard
     */
    Clipboard* getClipboard() { return m_clipboard; }

#ifdef HAVE_LIBLO
    /**
     * Return the plugin native GUI manager, if we have one
     */
    AudioPluginOSCGUIManager *getPluginGUIManager() { return m_pluginGUIManager; }
#endif

    /**
     * Plug a widget into our common shortcuts
     */
    void plugShortcuts(QWidget *widget, QShortcut *shortcut);

    /**
     * Override from QWidget
     * Toolbars and docks need special treatment
     */
    virtual void setCursor(const QCursor&);

    bool isTrackEditorPlayTracking() const;

    bool testAudioPath(QString op); // and open the dialog to set it if unset
    bool haveAudioImporter() const { return m_haveAudioImporter; }
    
    
    

protected:

    /**** File handling code that we don't want the outside world to use ****/
    /**/
    /**/

    /**
     * Create document from a file
     */
    RosegardenDocument* createDocument(QString filePath, ImportType type = ImportRG4);
    
    
    
    /**
     * Create a document from RG file
     */
    RosegardenDocument* createDocumentFromRGFile(QString filePath);

    /**
     * Create document from MIDI file
     */
    RosegardenDocument* createDocumentFromMIDIFile(QString filePath);

    /**
     * Create document from RG21 file
     */
    RosegardenDocument* createDocumentFromRG21File(QString filePath);

    /**
     * Create document from Hydrogen drum machine file
     */
    RosegardenDocument* createDocumentFromHydrogenFile(QString filePath);

    /**/
    /**/
    /***********************************************************************/

    /**
     * Set the 'Rewind' and 'Fast Forward' buttons in the transport
     * toolbar to AutoRepeat
     */
    void setRewFFwdToAutoRepeat();

    static const void* SequencerExternal;

    /// Raise the transport along
    virtual void showEvent(QShowEvent*);

    /**
     * read general Options again and initialize all variables like
     * the recent file list
     */
    void readOptions();

    /**
     * add an item pointing to the example files in the QFileDialog speedbar
     */
    void setupFileDialogSpeedbar();

    /**
     * create menus and toolbars
     */
    void setupActions();

    void setupRecentFilesMenu();

    /**
     * sets up the zoom toolbar
     */
    void initZoomToolbar();

    /**
     * sets up the statusbar for the main window by initialzing a
     * statuslabel.
     */
    void initStatusBar();

    /**
     * creates the centerwidget of the KTMainWindow instance and sets
     * it as the view
     */
    void initView();

    /**
     * queryClose is called by KTMainWindow on each closeEvent of a
     * window. Against the default implementation (only returns true),
     * this calls saveModified() on the document object to ask if the
     * document shall be saved if Modified; on cancel the closeEvent
     * is rejected.
     *
     * @see KTMainWindow#queryClose
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryClose();

    /**
     * queryExit is called by KTMainWindow when the last window of the
     * application is going to be closed during the closeEvent().
     * Against the default implementation that just returns true, this
     * calls saveOptions() to save the settings of the last window's
     * properties.
     *
     * @see KTMainWindow#queryExit
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryExit();

    /**
     * saves the window properties for each open window during session
     * end to the session config file, including saving the currently
     * opened file by a temporary filename provided by KApplication.
     *
     * @see KTMainWindow#saveProperties
     */
    virtual void saveGlobalProperties();

    /**
     * reads the session config file and restores the application's
     * state including the last opened files and documents by reading
     * the temporary files saved by saveProperties()
     *
     * @see KTMainWindow#readProperties
     */
    virtual void readGlobalProperties();

    QString getAudioFilePath();

    /**
     * Show a sequencer error to the user.  This is for errors from
     * the framework code; the playback code uses mapped compositions
     * to send these things back asynchronously.
     */
    void showError(QString error);

    /*
     * Return AudioManagerDialog
     */
    AudioManagerDialog* getAudioManagerDialog() { return m_audioManagerDialog; }

    /**
     * Ask the user for a file to save to, and check that it's
     * good and that (if it exists) the user agrees to overwrite.
     * Return a null string if the write should not go ahead.
     */
    QString getValidWriteFileName(QString extension, QString label);

    /**
     * Find any non-ASCII strings in a composition that has been
     * generated by MIDI import or any other procedure that produces
     * events with unknown text encoding, and ask the user what
     * encoding to translate them from.  This assumes all text strings
     * in the composition are of the same encoding, and that it is not
     * (known to be) utf8 (in which case no transcoding would be
     * necessary).
     */
    void fixTextEncodings(Composition *);
    QTextCodec *guessTextCodec(std::string);

    /**
     * Set the current document
     *
     * Do all the needed housework when the current document changes
     * (like closing edit views, emitting documentChanged signal, etc...)
     */
    void setDocument(RosegardenDocument*);

    /**
     * Jog a selection of segments by an amount
     */
    void jogSelection(timeT amount);

    void createAndSetupTransport();

signals:
    void startupStatusMessage(QString message);

    /// emitted just before the document is changed
    void documentAboutToChange();

    /// emitted when the current document changes
    void documentChanged(RosegardenDocument*);

    /// emitted when the set of selected segments changes (relayed from RosegardenMainViewWidget)
    void segmentsSelected(const SegmentSelection &);

    /// emitted when the composition state (selected track, solo, etc...) changes
    void compositionStateUpdate();

    /// emitted when instrument parameters change (relayed from InstrumentParameterBox)
    void instrumentParametersChanged(InstrumentId);

    /// emitted when a plugin dialog selects a plugin
    void pluginSelected(InstrumentId, int, int);

    /// emitted when a plugin dialog (un)bypasses a plugin
    void pluginBypassed(InstrumentId, int, bool);

public slots:

    /**
     * open a URL - used for Dn'D
     *
     * @param url : a string containing a url (protocol://foo/bar/file.rg)
     */
    virtual void slotOpenDroppedURL(QString url);

    /**
     * Open the document properties dialog on the Audio page
     */
    virtual void slotOpenAudioPathSettings();

    /**
     * open a new application window by creating a new instance of
     * RosegardenMainWindow
     */
    void slotFileNewWindow();

    /**
     * clears the document in the actual view to reuse it as the new
     * document
     */
    void slotFileNew();

    /**
     * open a file and load it into the document
     */
    void slotFileOpen();

    /**
     * opens a file from the recent files menu (according to action name)
     */
    void slotFileOpenRecent();

    /**
     * save a document
     */
    void slotFileSave();

    /**
     * save a document by a new filename; if asTemplate is true, the file will
     * be saved read-only, to make it harder to overwrite by accident in the
     * future
     */
    bool slotFileSaveAs(bool asTemplate = false);
    void slotFileSaveAsTemplate() { slotFileSaveAs(true); }

    /**
     * asks for saving if the file is modified, then closes the actual
     * file and window
     */
    void slotFileClose();

    /**
     * print the actual file
     */
    void slotFilePrint();

    /**
     * print preview 
     */
    void slotFilePrintPreview();

    /**
     * Let the user select a Rosegarden Project file for import
     */
    void slotImportProject();

    /**
     * Let the user select a MIDI file for import
     */
    void slotImportMIDI();

    /**
     * Revert to last loaded file
     */
    void slotRevertToSaved();

    /**
     * Let the user select a Rosegarden 2.1 file for import 
     */
    void slotImportRG21();

    /**
     * Select a Hydrogen drum machine file for import
     */
    void slotImportHydrogen();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMerge();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMergeMIDI();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMergeRG21();

    /**
     * Select a Hydrogen drum machine file for merge
     */
    void slotMergeHydrogen();

    /**
     * Let the user export a Rosegarden Project file
     */
    void slotExportProject();

    /**
     * Let the user enter a MIDI file to export to
     */
    void slotExportMIDI();

    /**
     * Let the user enter a Csound scorefile to export to
     */
    void slotExportCsound();

    /**
     * Let the user enter a Mup file to export to
     */
    void slotExportMup();

    /**
     * Let the user enter a LilyPond file to export to
     */
    void slotExportLilyPond();

    /**
     * Export to a temporary file and process
     */
    void slotPrintLilyPond();
    void slotPreviewLilyPond();
    void slotLilyPondViewProcessExited(QProcess *);

    /**
     * Let the user enter a MusicXml file to export to
     */
    void slotExportMusicXml();

    /**
     * closes all open windows by calling close() on each memberList
     * item until the list is empty, then quits the application.  If
     * queryClose() returns false because the user canceled the
     * saveModified() dialog, the closing breaks.
     */
    void slotQuit();
    
    /**
     * put the marked text/object into the clipboard and remove * it
     * from the document
     */
    void slotEditCut();

    /**
     * put the marked text/object into the clipboard
     */
    void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    void slotEditPaste();
    
    /**
     * Cut a time range (sections of segments, tempo, and time
     * signature events within that range).
     */
    void slotCutRange();

    /**
     * Copy a time range.
     */
    void slotCopyRange();

    /**
     * Paste the clipboard at the current pointer position, moving all
     * subsequent material along to make space.
     */
    void slotPasteRange();
    
    /**
     * Delete a time range.
     */
    void slotDeleteRange();
    
    /**
     * Insert a time range (asking the user for a duration).
     */
    void slotInsertRange();

    /**
     * select all segments on all tracks
     */
    void slotSelectAll();

    /**
     * delete selected segments, duh
     */
    void slotDeleteSelectedSegments();

    /**
     * Quantize the selected segments (after asking the user how)
     */
    void slotQuantizeSelection();

    /**
     * Quantize the selected segments by repeating the last iterative quantize
     */
    void slotRepeatQuantizeSelection();

    /**
     * Calculate timing/tempo info based on selected segment
     */
    void slotGrooveQuantize();

    /**
     * Rescale the selected segments by a factor requested from
     * the user
     */
    void slotRescaleSelection();

    /**
     * Split the selected segments on silences (or new timesig, etc)
     */
    void slotAutoSplitSelection();

    /**
     * Jog a selection left or right by an amount
     */
    void slotJogRight();
    void slotJogLeft();

    /**
     * Split the selected segments by pitch
     */
    void slotSplitSelectionByPitch();

    /**
     * Split the selected segments by recorded source
     */
    void slotSplitSelectionByRecordedSrc();

    /**
     * Split the selected segments at some time
     */
    void slotSplitSelectionAtTime();

    /**
     * Produce a harmony segment from the selected segments
     */
    void slotHarmonizeSelection();

    /**
     * Set the start times of the selected segments
     */
    void slotSetSegmentStartTimes();

    /**
     * Set the durations of the selected segments
     */
    void slotSetSegmentDurations();

    /**
     * Merge the selected segments
     */
    void slotJoinSegments();

    /**
     * Tempo to Segment length
     */
    void slotTempoToSegmentLength();
    void slotTempoToSegmentLength(QWidget* parent);

    /**
     * toggle segment labels
     */
    void slotToggleSegmentLabels();
    
    /**
     * open the default editor for each of the currently-selected segments
     */
    void slotEdit();

    /**
     * open an event list view for each of the currently-selected segments
     */
    void slotEditInEventList();

    /**
     * open a matrix view for each of the currently-selected segments
     */
    void slotEditInMatrix();

    /**
     * open a percussion matrix view for each of the currently-selected segments
     */
    void slotEditInPercussionMatrix();

    /**
     * open a notation view with all currently-selected segments in it
     */
    void slotEditAsNotation();

    /**
     * open a tempo/timesig edit view
     */
    void slotEditTempos();
    void slotEditTempos(timeT openAtTime);

    /**
     * Edit the tempo - called from a Transport signal
     */
    void slotEditTempo();
    void slotEditTempo(timeT atTime);
    void slotEditTempo(QWidget *parent);
    void slotEditTempo(QWidget *parent, timeT atTime);

    /**
     * Edit the time signature - called from a Transport signal
     */
    void slotEditTimeSignature();
    void slotEditTimeSignature(timeT atTime);
    void slotEditTimeSignature(QWidget *parent);
    void slotEditTimeSignature(QWidget *parent, timeT atTime);

    /**
     * Edit the playback pointer position - called from a Transport signal
     */
    void slotEditTransportTime();
    void slotEditTransportTime(QWidget *parent);

    /**
     * Change the length of the composition
     */
    void slotChangeCompositionLength();

    /**
     * open a dialog for document properties
     */
    void slotEditDocumentProperties();

    /**
     * Manage MIDI Devices
     */
    void slotManageMIDIDevices();

    /**
     * Manage plugin synths
     */
    void slotManageSynths();

    /**
     * Show the mixers
     */
    void slotOpenAudioMixer();
    void slotOpenMidiMixer();
    
    /**
     * Edit Banks/Programs
     */
    void slotEditBanks();

    /**
     * Edit Banks/Programs for a particular device
     */
    void slotEditBanks(DeviceId);

    /**
     * Edit Control Parameters for a particular device
     */
    void slotEditControlParameters(DeviceId);

    /**
     * Edit Document Markers
     */
    void slotEditMarkers();

    /**
     * Not an actual action slot : populates the set_track_instrument sub menu
     */
    void slotPopulateTrackInstrumentPopup();

    /**
     * Remap instruments
     */
    void slotRemapInstruments();

    /**
     * Modify MIDI filters
     */
    void slotModifyMIDIFilters();

    /**
     * Manage Metronome
     */
    void slotManageMetronome();

    /**
     * Save Studio as Default
     */
    void slotSaveDefaultStudio();

    /**
     * Import Studio from File
     */
    void slotImportStudio();

    /**
     * Import Studio from Autoload
     */
    void slotImportDefaultStudio();

    /**
     * Import Studio from File
     */
    void slotImportStudioFromFile(const QString &file);

    /**
     * Send MIDI_RESET to all MIDI devices
     */
    void slotResetMidiNetwork();
    
    /**
     * toggles the toolbar
     */
    void slotToggleToolBar();

    /**
     * toggles the transport window
     */
    void slotToggleTransport();
    void slotToggleTransportVisibility();

    /**
     * hides the transport window
     */
    //### Not used -- use slotToggleTransport
    // void slotHideTransport();

    /**
     * toggles the tools toolbar
     */
    void slotToggleToolsToolBar();

    /**
     * toggles the tracks toolbar
     */
    void slotToggleTracksToolBar();

    /**
     * toggles the editors toolbar
     */
    void slotToggleEditorsToolBar();

    /**
     * toggles the transport toolbar
     */
    void slotToggleTransportToolBar();

    /**
     * toggles the zoom toolbar
     */
    void slotToggleZoomToolBar();

    /**
     * toggles the statusbar
     */
    void slotToggleStatusBar();

    /**
     * changes the statusbar contents for the standard label
     * permanently, used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusMsg(QString text);

    /**
     * changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(QString text);

    /**
     * enables/disables the transport window
     */
    void slotEnableTransport(bool);

    /**
     * segment select tool
     */
    void slotPointerSelected();

    /**
     * segment eraser tool is selected
     */
    void slotEraseSelected();
    
    /**
     * segment draw tool is selected
     */
    void slotDrawSelected();
    
    /**
     * segment move tool is selected
     */
    void slotMoveSelected();

    /**
     * segment resize tool is selected
     */
    void slotResizeSelected();

    /*
     * Segment join tool
     *
     */
    void slotJoinSelected();

    /*
     * Segment split tool
     *
     */
    void slotSplitSelected();

    /**
     * Add one new track
     */
    void slotAddTrack();

    /**
     * Add new tracks
     */
    void slotAddTracks();

    /*
     * Delete Tracks
     */
    void slotDeleteTrack();

    /*
     * Modify track position
     */
    void slotMoveTrackUp();
    void slotMoveTrackDown();

    /**
     * timeT version of the same
     */
    void slotSetPointerPosition(timeT t);

    /**
     * Set the pointer position and start playing (from LoopRuler)
     */
    void slotSetPlayPosition(timeT position);

    /**
     * Set a loop
     */
    void slotSetLoop(timeT lhs, timeT rhs);


    /**
     * Update the transport with the bar, beat and unit times for
     * a given timeT
     */
    void slotDisplayBarTime(timeT t);


    /**
     * Transport controls
     */
    void slotPlay();
    void slotStop();
    void slotRewind();
    void slotFastforward();
    void slotRecord();
    void slotToggleRecord();
    void slotRewindToBeginning();
    void slotFastForwardToEnd();
    void slotJumpToTime(RealTime);
    void slotStartAtTime(RealTime);
    void slotRefreshTimeDisplay();
    void slotToggleTracking();

    /**
     * Called when the sequencer auxiliary process exits
     */
    //void slotSequencerExited(QProcess*); QThread::finished has no QProcess param
    //  current slotSequenceExited implementation doesn't use (or name!) this param
    void slotSequencerExited();

    /// When the transport closes 
    void slotCloseTransport();

    /**
     * called by RosegardenApplication when session management tells
     * it to save its state. This is to avoid saving the transport as
     * a 2nd main window
     */
    void slotDeleteTransport();
    
    /**
     * Put the GUI into a given Tool edit mode
     */
    void slotActivateTool(QString toolName);

    /**
     * Toggles either the play or record metronome according
     * to Transport status
     */
    void slotToggleMetronome();

    /*
     * Toggle the solo mode
     */
    void slotToggleSolo(bool);

    /**
     * Set and unset the loop from the transport loop button with
     * these slots.
     */
    void slotSetLoop();
    void slotUnsetLoop();

    /**
     * Set and unset the loop start/end time from the transport loop start/stop buttons with
     * these slots.
     */
    void slotSetLoopStart();
    void slotSetLoopStop();

    /**
     * Toggle the track labels on the TrackEditor
     */
    void slotToggleTrackLabels();

    /**
     * Toggle the rulers on the TrackEditor
     * (aka bar buttons)
     */
    void slotToggleRulers();

    /**
     * Toggle the tempo ruler on the TrackEditor
     */
    void slotToggleTempoRuler();

    /**
     * Toggle the chord-name ruler on the TrackEditor
     */
    void slotToggleChordNameRuler();

    /**
     * Toggle the segment canvas previews
     */
    void slotTogglePreviews();

    /**
     * Re-dock the parameters box to its initial position
     */
    void slotDockParametersBack();

    /**
     * The parameters box was closed
     */
    void slotParametersClosed();

    /**
     * The parameters box was docked back
     */
//    void slotParametersDockedBack(QDockWidget*, QDockWidget::DockPosition);    //&&& restore DockPosition
    void slotParametersDockedBack(QDockWidget*, int );    

    /**
     * Display tip-of-day dialog on demand
     */
    void slotShowTip();

    /*
     * Select Track up or down
     */
    void slotTrackUp();
    void slotTrackDown();

    /**
     * Mute/Unmute
     */
    void slotMuteAllTracks();
    void slotUnmuteAllTracks();
    void slotToggleMutedCurrentTrack();
    
    /**
     * Toggle arm (record) current track
     */
    void slotToggleRecordCurrentTrack();
    
    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */         
    void slotSaveOptions();

    /**
     * Show the configure dialog
     */
    void slotConfigure();

    /**
     * Show the key mappings
     *
     */
    void slotEditKeys();

    /**
     * Edit toolbars
     */
    void slotEditToolbars();

    /**
     * Update the toolbars after edition
     */
    void slotUpdateToolbars();

    /**
     * Zoom slider moved
     */
    void slotChangeZoom(int index);

    void slotZoomIn();
    void slotZoomOut();

    /**
     * Modify tempo
     */
    void slotChangeTempo(timeT time,
                         tempoT value,      
                         tempoT target,
                         TempoDialog::TempoDialogAction action);

    /**
     * Move a tempo change
     */
    void slotMoveTempo(timeT oldTime,
                       timeT newTime);

    /**
     * Remove a tempo change
     */
    void slotDeleteTempo(timeT time);

    /**
     * Add marker
     */
    void slotAddMarker(timeT time);

    /**
     * Remove a marker
     */
    void slotDeleteMarker(int id,
                          timeT time,
                          QString name,
                          QString description);

    /**
     * Document modified
     */
    void slotDocumentModified(bool modified = true);


    /**
     * This slot is here to be connected to RosegardenMainViewWidget's
     * stateChange signal. We use a bool for the 2nd arg rather than a
     * KXMLGUIClient::ReverseStateChange to spare the include of
     * kxmlguiclient.h just for one typedef.
     *
     * Hopefully we'll be able to get rid of this eventually,
     * I should slip this in KMainWindow for KDE 4.
     */
    void slotStateChanged(QString, bool noReverse);

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    void slotTestClipboard();

    /**
     * Show a 'play list' dialog
     */
    void slotPlayList();

    /**
     * Play the requested URL
     *
     * Stop current playback, close current document,
     * open specified document and play it.
     */
    void slotPlayListPlay(QString url);

    /**
     * Call up the online tutorial
     */
    void slotTutorial();

    /**
     * Surf to the bug reporting guidelines
     */
    void slotBugGuidelines();
    
    /**
     * View the trigger segments manager
     */
    void slotManageTriggerSegments();

    /**
     * View the audio file manager - and some associated actions
     */
    void slotAudioManager();

    void slotAddAudioFile(AudioFileId);
    void slotDeleteAudioFile(AudioFileId);
    void slotPlayAudioFile(AudioFileId,
                           const RealTime &,
                           const RealTime &);
    void slotCancelAudioPlayingFile(AudioFileId);
    void slotDeleteAllAudioFiles();

    /**
     * Reflect segment deletion from the audio manager
     */
    void slotDeleteSegments(const SegmentSelection&);

    void slotRepeatingSegments();
    void slotRelabelSegments();
    void slotTransposeSegments();
    
    /// Panic button pressed
    void slotPanic();

    // Auto-save
    //
    void slotAutoSave();

    // Auto-save update interval changes
    //
    void slotUpdateAutoSaveInterval(unsigned int interval);

    // Update the side-bar when the configuration page changes its style.
    //
    void slotUpdateSidebarStyle(unsigned int style);

    /**
     * called when the PlayList is being closed
     */
    void slotPlayListClosed();

    /**
     * called when the BankEditor is being closed
     */
    void slotBankEditorClosed();

    /**
     * called when the Device Manager is being closed
     */
    void slotDeviceManagerClosed();

    /**
     * called when the synth manager is being closed
     */
    void slotSynthPluginManagerClosed();

    /**
     * called when the Mixer is being closed
     */
    void slotAudioMixerClosed();
    void slotMidiMixerClosed();

    /**
     * when ControlEditor is being closed
     */
    void slotControlEditorClosed();

    /**
     * when MarkerEditor is being closed
     */
    void slotMarkerEditorClosed();

    /**
     * when TempoView is being closed
     */
    void slotTempoViewClosed();

    /**
     * when TriggerManager is being closed
     */
    void slotTriggerManagerClosed();

    /**
     * when AudioManagerDialog is being closed
     */
    void slotAudioManagerClosed();

    /**
     * Update the pointer position from the sequencer mmapped file when playing
     */
    void slotUpdatePlaybackPosition();

    /**
     * Update the CPU level meter
     */
    void slotUpdateCPUMeter(bool playing);

    /**
     * Update the monitor levels from the sequencer mmapped file when not playing
     * (slotUpdatePlaybackPosition does this among other things when playing)
     */
    void slotUpdateMonitoring();

    void slotCheckTransportStatus();

    /**
     * Create a plugin dialog for a given instrument and slot, or
     * raise an exising one.
     */
    void slotShowPluginDialog(QWidget *parent,
                              InstrumentId instrument,
                              int index);

    void slotPluginSelected(InstrumentId instrument,
                            int index, int plugin);

    /**
     * An external GUI has requested a port change.
     */
    void slotChangePluginPort(InstrumentId instrument,
                              int index, int portIndex, float value);

    /**
     * Our internal GUI has made a port change -- the
     * PluginPortInstance already contains the new value, but we need
     * to inform the sequencer and update external GUIs.
     */
    void slotPluginPortChanged(InstrumentId instrument,
                               int index, int portIndex);

    /**
     * An external GUI has requested a program change.
     */
    void slotChangePluginProgram(InstrumentId instrument,
                                 int index, QString program);

    /**
     * Our internal GUI has made a program change -- the
     * AudioPluginInstance already contains the new program, but we
     * need to inform the sequencer, update external GUIs, and update
     * the port values for the new program.
     */
    void slotPluginProgramChanged(InstrumentId instrument,
                                  int index);

    /**
     * An external GUI has requested a configure call.  (This can only
     * happen from an external GUI, we have no way to manage these
     * internally.)
     */
    void slotChangePluginConfiguration(InstrumentId, int index,
                                       bool global, QString key, QString value);
    void slotPluginDialogDestroyed(InstrumentId instrument,
                                   int index);
    void slotPluginBypassed(InstrumentId,
                            int index, bool bypassed);

    void slotShowPluginGUI(InstrumentId, int index);
    void slotStopPluginGUI(InstrumentId, int index);
    void slotPluginGUIExited(InstrumentId, int index);

    void slotDocumentDevicesResyncd();

    void slotTestStartupTester();
    
    void slotDebugDump();

    /**
     * Enable or disable the internal MIDI Thru routing. 
     * 
     * This policy is implemented at the sequencer side, controlled 
     * by this flag and also by the MIDI Thru filters.
     * 
     * @see ControlBlock::isMidiRoutingEnabled()
     * @see RosegardenSequencerApp::processAsynchronousEvents()
     * @see RosegardenSequencerApp::processRecordedEvents()
     */
    void slotEnableMIDIThruRouting();

    void slotShowToolHelp(const QString &);

    void slotNewerVersionAvailable(QString);
    
    void slotSetQuickMarker();
    
    void slotJumpToQuickMarker();    

    
    void slotOpenDeviceManagerNew();
private:


    //--------------- Data members ---------------------------------

    bool m_actionsSetup;

    /**
     * view is the main widget which represents your working area. The
     * View class should handle all events of the view widget.  It is
     * kept empty so you can create your view according to your
     * application's needs by changing the view class.
     */
    RosegardenMainViewWidget* m_view;
    RosegardenMainViewWidget* m_swapView;

    QDockWidget *m_mainDockWidget;
    QDockWidget* m_dockLeft;
    

    /**
     * doc represents your actual document and is created only
     * once. It keeps information such as filename and does the
     * serialization of your files.
     */
    RosegardenDocument* m_doc;
    
    
    /**
     *    Menus
     */
    RecentFiles m_recentFiles;
    
    SequencerThread *m_sequencerThread;
    bool m_sequencerCheckedIn;

#ifdef HAVE_LIBJACK
    QProcess* m_jackProcess;
#endif // HAVE_LIBJACK

    ZoomSlider<double> *m_zoomSlider;
    QLabel             *m_zoomLabel;

    ProgressBar *m_progressBar;
    
    
    QLabel* m_statusBarLabel1;
    // SequenceManager
    //
    SequenceManager *m_seqManager;

    // Transport dialog pointer
    //
    TransportDialog *m_transport;

    // Dialogs which depend on the document

    // Audio file manager
    //
    AudioManagerDialog *m_audioManagerDialog;

    bool m_originatingJump;

    // Use these in conjucntion with the loop button to
    // remember where a loop was if we've ever set one.
    timeT m_storedLoopStart;
    timeT m_storedLoopEnd;

    bool m_useSequencer;
    bool m_dockVisible;

    AudioPluginManager *m_pluginManager;

    DevicesManagerNew * m_devicesManagerNew;
    
    QTimer* m_autoSaveTimer;

    Clipboard *m_clipboard;

    SegmentParameterBox           *m_segmentParameterBox;
    InstrumentParameterBox        *m_instrumentParameterBox;
    TrackParameterBox             *m_trackParameterBox;

    PlayListDialog        *m_playList;
    DeviceManagerDialog   *m_deviceManager;
    SynthPluginManagerDialog *m_synthManager;
    AudioMixerWindow      *m_audioMixer;
    MidiMixerWindow       *m_midiMixer;
    BankEditorDialog      *m_bankEditor;
    MarkerEditor          *m_markerEditor;
    TempoView             *m_tempoView;
    TriggerSegmentManager *m_triggerSegmentManager;
    std::set<ControlEditorDialog *> m_controlEditors;
    std::map<int, AudioPluginDialog*> m_pluginDialogs;
#ifdef HAVE_LIBLO
    AudioPluginOSCGUIManager *m_pluginGUIManager;
#endif

    static RosegardenMainWindow *m_myself;

    static std::map<QProcess *, QTemporaryFile *> m_lilyTempFileMap;

    // Used to fetch the current sequencer position from the mmapped sequencer information file
    //
    QTimer *m_playTimer;
    QTimer *m_stopTimer;

    StartupTester *m_startupTester;

    bool m_firstRun;
    bool m_haveAudioImporter;

    RosegardenParameterArea *m_parameterArea;
    
#ifdef HAVE_LIRC        
    LircClient *m_lircClient;
    LircCommander *m_lircCommander;
#endif     
};


}

#endif
