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
#include "NoteInserter.h"
#include "RestInserter.h"
#include "ClefInserter.h"
#include "TextInserter.h"
#include "GuitarChordInserter.h"
#include "NotationMouseEvent.h"
#include "NotationSelector.h"
#include "NotationEraser.h"
#include "StaffLayout.h"

#include "base/RulerScale.h"

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/general/IconLoader.h"

#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/RawNoteRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>
#include <QTimer>

namespace Rosegarden
{

NotationWidget::NotationWidget() :
    m_document(0),
    m_view(0),
    m_scene(0),
    m_playTracking(true),
    m_hZoomFactor(1),
    m_vZoomFactor(1),
    m_referenceScale(0),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_tempoRuler(0),
    m_chordNameRuler(0),
    m_rawNoteRuler(0),
    m_layout(0)
{
    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and staves
    m_layout->setSpacing(0);

    // Remove black margins around the notation
    //m_layout->setContentsMargins(0, 0, 0, 0);

    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_view->setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);
    m_view->setBackgroundBrush(QBrush(IconLoader().loadPixmap("bg-paper-grey")));
    m_layout->addWidget(m_view, PANNED_ROW, MAIN_COL, 1, 1);

    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(80);
    m_hpanner->setBackgroundBrush(Qt::white);
    m_hpanner->setRenderHints(0);
//    m_hpanner->setRenderHints(QPainter::TextAntialiasing |
//                              QPainter::SmoothPixmapTransform);
    m_layout->addWidget(m_hpanner, PANNER_ROW, MAIN_COL, 1, 1);

    // Rulers being not defined still, they can't be added to m_layout.
    // This will be done in setSegments().

    // Move the scroll bar from m_view to NotationWidget
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_view->horizontalScrollBar(),
                        HSLIDER_ROW, MAIN_COL, 1, 1);

    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)),
            this, SLOT(slotHScrollBarRangeChanged(int, int)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedContentsScrolled()),
            this, SLOT(slotHScroll()));

    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotZoomInFromPanner()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotZoomOutFromPanner()));

    m_toolBox = new NotationToolBox(this);

    //!!! 
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>
        (m_toolBox->getTool(NoteInserter::ToolName));
    noteInserter->slotSetNote(Note::Crotchet);
    noteInserter->slotSetDots(0);
    m_currentTool = noteInserter;
    m_currentTool->ready();
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

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);
    m_hpanner->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

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


    m_layout->addWidget(m_topStandardRuler, TOPRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_bottomStandardRuler, BOTTOMRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_tempoRuler, TEMPORULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_chordNameRuler, CHORDNAMERULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_rawNoteRuler, RAWNOTERULER_ROW, MAIN_COL, 1, 1);

// Following lines are needed to have loop selection working in loop ruler,
// but there is no snap grid in notation ...
//    m_topStandardRuler->setSnapGrid(m_scene->getSnapGrid());
//    m_bottomStandardRuler->setSnapGrid(m_scene->getSnapGrid());

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

    //!!! attempt to scroll either to the start or to the current
    //!!! pointer position, and to the top of the staff... however,
    //!!! this doesn't work... why not?
    m_view->ensureVisible(QRectF(0, 0, 1, 1), 0, 0);
    slotPointerPositionChanged(m_document->getComposition().getPosition());
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

void
NotationWidget::slotSetLinearMode()
{
    if (m_scene) m_scene->setPageMode(StaffLayout::LinearMode);
}

void
NotationWidget::slotSetContinuousPageMode()
{
    if (m_scene) m_scene->setPageMode(StaffLayout::ContinuousPageMode);
}

void
NotationWidget::slotSetMultiPageMode()
{
    if (m_scene) m_scene->setPageMode(StaffLayout::MultiPageMode);
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
NotationWidget::slotSetNoteInserter()
{
    slotSetTool(NoteInserter::ToolName);
}

void
NotationWidget::slotSetRestInserter()
{
    slotSetTool(RestInserter::ToolName);
}

void
NotationWidget::slotSetInsertedNote(Note::Type type, int dots)
{
    NoteInserter *ni = dynamic_cast<NoteInserter *>(m_currentTool);
    if (ni) {
        ni->slotSetNote(type);
        ni->slotSetDots(dots);
        return;
    }

    RestInserter *ri = dynamic_cast<RestInserter *>(m_currentTool);
    if (ri) {
        ri->slotSetNote(type);
        ri->slotSetDots(dots);
        return;
    }
}

void
NotationWidget::slotSetAccidental(Accidental accidental, bool follow)
{
    // You don't have to be in note insertion mode to change the accidental
    NoteInserter *ni = dynamic_cast<NoteInserter *>
        (m_toolBox->getTool(NoteInserter::ToolName));
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
NotationWidget::slotPointerPositionChanged(timeT t)
{
    QObject *s = sender();
    bool fromDocument = (s == m_document);

    NOTATION_DEBUG << "NotationWidget::slotPointerPositionChanged to " << t << endl;

    if (!m_scene) return;

    QPointF p = m_scene->snapTimeToStaffPosition(t);
    //!!! p will also contain sensible Y (although not 100% sensible yet)
    double sceneX = p.x();

    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        m_view->slotHidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->slotShowPositionPointer(sceneX);
        m_hpanner->slotShowPositionPointer(sceneX);
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
            if (m_scene) m_scene->slotSetInsertCursorPosition(e->time, true, true); //!!!
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
    slotHScroll();
}

void
NotationWidget::slotHScroll()
{
    // Get time of the window left
    QPointF topLeft = m_view->mapToScene(0, 0);

    // Apply zoom correction (Offset of 20 found empirically : probably
    // some improvments are needed ...)
    int x = (topLeft.x() - 20) * m_hZoomFactor;

    // Scroll rulers accordingly
    m_topStandardRuler->slotScrollHoriz(x);
    m_bottomStandardRuler->slotScrollHoriz(x);
    m_tempoRuler->slotScrollHoriz(x);
    m_chordNameRuler->slotScrollHoriz(x);
    m_rawNoteRuler->slotScrollHoriz(x);
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
    if (visible) m_tempoRuler->show();
    else m_tempoRuler->hide();
}

void
NotationWidget::setChordNameRulerVisible(bool visible)
{
    if (visible) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
}

void
NotationWidget::setRawNoteRulerVisible(bool visible)
{
    if (visible) m_rawNoteRuler->show();
    else m_rawNoteRuler->hide();
}

void
NotationWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    slotHScroll();
}

}

#include "NotationWidget.moc"

