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

#include "NotationWidget.h"

#include "NotationScene.h"
#include "NotationToolBox.h"
#include "NoteRestInserter.h"
#include "ClefInserter.h"
#include "TextInserter.h"
#include "GuitarChordInserter.h"
#include "SymbolInserter.h"
#include "NotationMouseEvent.h"
#include "NotationSelector.h"
#include "NotationEraser.h"
#include "StaffLayout.h"
#include "HeadersGroup.h"

#include "base/RulerScale.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"
#include "base/Controllable.h"
#include "base/Studio.h"
#include "base/Instrument.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiTypes.h"

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/widgets/Thumbwheel.h"

#include "gui/general/IconLoader.h"

#include "gui/rulers/ControlRulerWidget.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/RawNoteRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QGraphicsView>
#include <QGridLayout>
#include <QBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsProxyWidget>
#include <QToolTip>
#include <QToolButton>

namespace Rosegarden
{

NotationWidget::NotationWidget() :
    m_document(0),
    m_view(0),
    m_scene(0),
    m_playTracking(true),
    m_hZoomFactor(1.0),
    m_vZoomFactor(1.0),
    m_referenceScale(0),
    m_inMove(false),
    m_lastZoomWasHV(true),
    m_lastV(0),
    m_lastH(0),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_tempoRuler(0),
    m_chordNameRuler(0),
    m_rawNoteRuler(0),
    m_headersGroup(0),
    m_headersView(0),
    m_headersScene(0),
    m_headersButtons(0),
    m_headersLastY(0),
    m_layout(0),
    m_linearMode(true),
    m_tempoRulerIsVisible(false),
    m_rawNoteRulerIsVisible(false),
    m_chordNameRulerIsVisible(false),
    m_headersAreVisible(false),
    m_chordMode(false),
    m_tripletMode(false),
    m_graceMode(false),
    m_hSliderHacked(false),
    m_vSliderHacked(false)
{
    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and staves
    m_layout->setSpacing(0);

    // Remove black margins around the notation
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_view->setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);
    m_view->setBackgroundBrush(QBrush(IconLoader().loadPixmap("bg-paper-grey")));
    m_layout->addWidget(m_view, PANNED_ROW, MAIN_COL, 1, 1);

    m_controlsWidget = new ControlRulerWidget;
    m_layout->addWidget(m_controlsWidget, CONTROLS_ROW, MAIN_COL, 1, 1);

    m_panner = new QWidget;
    m_pannerLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    m_pannerLayout->setContentsMargins(0, 0, 0, 0);
    m_pannerLayout->setSpacing(0);
    m_panner->setLayout(m_pannerLayout);

    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(80);
    m_hpanner->setBackgroundBrush(Qt::white);
    m_hpanner->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    m_hpanner->setRenderHints(0);

    m_pannerLayout->addWidget(m_hpanner);

    QFrame *controls = new QFrame;

    QGridLayout *controlsLayout = new QGridLayout;
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controls->setLayout(controlsLayout);

    m_HVzoom = new Thumbwheel(Qt::Vertical);
    m_HVzoom->setFixedSize(QSize(40, 40));
    m_HVzoom->setToolTip(tr("Zoom"));

    // +/- 20 clicks seems to be the reasonable limit
    m_HVzoom->setMinimumValue(-20);
    m_HVzoom->setMaximumValue(20);
    m_HVzoom->setDefaultValue(0);
    m_HVzoom->setBright(true);
    m_HVzoom->setShowScale(true);
    m_lastHVzoomValue = m_HVzoom->getValue();
    controlsLayout->addWidget(m_HVzoom, 0, 0, Qt::AlignCenter);

    connect(m_HVzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotPrimaryThumbwheelMoved(int)));

    m_Hzoom = new Thumbwheel(Qt::Horizontal);
    m_Hzoom->setFixedSize(QSize(50, 16));
    m_Hzoom->setToolTip(tr("Horizontal Zoom"));

    m_Hzoom->setMinimumValue(-25);
    m_Hzoom->setMaximumValue(60);
    m_Hzoom->setDefaultValue(0); 
    m_Hzoom->setBright(false);
    controlsLayout->addWidget(m_Hzoom, 1, 0);
    connect(m_Hzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotHorizontalThumbwheelMoved(int)));

    m_Vzoom = new Thumbwheel(Qt::Vertical);
    m_Vzoom->setFixedSize(QSize(16, 50));
    m_Vzoom->setToolTip(tr("Vertical Zoom"));
    m_Vzoom->setMinimumValue(-25);
    m_Vzoom->setMaximumValue(60);
    m_Vzoom->setDefaultValue(0);
    m_Vzoom->setBright(false);
    controlsLayout->addWidget(m_Vzoom, 0, 1, Qt::AlignRight);

    connect(m_Vzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotVerticalThumbwheelMoved(int)));

    // a blank QPushButton forced square looks better than the tool button did
    m_reset = new QPushButton;
    m_reset->setFixedSize(QSize(10, 10));
    m_reset->setToolTip(tr("Reset Zoom"));
    controlsLayout->addWidget(m_reset, 1, 1, Qt::AlignCenter);

    connect(m_reset, SIGNAL(clicked()), this, 
            SLOT(slotResetZoomClicked()));

    m_pannerLayout->addWidget(controls);

    m_layout->addWidget(m_panner, PANNER_ROW, HEADER_COL, 1, 2);

    m_headersView = new Panned;
    m_headersView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_headersView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_headersView, PANNED_ROW, HEADER_COL, 1, 1);


    // Rulers being not defined still, they can't be added to m_layout.
    // This will be done in setSegments().

    // Move the scroll bar from m_view to NotationWidget
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_view->horizontalScrollBar(),
                        HSLIDER_ROW, HEADER_COL, 1, 2);


    // Create the headers close button
    //
    // NOTE: I tried to style this QToolButton to resemble the parameter area
    // close button, but I could never get it to come out sensibly as a square
    // button with a reasonably sized X icon in it.  I tried all kinds of wild
    // variations.
    //
    // In the end, I took the way Yves had solved this problem and just replaced
    // his white X icon with a screen capture of the button I wanted to copy.
    // It doesn't hover correctly, but it doesn't look too bad, and seems as
    // close as I'm going to get to what I wanted.
    QToolButton *headersCloseButton = new QToolButton;
    headersCloseButton->setIcon(IconLoader().loadPixmap("header-close-button"));
    headersCloseButton->setIconSize(QSize(14, 14));
    connect(headersCloseButton, SIGNAL(clicked(bool)),
            this, SLOT(slotCloseHeaders()));

    // Insert the button in a layout to push it on the right
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(20);
    buttonsLayout->addWidget(headersCloseButton);

    // Put the layout inside a widget and the widget above the headers
    m_headersButtons = new QWidget;
    m_headersButtons->setLayout(buttonsLayout);
    m_layout->addWidget(m_headersButtons, TOPRULER_ROW, HEADER_COL, 1, 1);


    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)),
            this, SLOT(slotHScrollBarRangeChanged(int, int)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_controlsWidget, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_controlsWidget, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannerChanged(QRectF)),
             this, SLOT(slotAdjustHeadersVerticalPos(QRectF)));

    connect(m_view, SIGNAL(pannedContentsScrolled()),
            this, SLOT(slotHScroll()));

    connect(m_controlsWidget, SIGNAL(dragScroll(timeT)),
            this, SLOT(slotEnsureTimeVisible(timeT)));
    
    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotSyncPannerZoomIn()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotSyncPannerZoomOut()));

    connect(m_headersView, SIGNAL(wheelEventReceived(QWheelEvent *)),
            m_view, SLOT(slotEmulateWheelEvent(QWheelEvent *)));

    connect(this, SIGNAL(adjustNeeded(bool)),
            this, SLOT(slotAdjustHeadersHorizontalPos(bool)),
            Qt::QueuedConnection);

    m_toolBox = new NotationToolBox(this);

    //!!! 
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    m_currentTool = noteRestInserter;
    m_currentTool->ready();

    connect(this, SIGNAL(toolChanged(QString)),
            m_controlsWidget, SLOT(slotSetToolName(QString)));

    // crude, but finally effective!
    //
    // somewhere beyond the end of the ctor and into the event loop,
    // NotationWidget picks a stupid place to open the initial view to, and this
    // causes its Panned object's scrollbars to emit valueChanged() signals,
    // which we hijack as the timing cue that now is finally the appropriate
    // time to move the view to a sane place.  Currently this just physically
    // moves the scrollbars to 0 and 0, which is better than the almost random
    // not quite center this code initializes to, but this can probably be
    // improved now that the timing is established.
    connect(m_view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotInitialHSliderHack(int)));
    connect(m_view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotInitialVSliderHack(int)));
}

NotationWidget::~NotationWidget()
{
    delete m_scene;
}

void
NotationWidget::setSegments(RosegardenDocument *document,
                            std::vector<Segment *> segments)
{
    if (m_document) {
        disconnect(m_document, SIGNAL(pointerPositionChanged(timeT)),
                   this, SLOT(slotPointerPositionChanged(timeT)));
    }

    m_document = document;

    delete m_referenceScale;

    delete m_scene;
    m_scene = new NotationScene();
    m_scene->setNotationWidget(this);
    m_scene->setStaffs(document, segments);

    m_referenceScale = new ZoomableRulerScale(m_scene->getRulerScale());

    connect(m_scene, SIGNAL(mousePressed(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMousePress(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseMoved(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseMove(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseReleased(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseRelease(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseDoubleClicked(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseDoubleClick(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(segmentDeleted(Segment *)),
            this, SIGNAL(segmentDeleted(Segment *)));

    connect(m_scene, SIGNAL(sceneDeleted()),
            this, SIGNAL(sceneDeleted()));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);
    m_hpanner->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

    m_controlsWidget->setSegments(document, segments);
    m_controlsWidget->setViewSegment((ViewSegment *) m_scene->getCurrentStaff());

    connect(m_scene, SIGNAL(layoutUpdated()),
            m_controlsWidget, SLOT(slotUpdatePropertyRulers()));
    
    // For some reason this doesn't work in the constructor - not looked in detail
    connect(m_scene, SIGNAL(selectionChanged(EventSelection *)),
            m_controlsWidget, SLOT(slotSelectionChanged(EventSelection *)));

    m_topStandardRuler = new StandardRuler(document,
                                           m_referenceScale, 0, 25,
                                           false);

    m_bottomStandardRuler = new StandardRuler(document,
                                               m_referenceScale, 0, 25,
                                               true);

    m_tempoRuler = new TempoRuler(m_referenceScale,
                                  document,
                                  RosegardenMainWindow::self(),
                                  0.0,    // xorigin
                                  24,     // height
                                  true);  // small

    m_chordNameRuler = new ChordNameRuler(m_referenceScale,
                                          document,
                                          segments,
                                          0.0,     // xorigin
                                          24);     // height

    m_rawNoteRuler = new RawNoteRuler(m_referenceScale,
                                      segments[0],
                                      0.0,
                                      20);  // why not 24 as other rulers ?

    if (m_headersGroup) disconnect(m_headersGroup, SIGNAL(headersResized(int)),
                                   this, SLOT(slotHeadersResized(int)));
    m_headersGroup = new HeadersGroup(m_document);
    m_headersGroup->setTracks(this, m_scene);

    m_layout->addWidget(m_topStandardRuler, TOPRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_bottomStandardRuler, BOTTOMRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_tempoRuler, TEMPORULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_chordNameRuler, CHORDNAMERULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_rawNoteRuler, RAWNOTERULER_ROW, MAIN_COL, 1, 1);

    connect(m_topStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    
    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    m_topStandardRuler->connectRulerToDocPointer(document);
    m_bottomStandardRuler->connectRulerToDocPointer(document);

    m_tempoRuler->connectSignals();

    m_chordNameRuler->setReady();

    m_headersGroup->setFixedSize(m_headersGroup->sizeHint());
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());

    delete m_headersScene;
    m_headersScene = new QGraphicsScene();
    QGraphicsProxyWidget *headersProxy = m_headersScene->addWidget(m_headersGroup);
    m_headersView->setScene(m_headersScene);
    m_headersView->centerOn(headersProxy);

    m_headersView->setMinimumHeight(0);

    // If headers scene and notation scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF headersRect = m_headersScene->sceneRect();
    headersRect.setHeight(viewRect.height());
    m_headersScene->setSceneRect(headersRect);

    connect(m_headersGroup, SIGNAL(headersResized(int)),
            this, SLOT(slotHeadersResized(int)));
}

void
NotationWidget::setCanvasCursor(QCursor c)
{
    if (m_view) m_view->viewport()->setCursor(c);
}

Segment *
NotationWidget::getCurrentSegment()
{
    if (!m_scene) return 0;
    return m_scene->getCurrentSegment();
}

bool
NotationWidget::segmentsContainNotes() const
{
    if (!m_scene) return false;
    return m_scene->segmentsContainNotes();
}

void
NotationWidget::locatePanner(bool tall)
{
    m_layout->removeWidget(m_panner);
    if (tall) {
        m_panner->setMaximumHeight(QWIDGETSIZE_MAX);
        m_hpanner->setMaximumHeight(QWIDGETSIZE_MAX);
        m_panner->setMaximumWidth(80);
        m_hpanner->setMaximumWidth(80);
        m_pannerLayout->setDirection(QBoxLayout::TopToBottom);
        m_layout->addWidget(m_panner, PANNED_ROW, VPANNER_COL);
    } else {
        m_panner->setMaximumHeight(80);
        m_hpanner->setMaximumHeight(80);
        m_panner->setMaximumWidth(QWIDGETSIZE_MAX);
        m_hpanner->setMaximumWidth(QWIDGETSIZE_MAX);
        m_pannerLayout->setDirection(QBoxLayout::LeftToRight);
        m_layout->addWidget(m_panner, PANNER_ROW, HEADER_COL, 1, 2);
    }
}

void
NotationWidget::slotSetLinearMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) {
        locatePanner(false);
    }
    m_scene->setPageMode(StaffLayout::LinearMode);
    m_linearMode = true;
    hideOrShowRulers();
}

void
NotationWidget::slotSetContinuousPageMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) return;
    locatePanner(true);
    m_scene->setPageMode(StaffLayout::ContinuousPageMode);
    m_linearMode = false;
    hideOrShowRulers();
}

void
NotationWidget::slotSetMultiPageMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) {
        locatePanner(false);
    }
    m_scene->setPageMode(StaffLayout::MultiPageMode);
    m_linearMode = false;
    hideOrShowRulers();
}

void
NotationWidget::slotSetFontName(QString name)
{
    if (m_scene) m_scene->setFontName(name);
}

void
NotationWidget::slotSetFontSize(int size)
{
    if (m_scene) m_scene->setFontSize(size);
}

NotationTool *
NotationWidget::getCurrentTool() const
{
    return m_currentTool;
}

void
NotationWidget::slotSetTool(QString name)
{
    NotationTool *tool = dynamic_cast<NotationTool *>(m_toolBox->getTool(name));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
}

void
NotationWidget::slotSetEraseTool()
{
    slotSetTool(NotationEraser::ToolName);
}

void
NotationWidget::slotSetSelectTool()
{
    slotSetTool(NotationSelector::ToolName);
}

void
NotationWidget::slotSetNoteRestInserter()
{
    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetNoteInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    noteRestInserter->setToRestInserter(false); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetRestInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    noteRestInserter->setToRestInserter(true); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetInsertedNote(Note::Type type, int dots)
{
    NoteRestInserter *ni = dynamic_cast<NoteRestInserter *>(m_currentTool);
    if (ni) {
        
        ni->slotSetNote(type);
        ni->slotSetDots(dots);
        return;
    }
}

void
NotationWidget::slotSetAccidental(Accidental accidental, bool follow)
{
    // You don't have to be in note insertion mode to change the accidental
    NoteRestInserter *ni = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    if (ni) {
        ni->slotSetAccidental(accidental, follow);
        return;
    }
}

void
NotationWidget::slotSetClefInserter()
{
    slotSetTool(ClefInserter::ToolName);
}

void
NotationWidget::slotSetInsertedClef(Clef type)
{
    ClefInserter *ci = dynamic_cast<ClefInserter *>(m_currentTool);
    if (ci) ci->slotSetClef(type);
}

void
NotationWidget::slotSetTextInserter()
{
    slotSetTool(TextInserter::ToolName);
}

void
NotationWidget::slotSetGuitarChordInserter()
{
    slotSetTool(GuitarChordInserter::ToolName);
}

void
NotationWidget::slotSetPlayTracking(bool tracking)
{
    m_playTracking = tracking;
    if (m_playTracking) {
        m_view->slotEnsurePositionPointerInView(true);
    }
}

void
NotationWidget::slotTogglePlayTracking()
{
    slotSetPlayTracking(!m_playTracking);
}

void
NotationWidget::slotPointerPositionChanged(timeT t)
{
    QObject *s = sender();
    bool fromDocument = (s == m_document);

    NOTATION_DEBUG << "NotationWidget::slotPointerPositionChanged to " << t << endl;

    if (!m_scene) return;

    QLineF p = m_scene->snapTimeToStaffPosition(t);
    if (p == QLineF()) return;

    //!!! p will also contain sensible Y (although not 100% sensible yet)
    double sceneX = p.x1();
    double sceneY = std::min(p.y1(), p.y2());
    double height = fabsf(p.y2() - p.y1());

    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        m_view->slotHidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->slotShowPositionPointer(QPointF(sceneX, sceneY), height);
        m_hpanner->slotShowPositionPointer(QPointF(sceneX, sceneY), height);
    }

    if (getPlayTracking() || !fromDocument) {
        m_view->slotEnsurePositionPointerInView(fromDocument);
    }
}

void
NotationWidget::slotDispatchMousePress(const NotationMouseEvent *e)
{
    if (e->buttons & Qt::LeftButton) {
        if (e->modifiers & Qt::ControlModifier) {
            // the function this used was an empty NOP
            // if (m_scene) m_scene->slotSetInsertCursorPosition(e->time, true, true); //!!!
            return;
        }
    }

    if (!m_currentTool) return;

    //!!! todo: handle equivalents of NotationView::slotXXXItemPressed

    if (e->buttons & Qt::LeftButton) {
        m_currentTool->handleLeftButtonPress(e);
    } else if (e->buttons & Qt::MidButton) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::RightButton) {
        m_currentTool->handleRightButtonPress(e);
    }
}

void
NotationWidget::slotDispatchMouseMove(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    NotationTool::FollowMode mode = m_currentTool->handleMouseMove(e);
    
    if (mode != NotationTool::NoFollow) {
        m_lastMouseMoveScenePos = QPointF(e->sceneX, e->sceneY);
        slotEnsureLastMouseMoveVisible();
        QTimer::singleShot(100, this, SLOT(slotEnsureLastMouseMoveVisible()));
    }

    /*!!!
if (getCanvasView()->isTimeForSmoothScroll()) {

            if (follow & RosegardenCanvasView::FollowHorizontal) {
                getCanvasView()->slotScrollHorizSmallSteps(e->x());
            }

            if (follow & RosegardenCanvasView::FollowVertical) {
                getCanvasView()->slotScrollVertSmallSteps(e->y());
            }

        }
    }
    */
}

void
NotationWidget::slotEnsureLastMouseMoveVisible()
{
    if (m_inMove) return;
    m_inMove = true;
    QPointF pos = m_lastMouseMoveScenePos;
    if (m_scene) m_scene->constrainToSegmentArea(pos);
    m_view->ensureVisible(QRectF(pos, pos));
    m_inMove = false;
}    

void
NotationWidget::slotDispatchMouseRelease(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseRelease(e);
}

void
NotationWidget::slotDispatchMouseDoubleClick(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseDoubleClick(e);
}

EventSelection *
NotationWidget::getSelection() const
{
    if (m_scene) return m_scene->getSelection();
    else return 0;
}

void
NotationWidget::setSelection(EventSelection *selection, bool preview)
{
    if (m_scene) m_scene->setSelection(selection, preview);
}

timeT
NotationWidget::getInsertionTime() const
{
    if (m_scene) return m_scene->getInsertionTime();
    else return 0;
}

void
NotationWidget::slotZoomInFromPanner() 
{
    m_hZoomFactor /= 1.1;
    m_vZoomFactor /= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width()
                                                         * m_hZoomFactor);
    slotHScroll();
}

void
NotationWidget::slotZoomOutFromPanner() 
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width()
                                                         * m_hZoomFactor);
    slotHScroll();
}

void
NotationWidget::slotAdjustHeadersHorizontalPos(bool last)
{
// Sometimes, after a zoom change, the headers are no more horizontally
// aligned with the headers view.
// The following code is an attempt to reposition the headers in the view.
// Actually it doesn't succeed always (ie. with stormy-riders).

// Workaround :
//   - 1) The old method adjustHeadersHorizontalPos() is changed into a slot
//        called when the new signal adjustNeeded() is emitted.
//        This slot is connected with a Qt::QueuedConnection type connection
//        to delay as much as possible its execution.
//    -2) The headers refresh problem occurs each time the x0 (or xinit)
//        value defined below is <= 0 : When such a situation occurs, the slot
//        is calls itself again. Usually, the second call works.
//        The slot arg. "last" has been added to avoid an infinite
//        loop if x0 is never > 0.
//
// I don't like this code which is really a workaround. Just now I don't
// know the true cause of the problem. (When zoom factor is decreased, x0 is
// > 0 sometimes and < 0 some other times : why ?)
// This slot always works when it is called after some delay, but it fails
// sometimes when it is called without any delay.
//
//  Maybe another solution would be to use a timer to call the slot.
//  But what should be the delay ? Should it depend on the machine where
//  RG run ? Or on the Qt version ?
//  The previous solution seems better.

//std::cerr << "\nXproxy0=" << m_headersProxy->scenePos().x() << "\n";

    double xinit;

    double x = xinit = m_headersView->mapToScene(0, 0).x();
//std::cerr << " x0=" << x << "\n";

    // First trial
    if ((x > 1) || (x < -1)) {
        QRectF view = m_headersView->sceneRect();
        view.moveLeft(0.0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
    }
//std::cerr << "x1=" << x << "\n";

    // Second trial. Why isn't the first iteration always sufficient ?
    // Number of iterations is limited to 3.
    int n = 1;
    while ((x > 1) || (x < -1)) {
//std::cerr << "n=" << n << " xt2=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (n++ > 3) break;
    }

//std::cerr << "x2=" << x << "\n";

    // Third trial.
    // If precedent trial doesn't succeed, try again with a coefficient...
    // Number of iterations is limited to 6.    int m = 1;
    int m = 1;
    while ((x > 1) || (x < -1)) {
//std::cerr << "m=" << m << " xt3=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x * 0.477, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (m++ > 6) break;
    }

//std::cerr << "x3=" << x << "\n";

    // Probably totally useless here.
    m_headersView->update();

    // Now, sometimes, although x is null or almost null, the headers are
    // not fully visible !!??

//std::cerr << "Xproxy1=" << m_headersProxy->scenePos().x() << "\n";

    // Call again the current slot if we have some reason to think it
    // did not succeed and if it has been called in the current context
    // only once.
    // (See comment at the beginning of the slotAdjustHeadersHorizontalPos.)
    if (!last && xinit < 0.001) emit adjustNeeded(true);
}

void
NotationWidget::slotAdjustHeadersVerticalPos(QRectF r)
{
    r.setX(0);
    r.setWidth(m_headersView->sceneRect().width());

    // Misalignment between staffs and headers depends of the vertical
    // scrolling direction.
    // This is a hack : The symptoms are fixed (more or less) but still the
    //                  cause of the problem is unknown.
    double y = r.y();
    double delta = y > m_headersLastY ? - 2 : - 4;
    m_headersLastY = y;

    QRectF vr = m_view->mapToScene(m_view->rect()).boundingRect();
    r.setY(vr.y() + delta);
    r.setHeight(vr.height());

    m_headersView->setSceneRect(r);
}

double
NotationWidget::getViewLeftX()
{
    return m_view->mapToScene(0, 0).x();
}

int
NotationWidget::getNotationViewWidth()
{
    return m_view->width();
}

double
NotationWidget::getNotationSceneHeight()
{
    return m_scene->height();
}

void
NotationWidget::slotHScroll()
{
    // Get time of the window left
    QPointF topLeft = m_view->mapToScene(0, 0);
    double xs = topLeft.x();

    // Apply zoom correction (Offset of 20 found empirically : probably
    // some improvments are needed ...)
    int x = (xs - 20) * m_hZoomFactor;

    // Scroll rulers accordingly
    m_topStandardRuler->slotScrollHoriz(x);
    m_bottomStandardRuler->slotScrollHoriz(x);
    m_tempoRuler->slotScrollHoriz(x);
    m_chordNameRuler->slotScrollHoriz(x);
    m_rawNoteRuler->slotScrollHoriz(x);

    // Update staff headers
    m_headersGroup->slotUpdateAllHeaders(xs);

    emit adjustNeeded(false);
}

void
NotationWidget::slotHScrollBarRangeChanged(int min, int max)
{
    if (max > min) {
        m_view->horizontalScrollBar()->show(); 
    } else {
        m_view->horizontalScrollBar()->hide();
    }
}

void
NotationWidget::setTempoRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_tempoRuler->show();
    else m_tempoRuler->hide();
    m_tempoRulerIsVisible = visible;
}

void
NotationWidget::setChordNameRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
    m_chordNameRulerIsVisible = visible;
}

void
NotationWidget::setRawNoteRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_rawNoteRuler->show();
    else m_rawNoteRuler->hide();
    m_rawNoteRulerIsVisible = visible;
}

void
NotationWidget::setHeadersVisible(bool visible)
{
    if (visible && m_linearMode) {
        m_headersView->show();
        m_headersButtons->show();
    } else {
        m_headersView->hide();
        m_headersButtons->hide();
    }
    m_headersAreVisible = visible;
}

void
NotationWidget::toggleHeadersView()
{
    m_headersAreVisible = !m_headersAreVisible;
    if (m_headersAreVisible && m_linearMode) {
        m_headersView->show();
        m_headersButtons->show();
    } else {
        m_headersView->hide();
        m_headersButtons->hide();
    }
}

void
NotationWidget::slotCloseHeaders()
{
    setHeadersVisible(false);
}

void
NotationWidget::hideOrShowRulers()
{
    if (m_linearMode) {
        if (m_tempoRulerIsVisible) m_tempoRuler->show();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->show();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->show();
        if (m_headersAreVisible) m_headersView->show();
        m_bottomStandardRuler->show();
        m_topStandardRuler->show();
    } else {
        if (m_tempoRulerIsVisible) m_tempoRuler->hide();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->hide();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->hide();
        if (m_headersAreVisible) m_headersView->hide();
        m_bottomStandardRuler->hide();
        m_topStandardRuler->hide();
    }
}

void
NotationWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    slotHScroll();
}

void
NotationWidget::slotShowHeaderToolTip(QString toolTipText)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this);
}

void
NotationWidget::slotHeadersResized(int)
{
    // Set headers view width to accomodate headers width.   
    m_headersView->setFixedWidth(
        m_headersGroup->sizeHint().width() * m_hZoomFactor);
}

void
NotationWidget::slotSetSymbolInserter()
{
    slotSetTool(SymbolInserter::ToolName);
}

void
NotationWidget::slotSetInsertedSymbol(Symbol type)
{
    SymbolInserter *ci = dynamic_cast<SymbolInserter *>(m_currentTool);
    if (ci) ci->slotSetSymbol(type);
}

void
NotationWidget::setPointerPosition(timeT t)
{
    m_document->slotSetPointerPosition(t);
}

void
NotationWidget::slotHorizontalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25) v = -25;
    if (v > 60) v = 60;
    if (m_lastH < -25) m_lastH = -25;
    if (m_lastH > 60) m_lastH = 60;

    int steps = v - m_lastH;
    if (steps < 0) steps *= -1;

    bool zoomingIn = (v > m_lastH);
    double newZoom = m_hZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn) newZoom *= 1.1;
        else newZoom /= 1.1;
    }

    // switching from primary/panner to axis-independent
    if (m_lastZoomWasHV) {
        slotResetZoomClicked();
        m_HVzoom->setBright(false);
        m_Hzoom->setBright(true);
        m_Vzoom->setBright(true);
    }

    //std::cout << "v is: " << v << " h zoom factor was: " << m_lastH << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT") << std::endl;

    setHorizontalZoomFactor(newZoom);
    m_lastH = v;
    m_lastZoomWasHV = false;
}

void
NotationWidget::slotVerticalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25) v = -25;
    if (v > 60) v = 60;
    if (m_lastV < -25) m_lastV = -25;
    if (m_lastV > 60) m_lastV = 60;

    int steps = v - m_lastV;
    if (steps < 0) steps *= -1;

    bool zoomingIn = (v > m_lastV);
    double newZoom = m_vZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn) newZoom *= 1.1;
        else newZoom /= 1.1;
    }

    // switching from primary/panner to axis-independent
    if (m_lastZoomWasHV) {
        slotResetZoomClicked();
        m_HVzoom->setBright(false);
        m_Hzoom->setBright(true);
        m_Vzoom->setBright(true);
    }

    //std::cout << "v is: " << v << " z zoom factor was: " << m_lastV << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT") << std::endl;

    setVerticalZoomFactor(newZoom);
    m_lastV = v;
    m_lastZoomWasHV = false;
}

void
NotationWidget::slotPrimaryThumbwheelMoved(int v)
{
    // not sure what else to do; you can get things grotesquely out of whack
    // changing H or V independently and then trying to use the big zoom, so now
    // we reset when changing to the big zoom, and this behaves independently
   
    // switching from axi-independent to primary/panner
    if (!m_lastZoomWasHV) {
        slotResetZoomClicked();
        m_HVzoom->setBright(true);
        m_Hzoom->setBright(false);
        m_Vzoom->setBright(false);
    }

    // little bit of kludge work to deal with value manipulations that are
    // outside of the constraints imposed by the primary zoom wheel itself
    if (v < -20) v = -20;
    if (v > 20) v = 20;
    if (m_lastHVzoomValue < -20) m_lastHVzoomValue = -20;
    if (m_lastHVzoomValue > 20) m_lastHVzoomValue = 20;

    // When dragging the wheel up and down instead of mouse wheeling it, it
    // steps according to its speed.  I don't see a sure way (and after all
    // there are no docs!) to make sure dragging results in a smooth 1:1
    // relationship when compared with mouse wheeling, and we are just hijacking
    // slotZoomInFromPanner() here, so we will look at the number of steps
    // between the old value and the last one, and call the slot that many times
    // in order to enforce the 1:1 relationship.
    int steps = v - m_lastHVzoomValue;
    if (steps < 0) steps *= -1;

    for (int i = 0; i < steps; ++i) {
        if (v < m_lastHVzoomValue) slotZoomInFromPanner();
        else if (v > m_lastHVzoomValue) slotZoomOutFromPanner();
    }

    m_lastHVzoomValue = v;
    m_lastZoomWasHV = true;
}

void
NotationWidget::slotResetZoomClicked()
{
    std::cerr << "NotationWidget::slotResetZoomClicked()" << std::endl;

    m_hZoomFactor = 1.0;
    m_vZoomFactor = 1.0;
    if (m_referenceScale) {
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
        m_referenceScale->setYZoomFactor(m_vZoomFactor);
    }
    m_view->resetMatrix();
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersView->sizeHint().width());
    slotHScroll();

    // scale factor 1.0 = 100% zoom
    m_Hzoom->setValue(1);
    m_Vzoom->setValue(1);
    m_HVzoom->setValue(0);
    m_lastHVzoomValue = 0;
    m_lastH = 0;
    m_lastV = 0;
}

void
NotationWidget::slotSyncPannerZoomIn()
{
    int v = m_lastHVzoomValue - 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}

void
NotationWidget::slotSyncPannerZoomOut()
{
    int v = m_lastHVzoomValue + 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}


void
NotationWidget::setHorizontalZoomFactor(double factor)
{
    // NOTE: scaling the keyboard up and down works well for the primary zoom
    // because it maintains the same aspect ratio for each step.  I tried a few
    // different ways to deal with this before deciding that since
    // independent-axis zoom is a separate and mutually exclusive subsystem,
    // about the only sensible thing we can do is keep the keyboard scaled at
    // 1.0 horizontally, and only scale it vertically.  Git'r done.

    m_hZoomFactor = factor;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersView->sizeHint().width());
    slotHScroll();
}

void
NotationWidget::setVerticalZoomFactor(double factor)
{
    m_vZoomFactor = factor;
    if (m_referenceScale) m_referenceScale->setYZoomFactor(m_vZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersView->sizeHint().width());
}

double
NotationWidget::getHorizontalZoomFactor() const
{
    return m_hZoomFactor;
}

double
NotationWidget::getVerticalZoomFactor() const
{
    return m_vZoomFactor;
}

void
NotationWidget::slotInitialHSliderHack(int)
{
    if (m_hSliderHacked) return;

    m_hSliderHacked = true;

    std::cout << "h slider position was: " << m_view->horizontalScrollBar()->sliderPosition() << std::endl;;
    m_view->horizontalScrollBar()->setSliderPosition(0);
    std::cout << "h slider position now: " << m_view->horizontalScrollBar()->sliderPosition() << std::endl;;
}

void
NotationWidget::slotInitialVSliderHack(int)
{
    if (m_vSliderHacked) return;

    m_vSliderHacked = true;

    std::cout << "v slider position was: " << m_view->verticalScrollBar()->sliderPosition() << std::endl;;
    m_view->verticalScrollBar()->setSliderPosition(0);
    std::cout << "v slider position now: " << m_view->verticalScrollBar()->sliderPosition() << std::endl;;
}

void
NotationWidget::slotToggleVelocityRuler()
{
    m_controlsWidget->slotTogglePropertyRuler(BaseProperties::VELOCITY);
}

void
NotationWidget::slotTogglePitchbendRuler()
{
    m_controlsWidget->slotToggleControlRuler("PitchBend");
}

void
NotationWidget::slotAddControlRuler(QAction *action)
{
    QString name = action->text();

    std::cout << "my name is " << name.toStdString() << std::endl;

    // we just cheaply paste the code from NotationView that created the menu to
    // figure out what its indices must point to (and thinking about this whole
    // thing, I bet it's all buggy as hell in a multi-track view where the
    // active segment can change, and the segment's track's device could be
    // completely different from whatever was first used to create the menu...
    // there will probably be refresh problems and crashes and general bugginess
    // 20% of the time, but a solution that works 80% of the time is worth
    // shipping, I just read on some blog, and damn the torpedoes)
    Controllable *c =
        dynamic_cast<MidiDevice *>(getCurrentDevice());
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
        if (!c)
            return ;
    }

    const ControlList &list = c->getControlParameters();

    QString itemStr;
//  int i = 0;

    for (ControlList::const_iterator it = list.begin();
            it != list.end(); ++it) {

        // Pitch Bend is treated separately now, and there's no point in adding
        // "unsupported" controllers to the menu, so skip everything else
        if (it->getType() != Controller::EventType) continue;

        QString hexValue;
        hexValue.sprintf("(0x%x)", it->getControllerValue());

        // strings extracted from data files must be QObject::tr()
        QString itemStr = QObject::tr("%1 Controller %2 %3")
                                     .arg(QObject::tr(strtoqstr(it->getName())))
                                     .arg(it->getControllerValue())
                                     .arg(hexValue);
        
        if (name != itemStr) continue;

        std::cout << "name: " << name.toStdString() << " should match  itemStr: " << itemStr.toStdString() << std::endl;

        m_controlsWidget->slotAddControlRuler(*it);

//      if (i == menuIndex) m_controlsWidget->slotAddControlRuler(*p);
//      else i++;
    }   
}

Device *
NotationWidget::getCurrentDevice()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return 0;

    Studio &studio = m_document->getStudio();
    Instrument *instrument =
        studio.getInstrumentById
        (segment->getComposition()->getTrackById(segment->getTrack())->
         getInstrument());
    if (!instrument)
        return 0;

    return instrument->getDevice();
}


}

#include "NotationWidget.moc"
