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


#include "MatrixView.h"

#include "base/BaseProperties.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioLevel.h"
#include "base/Clipboard.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/LayoutEngine.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/PropertyName.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/Staff.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/CollapseNotesCommand.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/EventUnquantizeCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include "commands/edit/SetTriggerCommand.h"
#include "commands/matrix/MatrixInsertionCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/TriggerSegmentDialog.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/editors/notation/NotationElement.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/general/ActiveItem.h"
#include "gui/general/EditViewBase.h"
#include "gui/general/EditView.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/MidiPitchLabel.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/rulers/PercussionPitchRuler.h"
#include "gui/rulers/PitchRuler.h"
#include "gui/rulers/PropertyBox.h"
#include "gui/rulers/PropertyViewRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/QDeferScrollView.h"
#include "MatrixCanvasView.h"
#include "MatrixElement.h"
#include "MatrixEraser.h"
#include "MatrixHLayout.h"
#include "MatrixMover.h"
#include "MatrixPainter.h"
#include "MatrixResizer.h"
#include "MatrixSelector.h"
#include "MatrixStaff.h"
#include "MatrixToolBox.h"
#include "MatrixVLayout.h"
#include "PianoKeyboard.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include <klocale.h>
#include <kstddirs.h>
#include <kaction.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdockwidget.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kxmlguiclient.h>
#include <qcanvas.h>
#include <qcursor.h>
#include <qdialog.h>
#include <qlayout.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qscrollview.h>
#include <qsize.h>
#include <qslider.h>
#include <qstring.h>
#include <qwidget.h>
#include <qwmatrix.h>


namespace Rosegarden
{

static double xorigin = 0.0;


MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent,
                       bool drumMode)
        : EditView(doc, segments, 3, parent, "matrixview"),
        m_hlayout(&doc->getComposition()),
        m_referenceRuler(new ZoomableMatrixHLayoutRulerScale(m_hlayout)),
        m_vlayout(),
        m_snapGrid(new SnapGrid(&m_hlayout)),
        m_lastEndMarkerTime(0),
        m_hoveredOverAbsoluteTime(0),
        m_hoveredOverNoteName(0),
        m_selectionCounter(0),
        m_insertModeLabel(0),
        m_haveHoveredOverNote(false),
        m_previousEvPitch(0),
        m_dockLeft(0),
        m_canvasView(0),
        m_pianoView(0),
        m_localMapping(0),
        m_lastNote(0),
        m_quantizations(BasicQuantizer::getStandardQuantizations()),
        m_chordNameRuler(0),
        m_tempoRuler(0),
        m_playTracking(true),
        m_dockVisible(true),
        m_drumMode(drumMode),
        m_mouseInCanvasView(false)
{
    RG_DEBUG << "MatrixView ctor: drumMode " << drumMode << "\n";

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/toolbar");
    QPixmap matrixPixmap(pixmapDir + "/matrix.xpm");

    m_dockLeft = createDockWidget("params dock", matrixPixmap, 0L,
                                  i18n("Instrument Parameters"));
    m_dockLeft->manualDock(m_mainDockWidget,             // dock target
                           KDockWidget::DockLeft,  // dock site
                           20);                   // relation target/this (in percent)

    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
    connect(m_mainDockWidget, SIGNAL(docking(KDockWidget*, KDockWidget::DockPosition)),
            this, SLOT(slotParametersDockedBack(KDockWidget*, KDockWidget::DockPosition)));

    Composition &comp = doc->getComposition();

    m_toolBox = new MatrixToolBox(this);

    initStatusBar();

    connect(m_toolBox, SIGNAL(showContextHelp(const QString &)),
            this, SLOT(slotToolHelpChanged(const QString &)));

    QCanvas *tCanvas = new QCanvas(this);

    m_config->setGroup(MatrixViewConfigGroup);
    if (m_config->readBoolEntry("backgroundtextures-1.6-plus", true)) {
        QPixmap background;
        QString pixmapDir =
            KGlobal::dirs()->findResource("appdata", "pixmaps/");
	// We now use a lined background for the non-percussion matrix,
	// suggested and supplied by Alessandro Preziosi
	QString backgroundPixmap = isDrumMode() ? "bg-paper-white.xpm" : "bg-matrix-lines.xpm";
        if (background.load(QString("%1/misc/%2").
                            arg(pixmapDir, backgroundPixmap))) {
            tCanvas->setBackgroundPixmap(background);
        }
    }

    MATRIX_DEBUG << "MatrixView : creating staff\n";

    Track *track =
        comp.getTrackById(segments[0]->getTrack());

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());

    int resolution = 8;

    if (isDrumMode() && instr && instr->getKeyMapping()) {
        resolution = 11;
    }

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas,
                                           segments[i],
                                           m_snapGrid,
                                           i,
                                           resolution,
                                           this));
        // staff has one too many rows to avoid a half-row at the top:
        m_staffs[i]->setY( -resolution / 2);
        //!!!	if (isDrumMode()) m_staffs[i]->setX(resolution);
        if (i == 0)
            m_staffs[i]->setCurrent(true);
    }

    MATRIX_DEBUG << "MatrixView : creating canvas view\n";

    const MidiKeyMapping *mapping = 0;

    if (instr) {
        mapping = instr->getKeyMapping();
        if (mapping) {
            RG_DEBUG << "MatrixView: Instrument has key mapping: "
            << mapping->getName() << endl;
            m_localMapping = new MidiKeyMapping(*mapping);
            extendKeyMapping();
        } else {
            RG_DEBUG << "MatrixView: Instrument has no key mapping\n";
        }
    }

    m_pianoView = new QDeferScrollView(getCentralWidget());

    QWidget* vport = m_pianoView->viewport();

    if (isDrumMode() && mapping &&
            !m_localMapping->getMap().empty()) {
        m_pitchRuler = new PercussionPitchRuler(vport,
                                                m_localMapping,
                                                resolution); // line spacing
    } else {
        m_pitchRuler = new PianoKeyboard(vport);
    }

    m_pianoView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->addChild(m_pitchRuler);
    m_pianoView->setFixedWidth(m_pianoView->contentsWidth());

    m_grid->addWidget(m_pianoView, CANVASVIEW_ROW, 1);

    m_parameterBox = new InstrumentParameterBox(getDocument(), m_dockLeft);
    m_dockLeft->setWidget(m_parameterBox);

    RosegardenGUIApp *app = RosegardenGUIApp::self();
    connect(app,
            SIGNAL(pluginSelected(InstrumentId, int, int)),
            m_parameterBox,
            SLOT(slotPluginSelected(InstrumentId, int, int)));
    connect(app,
            SIGNAL(pluginBypassed(InstrumentId, int, bool)),
            m_parameterBox,
            SLOT(slotPluginBypassed(InstrumentId, int, bool)));
    connect(app,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            m_parameterBox,
            SLOT(slotInstrumentParametersChanged(InstrumentId)));
    connect(m_parameterBox,
            SIGNAL(instrumentParametersChanged(InstrumentId)),
            app,
            SIGNAL(instrumentParametersChanged(InstrumentId)));
    connect(m_parameterBox,
            SIGNAL(selectPlugin(QWidget *, InstrumentId, int)),
            app,
            SLOT(slotShowPluginDialog(QWidget *, InstrumentId, int)));
    connect(m_parameterBox,
            SIGNAL(showPluginGUI(InstrumentId, int)),
            app,
            SLOT(slotShowPluginGUI(InstrumentId, int)));
    connect(parent,  // RosegardenGUIView
            SIGNAL(checkTrackAssignments()),
            this,
            SLOT(slotCheckTrackAssignments()));

    // Assign the instrument
    //
    m_parameterBox->useInstrument(instr);

    if (m_drumMode) {
        connect(m_parameterBox,
                SIGNAL(instrumentPercussionSetChanged(Instrument *)),
                this,
                SLOT(slotPercussionSetChanged(Instrument *)));
    }

    // Set the snap grid from the stored size in the segment
    //
    int snapGridSize = m_staffs[0]->getSegment().getSnapGridSize();

    MATRIX_DEBUG << "MatrixView : Snap Grid Size = " << snapGridSize << endl;

    if (snapGridSize != -1) {
        m_snapGrid->setSnapTime(snapGridSize);
    } else {
        m_config->setGroup(MatrixViewConfigGroup);
        snapGridSize = m_config->readNumEntry
            ("Snap Grid Size", SnapGrid::SnapToBeat);
        m_snapGrid->setSnapTime(snapGridSize);
        m_staffs[0]->getSegment().setSnapGridSize(snapGridSize);
    }

    m_canvasView = new MatrixCanvasView(*m_staffs[0],
                                        m_snapGrid,
                                        m_drumMode,
                                        tCanvas,
                                        getCentralWidget());
    setCanvasView(m_canvasView);

    // do this after we have a canvas
    setupActions();
    setupAddControlRulerMenu();

    stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);

    // tool bars
    initActionsToolbar();
    initZoomToolbar();

    // Connect vertical scrollbars between matrix and piano
    //
    connect(m_canvasView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView, SIGNAL(zoomIn()), this, SLOT(slotZoomIn()));
    connect(m_canvasView, SIGNAL(zoomOut()), this, SLOT(slotZoomOut()));

    connect(m_pianoView, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_canvasView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // ensure the piano keyb keeps the right margins when the user toggles
    // the canvas view rulers
    //
    connect(m_canvasView, SIGNAL(bottomWidgetHeightChanged(int)),
            this, SLOT(slotCanvasBottomWidgetHeightChanged(int)));

    connect(m_canvasView, SIGNAL(mouseEntered()),
            this, SLOT(slotMouseEnteredCanvasView()));

    connect(m_canvasView, SIGNAL(mouseLeft()),
            this, SLOT(slotMouseLeftCanvasView()));

    /*
    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,            SLOT  (activeItemPressed(QMouseEvent*, QCanvasItem*)));
         */

    QObject::connect
    (getCanvasView(),
     SIGNAL(mousePressed(timeT,
                         int, QMouseEvent*, MatrixElement*)),
     this,
     SLOT(slotMousePressed(timeT,
                           int, QMouseEvent*, MatrixElement*)));

    QObject::connect
    (getCanvasView(),
     SIGNAL(mouseMoved(timeT, int, QMouseEvent*)),
     this,
     SLOT(slotMouseMoved(timeT, int, QMouseEvent*)));

    QObject::connect
    (getCanvasView(),
     SIGNAL(mouseReleased(timeT, int, QMouseEvent*)),
     this,
     SLOT(slotMouseReleased(timeT, int, QMouseEvent*)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
     this, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));

    QObject::connect
    (m_pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
     this, SLOT (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
     this, SLOT (slotKeyPressed(unsigned int, bool)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
     this, SLOT (slotKeySelected(unsigned int, bool)));

    QObject::connect
    (m_pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
     this, SLOT (slotKeyReleased(unsigned int, bool)));

    QObject::connect
    (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
     this, SLOT (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
    (doc, SIGNAL(pointerPositionChanged(timeT)),
     this, SLOT(slotSetPointerPosition(timeT)));

    MATRIX_DEBUG << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus
            (m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
        }
    }

    StandardRuler *topStandardRuler = new StandardRuler(getDocument(),
                                &m_hlayout, int(xorigin), 25,
                                false, getCentralWidget());
    topStandardRuler->setSnapGrid(m_snapGrid);
    setTopStandardRuler(topStandardRuler);

    StandardRuler *bottomStandardRuler = new StandardRuler(getDocument(),
                                   &m_hlayout, 0, 25,
                                   true, getBottomWidget());
    bottomStandardRuler->setSnapGrid(m_snapGrid);
    setBottomStandardRuler(bottomStandardRuler);

    topStandardRuler->connectRulerToDocPointer(doc);
    bottomStandardRuler->connectRulerToDocPointer(doc);

    // Disconnect the default connections for this signal from the
    // top ruler, and connect our own instead

    QObject::disconnect
    (topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)), 0, 0);

    QObject::connect
    (topStandardRuler->getLoopRuler(),
     SIGNAL(setPointerPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    QObject::connect
    (topStandardRuler,
     SIGNAL(dragPointerToPosition(timeT)),
     this, SLOT(slotSetInsertCursorPosition(timeT)));

    topStandardRuler->getLoopRuler()->setBackgroundColor
    (GUIPalette::getColour(GUIPalette::InsertCursorRuler));

    connect(topStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(topStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    connect(bottomStandardRuler->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(bottomStandardRuler->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotSetPointerPosition(timeT)));

    // Force height for the moment
    //
    m_pitchRuler->setFixedHeight(canvas()->height());


    updateViewCaption();

    // Add a velocity ruler
    //
    //!!!    addPropertyViewRuler(BaseProperties::VELOCITY);

    m_chordNameRuler = new ChordNameRuler
                       (m_referenceRuler, doc, segments, 0, 20, getCentralWidget());
    m_chordNameRuler->setStudio(&getDocument()->getStudio());
    addRuler(m_chordNameRuler);

    m_tempoRuler = new TempoRuler
                   (m_referenceRuler, doc, this, 0, 24, false, getCentralWidget());
    static_cast<TempoRuler *>(m_tempoRuler)->connectSignals();
    addRuler(m_tempoRuler);

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    timeT start = doc->getComposition().getLoopStart();
    timeT end = doc->getComposition().getLoopEnd();
    m_topStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);
    m_bottomStandardRuler->getLoopRuler()->slotSetLoopMarker(start, end);

    setCurrentSelection(0, false);

    // Change this if the matrix view ever has its own page
    // in the config dialog.
    setConfigDialogPageIndex(0);

    // default zoom
    m_config->setGroup(MatrixViewConfigGroup);
    double zoom = m_config->readDoubleNumEntry("Zoom Level",
                                               m_hZoomSlider->getCurrentSize());
    m_hZoomSlider->setSize(zoom);
    m_referenceRuler->setHScaleFactor(zoom);
    
    // Scroll view to centre middle-C and warp to pointer position
    //
    m_canvasView->scrollBy(0, m_staffs[0]->getCanvasYForHeight(60) / 2);

    slotSetPointerPosition(comp.getPosition());

    // All toolbars should be created before this is called
    setAutoSaveSettings("MatrixView", true);

    readOptions();
    setOutOfCtor();

    // Property and Control Rulers
    //
    if (getCurrentSegment()->getViewFeatures())
        slotShowVelocityControlRuler();
    setupControllerTabs();

    setRewFFwdToAutoRepeat();
    slotCompositionStateUpdate();
}

MatrixView::~MatrixView()
{
    slotSaveOptions();

    delete m_chordNameRuler;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // This looks silly but the reason is that on destruction of the
    // MatrixCanvasView, setCanvas() is called (this is in
    // ~QCanvasView so we can't do anything about it). This calls
    // QCanvasView::updateContentsSize(), which in turn updates the
    // view's scrollbars, hence calling QScrollBar::setValue(), and
    // sending the QSCrollbar::valueChanged() signal. But we have a
    // slot connected to that signal
    // (MatrixView::slotVerticalScrollPianoKeyboard), which scrolls
    // the pianoView. However at this stage the pianoView has already
    // been deleted, so a likely outcome is a crash.
    //
    // A solution is to zero out m_pianoView here, and to check if
    // it's non null in slotVerticalScrollPianoKeyboard.
    //
    m_pianoView = 0;

    delete m_snapGrid;

    if (m_localMapping)
        delete m_localMapping;
}

void MatrixView::slotSaveOptions()
{
    m_config->setGroup(MatrixViewConfigGroup);

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler", getToggleAction("show_tempo_ruler")->isChecked());
    m_config->writeEntry("Show Parameters", m_dockVisible);
    //getToggleAction("m_dockLeft->isVisible());

    m_config->sync();
}

void MatrixView::readOptions()
{
    EditView::readOptions();
    m_config->setGroup(MatrixViewConfigGroup);

    bool opt = false;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", false);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = m_config->readBoolEntry("Show Tempo Ruler", true);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = m_config->readBoolEntry("Show Parameters", true);
    if (!opt) {
        m_dockLeft->undock();
        m_dockLeft->hide();
        stateChanged("parametersbox_closed", KXMLGUIClient::StateNoReverse);
        m_dockVisible = false;
    }

}

void MatrixView::setupActions()
{
    EditViewBase::setupActions("matrix.rc");
    EditView::setupActions();

    //
    // Edition tools (eraser, selector...)
    //
    KRadioAction* toolAction = 0;

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/select.xpm"));

    toolAction = new KRadioAction(i18n("&Select and Edit"), icon, Key_F2,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Draw"), "pencil", Key_F3,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "draw");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Erase"), "eraser", Key_F4,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Move"), "move", Key_F5,
                                  this, SLOT(slotMoveSelected()),
                                  actionCollection(), "move");
    toolAction->setExclusiveGroup("tools");

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/resize.xpm");
    icon = QIconSet(pixmap);
    toolAction = new KRadioAction(i18n("Resi&ze"), icon, Key_F6,
                                  this, SLOT(slotResizeSelected()),
                                  actionCollection(), "resize");
    toolAction->setExclusiveGroup("tools");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("chord")));
    (new KToggleAction(i18n("C&hord Insert Mode"), icon, Key_H,
                       this, SLOT(slotUpdateInsertModeStatus()),
                       actionCollection(), "chord_mode"))->
    setChecked(false);

    pixmap.load(pixmapDir + "/toolbar/step_by_step.xpm");
    icon = QIconSet(pixmap);
    new KToggleAction(i18n("Ste&p Recording"), icon, 0, this,
                      SLOT(slotToggleStepByStep()), actionCollection(),
                      "toggle_step_by_step");

    pixmap.load(pixmapDir + "/toolbar/quantize.png");
    icon = QIconSet(pixmap);
    new KAction(EventQuantizeCommand::getGlobalName(), icon, Key_Equal, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(i18n("Repeat Last Quantize"), Key_Plus, this,
                SLOT(slotTransformsRepeatQuantize()), actionCollection(),
                "repeat_quantize");

    new KAction(CollapseNotesCommand::getGlobalName(), Key_Equal + CTRL, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(i18n("&Legato"), Key_Minus, this,
                SLOT(slotTransformsLegato()), actionCollection(),
                "legatoize");

    new KAction(ChangeVelocityCommand::getGlobalName(10), 0,
                Key_Up + SHIFT, this,
                SLOT(slotVelocityUp()), actionCollection(),
                "velocity_up");

    new KAction(ChangeVelocityCommand::getGlobalName( -10), 0,
                Key_Down + SHIFT, this,
                SLOT(slotVelocityDown()), actionCollection(),
                "velocity_down");

    new KAction(i18n("Set to Current Velocity"), 0, this,
                SLOT(slotSetVelocitiesToCurrent()), actionCollection(),
                "set_to_current_velocity");

    new KAction(i18n("Set Event &Velocities..."), 0, this,
                SLOT(slotSetVelocities()), actionCollection(),
                "set_velocities");

    new KAction(i18n("Trigger Se&gment..."), 0, this,
                SLOT(slotTriggerSegment()), actionCollection(),
                "trigger_segment");

    new KAction(i18n("Remove Triggers..."), 0, this,
                SLOT(slotRemoveTriggers()), actionCollection(),
                "remove_trigger");

    new KAction(i18n("Select &All"), Key_A + CTRL, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(i18n("&Delete"), Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");

    new KAction(i18n("Cursor &Back"), 0, Key_Left, this,
                SLOT(slotStepBackward()), actionCollection(),
                "cursor_back");

    new KAction(i18n("Cursor &Forward"), 0, Key_Right, this,
                SLOT(slotStepForward()), actionCollection(),
                "cursor_forward");

    new KAction(i18n("Cursor Ba&ck Bar"), 0, Key_Left + CTRL, this,
                SLOT(slotJumpBackward()), actionCollection(),
                "cursor_back_bar");

    new KAction(i18n("Cursor For&ward Bar"), 0, Key_Right + CTRL, this,
                SLOT(slotJumpForward()), actionCollection(),
                "cursor_forward_bar");

    new KAction(i18n("Cursor Back and Se&lect"), SHIFT + Key_Left, this,
                SLOT(slotExtendSelectionBackward()), actionCollection(),
                "extend_selection_backward");

    new KAction(i18n("Cursor Forward and &Select"), SHIFT + Key_Right, this,
                SLOT(slotExtendSelectionForward()), actionCollection(),
                "extend_selection_forward");

    new KAction(i18n("Cursor Back Bar and Select"), SHIFT + CTRL + Key_Left, this,
                SLOT(slotExtendSelectionBackwardBar()), actionCollection(),
                "extend_selection_backward_bar");

    new KAction(i18n("Cursor Forward Bar and Select"), SHIFT + CTRL + Key_Right, this,
                SLOT(slotExtendSelectionForwardBar()), actionCollection(),
                "extend_selection_forward_bar");

    new KAction(i18n("Cursor to St&art"), 0,
                /* #1025717: conflicting meanings for ctrl+a - dupe with Select All
                  Key_A + CTRL, */ this, 
                SLOT(slotJumpToStart()), actionCollection(),
                "cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
                SLOT(slotJumpToEnd()), actionCollection(),
                "cursor_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
                SLOT(slotJumpCursorToPlayback()), actionCollection(),
                "cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-play")));
    KAction *play = new KAction(i18n("&Play"), icon, Key_Enter, this,
                SIGNAL(play()), actionCollection(), "play");
    // Alternative shortcut for Play
    KShortcut playShortcut = play->shortcut();
    playShortcut.append( KKey(Key_Return + CTRL) );
    play->setShortcut(playShortcut);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
                SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
                SIGNAL(rewindPlayback()), actionCollection(),
                "playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
                SIGNAL(fastForwardPlayback()), actionCollection(),
                "playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
                SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
                "playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
                SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
                "playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
                SLOT(slotJumpPlaybackToCursor()), actionCollection(),
                "playback_pointer_to_cursor");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                      SLOT(slotToggleSolo()), actionCollection(),
                      "toggle_solo");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-tracking")));
    (new KToggleAction(i18n("Scro&ll to Follow Playback"), icon, Key_Pause, this,
                       SLOT(slotToggleTracking()), actionCollection(),
                       "toggle_tracking"))->setChecked(m_playTracking);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-panic")));
    new KAction(i18n("Panic"), icon, Key_P + CTRL + ALT, this,
                SIGNAL(panic()), actionCollection(), "panic");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
                SLOT(slotPreviewSelection()), actionCollection(),
                "preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
                SLOT(slotClearLoop()), actionCollection(),
                "clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
                SLOT(slotClearSelection()), actionCollection(),
                "clear_selection");

    //    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), "filter", Key_F + CTRL, this,
                SLOT(slotFilterSelection()), actionCollection(),
                "filter_selection");

    timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.push_back(SnapGrid::NoSnap);
    m_snapValues.push_back(SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 12);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 6);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 3);
    m_snapValues.push_back(crotchetDuration / 2);
    m_snapValues.push_back((crotchetDuration * 3) / 4);
    m_snapValues.push_back(crotchetDuration);
    m_snapValues.push_back((crotchetDuration * 3) / 2);
    m_snapValues.push_back(crotchetDuration * 2);
    m_snapValues.push_back(SnapGrid::SnapToBeat);
    m_snapValues.push_back(SnapGrid::SnapToBar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            new KAction(i18n("&No Snap"), 0, this,
                        SLOT(slotSetSnapFromAction()),
                        actionCollection(), "snap_none");
        } else if (d == SnapGrid::SnapToUnit) {
        } else if (d == SnapGrid::SnapToBeat) {
            new KAction(i18n("Snap to Bea&t"), Key_1, this,
                        SLOT(slotSetSnapFromAction()),
                        actionCollection(), "snap_beat");
        } else if (d == SnapGrid::SnapToBar) {
            new KAction(i18n("Snap to &Bar"), Key_5, this,
                        SLOT(slotSetSnapFromAction()),
                        actionCollection(), "snap_bar");
        } else {

            timeT err = 0;
            QString label = NotationStrings::makeNoteMenuLabel(d, true, err);
            QPixmap pixmap = NotePixmapFactory::toQPixmap
                (NotePixmapFactory::makeNoteMenuPixmap(d, err));

            KShortcut cut = 0;
            if (d == crotchetDuration / 16) cut = Key_0;
            else if (d == crotchetDuration / 8) cut = Key_3;
            else if (d == crotchetDuration / 4) cut = Key_6;
            else if (d == crotchetDuration / 2) cut = Key_8;
            else if (d == crotchetDuration) cut = Key_4;
            else if (d == crotchetDuration * 2) cut = Key_2;

            QString actionName = QString("snap_%1").arg(int((crotchetDuration * 4) / d));
            if (d == (crotchetDuration * 3) / 4) actionName = "snap_dotted_8";
            if (d == (crotchetDuration * 3) / 2) actionName = "snap_dotted_4";
            new KAction(i18n("Snap to %1").arg(label), pixmap, cut, this,
                        SLOT(slotSetSnapFromAction()), actionCollection(),
                        actionName);
        }
    }

    //
    // Settings menu
    //
    new KAction(i18n("Show Instrument Parameters"), 0, this,
                SLOT(slotDockParametersBack()),
                actionCollection(),
                "show_inst_parameters");

    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");

    createGUI(getRCFileName(), false);

    if (getSegmentsOnlyRestsAndClefs())
        actionCollection()->action("draw")->activate();
    else
        actionCollection()->action("select")->activate();
}

bool
MatrixView::isInChordMode()
{
    return ((KToggleAction *)actionCollection()->action("chord_mode"))->
           isChecked();
}

void MatrixView::slotDockParametersBack()
{
    m_dockLeft->dockBack();
}

void MatrixView::slotParametersClosed()
{
    stateChanged("parametersbox_closed");
    m_dockVisible = false;
}

void MatrixView::slotParametersDockedBack(KDockWidget* dw, KDockWidget::DockPosition)
{
    if (dw == m_dockLeft) {
        stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);
        m_dockVisible = true;
    }
}

void MatrixView::slotCheckTrackAssignments()
{
    Track *track =
        m_staffs[0]->getSegment().getComposition()->
        getTrackById(m_staffs[0]->getSegment().getTrack());

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());

    m_parameterBox->useInstrument(instr);
}

void MatrixView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverNoteName = new QLabel(sb);

    m_hoveredOverAbsoluteTime->setMinimumWidth(175);
    m_hoveredOverNoteName->setMinimumWidth(65);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    m_insertModeLabel = new QLabel(sb);
    m_insertModeLabel->setMinimumWidth(20);
    sb->addWidget(m_insertModeLabel);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);
}

void MatrixView::slotToolHelpChanged(const QString &s)
{
    QString msg = " " + s;
    if (m_toolContextHelp == msg) return;
    m_toolContextHelp = msg;

    m_config->setGroup(GeneralOptionsConfigGroup);
    if (!m_config->readBoolEntry("toolcontexthelp", true)) return;

    if (m_mouseInCanvasView) statusBar()->changeItem(m_toolContextHelp, 1);
}

void MatrixView::slotMouseEnteredCanvasView()
{
    m_config->setGroup(GeneralOptionsConfigGroup);
    if (!m_config->readBoolEntry("toolcontexthelp", true)) return;

    m_mouseInCanvasView = true;
    statusBar()->changeItem(m_toolContextHelp, 1);
}

void MatrixView::slotMouseLeftCanvasView()
{
    m_mouseInCanvasView = false;
    statusBar()->changeItem(KTmpStatusMsg::getDefaultMsg(), 1);
}

bool MatrixView::applyLayout(int staffNo,
                             timeT startTime,
                             timeT endTime)
{
    Profiler profiler("MatrixView::applyLayout", true);

    m_hlayout.reset();
    m_vlayout.reset();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo)
            continue;

        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
    }

    m_hlayout.finishLayout();
    m_vlayout.finishLayout();

    if (m_staffs[0]->getSegment().getEndMarkerTime() != m_lastEndMarkerTime ||
            m_lastEndMarkerTime == 0 ||
            isCompositionModified()) {
        readjustCanvasSize();
        m_lastEndMarkerTime = m_staffs[0]->getSegment().getEndMarkerTime();
    }

    return true;
}

void MatrixView::refreshSegment(Segment *segment,
                                timeT startTime, timeT endTime)
{
    Profiler profiler("MatrixView::refreshSegment", true);

    MATRIX_DEBUG << "MatrixView::refreshSegment(" << startTime
    << ", " << endTime << ")\n";

    applyLayout( -1, startTime, endTime);

    if (!segment)
        segment = m_segments[0];

    if (endTime == 0)
        endTime = segment->getEndTime();
    else if (startTime == endTime) {
        startTime = segment->getStartTime();
        endTime = segment->getEndTime();
    }

    m_staffs[0]->positionElements(startTime, endTime);
    repaintRulers();
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    MATRIX_DEBUG << "MatrixView::setViewSize() w = " << s.width() << endl;

    canvas()->resize(getXbyInverseWorldMatrix(s.width()), s.height());
    getCanvasView()->resizeContents(s.width(), s.height());

    MATRIX_DEBUG << "MatrixView::setViewSize() contentsWidth = " << getCanvasView()->contentsWidth() << endl;
}

void MatrixView::repaintRulers()
{
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++)
        m_propertyViewRulers[i].first->repaint();
}

void MatrixView::updateView()
{
    canvas()->update();
}

void MatrixView::setCurrentSelection(EventSelection* s, bool preview,
                                     bool redrawNow)
{
    //!!! rather too much here shared with notationview -- could much of
    // this be in editview?

    if (m_currentEventSelection == s)	{
        updateQuantizeCombo();
        return ;
    }

    if (m_currentEventSelection) {
        getStaff(0)->positionElements(m_currentEventSelection->getStartTime(),
                                      m_currentEventSelection->getEndTime());
    }

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    timeT startA, endA, startB, endB;

    if (oldSelection) {
        startA = oldSelection->getStartTime();
        endA = oldSelection->getEndTime();
        startB = s ? s->getStartTime() : startA;
        endB = s ? s->getEndTime() : endA;
    } else {
        // we know they can't both be null -- first thing we tested above
        startA = startB = s->getStartTime();
        endA = endB = s->getEndTime();
    }

    // refreshSegment takes start==end to mean refresh everything
    if (startA == endA)
        ++endA;
    if (startB == endB)
        ++endB;

    bool updateRequired = true;

    if (s) {

        bool foundNewEvent = false;

        for (EventSelection::eventcontainer::iterator i =
                    s->getSegmentEvents().begin();
                i != s->getSegmentEvents().end(); ++i) {

            if (oldSelection && oldSelection->getSegment() == s->getSegment()
                    && oldSelection->contains(*i))
                continue;

            foundNewEvent = true;

            if (preview) {
                long pitch;
                if ((*i)->get<Int>(BaseProperties::PITCH, pitch)) {
                    long velocity = -1;
                    (void)((*i)->get<Int>(BaseProperties::VELOCITY, velocity));
                    if (!((*i)->has(BaseProperties::TIED_BACKWARD) &&
                          (*i)->get<Bool>(BaseProperties::TIED_BACKWARD)))
                        playNote(s->getSegment(), pitch, velocity);
                }
            }
        }

        if (!foundNewEvent) {
            if (oldSelection &&
                    oldSelection->getSegment() == s->getSegment() &&
                    oldSelection->getSegmentEvents().size() ==
                    s->getSegmentEvents().size())
                updateRequired = false;
        }
    }

    if (updateRequired) {

        if ((endA >= startB && endB >= startA) &&
                (!s || !oldSelection ||
                 oldSelection->getSegment() == s->getSegment())) {

            Segment &segment(s ? s->getSegment() :
                             oldSelection->getSegment());

            if (redrawNow) {
                // recolour the events now
                getStaff(segment)->positionElements(std::min(startA, startB),
                                                    std::max(endA, endB));
            } else {
                // mark refresh status and then request a repaint
                segment.getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(segment)->getId()]).
                push(std::min(startA, startB), std::max(endA, endB));
            }

        } else {
            // do two refreshes, one for each -- here we know neither is null

            if (redrawNow) {
                // recolour the events now
                getStaff(oldSelection->getSegment())->positionElements(startA,
                        endA);

                getStaff(s->getSegment())->positionElements(startB, endB);
            } else {
                // mark refresh status and then request a repaint

                oldSelection->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(oldSelection->getSegment())->getId()]).
                push(startA, endA);

                s->getSegment().getRefreshStatus
                (m_segmentsRefreshStatusIds
                 [getStaff(s->getSegment())->getId()]).
                push(startB, endB);
            }
        }
    }

    delete oldSelection;

    if (s) {

        int eventsSelected = s->getSegmentEvents().size();
        m_selectionCounter->setText
        (i18n("  1 event selected ",
              "  %n events selected ", eventsSelected));

    } else {
        m_selectionCounter->setText(i18n("  No selection "));
    }

    m_selectionCounter->update();

    slotSetCurrentVelocityFromSelection();

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (s) {
        stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
        if (s->contains(Note::EventType)) {
            stateChanged("have_notes_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
        if (s->contains(Note::EventRestType)) {
            stateChanged("have_rests_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
    }

    updateQuantizeCombo();

    if (redrawNow)
        updateView();
    else
        update();
}

void MatrixView::updateQuantizeCombo()
{
    timeT unit = 0;

    if (m_currentEventSelection) {
        unit =
            BasicQuantizer::getStandardQuantization
            (m_currentEventSelection);
    } else {
        unit =
            BasicQuantizer::getStandardQuantization
            (&(m_staffs[0]->getSegment()));
    }

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
        if (unit == m_quantizations[i]) {
            m_quantizeCombo->setCurrentItem(i);
            return ;
        }
    }

    m_quantizeCombo->setCurrentItem(m_quantizeCombo->count() - 1); // "Off"
}

void MatrixView::slotPaintSelected()
{
    EditTool* painter = m_toolBox->getTool(MatrixPainter::ToolName);

    setTool(painter);
}

void MatrixView::slotEraseSelected()
{
    EditTool* eraser = m_toolBox->getTool(MatrixEraser::ToolName);

    setTool(eraser);
}

void MatrixView::slotSelectSelected()
{
    EditTool* selector = m_toolBox->getTool(MatrixSelector::ToolName);

    connect(selector, SIGNAL(gotSelection()),
            this, SLOT(slotNewSelection()));

    connect(selector, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

    setTool(selector);
}

void MatrixView::slotMoveSelected()
{
    EditTool* mover = m_toolBox->getTool(MatrixMover::ToolName);

    setTool(mover);
}

void MatrixView::slotResizeSelected()
{
    EditTool* resizer = m_toolBox->getTool(MatrixResizer::ToolName);

    setTool(resizer);
}

void MatrixView::slotTransformsQuantize()
{
    if (!m_currentEventSelection)
        return ;

    QuantizeDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
        addCommandToHistory(new EventQuantizeCommand
                            (*m_currentEventSelection,
                             dialog.getQuantizer()));
    }
}

void MatrixView::slotTransformsRepeatQuantize()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(i18n("Quantizing..."), this);
    addCommandToHistory(new EventQuantizeCommand
                        (*m_currentEventSelection,
                         "Quantize Dialog Grid", false)); // no i18n (config group name)
}

void MatrixView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), this);

    addCommandToHistory(new CollapseNotesCommand
                        (*m_currentEventSelection));
}

void MatrixView::slotTransformsLegato()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(i18n("Making legato..."), this);
    addCommandToHistory(new EventQuantizeCommand
                        (*m_currentEventSelection,
                         new LegatoQuantizer(0))); // no quantization
}

void MatrixView::slotMousePressed(timeT time, int pitch,
                                  QMouseEvent* e, MatrixElement* el)
{
    MATRIX_DEBUG << "MatrixView::mousePressed at pitch "
    << pitch << ", time " << time << endl;

    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;

    m_tool->handleMousePress(time, pitch, 0, e, el);

    if (e->button() != RightButton) {
        getCanvasView()->startAutoScroll();
    }

    // play a preview
    //playPreview(pitch);
}

void MatrixView::slotMouseMoved(timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;

    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        updateView();
    } else {
        int follow = m_tool->handleMouseMove(time, pitch, e);
        getCanvasView()->setScrollDirectionConstraint(follow);

        //        if (follow != RosegardenCanvasView::NoFollow) {
        //            getCanvasView()->doAutoScroll();
        //        }

        // play a preview
        if (pitch != m_previousEvPitch) {
            //playPreview(pitch);
            m_previousEvPitch = pitch;
        }
    }

}

void MatrixView::slotMouseReleased(timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time)
        time = curSegmentStartTime;

    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }

    // send the real event time now (not adjusted for beginning of bar)
    m_tool->handleMouseRelease(time, pitch, e);
    m_previousEvPitch = 0;
    getCanvasView()->stopAutoScroll();
}

void
MatrixView::slotHoveredOverNoteChanged(int evPitch,
                                       bool haveEvent,
                                       timeT evTime)
{
    MidiPitchLabel label(evPitch);

    if (haveEvent) {

        m_haveHoveredOverNote = true;

        int bar, beat, fraction, remainder;
        getDocument()->getComposition().getMusicalTimeForAbsoluteTime
        (evTime, bar, beat, fraction, remainder);

        RealTime rt =
            getDocument()->getComposition().getElapsedRealTime(evTime);
        long ms = rt.msec();

        QString msg = i18n("Note: %1 (%2.%3s)")
                  .arg(QString("%1-%2-%3-%4")
                       .arg(QString("%1").arg(bar + 1).rightJustify(3, '0'))
                       .arg(QString("%1").arg(beat).rightJustify(2, '0'))
                       .arg(QString("%1").arg(fraction).rightJustify(2, '0'))
                       .arg(QString("%1").arg(remainder).rightJustify(2, '0')))
                  .arg(rt.sec)
                  .arg(QString("%1").arg(ms).rightJustify(3, '0'));

        m_hoveredOverAbsoluteTime->setText(msg);
    }

    m_haveHoveredOverNote = false;

    m_hoveredOverNoteName->setText(i18n("%1 (%2)")
                                   .arg(label.getQString())
                                   .arg(evPitch));

    m_pitchRuler->drawHoverNote(evPitch);
}

void
MatrixView::slotHoveredOverKeyChanged(unsigned int y)
{
    MatrixStaff& staff = *(m_staffs[0]);

    int evPitch = staff.getHeightAtCanvasCoords( -1, y);

    if (evPitch != m_previousEvPitch) {
        MidiPitchLabel label(evPitch);
        m_hoveredOverNoteName->setText(QString("%1 (%2)").
                                       arg(label.getQString()).arg(evPitch));
        m_previousEvPitch = evPitch;
    }
}

void
MatrixView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    if (m_haveHoveredOverNote) return;

    timeT t = time;

    int bar, beat, fraction, remainder;
    getDocument()->getComposition().getMusicalTimeForAbsoluteTime
    (t, bar, beat, fraction, remainder);

    RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    // At the advice of doc.trolltech.com/3.0/qstring.html#sprintf
    // we replaced this    QString format("%ld (%ld.%03lds)");
    // to support Unicode

    QString message = i18n("Time: %1 (%2.%3s)")
                      .arg(QString("%1-%2-%3-%4")
                           .arg(QString("%1").arg(bar + 1).rightJustify(3, '0'))
                           .arg(QString("%1").arg(beat).rightJustify(2, '0'))
                           .arg(QString("%1").arg(fraction).rightJustify(2, '0'))
                           .arg(QString("%1").arg(remainder).rightJustify(2, '0')))
                      .arg(rt.sec)
                      .arg(QString("%1").arg(ms).rightJustify(3, '0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

void
MatrixView::slotSetPointerPosition(timeT time)
{
    slotSetPointerPosition(time, m_playTracking);
}

void
MatrixView::slotSetPointerPosition(timeT time, bool scroll)
{
    Composition &comp = getDocument()->getComposition();
    int barNo = comp.getBarNumber(time);

    if (barNo >= m_hlayout.getLastVisibleBarOnStaff(*m_staffs[0])) {

        Segment &seg = m_staffs[0]->getSegment();

        if (seg.isRepeating() && time < seg.getRepeatEndTime()) {
            time =
                seg.getStartTime() +
                ((time - seg.getStartTime()) %
                 (seg.getEndMarkerTime() - seg.getStartTime()));
            m_staffs[0]->setPointerPosition(m_hlayout, time);
        } else {
            m_staffs[0]->hidePointer();
            scroll = false;
        }
    } else if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[0])) {
        m_staffs[0]->hidePointer();
        scroll = false;
    } else {
        m_staffs[0]->setPointerPosition(m_hlayout, time);
    }

    if (scroll && !getCanvasView()->isAutoScrolling())
        getCanvasView()->slotScrollHoriz(static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));

    updateView();
}

void
MatrixView::slotSetInsertCursorPosition(timeT time, bool scroll)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event or grid line.

    Segment &s = m_staffs[0]->getSegment();

    if (time < s.getStartTime())     time = s.getStartTime();
    if (time > s.getEndMarkerTime()) time = s.getEndMarkerTime();
    
    m_staffs[0]->setInsertCursorPosition(m_hlayout, time);

    if (scroll && !getCanvasView()->isAutoScrolling()) {
        getCanvasView()->slotScrollHoriz
            (static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));
    }

    updateView();
}

void MatrixView::slotEditCut()
{
    MATRIX_DEBUG << "MatrixView::slotEditCut()\n";

    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
                                       getDocument()->getClipboard()));
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
                                        getDocument()->getClipboard()));

    emit usedSelection();
}

void MatrixView::slotEditPaste()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return ;
    }

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    PasteEventsCommand *command = new PasteEventsCommand
                                  (m_staffs[0]->getSegment(), getDocument()->getClipboard(),
                                   getInsertionTime(), PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
        addCommandToHistory(command);
        setCurrentSelection(new EventSelection(command->getPastedEvents()));
    }
}

void MatrixView::slotEditDelete()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));

    // clear and clear
    setCurrentSelection(0, false);
}

void MatrixView::slotKeyPressed(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    MatrixStaff& staff = *(m_staffs[0]);
    MidiByte evPitch = staff.getHeightAtCanvasCoords( -1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    Track *track = comp.getTrackById(
                       staff.getSegment().getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNote,
                   evPitch + staff.getSegment().getTranspose(),
                   MidiMaxValue,
                   RealTime::zeroTime,
                   RealTime::zeroTime,
                   RealTime::zeroTime);
    StudioControl::sendMappedEvent(mE);

}

void MatrixView::slotKeySelected(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    MatrixStaff& staff = *(m_staffs[0]);
    Segment &segment(staff.getSegment());
    MidiByte evPitch = staff.getHeightAtCanvasCoords( -1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    EventSelection *s = new EventSelection(segment);

    for (Segment::iterator i = segment.begin();
            segment.isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Note::EventType) &&
                (*i)->has(BaseProperties::PITCH)) {

            MidiByte p = (*i)->get
                         <Int>
                         (BaseProperties::PITCH);
            if (p >= std::min(m_firstNote, evPitch) &&
                    p <= std::max(m_firstNote, evPitch)) {
                s->addEvent(*i);
            }
        }
    }

    if (m_currentEventSelection) {
        // allow addFromSelection to deal with eliminating duplicates
        s->addFromSelection(m_currentEventSelection);
    }

    setCurrentSelection(s, false);

    // now play the note as well

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();
    Track *track = comp.getTrackById(segment.getTrack());
    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   evPitch + segment.getTranspose(),
                   MidiMaxValue,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);
    StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotKeyReleased(unsigned int y, bool repeating)
{
    MatrixStaff& staff = *(m_staffs[0]);
    int evPitch = staff.getHeightAtCanvasCoords(-1, y);

    if (m_lastNote == evPitch && repeating)
        return;

    Rosegarden::Segment &segment(staff.getSegment());

    // send note off (note on at zero velocity)

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(segment.getTrack());
    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    evPitch = evPitch + segment.getTranspose();
    if (evPitch < 0 || evPitch > 127) return;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNote,
                               evPitch,
                               0,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime);
    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotVerticalScrollPianoKeyboard(int y)
{
    if (m_pianoView) // check that the piano view still exists (see dtor)
        m_pianoView->setContentsPos(0, y);
}

void MatrixView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    Segment &segment = *getCurrentSegment();
    int pitch = 0;

    Accidental accidental =
        Accidentals::NoAccidental;

    timeT time(getInsertionTime());
    if (time >= segment.getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    ::Rosegarden::Key key = segment.getKeyAtTime(time);
    Clef clef = segment.getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {

        KMessageBox::sorry
        (this, i18n("Unknown note insert action %1").arg(name));
        return ;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    modelEvent.set<String>(BaseProperties::ACCIDENTAL, accidental);
    timeT endTime(time + m_snapGrid->getSnapTime(time));

    MatrixInsertionCommand* command =
        new MatrixInsertionCommand(segment, time, endTime, &modelEvent);

    addCommandToHistory(command);

    if (!isInChordMode()) {
        slotSetInsertCursorPosition(endTime);
    }
}

void MatrixView::closeWindow()
{
    delete this;
}

bool MatrixView::canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
        sinceLastCutOff = 0;
        lastCutOff = now;
    } else {
        if (sinceLastCutOff >= 20) {
            // don't permit more than 20 notes per second, to avoid
            // gungeing up the sound drivers
            MATRIX_DEBUG << "Rejecting preview (too busy)" << endl;
            return false;
        }
    }

    return true;
}

void MatrixView::playNote(Event *event)
{
    // Only play note events
    //
    if (!event->isa(Note::EventType))
        return ;

    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    // Get the Instrument
    //
    Track *track = comp.getTrackById(
                       m_staffs[0]->getSegment().getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    if (ins == 0)
        return ;

    if (!canPreviewAnotherNote())
        return ;

    // Get a velocity
    //
    MidiByte velocity = MidiMaxValue / 4; // be easy on the user's ears
    long eventVelocity = 0;
    if (event->get
            <Int>(BaseProperties::VELOCITY, eventVelocity))
        velocity = eventVelocity;

    RealTime duration =
        comp.getElapsedRealTime(event->getDuration());

    // create
    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   (MidiByte)
                   event->get
                   <Int>
                   (BaseProperties::PITCH) +
                   m_staffs[0]->getSegment().getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   duration,
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}

void MatrixView::playNote(const Segment &segment, int pitch,
                          int velocity)
{
    Composition &comp = getDocument()->getComposition();
    Studio &studio = getDocument()->getStudio();

    Track *track = comp.getTrackById(segment.getTrack());

    Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return ;

    if (velocity < 0)
        velocity = getCurrentVelocity();

    MappedEvent mE(ins->getId(),
                   MappedEvent::MidiNoteOneShot,
                   pitch + segment.getTranspose(),
                   velocity,
                   RealTime::zeroTime,
                   RealTime(0, 250000000),
                   RealTime::zeroTime);

    StudioControl::sendMappedEvent(mE);
}

MatrixStaff*
MatrixView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment)
            return m_staffs[i];
    }

    return 0;
}

void
MatrixView::setSingleSelectedEvent(int staffNo, Event *event,
                                   bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
                           preview, redrawNow);
}

void
MatrixView::setSingleSelectedEvent(Segment &segment,
                                   Event *event,
                                   bool preview, bool redrawNow)
{
    setCurrentSelection(0, false);

    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);

    //!!!
    // this used to say
    //   setCurrentSelection(selection, true)
    // since the default arg for preview is false, this changes the
    // default semantics -- test what circumstance this matters in
    // and choose an acceptable solution for both matrix & notation
    setCurrentSelection(selection, preview, redrawNow);
}

void
MatrixView::slotNewSelection()
{
    MATRIX_DEBUG << "MatrixView::slotNewSelection\n";

    //    m_parameterBox->setSelection(m_currentEventSelection);
}

void
MatrixView::slotSetSnapFromIndex(int s)
{
    slotSetSnap(m_snapValues[s]);
}

void
MatrixView::slotSetSnapFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(5) == "snap_") {
        int snap = name.right(name.length() - 5).toInt();
        if (snap > 0) {
            slotSetSnap(Note(Note::Semibreve).getDuration() / snap);
        } else if (name.left(12) == "snap_dotted_") {
            snap = name.right(name.length() - 12).toInt();
            slotSetSnap((3*Note(Note::Semibreve).getDuration()) / (2*snap));
        } else if (name == "snap_none") {
            slotSetSnap(SnapGrid::NoSnap);
        } else if (name == "snap_beat") {
            slotSetSnap(SnapGrid::SnapToBeat);
        } else if (name == "snap_bar") {
            slotSetSnap(SnapGrid::SnapToBar);
        } else if (name == "snap_unit") {
            slotSetSnap(SnapGrid::SnapToUnit);
        } else {
            MATRIX_DEBUG << "Warning: MatrixView::slotSetSnapFromAction: unrecognised action " << name << endl;
        }
    }
}

void
MatrixView::slotSetSnap(timeT t)
{
    MATRIX_DEBUG << "MatrixView::slotSetSnap: time is " << t << endl;
    m_snapGrid->setSnapTime(t);

    for (unsigned int i = 0; i < m_snapValues.size(); ++i) {
        if (m_snapValues[i] == t) {
            m_snapGridCombo->setCurrentItem(i);
            break;
        }
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i)
        m_staffs[i]->sizeStaff(m_hlayout);

    m_segments[0]->setSnapGridSize(t);

    m_config->setGroup(MatrixViewConfigGroup);
    m_config->writeEntry("Snap Grid Size", t);

    updateView();
}

void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

    timeT unit =
        ((unsigned int)q < m_quantizations.size() ? m_quantizations[q] : 0);

    Quantizer *quant =
        new BasicQuantizer
        (unit ? unit :
         Note(Note::Shortest).getDuration(), false);

    if (unit) {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
        if (m_currentEventSelection &&
                m_currentEventSelection->getAddedEvents()) {
            addCommandToHistory(new EventQuantizeCommand
                                (*m_currentEventSelection, quant));
        } else {
            Segment &s = m_staffs[0]->getSegment();
            addCommandToHistory(new EventQuantizeCommand
                                (s, s.getStartTime(), s.getEndMarkerTime(),
                                 quant));
        }
    } else {
        KTmpStatusMsg msg(i18n("Unquantizing..."), this);
        if (m_currentEventSelection &&
                m_currentEventSelection->getAddedEvents()) {
            addCommandToHistory(new EventUnquantizeCommand
                                (*m_currentEventSelection, quant));
        } else {
            Segment &s = m_staffs[0]->getSegment();
            addCommandToHistory(new EventUnquantizeCommand
                                (s, s.getStartTime(), s.getEndMarkerTime(),
                                 quant));
        }
    }
}

void
MatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar" << endl;

    KToolBar *actionsToolbar = toolBar("Actions Toolbar");

    if (!actionsToolbar) {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    // The SnapGrid combo and Snap To... menu items
    //
    QLabel *sLabel = new QLabel(i18n(" Grid: "), actionsToolbar, "kde toolbar widget");
    sLabel->setIndent(10);

    QPixmap noMap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    m_snapGridCombo = new KComboBox(actionsToolbar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            m_snapGridCombo->insertItem(i18n("None"));
        } else if (d == SnapGrid::SnapToUnit) {
            m_snapGridCombo->insertItem(i18n("Unit"));
        } else if (d == SnapGrid::SnapToBeat) {
            m_snapGridCombo->insertItem(i18n("Beat"));
        } else if (d == SnapGrid::SnapToBar) {
            m_snapGridCombo->insertItem(i18n("Bar"));
        } else {
            timeT err = 0;
            QString label = NotationStrings::makeNoteMenuLabel(d, true, err);
            QPixmap pixmap = NotePixmapFactory::toQPixmap
                (NotePixmapFactory::makeNoteMenuPixmap(d, err));
            m_snapGridCombo->insertItem((err ? noMap : pixmap), label);
        }

        if (d == m_snapGrid->getSnapSetting()) {
            m_snapGridCombo->setCurrentItem(m_snapGridCombo->count() - 1);
        }
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    // Velocity combo.  Not a spin box, because the spin box is too
    // slow to use unless we make it typeable into, and then it takes
    // focus away from our more important widgets

    QLabel *vlabel = new QLabel(i18n(" Velocity: "), actionsToolbar, "kde toolbar widget");
    vlabel->setIndent(10);
    
    m_velocityCombo = new KComboBox(actionsToolbar);
    for (int i = 0; i <= 127; ++i) {
        m_velocityCombo->insertItem(QString("%1").arg(i));
    }
    m_velocityCombo->setCurrentItem(100); //!!! associate with segment

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(i18n(" Quantize: "), actionsToolbar, "kde toolbar widget");
    qLabel->setIndent(10);

    m_quantizeCombo = new KComboBox(actionsToolbar);

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

        timeT time = m_quantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeNoteMenuPixmap(time, error));
        m_quantizeCombo->insertItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->insertItem(noMap, i18n("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));
}

void
MatrixView::initZoomToolbar()
{
    MATRIX_DEBUG << "MatrixView::initZoomToolbar" << endl;

    KToolBar *zoomToolbar = toolBar("Zoom Toolbar");

    if (!zoomToolbar) {
        MATRIX_DEBUG << "MatrixView::initZoomToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    std::vector<double> zoomSizes; // in units-per-pixel

    //double defaultBarWidth44 = 100.0;
    //double duration44 = TimeSignature(4,4).getBarDuration();

    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0, 20.0 };
    // Zoom labels
    //
    for (unsigned int i = 0; i < sizeof(factors) / sizeof(factors[0]); ++i) {
//         zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));

//         zoomSizes.push_back(factors[i] / 2); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
        zoomSizes.push_back(factors[i]);
    }

    m_hZoomSlider = new ZoomSlider<double>
                    (zoomSizes, -1, QSlider::Horizontal, zoomToolbar, "kde toolbar widget");
    m_hZoomSlider->setTracking(true);
    m_hZoomSlider->setFocusPolicy(QWidget::NoFocus);

    m_zoomLabel = new QLabel(zoomToolbar, "kde toolbar widget");
    m_zoomLabel->setIndent(10);
    m_zoomLabel->setFixedWidth(80);

    connect(m_hZoomSlider,
            SIGNAL(valueChanged(int)),
            SLOT(slotChangeHorizontalZoom(int)));

}

void
MatrixView::slotChangeHorizontalZoom(int)
{
    double zoomValue = m_hZoomSlider->getCurrentSize();

    //     m_zoomLabel->setText(i18n("%1%").arg(zoomValue*100.0 * 2)); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
    m_zoomLabel->setText(i18n("%1%").arg(zoomValue*100.0));

    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
    << zoomValue << endl;

    m_referenceRuler->setHScaleFactor(zoomValue);
    
    if (m_tempoRuler)
        m_tempoRuler->repaint();
    if (m_chordNameRuler)
        m_chordNameRuler->repaint();

    // Set zoom matrix
    //
    QWMatrix zoomMatrix;
    zoomMatrix.scale(zoomValue, 1.0);
    m_canvasView->setWorldMatrix(zoomMatrix);

    // make control rulers zoom too
    //
    setControlRulersZoom(zoomMatrix);

    if (m_topStandardRuler)
        m_topStandardRuler->setHScaleFactor(zoomValue);
    if (m_bottomStandardRuler)
        m_bottomStandardRuler->setHScaleFactor(zoomValue);

    for (unsigned int i = 0; i < m_propertyViewRulers.size(); ++i) {
        m_propertyViewRulers[i].first->setHScaleFactor(zoomValue);
        m_propertyViewRulers[i].first->repaint();
    }

    if (m_topStandardRuler)
        m_topStandardRuler->update();
    if (m_bottomStandardRuler)
        m_bottomStandardRuler->update();

    m_config->setGroup(MatrixViewConfigGroup);
    m_config->writeEntry("Zoom Level", zoomValue);

    // If you do adjust the viewsize then please remember to
    // either re-center() or remember old scrollbar position
    // and restore.
    //

    int newWidth = computePostLayoutWidth();

    //     int newWidth = int(getXbyWorldMatrix(getCanvasView()->canvas()->width()));

    // We DO NOT resize the canvas(), only the area it's displaying on
    //
    getCanvasView()->resizeContents(newWidth, getViewSize().height());

    // This forces a refresh of the h. scrollbar, even if the canvas width
    // hasn't changed
    //
    getCanvasView()->polish();

    getCanvasView()->slotScrollHoriz
        (getXbyWorldMatrix(m_staffs[0]->getLayoutXOfInsertCursor()));
}

void
MatrixView::slotZoomIn()
{
    m_hZoomSlider->increment();
}

void
MatrixView::slotZoomOut()
{
    m_hZoomSlider->decrement();
}

void
MatrixView::scrollToTime(timeT t)
{
    double layoutCoord = m_hlayout.getXForTime(t);
    getCanvasView()->slotScrollHoriz(int(layoutCoord));
}

int
MatrixView::getCurrentVelocity() const
{
    return m_velocityCombo->currentItem();
}

void
MatrixView::slotSetCurrentVelocity(int value)
{
    m_velocityCombo->setCurrentItem(value);
}


void
MatrixView::slotSetCurrentVelocityFromSelection()
{
    if (!m_currentEventSelection) return;

    float totalVelocity = 0;
    int count = 0;

    for (EventSelection::eventcontainer::iterator i =
             m_currentEventSelection->getSegmentEvents().begin();
         i != m_currentEventSelection->getSegmentEvents().end(); ++i) {

        if ((*i)->has(BaseProperties::VELOCITY)) {
            totalVelocity += (*i)->get<Int>(BaseProperties::VELOCITY);
            ++count;
        }
    }

    if (count > 0) {
        slotSetCurrentVelocity((totalVelocity / count) + 0.5);
    }
}

unsigned int
MatrixView::addPropertyViewRuler(const PropertyName &property)
{
    // Try and find this controller if it exists
    //
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++) {
        if (m_propertyViewRulers[i].first->getPropertyName() == property)
            return i;
    }

    int height = 20;

    PropertyViewRuler *newRuler = new PropertyViewRuler(&m_hlayout,
                                  m_segments[0],
                                  property,
                                  xorigin,
                                  height,
                                  getCentralWidget());

    addRuler(newRuler);

    PropertyBox *newControl = new PropertyBox(strtoqstr(property),
                              m_parameterBox->width() + m_pitchRuler->width(),
                              height,
                              getCentralWidget());

    addPropertyBox(newControl);

    m_propertyViewRulers.push_back(
        std::pair<PropertyViewRuler*, PropertyBox*>(newRuler, newControl));

    return m_propertyViewRulers.size() - 1;
}

bool
MatrixView::removePropertyViewRuler(unsigned int number)
{
    if (number > m_propertyViewRulers.size() - 1)
        return false;

    std::vector<std::pair<PropertyViewRuler*, PropertyBox*> >::iterator it
    = m_propertyViewRulers.begin();
    while (number--)
        it++;

    delete it->first;
    delete it->second;
    m_propertyViewRulers.erase(it);

    return true;
}

RulerScale*
MatrixView::getHLayout()
{
    return &m_hlayout;
}

Staff*
MatrixView::getCurrentStaff()
{
    return getStaff(0);
}

Segment *
MatrixView::getCurrentSegment()
{
    MatrixStaff *staff = getStaff(0);
    return (staff ? &staff->getSegment() : 0);
}

timeT
MatrixView::getInsertionTime()
{
    MatrixStaff *staff = m_staffs[0];
    return staff->getInsertCursorTime(m_hlayout);
}

void
MatrixView::slotStepBackward()
{
    timeT time(getInsertionTime());
    slotSetInsertCursorPosition(SnapGrid(&m_hlayout).snapTime
                                (time - 1,
                                 SnapGrid::SnapLeft));
}

void
MatrixView::slotStepForward()
{
    timeT time(getInsertionTime());
    slotSetInsertCursorPosition(SnapGrid(&m_hlayout).snapTime
                                (time + 1,
                                 SnapGrid::SnapRight));
}

void
MatrixView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
MatrixView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

void
MatrixView::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

void
MatrixView::slotSelectAll()
{
    Segment *segment = m_segments[0];
    Segment::iterator it = segment->begin();
    EventSelection *selection = new EventSelection(*segment);

    for (; segment->isBeforeEndMarker(it); it++)
        if ((*it)->isa(Note::EventType))
            selection->addEvent(*it);

    setCurrentSelection(selection, false);
}

void MatrixView::slotPreviewSelection()
{
    if (!m_currentEventSelection)
        return ;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
                               m_currentEventSelection->getEndTime());
}

void MatrixView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}

void MatrixView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_tool);

    if (!selector) {
        slotSelectSelected();
    } else {
        setCurrentSelection(0);
    }
}

void MatrixView::slotFilterSelection()
{
    RG_DEBUG << "MatrixView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotFilterSelection- accepted" << endl;

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

        if (haveEvent)
            setCurrentSelection(newSelection);
        else
            setCurrentSelection(0);
    }
}

void
MatrixView::readjustCanvasSize()
{
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        MatrixStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);

        //         if (staff.getTotalWidth() + staff.getX() > maxWidth) {
        //             maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        //         }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            if (isDrumMode()) {
                maxHeight = staff.getTotalHeight() + staff.getY() + 5;
            } else {
                maxHeight = staff.getTotalHeight() + staff.getY() + 1;
            }
        }

    }

    int newWidth = computePostLayoutWidth();

    // now get the EditView to do the biz
    readjustViewSize(QSize(newWidth, maxHeight), true);

    repaintRulers();
}

void MatrixView::slotVelocityUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Raising velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand(10, *m_currentEventSelection));

    slotSetCurrentVelocityFromSelection();
}

void MatrixView::slotVelocityDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Lowering velocities..."), this);

    addCommandToHistory
    (new ChangeVelocityCommand( -10, *m_currentEventSelection));

    slotSetCurrentVelocityFromSelection();
}

void
MatrixView::slotSetVelocities()
{
    if (!m_currentEventSelection)
        return ;

    EventParameterDialog dialog(this,
                                i18n("Set Event Velocities"),
                                BaseProperties::VELOCITY,
                                getCurrentVelocity());

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Setting Velocities..."), this);
        addCommandToHistory(new SelectionPropertyCommand
                            (m_currentEventSelection,
                             BaseProperties::VELOCITY,
                             dialog.getPattern(),
                             dialog.getValue1(),
                             dialog.getValue2()));
    }
}

void
MatrixView::slotSetVelocitiesToCurrent()
{
    if (!m_currentEventSelection) return;

    addCommandToHistory(new SelectionPropertyCommand
                        (m_currentEventSelection,
                         BaseProperties::VELOCITY,
                         FlatPattern,
                         getCurrentVelocity(),
                         getCurrentVelocity()));
}

void
MatrixView::slotTriggerSegment()
{
    if (!m_currentEventSelection)
        return ;

    TriggerSegmentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted)
        return ;

    addCommandToHistory(new SetTriggerCommand(*m_currentEventSelection,
                        dialog.getId(),
                        true,
                        dialog.getRetune(),
                        dialog.getTimeAdjust(),
                        Marks::NoMark,
                        i18n("Trigger Segment")));
}

void
MatrixView::slotRemoveTriggers()
{
    if (!m_currentEventSelection)
        return ;

    addCommandToHistory(new ClearTriggersCommand(*m_currentEventSelection,
                        i18n("Remove Triggers")));
}

void
MatrixView::slotToggleChordsRuler()
{
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void
MatrixView::slotToggleTempoRuler()
{
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void
MatrixView::paintEvent(QPaintEvent* e)
{
    //!!! There's a lot of code shared between matrix and notation for
    // dealing with step recording (the insertable note event stuff).
    // It should probably be factored out into a base class, but I'm
    // not sure I wouldn't rather wait until the functionality is all
    // sorted in both matrix and notation so we can be sure how much
    // of it is actually common.

    EditView::paintEvent(e);

    // now deal with any backlog of insertable notes that appeared
    // during paint (because it's not safe to modify a segment from
    // within a sub-event-loop in a processEvents call from a paint)
    if (!m_pendingInsertableNotes.empty()) {
        std::vector<std::pair<int, int> > notes = m_pendingInsertableNotes;
        m_pendingInsertableNotes.clear();
        for (unsigned int i = 0; i < notes.size(); ++i) {
            slotInsertableNoteEventReceived(notes[i].first, notes[i].second, true);
        }
    }
}

void
MatrixView::updateViewCaption()
{
    // Set client label
    //
    QString view = i18n("Matrix");
    if (isDrumMode())
        view = i18n("Percussion");

    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        setCaption(i18n("%1 - Segment Track #%2 - %3")
                   .arg(getDocument()->getTitle())
                   .arg(trackPosition + 1)
                   .arg(view));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setCaption(i18n("%1 - All Segments - %2")
                   .arg(getDocument()->getTitle())
                   .arg(view));

    } else {

        setCaption(i18n("%1 - 1 Segment - %2",
                        "%1 - %n Segments - %2",
                        m_segments.size())
                   .arg(getDocument()->getTitle())
                   .arg(view));
    }
}

int MatrixView::computePostLayoutWidth()
{
    Segment *segment = m_segments[0];
    Composition *composition = segment->getComposition();
    int endX = int(m_hlayout.getXForTime
                   (composition->getBarEndForTime
                    (segment->getEndMarkerTime())));
    int startX = int(m_hlayout.getXForTime
                     (composition->getBarStartForTime
                      (segment->getStartTime())));

    int newWidth = int(getXbyWorldMatrix(endX - startX));

    MATRIX_DEBUG << "MatrixView::readjustCanvasSize() : startX = "
    << startX
    << " endX = " << endX
    << " newWidth = " << newWidth
    << " endmarkertime : " << segment->getEndMarkerTime()
    << " barEnd for time : " << composition->getBarEndForTime(segment->getEndMarkerTime())
    << endl;

    newWidth += 12;
    if (isDrumMode())
        newWidth += 12;

    return newWidth;
}

bool MatrixView::getMinMaxPitches(int& minPitch, int& maxPitch)
{
    minPitch = MatrixVLayout::maxMIDIPitch + 1;
    maxPitch = MatrixVLayout::minMIDIPitch - 1;

    std::vector<MatrixStaff*>::iterator sit;
    for (sit = m_staffs.begin(); sit != m_staffs.end(); ++sit) {

        MatrixElementList *mel = (*sit)->getViewElementList();
        MatrixElementList::iterator eit;
        for (eit = mel->begin(); eit != mel->end(); ++eit) {

            NotationElement *el = static_cast<NotationElement*>(*eit);
            if (el->isNote()) {
                Event* ev = el->event();
                int pitch = ev->get
                            <Int>
                            (BaseProperties::PITCH);
                if (minPitch > pitch)
                    minPitch = pitch;
                if (maxPitch < pitch)
                    maxPitch = pitch;
            }
        }
    }

    return maxPitch >= minPitch;
}

void MatrixView::extendKeyMapping()
{
    int minStaffPitch, maxStaffPitch;
    if (getMinMaxPitches(minStaffPitch, maxStaffPitch)) {
        int minKMPitch = m_localMapping->getPitchForOffset(0);
        int maxKMPitch = m_localMapping->getPitchForOffset(0)
                         + m_localMapping->getPitchExtent() - 1;
        if (minStaffPitch < minKMPitch)
            m_localMapping->getMap()[minStaffPitch] = std::string("");
        if (maxStaffPitch > maxKMPitch)
            m_localMapping->getMap()[maxStaffPitch] = std::string("");
    }
}

void
MatrixView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    // hjj:
    // The default insertion mode is implemented equivalently in
    // notationviewslots.cpp:
    //  - proceed if notes do not overlap
    //  - make the chord if notes do overlap, and do not proceed

    static int numberOfNotesOn = 0;
    static time_t lastInsertionTime = 0;
    if (!noteOn) {
        numberOfNotesOn--;
        return ;
    }

    KToggleAction *action = dynamic_cast<KToggleAction *>
                            (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (!action->isChecked())
        return ;

    if (m_inPaintEvent) {
        m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
        return ;
    }

    Segment &segment = *getCurrentSegment();

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment.getTranspose();

    KTmpStatusMsg msg(i18n("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    static timeT insertionTime(getInsertionTime());
    if (insertionTime >= segment.getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    time_t now;
    time (&now);
    double elapsed = difftime(now, lastInsertionTime);
    time (&lastInsertionTime);

    if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
        numberOfNotesOn = 0;
        insertionTime = getInsertionTime();
    }
    numberOfNotesOn++;
    timeT endTime(insertionTime + m_snapGrid->getSnapTime(insertionTime));

    if (endTime <= insertionTime) {
        static bool showingError = false;
        if (showingError)
            return ;
        showingError = true;
        KMessageBox::sorry(this, i18n("Can't insert note: No grid duration selected"));
        showingError = false;
        return ;
    }

    MatrixInsertionCommand* command =
        new MatrixInsertionCommand(segment, insertionTime, endTime, &modelEvent);

    addCommandToHistory(command);

    if (!isInChordMode()) {
        slotSetInsertCursorPosition(endTime);
    }
}

void
MatrixView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
MatrixView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
MatrixView::slotToggleStepByStep()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
                            (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (action->isChecked()) { // after toggling, that is
        emit stepByStepTargetRequested(this);
    } else {
        emit stepByStepTargetRequested(0);
    }
}

void
MatrixView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
        message = i18n(" Chord ");
    } else {
        message = "";
    }
    m_insertModeLabel->setText(message);
}

void
MatrixView::slotStepByStepTargetRequested(QObject *obj)
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
                            (actionCollection()->action("toggle_step_by_step"));
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    action->setChecked(obj == this);
}

void
MatrixView::slotInstrumentLevelsChanged(InstrumentId id,
                                        const LevelInfo &info)
{
    if (!m_parameterBox)
        return ;

    Composition &comp = getDocument()->getComposition();

    Track *track =
        comp.getTrackById(m_staffs[0]->getSegment().getTrack());
    if (!track || track->getInstrument() != id)
        return ;

    Instrument *instr = getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());
    if (!instr || instr->getType() != Instrument::SoftSynth)
        return ;

    float dBleft = AudioLevel::fader_to_dB
                   (info.level, 127, AudioLevel::LongFader);
    float dBright = AudioLevel::fader_to_dB
                    (info.levelRight, 127, AudioLevel::LongFader);

    m_parameterBox->setAudioMeter(dBleft, dBright,
                                  AudioLevel::DB_FLOOR,
                                  AudioLevel::DB_FLOOR);
}

void
MatrixView::slotPercussionSetChanged(Instrument * newInstr)
{
    // Must be called only when in drum mode
    assert(m_drumMode);

    int resolution = 8;
    if (newInstr && newInstr->getKeyMapping()) {
        resolution = 11;
    }

    const MidiKeyMapping *mapping = 0;
    if (newInstr) {
        mapping = newInstr->getKeyMapping();
    }

    // Construct a local new keymapping :
    if (m_localMapping)
        delete m_localMapping;
    if (mapping) {
        m_localMapping = new MidiKeyMapping(*mapping);
        extendKeyMapping();
    } else {
        m_localMapping = 0;
    }

    m_staffs[0]->setResolution(resolution);

    delete m_pitchRuler;

    QWidget *vport = m_pianoView->viewport();

    // Create a new pitchruler widget
    PitchRuler *pitchRuler;
    if (newInstr && newInstr->getKeyMapping() &&
            !newInstr->getKeyMapping()->getMap().empty()) {
        pitchRuler = new PercussionPitchRuler(vport,
                                              m_localMapping,
                                              resolution); // line spacing
    } else {
        pitchRuler = new PianoKeyboard(vport);
    }


    QObject::connect
    (pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
     this, SLOT (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
    (pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
     this, SLOT (slotKeyPressed(unsigned int, bool)));

    QObject::connect
    (pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
     this, SLOT (slotKeySelected(unsigned int, bool)));

    QObject::connect
    (pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
     this, SLOT (slotKeyReleased(unsigned int, bool)));

    // Replace the old pitchruler widget
    m_pitchRuler = pitchRuler;
    m_pianoView->addChild(m_pitchRuler);
    m_pitchRuler->show();
    m_pianoView->setFixedWidth(pitchRuler->sizeHint().width());

    // Update matrix canvas
    readjustCanvasSize();
    bool layoutApplied = applyLayout();
    if (!layoutApplied)
        KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        m_staffs[0]->positionAllElements();
        m_staffs[0]->getSegment().getRefreshStatus
        (m_segmentsRefreshStatusIds[0]).setNeedsRefresh(false);
        update();
    }
}

void
MatrixView::slotCanvasBottomWidgetHeightChanged(int newHeight)
{
    m_pianoView->setBottomMargin(newHeight +
                                 m_canvasView->horizontalScrollBar()->height());
}

MatrixCanvasView* MatrixView::getCanvasView()
{
    return dynamic_cast<MatrixCanvasView *>(m_canvasView);
}

}
#include "MatrixView.moc"
