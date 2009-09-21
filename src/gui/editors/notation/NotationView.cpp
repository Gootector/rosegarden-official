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

#include "NotationView.h"

#include "NotationWidget.h"
#include "NotationScene.h"
#include "NotationCommandRegistry.h"
#include "NoteStyleFactory.h"
#include "NoteFontFactory.h"
#include "NotationStrings.h"
#include "NoteRestInserter.h"
#include "NotationSelector.h"
#include "HeadersGroup.h"

#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"

#include "misc/ConfigGroups.h"

#include "base/Clipboard.h"
#include "base/Selection.h"
#include "base/NotationQuantizer.h"
#include "base/NotationTypes.h"
#include "base/NotationRules.h"
#include "base/BaseProperties.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/AnalysisTypes.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"

#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/CutAndCloseCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/InsertTriggerNoteCommand.h"
#include "commands/edit/SetTriggerCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/RescaleCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/edit/InvertCommand.h"
#include "commands/edit/RetrogradeCommand.h"
#include "commands/edit/RetrogradeInvertCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/SetLyricsCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"

#include "commands/notation/InterpretCommand.h"
#include "commands/notation/ClefInsertionCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/MultiKeyInsertionCommand.h"
#include "commands/notation/SustainInsertionCommand.h"
#include "commands/notation/TupletCommand.h"

#include "commands/segment/PasteToTriggerSegmentCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/segment/SegmentSyncCommand.h"

#include "gui/dialogs/PasteNotationDialog.h"
#include "gui/dialogs/InterpretDialog.h"
#include "gui/dialogs/MakeOrnamentDialog.h"
#include "gui/dialogs/UseOrnamentDialog.h"
#include "gui/dialogs/ClefDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/KeySignatureDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/TupletDialog.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/LyricEditDialog.h"

#include "gui/general/IconLoader.h"
#include "gui/general/LilyPondProcessor.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/general/ClefIndex.h"

#include "gui/widgets/TmpStatusMsg.h"
#include "gui/widgets/ProgressBar.h"

#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"

#include "gui/editors/parameters/TrackParameterBox.h"

#include "document/io/LilyPondExporter.h"

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryFile>
#include <QToolBar>
#include <QInputDialog>
#include <QStatusBar>
#include <QProgressBar>

#include <algorithm>

namespace Rosegarden
{

using namespace Accidentals;

NewNotationView::NewNotationView(RosegardenDocument *doc,
                                 std::vector<Segment *> segments,
                                 QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_document(doc),
    m_durationPressed(0),
    m_accidentalPressed(0),
    m_durationMode(InsertingRests) //Fool morphDurationToolbar during first call
{
    m_notationWidget = new NotationWidget();
    setCentralWidget(m_notationWidget);
    m_notationWidget->setSegments(doc, segments);

    // Many actions are created here
    m_commandRegistry = new NotationCommandRegistry(this);

    setupActions();
    createGUI("notation.rc");
    slotUpdateMenuStates();

    setIcon(IconLoader().loadPixmap("window-notation"));

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdateMenuStates()));

    connect(m_notationWidget->getScene(), SIGNAL(selectionChanged()),
            this, SLOT(slotUpdateMenuStates()));

    //Initialize NoteRestInserter and DurationToolbar
    initializeNoteRestInserter();

    // Determine default action stolen from MatrixView.cpp
    // Toggle the desired tool off and then trigger it on again, to
    // make sure its signal is called at least once (as would not
    // happen if the tool was on by default otherwise)
    QAction *toolAction = 0;
    if (!m_notationWidget->segmentsContainNotes()) {
        toolAction = findAction("draw");
    } else {
        toolAction = findAction("select");
    }
    if (toolAction) {
        NOTATION_DEBUG << "initial state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
        if (toolAction->isChecked()) toolAction->toggle();
        NOTATION_DEBUG << "newer state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
        toolAction->trigger();
        NOTATION_DEBUG << "newest state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
    }

    // Set display configuration
    bool visible;
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    // Set initial notation layout mode
    int layoutMode = settings.value("layoutmode", 0).toInt();
    switch(layoutMode) {
        case 0 :
            findAction("linear_mode")->setChecked(true);
            findAction("continuous_page_mode")->setChecked(false);
            findAction("multi_page_mode")->setChecked(false);
            slotLinearMode();
        break;
        case 1 :
            findAction("linear_mode")->setChecked(false);
            findAction("continuous_page_mode")->setChecked(true);
            findAction("multi_page_mode")->setChecked(false);
            slotContinuousPageMode();
        break;
        case 2 : 
            findAction("linear_mode")->setChecked(false);
            findAction("continuous_page_mode")->setChecked(false);
            findAction("multi_page_mode")->setChecked(true);
            slotMultiPageMode(); 
        break;
    }

    // Set initial visibility of chord name ruler, ...
    visible = settings.value("Chords ruler shown",
                          findAction("show_chords_ruler")->isChecked()
                         ).toBool();
    findAction("show_chords_ruler")->setChecked(visible);
    m_notationWidget->setChordNameRulerVisible(visible);

    // ... raw note ruler, ...
    visible = settings.value("Raw note ruler shown",
                          findAction("show_raw_note_ruler")->isChecked()
                         ).toBool();
    findAction("show_raw_note_ruler")->setChecked(visible);
    m_notationWidget->setRawNoteRulerVisible(visible);

    // ... tempo ruler ...
    visible = settings.value("Tempo ruler shown",
                          findAction("show_tempo_ruler")->isChecked()
                         ).toBool();
    findAction("show_tempo_ruler")->setChecked(visible);
    m_notationWidget->setTempoRulerVisible(visible);

    // ... and staff headers.
    visible = settings.value("shownotationheader").toInt()
              != HeadersGroup::ShowNever;
        // ShowWhenNeeded and ShowAlways not used yet ...
    m_notationWidget->setHeadersVisible(visible);

    settings.endGroup();

    initStatusBar();

    updateWindowTitle();
    connect(m_document, SIGNAL(documentModified(bool)),
            this, SLOT(updateWindowTitle(bool)));

    // Restore window geometry
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Notation_View").toByteArray());
    settings.endGroup();
}

NewNotationView::~NewNotationView()
{
    //!!! Odd that the window geometry save bit didn't work in here.  The little
    // message printed to std::cerr didn't fire, so unless I'm just really
    // obtuse and illiterate about something, I don't think this dtor ever fires
    // and we probably have a memory leak.
    delete m_commandRegistry;
}

void
NewNotationView::closeEvent(QCloseEvent *event)
{
    // Save window geometry
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    std::cerr << "storing window geometry for notation view" << std::endl;
    settings.setValue("Notation_View", this->saveGeometry());
    settings.endGroup();

    QWidget::closeEvent(event);
}

void
NewNotationView::setupActions()
{
    //setup actions common to all views.
    setupBaseActions(true);

    //"file" MenuBar menu
    // "file_save"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("file_print_lilypond", SLOT(slotPrintLilyPond()));
    createAction("file_preview_lilypond", SLOT(slotPreviewLilyPond()));

    // "file_close"
    // Created in EditViewBase::setupActions() via creatAction()

    // "edit" MenuBar menu
    // "edit_undo"
    // Created in EditViewBase::setupActions() via creatAction()

    // "edit_redo"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("cut_and_close", SLOT(slotEditCutAndClose()));
    createAction("general_paste", SLOT(slotEditGeneralPaste()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("move_events_up_staff", SLOT(slotMoveEventsUpStaff()));
    createAction("move_events_down_staff", SLOT(slotMoveEventsDownStaff()));
    createAction("select_from_start", SLOT(slotEditSelectFromStart()));
    createAction("select_to_end", SLOT(slotEditSelectToEnd()));
    createAction("select_whole_staff", SLOT(slotEditSelectWholeStaff()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("filter_selection", SLOT(slotFilterSelection()));
    
    //"view" MenuBar menu
    // "note_font_actionmenu" subMenu
    // Custom Code. Coded below.

    // "note_font_size_actionmenu" subMenu
    // Custom Code. Coded below.
    
    // "stretch_actionmenu" subMenu
    // Custom Code. Coded below.
    // Code deactivated.

    // "proportion_actionmenu" subMenu
    // Custom Code. Coded below.
    // Code deactivated.

    // "layout" submenu 
    createAction("linear_mode", SLOT(slotLinearMode()));
    createAction("continuous_page_mode", SLOT(slotContinuousPageMode()));
    createAction("multi_page_mode", SLOT(slotMultiPageMode()));

    createAction("lyric_editor", SLOT(slotEditLyrics()));
    createAction("show_velocity_control_ruler", SLOT(slotShowVelocityControlRuler()));

    // "add_control_ruler" subMenu
    // was disabled in kde3 version:
    // createAction("add_control_ruler", SLOT(slotShowPropertyControlRuler()));

    createAction("show_track_headers", SLOT(slotShowHeadersGroup()));

    //"document" Menubar menu
    createAction("add_tempo", SLOT(slotAddTempo()));
    createAction("add_time_signature", SLOT(slotAddTimeSignature()));

    //"segment" Menubar menu
    // "open-with" subMenu
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("add_clef", SLOT(slotEditAddClef()));
    createAction("add_key_signature", SLOT(slotEditAddKeySignature()));
    createAction("add_sustain_down", SLOT(slotEditAddSustainDown()));
    createAction("add_sustain_up", SLOT(slotEditAddSustainUp()));

    // "set_segment_start"
    // Created in EditViewBase::setupActions() via creatAction()
    
    // "set_segment_duration"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("transpose_segment", SLOT(slotEditTranspose()));
    createAction("switch_preset", SLOT(slotEditSwitchPreset()));

    //"Notes" Menubar menu

    // "Marks" subMenu
    //Created in Constructor via NotationCommandRegistry()
    //with AddMarkCommand::registerCommand()
    //with RemoveMarksCommand::registerCommand()

    // "ornaments" subMenu
    createAction("use_ornament", SLOT(slotUseOrnament()));
    createAction("make_ornament", SLOT(slotMakeOrnament()));
    createAction("remove_ornament", SLOT(slotRemoveOrnament()));

    // "Fingering" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with AddFingeringMarkCommand::registerCommand()
    // with RemoveFingeringMarksCommand::registerCommand()

    // "Slashes" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with AddSlashesCommand::registerCommand()

    // "note_style_actionmenu" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStyleCommand::registerCommand()
    // actionCreate really should be a custon code. Oh, well.


    // "Respell" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with RespellCommand::registerCommand()
    
    // "stem_up"
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStemsCommand::registerCommand()
    
    // "stem_down"
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStemsCommand::registerCommand()
    
    // "restore_stems"
    // Created in Constructor via NotationCommandRegistry()
    // with RestoreStemsCommand::registerCommand();

    //"Phrase" Menubar menu
    // "make_chord"
    // Created in Constructor via NotationCommandRegistry()
    // with MakeChordCommand::registerCommand();

    // "beam"
    // Created in Constructor via NotationCommandRegistry()
    // with BeamCommand::registerCommand();

    // "auto_beam"
    // Created in Constructor via NotationCommandRegistry()
    // with AutoBeamCommand::registerCommand();

    // "break_group"
    // Created in Constructor via NotationCommandRegistry()
    // with BreakCommand::registerCommand();

    // "remove_indications"

    createAction("simple_tuplet", SLOT(slotGroupSimpleTuplet()));
    createAction("tuplet", SLOT(slotGroupGeneralTuplet()));

    //Where are "break_tuplet", "slur", & "phrasing_slur" created?
    
    //"Slurs" subMenu
    //where are "restore_slurs", "slurs_above", "slurs_below" created?

    //Where are "tie_notes", "untie_notes", created?

    //"Ties" subMenu
    //"restore_ties", "ties_above", & "ties_below" created?
    
    //Where are "crescendo" & "decrescendo" created?

    //"octaves" subMenu
    //Where are "octave_2up", "octave_up", "octave_down",
    //"octave_down", & "octave_2down" created?

    //Actions first appear in "Adjust" Menubar menu

    //"rests" subMenu
    createAction("normalize_rests", SLOT(slotTransformsNormalizeRests()));
    //Where is "collapse_rests_aggresively" created?

    //"transform_notes" subMenu
    createAction("collapse_notes", SLOT(slotTransformsCollapseNotes()));
    //Where are "make_notes_viable" & "de_counterpoint" created?

    //Quantitize subMenu
    createAction("quantize", SLOT(slotTransformsQuantize()));
    //Where are "fix_quantization" & "remove_quantization" created?

    createAction("interpret", SLOT(slotTransformsInterpret()));

    //"Rescale" subMenu
    createAction("halve_durations", SLOT(slotHalveDurations()));
    createAction("double_durations", SLOT(slotDoubleDurations()));
    createAction("rescale", SLOT(slotRescale()));

    //"Transpose" subMenu
    createAction("transpose_up", SLOT(slotTransposeUp()));
    createAction("transpose_down", SLOT(slotTransposeDown()));
    createAction("transpose_up_octave", SLOT(slotTransposeUpOctave()));
    createAction("transpose_down_octave", SLOT(slotTransposeDownOctave()));
    createAction("general_transpose", SLOT(slotTranspose()));
    createAction("general_diatonic_transpose", SLOT(slotDiatonicTranspose()));

    //"Convert" subMenu
    createAction("invert", SLOT(slotInvert()));
    createAction("retrograde", SLOT(slotRetrograde()));
    createAction("retrograde_invert", SLOT(slotRetrogradeInvert()));

    //"velocities" subMenu
    createAction("velocity_up", SLOT(slotVelocityUp()));
    createAction("velocity_down", SLOT(slotVelocityDown()));
    createAction("set_velocities", SLOT(slotSetVelocities()));

    //"fine_positioning" subMenu
    //Where are "fine_position_restore", "fine_position_left",
    //"fine_position_right", "fine_position_up" & 
    //"fine_position_down" created?

    //"fine_timing" subMenu
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));

    //"visibility" subMenu
    //Where are "make_invisible" & "make_visible" created?

    //Actions first appear in "Tools" Menubar menu
    createAction("select", SLOT(slotSetSelectTool()));
    createAction("erase", SLOT(slotSetEraseTool()));
    createAction("draw", SLOT(slotSetNoteRestInserter()));

    // These actions do as their names imply, and in this case, the toggle will
    // call one or the other of these
    // These rely on .rc script keeping the right state visible
    createAction("switch_to_rests", SLOT(slotSwitchToRests()));
    createAction("switch_to_notes", SLOT(slotSwitchToNotes()));

    // These actions always just pass straight to the toggle.
    // These rely on .rc script keeping the right state visible
    createAction("switch_dots_on", SLOT(slotToggleDot()));
    createAction("switch_dots_off", SLOT(slotToggleDot()));

    // Menu uses now "Switch to Notes", "Switch to Rests" and "Durations".
    createAction("duration_breve", SLOT(slotNoteAction()));
    createAction("duration_semibreve", SLOT(slotNoteAction()));
    createAction("duration_minim", SLOT(slotNoteAction()));
    createAction("duration_crotchet", SLOT(slotNoteAction()));
    createAction("duration_quaver", SLOT(slotNoteAction()));
    createAction("duration_semiquaver", SLOT(slotNoteAction()));
    createAction("duration_demisemi", SLOT(slotNoteAction()));
    createAction("duration_hemidemisemi", SLOT(slotNoteAction()));
    createAction("duration_dotted_breve", SLOT(slotNoteAction()));
    createAction("duration_dotted_semibreve", SLOT(slotNoteAction()));
    createAction("duration_dotted_minim", SLOT(slotNoteAction()));
    createAction("duration_dotted_crotchet", SLOT(slotNoteAction()));
    createAction("duration_dotted_quaver", SLOT(slotNoteAction()));
    createAction("duration_dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("duration_dotted_demisemi", SLOT(slotNoteAction()));
    createAction("duration_dotted_hemidemisemi", SLOT(slotNoteAction()));

    // since we can't create toolbars with disabled icons, and to avoid having
    // to draw a lot of fancy icons for disabled durations, we have this dummy
    // filler to keep spacing the same across all toolbars, and there have to
    // two of them
    createAction("dummy_1", SLOT());

    //"NoteTool" subMenu
    //NEED to create action methods
    createAction("breve", SLOT(slotNoteAction()));
    createAction("semibreve", SLOT(slotNoteAction()));
    createAction("minim", SLOT(slotNoteAction()));
    createAction("crotchet", SLOT(slotNoteAction()));
    createAction("quaver", SLOT(slotNoteAction()));
    createAction("semiquaver", SLOT(slotNoteAction()));
    createAction("demisemi", SLOT(slotNoteAction()));
    createAction("hemidemisemi", SLOT(slotNoteAction()));
    createAction("dotted_breve", SLOT(slotNoteAction()));
    createAction("dotted_semibreve", SLOT(slotNoteAction()));
    createAction("dotted_minim", SLOT(slotNoteAction()));
    createAction("dotted_crotchet", SLOT(slotNoteAction()));
    createAction("dotted_quaver", SLOT(slotNoteAction()));
    createAction("dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("dotted_demisemi", SLOT(slotNoteAction()));
    createAction("dotted_hemidemisemi", SLOT(slotNoteAction()));

    //"RestTool" subMenu
    //NEED to create action methods
    createAction("rest_breve", SLOT(slotNoteAction()));
    createAction("rest_semibreve", SLOT(slotNoteAction()));
    createAction("rest_minim", SLOT(slotNoteAction()));
    createAction("rest_crotchet", SLOT(slotNoteAction()));
    createAction("rest_quaver", SLOT(slotNoteAction()));
    createAction("rest_semiquaver", SLOT(slotNoteAction()));
    createAction("rest_demisemi", SLOT(slotNoteAction()));
    createAction("rest_hemidemisemi", SLOT(slotNoteAction()));
    createAction("rest_dotted_breve", SLOT(slotNoteAction()));
    createAction("rest_dotted_semibreve", SLOT(slotNoteAction()));
    createAction("rest_dotted_minim", SLOT(slotNoteAction()));
    createAction("rest_dotted_crotchet", SLOT(slotNoteAction()));
    createAction("rest_dotted_quaver", SLOT(slotNoteAction()));
    createAction("rest_dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("rest_dotted_demisemi", SLOT(slotNoteAction()));
    createAction("rest_dotted_hemidemisemi", SLOT(slotNoteAction()));

    //"Accidentals" submenu
    createAction("no_accidental", SLOT(slotNoAccidental()));
    createAction("follow_accidental", SLOT(slotFollowAccidental()));
    createAction("sharp_accidental", SLOT(slotSharp()));
    createAction("flat_accidental", SLOT(slotFlat()));
    createAction("natural_accidental", SLOT(slotNatural()));
    createAction("double_sharp_accidental", SLOT(slotDoubleSharp()));
    createAction("double_flat_accidental", SLOT(slotDoubleFlat()));
    
    //JAS "Clefs" subMenu
    createAction("treble_clef", SLOT(slotClefAction()));
    createAction("alto_clef", SLOT(slotClefAction()));
    createAction("tenor_clef", SLOT(slotClefAction()));
    createAction("bass_clef", SLOT(slotClefAction()));

    createAction("text", SLOT(slotText()));
    createAction("guitarchord", SLOT(slotGuitarChord()));

    // "Symbols" (sub)Menu
    createAction("add_segno", SLOT(slotSymbolAction()));
    createAction("add_coda", SLOT(slotSymbolAction()));
    createAction("add_breath", SLOT(slotSymbolAction()));

    //JAS "Move" subMenu
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));

    createAction("cursor_up_staff", SLOT(slotCurrentStaffUp()));
    createAction("cursor_down_staff", SLOT(slotCurrentStaffDown()));
    createAction("cursor_prior_segment", SLOT(slotCurrentSegmentPrior()));
    createAction("cursor_next_segment", SLOT(slotCurrentSegmentNext()));

    //"Transport" subMenu
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("cursor_back", SLOT(slotStepBackward()));
    createAction("cursor_forward", SLOT(slotStepForward()));    
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("toggle_solo", SLOT(slotToggleSolo()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SIGNAL(panic()));

    //"insert_note_actionmenu" coded below. 

    createAction("chord_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("triplet_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("grace_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("toggle_step_by_step", SLOT(slotToggleStepByStep()));

    //Actions first appear in "settings" Menubar menu
    //"toolbars" subMenu
    //Where is "options_show_toolbar" created?
    createAction("show_general_toolbar", SLOT(slotToggleGeneralToolBar()));
    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_duration_toolbar", SLOT(slotToggleDurationToolBar()));
    createAction("show_accidentals_toolbar", SLOT(slotToggleAccidentalsToolBar()));
    createAction("show_clefs_toolbar", SLOT(slotToggleClefsToolBar()));
    createAction("show_marks_toolbar", SLOT(slotToggleMarksToolBar()));
    createAction("show_group_toolbar", SLOT(slotToggleGroupToolBar()));
    createAction("show_symbol_toolbar", SLOT(slotToggleSymbolsToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_layout_toolbar", SLOT(slotToggleLayoutToolBar()));

    //"rulers" subMenu
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_raw_note_ruler", SLOT(slotToggleRawNoteRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));

    createAction("show_annotations", SLOT(slotToggleAnnotations()));
    createAction("show_lilypond_directives", SLOT(slotToggleLilyPondDirectives()));

    createAction("lilypond_directive", SLOT(slotLilyPondDirective()));
    createAction("debug_dump", SLOT(slotDebugDump()));
    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    //!!! not here yet createAction("move_selection_left", SLOT(slotMoveSelectionLeft()));
    //&&& NB Play has two shortcuts (Enter and Ctrl+Return) -- need to
    // ensure both get carried across somehow
    createAction("add_dot", SLOT(slotAddDot()));
    createAction("add_notation_dot", SLOT(slotAddDotNotationOnly()));

    //JAS actions copied from EditView::setupActions()
// was disabled in kde3 version:
// createAction("show_controller_events_ruler", SLOT(slotShowControllerEventsRuler()));
// was disabled in kde3 version:
// createAction("add_control_ruler", SLOT(slotShowPropertyControlRuler()));
    createAction("insert_control_ruler_item", SLOT(slotInsertControlRulerItem()));
    createAction("erase_control_ruler_item", SLOT(slotEraseControlRulerItem()));
    createAction("clear_control_ruler_item", SLOT(slotClearControlRulerItem()));
    createAction("start_control_line_item", SLOT(slotStartControlLineItem()));
    createAction("flip_control_events_forward", SLOT(slotFlipForwards()));
    createAction("flip_control_events_back", SLOT(slotFlipBackwards()));
    createAction("draw_property_line", SLOT(slotDrawPropertyLine()));
    createAction("select_all_properties", SLOT(slotSelectAllProperties()));

    //JAS insert note section is a rewrite
    //JAS from EditView::createInsertPitchActionMenu()
    for (int octave = 0; octave <= 2; ++octave) {
        QString octaveSuffix;
        if (octave == 1) octaveSuffix = "_high";
        else if (octave == 2) octaveSuffix = "_low";

        createAction(QString("insert_0%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_0_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_2_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_2%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_3%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_3_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_6_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_6%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
    }
    createAction(QString("insert_rest"),SLOT(slotInsertRestFromAction()));

    std::set<QString> fs(NoteFontFactory::getFontNames());
    std::vector<QString> f;
    for (std::set<QString>::const_iterator i = fs.begin(); i != fs.end(); ++i) {
        f.push_back(*i);
    }
    std::sort(f.begin(), f.end());

    // Custom Note Font Menu creator
    QMenu *fontActionMenu = new QMenu(tr("Note &Font"), this); 
    fontActionMenu->setObjectName("note_font_actionmenu");

    QActionGroup *ag = new QActionGroup(this);
    QString defaultFontName = NoteFontFactory::getDefaultFontName();

    for (std::vector<QString>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(*i);

        QAction *a = createAction("note_font_" + fontQName,
                                  SLOT(slotChangeFontFromAction()));

        ag->addAction(a);

        a->setText(fontQName);
        a->setCheckable(true);
        a->setChecked(*i == defaultFontName);

        fontActionMenu->addAction(a);        
    }

    //&&& add fontActionMenu to the appropriate super-menu

      QMenu *fontSizeActionMenu = new QMenu(tr("Si&ze"), this);
      fontSizeActionMenu->setObjectName("note_font_size_actionmenu");
      ag = new QActionGroup(this);
      int defaultFontSize = NoteFontFactory::getDefaultSize(defaultFontName);

    //setupFontSizeMenu();

    //JAS from OldNotationView::setupFontSizeMenu()
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(defaultFontName);

    for (unsigned int i = 0; i < sizes.size(); ++i) {

        QString actionName = QString("note_font_size_%1").arg(sizes[i]);

        QAction *sizeAction = createAction(actionName,
                                SLOT(slotChangeFontSizeFromAction()));
        sizeAction->setText(tr("%n pixel(s)", "", sizes[i]));
        sizeAction->setCheckable(true);
        ag->addAction(sizeAction);

        sizeAction->setChecked(sizes[i] == defaultFontSize);
        fontSizeActionMenu->addAction(sizeAction);
    }

    //&&& add m_fontSizeActionMenu to the appropriate super-menu

/*!!!
    QMenu *spacingActionMenu = new QMenu(tr("S&pacing"), this);
    spacingActionMenu->setObjectName("stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    ag = new QActionGroup(this);

    for (std::vector<int>::iterator i = spacings.begin();
         i != spacings.end(); ++i) {

        QAction *a = createAction(QString("spacing_%1").arg(*i),
                                  SLOT(slotChangeSpacingFromAction()));

        ag->addAction(a);
        a->setText(QString("%1%").arg(*i));
        a->setCheckable(true);
        a->setChecked(*i == defaultSpacing);

        spacingActionMenu->addAction(a);
    }

    //&&& add spacingActionMenu to the appropriate super-menu


    QMenu *proportionActionMenu = new QMenu(tr("Du&ration Factor"), this);
    proportionActionMenu->setObjectName("proportion_actionmenu");

    int defaultProportion = m_hlayout->getProportion();
    std::vector<int> proportions = NotationHLayout::getAvailableProportions();

    ag = new QActionGroup(this);

    for (std::vector<int>::iterator i = proportions.begin();
         i != proportions.end(); ++i) {

        QString name = QString("%1%").arg(*i);
        if (*i == 0) name = tr("None");

        QAction *a = createAction(QString("proportion_%1").arg(*i),
                                  SLOT(slotChangeSpacingFromAction()));

        ag->addAction(a);
        a->setText(name);
        a->setCheckable(true);
        a->setChecked(*i == defaultProportion);

        proportionActionMenu->addAction(a);
    }
*/
    //&&& add proportionActionMenu to the appropriate super-menu

}

void 
NewNotationView::slotUpdateMenuStates()
{
    NOTATION_DEBUG << "NotationView::slotUpdateMenuStates" << endl;

    // 1. set selection-related states

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    leaveActionState("have_selection");
    leaveActionState("have_notes_in_selection");
    leaveActionState("have_rests_in_selection");

    if (!m_notationWidget) return;

    EventSelection *selection = m_notationWidget->getSelection();

    if (selection) {

        NOTATION_DEBUG << "NotationView::slotUpdateMenuStates: Have selection; it's " << selection << " covering range from " << selection->getStartTime() << " to " << selection->getEndTime() << " (" << selection->getSegmentEvents().size() << " events)" << endl;

        enterActionState("have_selection");
        if (selection->contains(Note::EventType)) {
            enterActionState("have_notes_in_selection");
        }
        if (selection->contains(Note::EventRestType)) {
            enterActionState("have_rests_in_selection");
        }

    } else {

        NOTATION_DEBUG << "Do not have a selection" << endl;
    }

    // 2. set inserter-related states
    NoteRestInserter *currentTool = dynamic_cast<NoteRestInserter *>(m_notationWidget->getCurrentTool());
    if (currentTool) {
        NOTATION_DEBUG << "Have NoteRestInserter " << endl;
        enterActionState("note_rest_tool_current");
        
    } else {
        NOTATION_DEBUG << "Do note have NoteRestInserter " << endl;
        leaveActionState("note_rest_tool_current");
    }
}

void
NewNotationView::initStatusBar()
{
    QStatusBar* sb = statusBar();

    m_hoveredOverNoteName = new QLabel(sb);
    m_hoveredOverNoteName->setMinimumWidth(32);
    sb->addWidget(m_hoveredOverNoteName);

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);
    sb->addWidget(m_hoveredOverAbsoluteTime);

    QWidget *hbox = new QWidget(sb);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hbox->setLayout(hboxLayout);

    m_currentNotePixmap = new QLabel(hbox);
    m_currentNotePixmap->setMinimumWidth(20);
    hboxLayout->addWidget(m_currentNotePixmap);

    m_insertModeLabel = new QLabel(hbox);
    hboxLayout->addWidget(m_insertModeLabel);

    m_annotationsLabel = new QLabel(hbox);
    hboxLayout->addWidget(m_annotationsLabel);

    m_lilyPondDirectivesLabel = new QLabel(hbox);
    hboxLayout->addWidget(m_lilyPondDirectivesLabel);

    sb->addWidget(hbox);

    sb->showMessage(TmpStatusMsg::getDefaultMsg(), TmpStatusMsg::getDefaultId());
	
    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar = new ProgressBar(100, true, sb);
    m_progressBar->setMinimumWidth(100);
    m_progressBar->setMaximumWidth(100);

    sb->addPermanentWidget(m_progressBar);
    sb->setContentsMargins(0, 0, 0, 0);

    sb->showMessage("This is a test of the emergency broadcast system. This is only a test.");
}


bool NewNotationView::exportLilyPondFile(QString file, bool forPreview)
{
    QString caption = "", heading = "";
    if (forPreview) {
        caption = tr("LilyPond Preview Options");
        heading = tr("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, m_doc, caption, heading);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    LilyPondExporter e(this, m_doc, std::string(QFile::encodeName(file)));

    if (!e.write()) {
        QMessageBox::warning(this, tr("Rosegarden"), tr("Export failed.  The file could not be opened for writing."));
        return false;
    }

    return true;
}

void NewNotationView::slotPrintLilyPond()
{
    TmpStatusMsg msg(tr("Printing with LilyPond..."), this);

    QString filename = getLilyPondTmpFilename();

    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Print, filename);

    dialog->exec();
}

void NewNotationView::slotPreviewLilyPond()
{
    TmpStatusMsg msg(tr("Previewing with LilyPond..."), this);

    QString filename = getLilyPondTmpFilename();

    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Preview, filename);

    dialog->exec();
}

QString
NewNotationView::getLilyPondTmpFilename()
{
    QString mask = QString("%1/rosegarden_tmp_XXXXXX.ly").arg(QDir::tempPath());
    std::cerr << "RosegardenMainWindow::getLilyPondTmpName() - using tmp file: " << qstrtostr(mask) << std::endl;

    QTemporaryFile *file = new QTemporaryFile(mask);
    file->setAutoRemove(true);
    if (!file->open()) {
        // getLilyPondTmpFilename() in RosegardenMainWindow:: and in NewNotationView:: are nearly in sync.
        // However, the following line is NOT commented out in RosegardenMainWindow::getLilyPondTmpFilename()
        // CurrentProgressDialog::freeze();
        QMessageBox::warning(this, tr("Rosegarden"),
                                       tr("<qt><p>Failed to open a temporary file for LilyPond export.</p>"
                                          "<p>This probably means you have run out of disk space on <pre>%1</pre></p></qt>").
                                       arg(QDir::tempPath()));
        delete file;
        return QString();
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename

    return filename;
}


void
NewNotationView::slotLinearMode()
{
    enterActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetLinearMode();
}

void
NewNotationView::slotContinuousPageMode()
{
    leaveActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetContinuousPageMode();
}

void
NewNotationView::slotMultiPageMode()
{
    leaveActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetMultiPageMode();
}

void
NewNotationView::slotShowHeadersGroup()
{
    if (m_notationWidget) m_notationWidget->toggleHeadersView();
}

void
NewNotationView::slotChangeFontFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();
    if (name.left(10) == "note_font_") {
        name = name.right(name.length() - 10);
        if (m_notationWidget) m_notationWidget->slotSetFontName(name);
    } else {
        QMessageBox::warning
            (this, tr("Rosegarden"), tr("Unknown font action %1").arg(name));
    }
}

void
NewNotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(15) == "note_font_size_") {
        name = name.right(name.length() - 15);
        bool ok = false;
        int size = name.toInt(&ok);
        if (ok) {
            if (m_notationWidget) m_notationWidget->slotSetFontSize(size);
            return;
        } 
    }
    QMessageBox::warning
        (this, tr("Rosegarden"), tr("Unknown font size action %1").arg(name));
}

Segment *
NewNotationView::getCurrentSegment()
{
    if (m_notationWidget) return m_notationWidget->getCurrentSegment();
    else return 0;
}

EventSelection *
NewNotationView::getSelection() const
{
    if (m_notationWidget) return m_notationWidget->getSelection();
    else return 0;
}

void
NewNotationView::setSelection(EventSelection *selection, bool preview)
{
    if (m_notationWidget) m_notationWidget->setSelection(selection, preview);
}

timeT
NewNotationView::getInsertionTime() const
{
    if (m_notationWidget) return m_notationWidget->getInsertionTime();
    else return 0;
}

void
NewNotationView::slotEditCut()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, m_document->getClipboard()));
}

void
NewNotationView::slotEditDelete()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new EraseCommand(*selection));
}

void
NewNotationView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, m_document->getClipboard()));
}

void
NewNotationView::slotEditCutAndClose()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutAndCloseCommand(*selection, m_document->getClipboard()));
}

void
NewNotationView::slotEditPaste()
{
    Clipboard *clipboard = m_document->getClipboard();

    if (clipboard->isEmpty()) return;
    if (!clipboard->isSingleSegment()) {
        slotStatusHelpMsg(tr("Can't paste multiple Segments into one"));
        return;
    }

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
        (clipboard->getSingleSegment()->getEndTime() -
         clipboard->getSingleSegment()->getStartTime());

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        settings.value("pastetype",
                       PasteEventsCommand::Restricted).toUInt();

    PasteEventsCommand *command = new PasteEventsCommand
        (*segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        // NOTES: To get a reasonable presentation of the standard and detailed
        // text, we have to build up our own QMessageBox
        //
        // The old RESTRICTED_PASTE_DESCRIPTION was removed because it was
        // impossible to get the translation, which had to be done in the
        // QObject::tr() context, to work in this context here.  Qt is really
        // quirky that way.  Instead, I'm just block copying all of this now
        // that I've reworked it.  Is this copy you're looking at the original,
        // or the copy?  Only I know for sure, and I'll never tell!  Bwa haha!
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Rosegarden"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Couldn't paste at this point."));
        msgBox.setInformativeText(tr("<qt><p>The Restricted paste type requires enough empty space (containing only rests) at the paste position to hold all of the events to be pasted.</p><p>Not enough space was found.</p><p>If you want to paste anyway, consider using one of the other paste types from the <b>Paste...</b> option on the Edit menu.  You can also change the default paste type to something other than Restricted if you wish.</p></qt>"));                      
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        delete command;
    } else {
        CommandHistory::getInstance()->addCommand(command);
        setSelection(new EventSelection(command->getPastedEvents()), false);
//!!!        slotSetInsertCursorPosition(endTime, true, false);
        m_document->slotSetPointerPosition(endTime);
    }

    settings.endGroup();
}

void
NewNotationView::slotEditGeneralPaste()
{
    Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(tr("Clipboard is empty"));
        return ;
    }

    slotStatusHelpMsg(tr("Inserting clipboard contents..."));

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        settings.value("pastetype",
                       PasteEventsCommand::Restricted).toUInt();
    
    PasteNotationDialog dialog(this, defaultType);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();
        if (dialog.setAsDefault()) {
            //###settings.beginGroup( NotationViewConfigGroup );
            settings.setValue("pastetype", type);
        }

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() -
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (*segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            // NOTES: To get a reasonable presentation of the standard and detailed
            // text, we have to build up our own QMessageBox
            //
            // The old RESTRICTED_PASTE_DESCRIPTION was removed because it was
            // impossible to get the translation, which had to be done in the
            // QObject::tr() context, to work in this context here.  Qt is really
            // quirky that way.  Instead, I'm just block copying all of this now
            // that I've reworked it.  Is this copy you're looking at the original,
            // or the copy?  Only I know for sure, and I'll never tell!  Bwa haha!
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Rosegarden"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("Couldn't paste at this point."));
            msgBox.setInformativeText(tr("<qt><p>The Restricted paste type requires enough empty space (containing only rests) at the paste position to hold all of the events to be pasted.</p><p>Not enough space was found.</p><p>If you want to paste anyway, consider using one of the other paste types from the <b>Paste...</b> option on the Edit menu.  You can also change the default paste type to something other than Restricted if you wish.</p></qt>"));                      
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            delete command;
        } else {
            CommandHistory::getInstance()->addCommand(command);
            setSelection(new EventSelection(*segment, insertionTime, endTime),
                         false);
//!!!            slotSetInsertCursorPosition(endTime, true, false);
            m_document->slotSetPointerPosition(endTime);
        }
    }

    settings.endGroup();
}

void NewNotationView::slotPreviewSelection()
{
    if (getSelection())
        return ;

    getDocument()->slotSetLoop(getSelection()->getStartTime(),
                               getSelection()->getEndTime());
}

void NewNotationView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}

void NewNotationView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    NotationSelector *selector = dynamic_cast<NotationSelector *>(m_notationWidget->getCurrentTool());

    if (!selector) {
        slotSetSelectTool();
    } else {
        setSelection(0, false);
    }
}

void NewNotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    segment->getStartTime(),
                                    t),
                 false);
}

void NewNotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    t,
                                    segment->getEndMarkerTime()),
                 false);
}

void NewNotationView::slotEditSelectWholeStaff()
{
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    segment->getStartTime(),
                                    segment->getEndMarkerTime()),
                 false);
}

void NewNotationView::slotFilterSelection()
{
    NOTATION_DEBUG << "NewNotationView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = getSelection();
    if (!segment || !existingSelection)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotFilterSelection- accepted" << endl;

        bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
                 ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
                haveEvent = true;
                newSelection->addEvent(*i);
            }
        }

        if (haveEvent) {
            setSelection(newSelection, false);
        } else {
            setSelection(0, false);
        }
    }
}

void NewNotationView::slotVelocityUp()
{
    if (getSelection())
        return ;
    TmpStatusMsg msg(tr("Raising velocities..."), this);

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            10, *getSelection()));
}

void NewNotationView::slotVelocityDown()
{
    if (!getSelection())
        return ;
    TmpStatusMsg msg(tr("Lowering velocities..."), this);

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            -10, *getSelection()));
}

int NewNotationView::getVelocityFromSelection()
{
    if (!getSelection()) return 0;

    float totalVelocity = 0;
    int count = 0;

    for (EventSelection::eventcontainer::iterator i =
             getSelection()->getSegmentEvents().begin();
         i != getSelection()->getSegmentEvents().end(); ++i) {

        if ((*i)->has(BaseProperties::VELOCITY)) {
            totalVelocity += (*i)->get<Int>(BaseProperties::VELOCITY);
            ++count;
        }
    }

    if (count > 0) {
        return (totalVelocity / count) + 0.5;
    }
    return 0;
}

void NewNotationView::slotSetVelocities()
{
    if (!getSelection())
        return ;

    EventParameterDialog dialog(this,
                                tr("Set Event Velocities"),
                                BaseProperties::VELOCITY,
                                getVelocityFromSelection());

    if (dialog.exec() == QDialog::Accepted) {
        TmpStatusMsg msg(tr("Setting Velocities..."), this);
        CommandHistory::getInstance()->addCommand(
                new SelectionPropertyCommand(
                        getSelection(),
                        BaseProperties::VELOCITY,
                        dialog.getPattern(),
                        dialog.getValue1(),
                        dialog.getValue2()));
    }
}

void NewNotationView::slotToggleGeneralToolBar()
{
    toggleNamedToolBar("General Toolbar");
}

void NewNotationView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void NewNotationView::slotToggleDurationToolBar()
{
    toggleNamedToolBar("Duration Toolbar");
}

void NewNotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("Accidentals Toolbar");
}

void NewNotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("Clefs Toolbar");
}

void NewNotationView::slotToggleMarksToolBar()
{
    toggleNamedToolBar("Marks Toolbar");
}

void NewNotationView::slotToggleGroupToolBar()
{
    toggleNamedToolBar("Group Toolbar");
}

void NewNotationView::slotToggleSymbolsToolBar()
{
    toggleNamedToolBar("Symbols Toolbar");
}

void NewNotationView::slotToggleLayoutToolBar()
{
    toggleNamedToolBar("Layout Toolbar");
}

void NewNotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
}

void NewNotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
// 	QToolBar *namedToolBar = toolBar(toolBarName);
	QToolBar *namedToolBar = findChild<QToolBar*>(toolBarName);

    if (!namedToolBar) {
        NOTATION_DEBUG << "NewNotationView::toggleNamedToolBar() : toolBar "
                       << toolBarName << " not found" << endl;
        return ;
    }

    if (!force) {

        if (namedToolBar->isVisible())
            namedToolBar->hide();
        else
            namedToolBar->show();
    } else {

        if (*force)
            namedToolBar->show();
        else
            namedToolBar->hide();
    }

//     setSettingsDirty();	//&&& not required ?

}

void
NewNotationView::slotSetSelectTool()
{
    if (m_notationWidget) m_notationWidget->slotSetSelectTool();
    slotUpdateMenuStates();
}    

void
NewNotationView::slotSetEraseTool()
{
    if (m_notationWidget) m_notationWidget->slotSetEraseTool();
    slotUpdateMenuStates();
}    

void
NewNotationView::slotSetNoteRestInserter()
{
    NOTATION_DEBUG << "NewNotationView::slotSetNoteRestInserter : entered. " << endl;

    if (m_notationWidget) m_notationWidget->slotSetNoteRestInserter();

    //Must ensure it is set since may be called from multiple actions. 
    findAction("draw")->setChecked(true);
    slotUpdateMenuStates();
}    

void
NewNotationView::slotSwitchToNotes()
{
    NOTATION_DEBUG << "NewNotationView::slotSwitchToNotes : entered. " << endl;

    QString actionName = "";
    if (m_notationWidget) {
        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if (!currentInserter) {
            NOTATION_DEBUG << "NewNotationView::slotSwitchToNotes() : expected"
                       << " NoteRestInserter as current tool.  Silent exit."
                       << endl;
            return;
        }

        Note::Type unitType = currentInserter->getCurrentNote()
            .getNoteType();
        int dots = (currentInserter->getCurrentNote().getDots() ? 1 : 0);
        actionName = NotationStrings::getReferenceName(Note(unitType,dots));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetNoteInserter();
    }

    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    QAction *currentAction = findAction(actionName);
    currentAction->setChecked(true);
    
    // This code and last line above used to maintain exclusive state
    // of the Duration Toolbar so we can reactivate the NoteRestInserter
    // even when from a pressed button on the bar.
    
    // Now un-select previous pressed button pressed
    if (currentAction != m_durationPressed) {
        m_durationPressed->setChecked(false);
        m_durationPressed = currentAction;
    }

    morphDurationMonobar();

    slotUpdateMenuStates();
}

void
NewNotationView::slotSwitchToRests()
{
    NOTATION_DEBUG << "NewNotationView::slotSwitchToRests : entered. " << endl;

    QString actionName = "";
    if (m_notationWidget) {
        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if (!currentInserter) {
            NOTATION_DEBUG << "NewNotationView::slotSwitchToRests() : expected"
                       << " NoteRestInserter as current tool.  Silent exit."
                       << endl;
            return;
        }

        Note::Type unitType = currentInserter->getCurrentNote()
            .getNoteType();
        int dots = (currentInserter->getCurrentNote().getDots() ? 1 : 0);
        actionName = NotationStrings::getReferenceName(Note(unitType,dots));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetRestInserter();
    }

    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    findAction(QString("rest_%1").arg(actionName))->setChecked(true);


    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    QAction *currentAction = findAction(QString("rest_%1").arg(actionName));
    currentAction->setChecked(true);
    
    // This code and last line above used to maintain exclusive state
    // of the Duration Toolbar so we can reactivate the NoteRestInserter
    // even when from a pressed button on the bar.
    
    // Now un-select previous pressed button pressed
    if (currentAction != m_durationPressed) {
        m_durationPressed->setChecked(false);
        m_durationPressed = currentAction;
    }

    morphDurationMonobar();

    slotUpdateMenuStates();
}

void
NewNotationView::morphDurationMonobar()
{
    NOTATION_DEBUG << "NewNotationView::morphDurationMonobar : entered. " << endl;

    NoteRestInserter *currentInserter = 0; 
    if(m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
        (m_notationWidget->getCurrentTool());
    }

    if (!currentInserter)
    {
        // Morph called when NoteRestInserter not set as current tool
        NOTATION_DEBUG << "NewNotationView::morphNotationToolbar() : expected"
               << " NoteRestInserter.  Silent Exit." 
               << endl;
        return;

    }
    // Retrieve duration and dot values
    int dots = currentInserter->getCurrentNote().getDots();
    Note::Type note = currentInserter->getCurrentNote().getNoteType();

    // Determine duration tooolbar mode
    DurationMonobarModeType newMode = InsertingNotes;
    if (currentInserter->isaRestInserter()) {
        newMode = (dots ? InsertingDottedRests : InsertingRests);
    } else {
        newMode = (dots ? InsertingDottedNotes : InsertingNotes);
    }
    
    //Convert to English for debug purposes.        
    std::string modeStr;
    switch (newMode) {

    case InsertingNotes: modeStr = "Notes Toolbar"; break;
    case InsertingDottedNotes: modeStr = "Dotted Notes Toolbar"; break;
    case InsertingRests: modeStr = "Rests Toolbar"; break;
    case InsertingDottedRests: modeStr = "Dotted Rests Toolbar"; break;
    default: modeStr = "WTF?  This won't be pretty.";

    }
    NOTATION_DEBUG << "NewNotationView::morphDurationMonobar: morphing to "
        << modeStr << endl;

    if (newMode == m_durationMode && note != Note::Shortest && dots) {
        NOTATION_DEBUG << "NewNotationView::morphDurationMonobar: new "
            << "mode and last mode are the same.  exit wothout morphing."
            << endl;
        return;
    }
    
    // Turn off current state (or last state--depending on perspective.)
    switch (m_durationMode) {

    case InsertingNotes:
        leaveActionState("note_0_dot_mode");
        break;

    case InsertingDottedNotes:
        leaveActionState("note_1_dot_mode");
        break;

    case InsertingRests:
        leaveActionState("rest_0_dot_mode");
        break;

    case InsertingDottedRests:
        leaveActionState("rest_1_dot_mode");
        break;

    default:
        NOTATION_DEBUG << "NewNotationView::morphDurationMonobar:  None of "
            << "The standard four modes were selected for m_durationMode. "
            << "How did that happen?" << endl;
    }

    // transfer new mode to member for next recall.
    m_durationMode = newMode;
    
    // Now morph to new state.
    switch (newMode) {

    case InsertingNotes:
        enterActionState("note_0_dot_mode");
        break;

    case InsertingDottedNotes:
        enterActionState("note_1_dot_mode");
        break;

    case InsertingRests:
        enterActionState("rest_0_dot_mode");
        break;

    case InsertingDottedRests:
        enterActionState("rest_1_dot_mode");
        break;
    default:
        NOTATION_DEBUG << "NewNotationView::morphDurationMonobar:  None of "
            << "The standard four modes were selected for newMode. "
            << "How did that happen?" << endl;
    }

    // This code to manage shortest dotted note selection.
    // Disable the shortcut in the menu for shortest duration.
    if (note == Note::Shortest && !dots) {
        NOTATION_DEBUG << "NewNotationView::morphDurationMonobar:  shortest "
            << "note / no dots.  disable off +. action";
        QAction *switchDots = findAction("switch_dots_on");
        switchDots->setEnabled(false);
    }
}

void
NewNotationView::initializeNoteRestInserter()
{     
    // Set Default Duration based on Time Signature denominator.
    // The default unitType is taken from the denominator of the time signature:
    //   e.g. 4/4 -> 1/4, 6/8 -> 1/8, 2/2 -> 1/2.
    TimeSignature sig = getDocument()->getComposition().getTimeSignatureAt(getInsertionTime());
    Note::Type unitType = sig.getUnit();

    QString actionName = NotationStrings::getReferenceName(Note(unitType,0));
    actionName.replace(QRegExp("-"), "_");

    //Initialize Duration Toolbar (hide all buttons)   
    leaveActionState("note_0_dot_mode");
    leaveActionState("note_1_dot_mode");
    leaveActionState("rest_0_dot_mode");
    leaveActionState("rest_1_dot_mode");
    
    //Change exclusive settings so we can retrigger Duration Toolbar
    //actions when button needed is pressed.
    //exclusive state maintianed via slotSwitchToRests() / slotSwitchToNotes().
    findGroup("duration_toolbar")->setExclusive(false);


    // Initialize the m_durationPressed so we don't have to null check elswhere.
    m_durationPressed = findAction(QString("duration_%1").arg(actionName));

    // Counting on a InsertingRests to be stored in NoteRestInserter::
    // m_durationMode which it was passed in the constructor.  This will
    // ensure morphDurationMonobar always fires correctly since
    // a duration_ shortcut is always tied to the note palette.
    m_durationPressed->trigger();

    //Change exclusive settings so we can retrigger Accidental Toolbar
    //actions when button needed is pressed.
    //exclusive state maintianed via manageAccidentalAction().
    findGroup("accidentals")->setExclusive(false);

    // Initialize the m_durationPressed so we don't have to null check elswhere.
    m_accidentalPressed = findAction("no_accidental");
}

int
NewNotationView::getPitchFromNoteInsertAction(QString name,
                                              Accidental &accidental,
                                              const Clef &clef,
                                              const Rosegarden::Key &key)
{
    using namespace Accidentals;

    accidental = NoAccidental;

    if (name.left(7) == "insert_") {

        name = name.right(name.length() - 7);

        int modify = 0;
        int octave = 0;

        if (name.right(5) == "_high") {

            octave = 1;
            name = name.left(name.length() - 5);

        } else if (name.right(4) == "_low") {

            octave = -1;
            name = name.left(name.length() - 4);
        }

        if (name.right(6) == "_sharp") {

            modify = 1;
            accidental = Sharp;
            name = name.left(name.length() - 6);

        } else if (name.right(5) == "_flat") {

            modify = -1;
            accidental = Flat;
            name = name.left(name.length() - 5);
        }

        int scalePitch = name.toInt();

        if (scalePitch < 0 || scalePitch > 7) {
            NOTATION_DEBUG << "NotationView::getPitchFromNoteInsertAction: pitch "
            << scalePitch << " out of range, using 0" << endl;
            scalePitch = 0;
        }

        Pitch clefPitch(clef.getAxisHeight(), clef, key, NoAccidental);

        int pitchOctave = clefPitch.getOctave() + octave;

        std::cerr << "NewNotationView::getPitchFromNoteInsertAction:"
                  << " key = " << key.getName() 
                  << ", clef = " << clef.getClefType() 
                  << ", octaveoffset = " << clef.getOctaveOffset() << std::endl;
        std::cerr << "NewNotationView::getPitchFromNoteInsertAction: octave = " << pitchOctave << std::endl;

        // We want still to make sure that when (i) octave = 0,
        //  (ii) one of the noteInScale = 0..6 is
        //  (iii) at the same heightOnStaff than the heightOnStaff of the key.
        int lowestNoteInScale = 0;
        Pitch lowestPitch(lowestNoteInScale, clefPitch.getOctave(), key, NoAccidental);

        int heightToAdjust = (clefPitch.getHeightOnStaff(clef, key) - lowestPitch.getHeightOnStaff(clef, key));
        for (; heightToAdjust < 0; heightToAdjust += 7) pitchOctave++;
        for (; heightToAdjust > 6; heightToAdjust -= 7) pitchOctave--;

        std::cerr << "NewNotationView::getPitchFromNoteInsertAction: octave = " << pitchOctave << " (adjusted)" << std::endl;

        Pitch pitch(scalePitch, pitchOctave, key, accidental);
        return pitch.getPerformancePitch();

    } else {

        throw Exception("Not an insert action",
                        __FILE__, __LINE__);
    }
}

void NewNotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    NoteRestInserter *currentInserter = 0;
    if(m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if(!currentInserter) {
            //set the NoteRestInserter as current
            slotSetNoteRestInserter();
            //re-fetch the current tool for analysis
            currentInserter = dynamic_cast<NoteRestInserter *>
                (m_notationWidget->getCurrentTool());
        }
    
        if (currentInserter) {
            if (currentInserter->isaRestInserter()) {
                slotSwitchToNotes();
            }
            int pitch = 0;
            Accidental accidental = Accidentals::NoAccidental;

            timeT insertionTime = getInsertionTime();
            static Rosegarden::Key key = segment->getKeyAtTime(insertionTime);
            static Clef clef = segment->getClefAtTime(insertionTime);

            try {

                std::cerr << "NewNotationView::slotInsertNoteFromAction: time = "
                    << insertionTime << ", key = " << key.getName()
                    << ", clef = " << clef.getClefType() << ", octaveoffset = "
                    << clef.getOctaveOffset() << std::endl;

                pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

            } catch (...) {

                QMessageBox::warning
                    (this, tr("Rosegarden"),  tr("Unknown note insert action %1").arg(name));
                return ;
            }

            TmpStatusMsg msg(tr("Inserting note"), this);

            NOTATION_DEBUG << "Inserting note at pitch " << pitch << endl;
            currentInserter->insertNote(*segment, insertionTime,
                pitch, accidental);
        }
    }
}

void NewNotationView::slotInsertRestFromAction()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;
    
    NoteRestInserter *currentInserter = 0;
    if(m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if(!currentInserter) {
            //set the NoteRestInserter as current
            slotSetNoteRestInserter();
            //re-fetch the current tool for analysis
            currentInserter = dynamic_cast<NoteRestInserter *>
                (m_notationWidget->getCurrentTool());
        }
    
        if (currentInserter) {
            if (!currentInserter->isaRestInserter()) {
                slotSwitchToRests();
            }
           timeT insertionTime = getInsertionTime();

           currentInserter->insertNote(*segment, insertionTime,
               0, Accidentals::NoAccidental, true);
        }
    }
}

void
NewNotationView::slotToggleDot()
{
    NOTATION_DEBUG << "NewNotationView::slotToggleDot : entered. " << endl;
    if (m_notationWidget) {
        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentInserter) {
            NOTATION_DEBUG << "NewNotationView::slotToggleDot : expected "
                << "NoteRestInserter as current tool.  Silent exit."
                << endl;       
            return ;
        }
        Note note = currentInserter->getCurrentNote();

        Note::Type noteType = note.getNoteType();
        int noteDots = (note.getDots() ? 0 : 1); // Toggle the dot state
        
        if (noteDots && noteType == Note::Shortest)
        {
            // This might have been invoked via a keboard shortcut or other
            // toggling the +. button when the shortest note was pressed.
            // RG does not render dotted versions of its shortest duration
            // and rounds it up to the next duration.
            // Following RG's lead on this makes the inteface feel off since
            // This moves the toggle to the next longest duration without
            // switching the pallete to dots.
            // So just leave the duration alone and don't toggle the dot
            // in this case.
            noteDots = 0;
        }

        QString actionName(NotationStrings::getReferenceName(Note(noteType,noteDots)));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetInsertedNote(noteType, noteDots);
        if (currentInserter->isaRestInserter()) {
            slotSwitchToRests();
        } else {
            slotSwitchToNotes();
        }
    }
}

void
NewNotationView::slotNoteAction()
{
    NOTATION_DEBUG << "NewNotationView::slotNoteAction : entered. " << endl;

    QObject *s = sender();
    QString name = s->objectName();
    QString noteToolbarName;

    //Set defaults for duration_ shortcut calls
    bool rest = false;  
    int dots = 0;

    if (m_notationWidget) {
        NoteRestInserter *currentTool = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentTool) {
            //Must select NoteRestInserter tool as current Tool
            slotSetNoteRestInserter();
            //Now re-fetch the current tool for analysis.
            currentTool = dynamic_cast<NoteRestInserter *>(m_notationWidget
                ->getCurrentTool());
        }
        if (name.startsWith("duration_")) {
            name = name.replace("duration_", "");
            NOTATION_DEBUG << "NewNotationView::slotNoteAction : "
                << "Duration shortcut called." << endl;
            //duration shortcut called from keyboard or menu.
            //Must switch to insert Notes mode.

        } else if (currentTool->isaRestInserter()) {
            NOTATION_DEBUG << "NewNotationView::slotNoteAction : "
                << "Have rest inserter." << endl;
            if (name.startsWith("rest_")) {
                name = name.replace("rest_", "");
            }
            rest = true;
        } else {
            NOTATION_DEBUG << "NewNotationView::slotNoteAction : "
                << "Have note inserter." << endl;
        }
    }

    if (name.startsWith("dotted_")) {
        dots = 1;
        name = name.replace("dotted_", "");
    }

    Note::Type type = NotationStrings::getNoteForName(name).getNoteType();
    if (m_notationWidget) {
        m_notationWidget->slotSetInsertedNote(type, dots);
        if (rest) {
            slotSwitchToRests();
        } else {
            slotSwitchToNotes();
        }
    }
    //!!! todo: set status bar indication
}

void
NewNotationView::manageAccidentalAction(QString actionName)
{
     NOTATION_DEBUG << "NewNotationView::manageAccidentalAction: enter. "
         << "actionName = " << actionName << "." << endl;

    // Manage exclusive group setting since group->isExclusive() == false.
    QAction *currentAction = findAction(actionName);
    // Force the current button to be pressed
    currentAction->setChecked(true);
    if (m_accidentalPressed != currentAction) {
        m_accidentalPressed->setChecked(false);
        m_accidentalPressed = currentAction;
    }

    // Set The Note / Rest Inserter Tool as curretn tool if needed.
    if (m_notationWidget) {
        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentInserter) {
            slotSetNoteRestInserter();
            
            // re-fetch tool for analysis.
            currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        }
        if (currentInserter->isaRestInserter()) {
            slotSwitchToNotes();
        }
    }
    
}

void
NewNotationView::slotNoAccidental()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(NoAccidental, false);
}

void
NewNotationView::slotFollowAccidental()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(NoAccidental, true);
}

void
NewNotationView::slotSharp()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Sharp, false);
}

void
NewNotationView::slotFlat()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Flat, false);
}

void
NewNotationView::slotNatural()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Natural, false);
}

void
NewNotationView::slotDoubleSharp()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(DoubleSharp, false);
}

void
NewNotationView::slotDoubleFlat()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(DoubleFlat, false);
}

void
NewNotationView::slotClefAction()
{
    QObject *s = sender();
    QString n = s->objectName();

    Clef type = Clef::Treble;

    if (n == "treble_clef") type = Clef::Treble;
    else if (n == "alto_clef") type = Clef::Alto;
    else if (n == "tenor_clef") type = Clef::Tenor;
    else if (n == "bass_clef") type = Clef::Bass;

/*!!! todo: restore status bar indication
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-treble")));
*/
    if (!m_notationWidget) return;
    m_notationWidget->slotSetClefInserter();
    m_notationWidget->slotSetInsertedClef(type);
    slotUpdateMenuStates();
}

void
NewNotationView::slotText()
{
/*!!! todo: restore
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
*/
    if (!m_notationWidget) return;
    m_notationWidget->slotSetTextInserter();
    slotUpdateMenuStates();
}

void
NewNotationView::slotGuitarChord()
{
/*!!! todo: restore
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
*/
    if (!m_notationWidget) return;
    m_notationWidget->slotSetGuitarChordInserter();
    slotUpdateMenuStates();
}

void
NewNotationView::slotTransformsQuantize()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    QuantizeDialog dialog(this, true);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
             (new EventQuantizeCommand
              (*selection,
               dialog.getQuantizer()));
    }
}

void
NewNotationView::slotTransformsInterpret()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    InterpretDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new InterpretCommand
             (*selection,
              getDocument()->getComposition().getNotationQuantizer(),
              dialog.getInterpretations()));
    }
}

void
NewNotationView::slotMakeOrnament()
{
    if (!getSelection())
        return ;

    EventSelection::eventcontainer &ec =
        getSelection()->getSegmentEvents();

    int basePitch = -1;
    int baseVelocity = -1;

    NoteStyle *style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);

    for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
        if ((*i)->isa(Note::EventType)) {
            if ((*i)->has(BaseProperties::PITCH)) {
                basePitch = (*i)->get<Int>(BaseProperties::PITCH);
                style = NoteStyleFactory::getStyleForEvent(*i);
                if (baseVelocity != -1) break;
            }
            if ((*i)->has(BaseProperties::VELOCITY)) {
                baseVelocity = (*i)->get<Int>(BaseProperties::VELOCITY);
                if (basePitch != -1) break;
            }
        }
    }

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT absTime = getSelection()->getStartTime();
    timeT duration = getSelection()->getTotalDuration();
    Note note(Note::getNearestNote(duration));

    Track *track =
        segment->getComposition()->getTrackById(segment->getTrack());
    QString name;
    int barNo = segment->getComposition()->getBarNumber(absTime);
    if (track) {
        name = QString(tr("Ornament track %1 bar %2").arg(track->getPosition() + 1).arg(barNo + 1));
    } else {
        name = QString(tr("Ornament bar %1").arg(barNo + 1));
    }

    MakeOrnamentDialog dialog(this, name, basePitch);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    name = dialog.getName();
    basePitch = dialog.getBasePitch();

    MacroCommand *command = new MacroCommand(tr("Make Ornament"));

    command->addCommand(new CopyCommand
                        (*getSelection(),
                         getDocument()->getClipboard()));

    command->addCommand(new CutCommand
                        (*getSelection(),
                         getDocument()->getClipboard()));

    command->addCommand(new PasteToTriggerSegmentCommand
                        (&getDocument()->getComposition(),
                         getDocument()->getClipboard(),
                         name, basePitch));

    command->addCommand(new InsertTriggerNoteCommand
                        (*segment, absTime, note, basePitch, baseVelocity,
                         style->getName(),
                         getDocument()->getComposition().getNextTriggerSegmentId(),
                         true,
                         BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH,
                         Marks::NoMark));

    CommandHistory::getInstance()->addCommand(command);
}

void
NewNotationView::slotUseOrnament()
{
    // Take an existing note and match an ornament to it.

    if (!getSelection())
        return ;

    UseOrnamentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted)
        return ;

    CommandHistory::getInstance()->addCommand(
            new SetTriggerCommand(*getSelection(),
                                  dialog.getId(),
                                  true,
                                  dialog.getRetune(),
                                  dialog.getTimeAdjust(),
                                  dialog.getMark(),
                                  tr("Use Ornament")));
}

void
NewNotationView::slotRemoveOrnament()
{
    if (!getSelection())
        return ;

    CommandHistory::getInstance()->addCommand(
            new ClearTriggersCommand(*getSelection(),
                                     tr("Remove Ornaments")));
}

void
NewNotationView::slotEditAddClef()
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();
    static Clef lastClef = segment->getClefAtTime(insertionTime);

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    ClefDialog dialog(this, scene->getNotePixmapFactory(), lastClef);

    if (dialog.exec() == QDialog::Accepted) {

        ClefDialog::ConversionType conversion = dialog.getConversionType();

        bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
        bool shouldTranspose = (conversion == ClefDialog::Transpose);

        CommandHistory::getInstance()->addCommand(
                new ClefInsertionCommand(*segment,
                                         insertionTime,
                                         dialog.getClef(),
                                         shouldChangeOctave,
                                         shouldTranspose));

        lastClef = dialog.getClef();
    } 
}

void
NewNotationView::slotEditAddKeySignature()
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();
    static Clef clef = segment->getClefAtTime(insertionTime);
    static Key key = segment->getKeyAtTime(insertionTime);

    //!!! experimental:
    CompositionTimeSliceAdapter adapter
        (&getDocument()->getComposition(), insertionTime,
         getDocument()->getComposition().getDuration());
    AnalysisHelper helper;
    key = helper.guessKey(adapter);

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;


    KeySignatureDialog dialog(this,
                              scene->getNotePixmapFactory(),
                              clef,
                              key,
                              true,
                              true,
                              tr("Estimated key signature shown"));

    if (dialog.exec() == QDialog::Accepted &&
        dialog.isValid()) {

        KeySignatureDialog::ConversionType conversion =
            dialog.getConversionType();

        bool transposeKey = dialog.shouldBeTransposed();
        bool applyToAll = dialog.shouldApplyToAll();
    bool ignorePercussion = dialog.shouldIgnorePercussion();

        if (applyToAll) {
            CommandHistory::getInstance()->addCommand(
                    new MultiKeyInsertionCommand(
                            getDocument(),
                            insertionTime, dialog.getKey(),
                            conversion == KeySignatureDialog::Convert,
                            conversion == KeySignatureDialog::Transpose,
                            transposeKey,
                            ignorePercussion));
        } else {
            CommandHistory::getInstance()->addCommand(
                    new KeyInsertionCommand(*segment,
                                            insertionTime,
                                            dialog.getKey(),
                                            conversion == KeySignatureDialog::Convert,
                                            conversion == KeySignatureDialog::Transpose,
                                            transposeKey,
                                            false));
        }
    }
}

void
NewNotationView::slotEditAddSustain(bool down)
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();

    Studio *studio = &getDocument()->getStudio();
    Track *track = segment->getComposition()->getTrackById(segment->getTrack());

    if (track) {

        Instrument *instrument = studio->getInstrumentById
            (track->getInstrument());
        if (instrument) {
            MidiDevice *device = dynamic_cast<MidiDevice *>
                (instrument->getDevice());
            if (device) {
                for (ControlList::const_iterator i =
                         device->getControlParameters().begin();
                     i != device->getControlParameters().end(); ++i) {

                    if (i->getType() == Controller::EventType &&
                        (i->getName() == "Sustain" ||
                         strtoqstr(i->getName()) == tr("Sustain"))) {

                        CommandHistory::getInstance()->addCommand(
                                new SustainInsertionCommand(*segment, insertionTime, down,
                                                            i->getControllerValue()));
                        return ;
                    }
                }
            } else if (instrument->getDevice() &&
                       instrument->getDevice()->getType() == Device::SoftSynth) {
                CommandHistory::getInstance()->addCommand(
                        new SustainInsertionCommand(*segment, insertionTime, down, 64));
            }
        }
    }

    QMessageBox::warning(this, tr("Rosegarden"), tr("There is no sustain controller defined for this device.\nPlease ensure the device is configured correctly in the Manage MIDI Devices dialog in the main window."));
}

void
NewNotationView::slotEditAddSustainDown()
{
    slotEditAddSustain(true);
}

void
NewNotationView::slotEditAddSustainUp()
{
    slotEditAddSustain(false);
}

void
NewNotationView::slotEditTranspose()
{
    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();

    if (!ok || (semitones == 0 && steps == 0)) return;

    // TODO combine commands into one 
    for (size_t i = 0; i < m_segments.size(); i++)
    {
        CommandHistory::getInstance()->addCommand(new SegmentTransposeCommand(
                *(m_segments[i]), 
                intervalDialog.getChangeKey(), steps, semitones, 
                intervalDialog.getTransposeSegmentBack()));
    }

    // Fix #1885520 (Update track parameter widget when transpose changed from notation)
    RosegardenMainWindow::self()->getView()->getTrackParameterBox()->slotUpdateControls(-1);

    // And update track headers likewise
//&&& no track headers yet
//&&&    m_headersGroup->slotUpdateAllHeaders(getCanvasLeftX(), 0, true);
}

void
NewNotationView::slotEditSwitchPreset()
{
    PresetHandlerDialog dialog(this, true);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    if (dialog.getConvertAllSegments()) {
        // get all segments for this track and convert them.
        Composition& comp = getDocument()->getComposition();
        TrackId selectedTrack = getCurrentSegment()->getTrack();

    // satisfy #1885251 the way that seems most reasonble to me at the
    // moment, only changing track parameters when acting on all segments on
    // this track from the notation view 
    //
    //!!! This won't be undoable, and I'm not sure if that's seriously
    // wrong, or just mildly wrong, but I'm betting somebody will tell me
    // about it if this was inappropriate
    Track *track = comp.getTrackById(selectedTrack);
    track->setPresetLabel( qstrtostr(dialog.getName()) );
    track->setClef(dialog.getClef());
    track->setTranspose(dialog.getTranspose());
    track->setLowestPlayable(dialog.getLowRange());
    track->setHighestPlayable(dialog.getHighRange());

        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            comp.getSegments(), selectedTrack,
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    } else {
        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            m_segments, 
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    }

    m_doc->slotDocumentModified();

    // Fix #1885520 (Update track parameter widget when preset changed from notation)
    RosegardenMainWindow::self()->getView()->getTrackParameterBox()->slotUpdateControls(-1);
}

void
NewNotationView::slotToggleChordsRuler()
{
    bool visible = findAction("show_chords_ruler")->isChecked();

    m_notationWidget->setChordNameRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Chords ruler shown", visible);
    settings.endGroup();
}

void
NewNotationView::slotToggleTempoRuler()
{
    bool visible = findAction("show_tempo_ruler")->isChecked();

    m_notationWidget->setTempoRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Tempo ruler shown", visible);
    settings.endGroup();
}

// start of code formerly located in EditView.cpp
// --

void
NewNotationView::slotAddTempo()
{
    timeT insertionTime = getInsertionTime();

    TempoDialog tempoDlg(this, getDocument());

    connect(&tempoDlg,
             SIGNAL(changeTempo(timeT,
                    tempoT,
                    tempoT,
                    TempoDialog::TempoDialogAction)),
                    this,
                    SIGNAL(changeTempo(timeT,
                           tempoT,
                           tempoT,
                           TempoDialog::TempoDialogAction)));

    tempoDlg.setTempoPosition(insertionTime);
    tempoDlg.exec();
}

void
NewNotationView::slotAddTimeSignature()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    Composition *composition = segment->getComposition();
    timeT insertionTime = getInsertionTime();

    TimeSignatureDialog *dialog = 0;
    int timeSigNo = composition->getTimeSignatureNumberAt(insertionTime);

    if (timeSigNo >= 0) {

        dialog = new TimeSignatureDialog
                (this, composition, insertionTime,
                 composition->getTimeSignatureAt(insertionTime));

    } else {

        timeT endTime = composition->getDuration();
        if (composition->getTimeSignatureCount() > 0) {
            endTime = composition->getTimeSignatureChange(0).first;
        }

        CompositionTimeSliceAdapter adapter
                (composition, insertionTime, endTime);
        AnalysisHelper helper;
        TimeSignature timeSig = helper.guessTimeSignature(adapter);

        dialog = new TimeSignatureDialog
                (this, composition, insertionTime, timeSig, false,
                 tr("Estimated time signature shown"));
    }

    if (dialog->exec() == QDialog::Accepted) {

        insertionTime = dialog->getTime();

        if (dialog->shouldNormalizeRests()) {

            CommandHistory::getInstance()->addCommand(new AddTimeSignatureAndNormalizeCommand
                    (composition, insertionTime,
                     dialog->getTimeSignature()));

        } else {

            CommandHistory::getInstance()->addCommand(new AddTimeSignatureCommand
                    (composition, insertionTime,
                     dialog->getTimeSignature()));
        }
    }

    delete dialog;
}

void
NewNotationView::slotToggleRawNoteRuler()
{
    bool visible = findAction("show_raw_note_ruler")->isChecked();

    m_notationWidget->setRawNoteRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Raw note ruler shown", visible);
    settings.endGroup();
}

void
NewNotationView::slotToggleTracking()
{
    if (m_notationWidget) m_notationWidget->slotTogglePlayTracking();
}

void
NewNotationView::updateWindowTitle(bool m)
{
    QString indicator = (m ? "*" : "");

    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();
        //	std::cout << std::endl << std::endl << std::endl << "DEBUG TITLE BAR: " << getDocument()->getTitle() << std::endl << std::endl << std::endl;
        setWindowTitle(tr("%1%2 - Segment Track #%3 - Notation")
                      .arg(indicator)
                      .arg(getDocument()->getTitle())
                      .arg(trackPosition + 1));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setWindowTitle(tr("%1%2 - All Segments - Notation")
                      .arg(indicator)
                      .arg(getDocument()->getTitle()));

    } else {

        setWindowTitle(tr("%1%2 - %n Segment(s) - Notation", "", m_segments.size())
                    .arg(indicator)
                    .arg(getDocument()->getTitle()));

    }
}

void 
NewNotationView::slotGroupSimpleTuplet()
{
    slotGroupTuplet(true);
}

void 
NewNotationView::slotGroupGeneralTuplet()
{
    slotGroupTuplet(false);
}

void 
NewNotationView::slotGroupTuplet(bool simple)
{
    timeT t = 0;
    timeT unit = 0;
    int tupled = 2;
    int untupled = 3;
    Segment *segment = 0;
    bool hasTimingAlready = false;
    EventSelection *selection = getSelection();

    if (selection) {
        t = selection->getStartTime();

        timeT duration = selection->getTotalDuration();
        Note::Type unitType = Note::getNearestNote(duration / 3, 0)
                                                   .getNoteType();
        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType, duration);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &selection->getSegment();

    } else {

        t = getInsertionTime();

        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *> (m_notationWidget->getCurrentTool());

        Note::Type unitType;

// Should fix this too (maybe go fetch the NoteRestTool and check its duration).
        if (currentInserter) {
            unitType = currentInserter->getCurrentNote().getNoteType();
        } else {
            unitType = Note::Quaver;
        }

        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = getCurrentSegment();
    }

    CommandHistory::getInstance()->addCommand(new TupletCommand
                                              (*segment, t, unit, untupled, 
                                              tupled, hasTimingAlready));

    if (!hasTimingAlready) {
//        slotSetInsertCursorPosition(t + (unit * tupled), true, false);
        m_document->slotSetPointerPosition(t + (unit * tupled));
    }
}

void
NewNotationView::slotUpdateInsertModeStatus()
{
    QString tripletMessage = tr("Triplet");
    QString chordMessage = tr("Chord");
    QString graceMessage = tr("Grace");
    QString message;

    m_notationWidget->setChordMode(isInChordMode());
    m_notationWidget->setTripletMode(isInTripletMode());
    m_notationWidget->setGraceMode(isInGraceMode());

// We don't have a status bar yet, but somebody needs to bring it back across
// and uncomment all these various status messages here and throughout
//    if (isInTripletMode()) {
//        message = tr("%1 %2").arg(message).arg(tripletMessage);
//    }
//
//    if (isInChordMode()) {
//        message = tr("%1 %2").arg(message).arg(chordMessage);
//    }
//
//    if (isInGraceMode()) {
//        message = tr("%1 %2").arg(message).arg(graceMessage);
//    }
//
//    m_insertModeLabel->setText(message);
}

bool
NewNotationView::isInChordMode()
{
    QAction* tac = findAction("chord_mode");
    return tac->isChecked();
}

bool
NewNotationView::isInTripletMode()
{
    QAction* tac = findAction("triplet_mode");
    return tac->isChecked();
}

bool
NewNotationView::isInGraceMode()
{
    QAction* tac = findAction("grace_mode");
    return tac->isChecked();
}

void
NewNotationView::slotSymbolAction()
{
    QObject *s = sender();
    QString n = s->objectName();

    Symbol type = Symbol::Segno;

    if (n == "add_segno") type = Symbol::Segno;
    else if (n == "add_coda") type = Symbol::Coda;
    else if (n == "add_breath") type = Symbol::Breath;

    if (!m_notationWidget) return;
    m_notationWidget->slotSetSymbolInserter();
    m_notationWidget->slotSetInsertedSymbol(type);
    slotUpdateMenuStates();
}

void
NewNotationView::slotHalveDurations()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new RescaleCommand(*getSelection(),
                                              getSelection()->getTotalDuration() / 2,
                                              false));
}

void
NewNotationView::slotDoubleDurations()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new RescaleCommand(*getSelection(),
                                              getSelection()->getTotalDuration() * 2,
                                              false));
}

void
NewNotationView::slotRescale()
{
    if (!getSelection()) return ;

    RescaleDialog dialog
    (this,
     &getDocument()->getComposition(),
     getSelection()->getStartTime(),
     getSelection()->getEndTime() -
     getSelection()->getStartTime(),
     true,
     true);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(new RescaleCommand
                                                  (*getSelection(),
                                                  dialog.getNewDuration(),
                                                  dialog.shouldCloseGap()));
    }
}

void
NewNotationView::slotTransposeUp()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              (1, *getSelection()));
}

void
NewNotationView::slotTransposeDown()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              ( -1, *getSelection()));
}

void
NewNotationView::slotTransposeUpOctave()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              (12, *getSelection()));
}

void
NewNotationView::slotTransposeDownOctave()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              ( -12, *getSelection()));
}

void
NewNotationView::slotTranspose()
{
    EventSelection *selection = getSelection();
    if (!selection) std::cout << "Hint: selection is NULL in slotTranpose() " <<
 std::endl;
    if (!selection) return;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    int dialogDefault = settings.value("lasttransposition", 0).toInt() ;

    bool ok = false;
    int semitones = QInputDialog::getInteger
            (tr("Transpose"),
             tr("By number of semitones: "),
                dialogDefault, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    settings.setValue("lasttransposition", semitones);

    CommandHistory::getInstance()->addCommand(new TransposeCommand
            (semitones, *selection));

    settings.endGroup();
}

void
NewNotationView::slotDiatonicTranspose()
{
    if (!getSelection()) return ;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    IntervalDialog intervalDialog(this);
    int ok = intervalDialog.exec();
        //int dialogDefault = settings.value("lasttransposition", 0).toInt() ;
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();
    settings.endGroup();

    if (!ok || (semitones == 0 && steps == 0)) return;

    if (intervalDialog.getChangeKey())
    {
        std::cout << "Transposing changing keys is not currently supported on selections" << std::endl;
    }
    else
    {
        // Transpose within key
                //std::cout << "Transposing semitones, steps: " << semitones << ", " << steps << std::endl;
        CommandHistory::getInstance()->addCommand(new TransposeCommand
                                                  (semitones, steps,
                                                  *getSelection()));
    }
}

void
NewNotationView::slotInvert()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new InvertCommand
                                              (semitones, *getSelection()));
}

void
NewNotationView::slotRetrograde()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new RetrogradeCommand
                                              (semitones, *getSelection()));
}

void
NewNotationView::slotRetrogradeInvert()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new RetrogradeInvertCommand
                                              (semitones, *getSelection()));
}

void
NewNotationView::slotJogLeft()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    RG_DEBUG << "NewNotationView::slotJogLeft" << endl;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              -Note(Note::Demisemiquaver).getDuration(),
                                              false,  // don't use notation timings
                                              *selection));
}

void
NewNotationView::slotJogRight()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    RG_DEBUG << "NewNotationView::slotJogRight"<< endl;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              Note(Note::Demisemiquaver).getDuration(),
                                              false,  // don't use notation timings
                                              *selection));
}

void
NewNotationView::slotStepBackward()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT time = getInsertionTime();
    Segment::iterator i = segment->findTime(time);

    while (i != segment->begin() &&
           (i == segment->end() || (*i)->getNotationAbsoluteTime() >= time)){
        --i;
    }

    if (i != segment->end()){
        m_document->slotSetPointerPosition((*i)->getNotationAbsoluteTime());
    }
}

void
NewNotationView::slotStepForward()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT time = getInsertionTime();
    Segment::iterator i = segment->findTime(time);

    while (i != segment->end() && (*i)->getNotationAbsoluteTime() <= time) ++i;

    if (i == segment->end()){
        m_document->slotSetPointerPosition(segment->getEndMarkerTime());
    } else {
        m_document->slotSetPointerPosition((*i)->getNotationAbsoluteTime());
    }
}

void
NewNotationView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
NewNotationView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
NewNotationView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    //!!! Problematic.  Ideally we wouldn't insert events into windows
    //that weren't actually visible, otherwise all hell could break
    //loose (metaphorically speaking, I should probably add).  I did
    //think of checking isActiveWindow() and returning if the current
    //window wasn't active, but that will prevent anyone from
    //step-recording from e.g. vkeybd, which cannot be used without
    //losing focus (and thus active-ness) from the Rosegarden window.

    //!!! I know -- we'll keep track of which edit view (or main view,
    //or mixer, etc) is active, and we'll only allow insertion into
    //the most recently activated.  How about that?

    /* was toggle */
//      QAction *action = dynamic_cast<QAction*>
//         (actionCollection()->action("toggle_step_by_step"));
        QAction *action = findAction("toggle_step_by_step");
    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (!action->isChecked())
        return ;

    Segment *segment = getCurrentSegment();

    NoteRestInserter *noteInserter = dynamic_cast<NoteRestInserter *>
                                     (m_notationWidget->getCurrentTool());
    if (!noteInserter) {
        static bool showingError = false;
        if (showingError)
            return ;
        showingError = true;
        /* was sorry */ QMessageBox::warning(this, "", tr("Can't insert note: No note duration selected"));
        showingError = false;
        return ;
    }

//    if (m_inPaintEvent) {
//        NOTATION_DEBUG << "NotationView::slotInsertableNoteEventReceived: in paint event already" << endl;
//        if (noteOn) {
//            m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
//        }
//        return ;
//    }

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment->getTranspose();

    //    TmpStatusMsg msg(tr("Inserting note"), this);

    // We need to ensure that multiple notes hit at once come out as
    // chords, without imposing the interpretation that overlapping
    // notes are always chords and without getting too involved with
    // the actual absolute times of the notes (this is still step
    // editing, not proper recording).

    // First, if we're in chord mode, there's no problem.

    static int numberOfNotesOn = 0;
    static timeT insertionTime = getInsertionTime();
    static time_t lastInsertionTime = 0;

    if (isInChordMode()) {
        if (!noteOn)
            return ;
        NOTATION_DEBUG << "Inserting note in chord at pitch " << pitch << endl;
        noteInserter->insertNote(*segment, getInsertionTime(), pitch,
                                 Accidentals::NoAccidental,
                                 true);

    } else {

        if (!noteOn) {
            numberOfNotesOn--;
        } else if (noteOn) {
            // Rules:
            //
            // * If no other note event has turned up within half a
            //   second, insert this note and advance.
            //
            // * Relatedly, if this note is within half a second of
            //   the previous one, they're chords.  Insert the previous
            //   one, don't advance, and use the same rules for this.
            //
            // * If a note event turns up before that time has elapsed,
            //   we need to wait for the note-off events: if the second
            //   note happened less than half way through the first,
            //   it's a chord.
            //
            // We haven't implemented these yet... For now:
            //
            // Rules (hjj):
            //
            // * The overlapping notes are always included in to a chord.
            //   This is the most convenient for step inserting of chords.
            //
            // * The timer resets the numberOfNotesOn, if noteOff signals were
            //   drop out for some reason (which has not been encountered yet).

            time_t now;
            time (&now);
            double elapsed = difftime(now, lastInsertionTime);
            time (&lastInsertionTime);

            if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
                numberOfNotesOn = 0;
                insertionTime = getInsertionTime();
            }
            numberOfNotesOn++;

            noteInserter->insertNote(*segment, insertionTime, pitch,
                                     Accidentals::NoAccidental,
                                     true);
        }
    }
}

void
NewNotationView::slotEditLyrics()
{
    Segment *segment = getCurrentSegment();
    int oldVerseCount = 1;
    
    // The loop below is identical with the one in LyricEditDialog::countVerses() 
    // Maybe countVerses() should be moved to a Segment manipulating class ? (hjj)
    for (Segment::iterator i = segment->begin();
         segment->isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Text::EventType)) {

            std::string textType;
            if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
                textType == Text::Lyric) {

                long verse = 0;
                (*i)->get<Int>(Text::LyricVersePropertyName, verse);

                if (verse >= oldVerseCount) oldVerseCount = verse + 1;
            }
        }
    }

    LyricEditDialog dialog(this, segment);

    if (dialog.exec() == QDialog::Accepted) {

        MacroCommand *macro = new MacroCommand
            (SetLyricsCommand::getGlobalName());

        for (int i = 0; i < dialog.getVerseCount(); ++i) {
            SetLyricsCommand *command = new SetLyricsCommand
                (segment, i, dialog.getLyricData(i));
            macro->addCommand(command);
        }
        for (int i = dialog.getVerseCount(); i < oldVerseCount; ++i) {
            // (hjj) verse count decreased, delete extra verses.
            SetLyricsCommand *command = new SetLyricsCommand
                (segment, i, QString(""));
            macro->addCommand(command);
        }

        CommandHistory::getInstance()->addCommand(macro);
    }
}


void
NewNotationView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NewNotationView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    int bar, beat, fraction, remainder;
    getDocument()->getComposition().getMusicalTimeForAbsoluteTime
        (t, bar, beat, fraction, remainder);

    //    QString message;
    //    QString format("%ld (%ld.%03lds)");
    //    format = tr("Time: %1").arg(format);
    //    message.sprintf(format, t, rt.sec, ms);

    QString message = tr("Time: %1 (%2.%3s)")
         .arg(QString("%1-%2-%3-%4")
             .arg(QString("%1").arg(bar + 1).rightJustified(3, '0'))
             .arg(QString("%1").arg(beat).rightJustified(2, '0'))
             .arg(QString("%1").arg(fraction).rightJustified(2, '0'))
             .arg(QString("%1").arg(remainder).rightJustified(2, '0')))
         .arg(rt.sec)
         .arg(QString("%1").arg(ms).rightJustified(3, '0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

}

#include "NotationView.moc"
