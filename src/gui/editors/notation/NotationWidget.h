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

#ifndef _RG_NOTATION_WIDGET_H_
#define _RG_NOTATION_WIDGET_H_

#include "StaffLayout.h"

#include "base/NotationTypes.h"
#include "gui/general/SelectionManager.h"
#include "gui/widgets/Thumbwheel.h"

#include <QWidget>
#include <QPushButton>
#include <QBoxLayout>

#include <vector>

class QGridLayout;
class QString;
class QGraphicsScene;

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class NotationScene;
class Note;
class NotationToolBox;
class NotationTool;
class NotationMouseEvent;
class Panner;
class Panned;
class ZoomableRulerScale;
class StandardRuler;
class TempoRuler;
class ChordNameRuler;
class RawNoteRuler;
class ControlRulerWidget;
class HeadersGroup;

class NotationWidget : public QWidget,
                       public SelectionManager
{
    Q_OBJECT

public:
    NotationWidget();
    virtual ~NotationWidget();

    void setSegments(RosegardenDocument *document, 
                     std::vector<Segment *> segments);

    NotationScene *getScene() { return m_scene; }

    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview);

    timeT getInsertionTime() const;

    bool isInChordMode() { return m_chordMode; }
    bool isInTripletMode() { return m_tripletMode; }
    bool isInGraceMode() { return m_graceMode; }

    void setChordMode(bool state = true) { m_chordMode = state; }
    void setTripletMode(bool state = true) { m_tripletMode = state;}
    void setGraceMode(bool state = true) { m_graceMode = state; }

    bool getPlayTracking() const { return m_playTracking; }

    NotationToolBox *getToolBox() { return m_toolBox; }
    NotationTool *getCurrentTool() const;

    void setCanvasCursor(QCursor cursor);

    Segment *getCurrentSegment();
    bool segmentsContainNotes() const;

    void setTempoRulerVisible(bool visible);
    void setChordNameRulerVisible(bool visible);
    void setRawNoteRulerVisible(bool visible);
    void setHeadersVisible(bool visible);
    void toggleHeadersView();

    double getViewLeftX();
    int getNotationViewWidth();
    double getNotationSceneHeight();

    void setPointerPosition(timeT);

    void setHorizontalZoomFactor(double factor);
    void setVerticalZoomFactor(double factor);

    double getHorizontalZoomFactor() const;
    double getVerticalZoomFactor() const;

signals:
    void segmentDeleted(Segment *);
    void sceneDeleted();

public slots:
    void slotSetTool(QString name);
    void slotSetSelectTool();
    void slotSetEraseTool();
    void slotSetNoteRestInserter();
    void slotSetNoteInserter();
    void slotSetRestInserter();
    void slotSetInsertedNote(Note::Type type, int dots);
    void slotSetAccidental(Accidental accidental, bool follow);
    void slotSetClefInserter();
    void slotSetInsertedClef(Clef type);
    void slotSetTextInserter();
    void slotSetGuitarChordInserter();
    void slotSetLinearMode();
    void slotSetContinuousPageMode();
    void slotSetMultiPageMode();
    void slotSetFontName(QString);
    void slotSetFontSize(int);
    void slotSetPlayTracking(bool);
    void slotTogglePlayTracking();
    void slotSetSymbolInserter();
    void slotSetInsertedSymbol(Symbol type);

protected:
    virtual void showEvent(QShowEvent * event);
    void hideOrShowRulers();

protected slots:
    void slotDispatchMousePress(const NotationMouseEvent *);
    void slotDispatchMouseRelease(const NotationMouseEvent *);
    void slotDispatchMouseMove(const NotationMouseEvent *);
    void slotDispatchMouseDoubleClick(const NotationMouseEvent *);

    void slotPointerPositionChanged(timeT);
    void slotEnsureLastMouseMoveVisible();

    void slotZoomInFromPanner();
    void slotZoomOutFromPanner();

    void slotHScroll();
    void slotHScrollBarRangeChanged(int min, int max);

    /// The horizontal zoom thumbwheel moved
    void slotHorizontalThumbwheelMoved(int);

    /// The vertical zoom thumbwheel moved
    void slotVerticalThumbwheelMoved(int);

    /// The primary (combined axes) thumbwheel moved
    void slotPrimaryThumbwheelMoved(int);

    /// Reset the zoom to 100% and reset the zoomy wheels
    void slotResetZoomClicked();

    /// Trap a zoom in from the panner and sync it to the primary thumb wheel
    void slotSyncPannerZoomIn();

    /// Trap a zoom out from the panner and sync it to the primary thumb wheel
    void slotSyncPannerZoomOut();

    void slotShowHeaderToolTip(QString toolTipText);
    void slotHeadersResized(int width);
    void slotAdjustHeadersHorizontalPos(bool last);
    void slotAdjustHeadersVerticalPos(QRectF r);
    void slotCloseHeaders();

signals :
    void adjustNeeded(bool last);

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    NotationScene *m_scene; // I own this
    bool m_playTracking;
    double m_hZoomFactor;
    double m_vZoomFactor;
    ZoomableRulerScale *m_referenceScale; // I own this (refers to scene scale)
    NotationToolBox *m_toolBox;
    NotationTool *m_currentTool;
    bool m_inMove;
    QPointF m_lastMouseMoveScenePos;

    QWidget     *m_panner;
    QBoxLayout  *m_pannerLayout;
    Thumbwheel  *m_HVzoom;
    Thumbwheel  *m_Hzoom;
    Thumbwheel  *m_Vzoom;
    QPushButton *m_reset;

    /** The primary zoom wheel behaves just like using the mouse wheel over any
     * part of the Panner.  We don't need to keep track of absolute values here,
     * just whether we rolled up or down.  We'll do that by keeping track of the
     * last setting and comparing it to see which way it moved.
     */
    int m_lastHVzoomValue;
    bool m_lastZoomWasHV;
    int m_lastV;
    int m_lastH;

    StandardRuler *m_topStandardRuler; // I own this
    StandardRuler *m_bottomStandardRuler; // I own this
    TempoRuler *m_tempoRuler; // I own this
    ChordNameRuler *m_chordNameRuler; // I own this
    RawNoteRuler *m_rawNoteRuler; // I own this
    ControlRulerWidget *m_controlsWidget; // I own this

    HeadersGroup *m_headersGroup; // I own this
    Panned *m_headersView; // I own this
    QGraphicsScene *m_headersScene; // I own this
    QWidget *m_headersButtons; // I own this
    double m_headersLastY;

    QGridLayout *m_layout; // I own this

    bool m_linearMode;
    bool m_tempoRulerIsVisible;         // Only valid in linear mode
    bool m_rawNoteRulerIsVisible;       // Only valid in linear mode
    bool m_chordNameRulerIsVisible;     // Only valid in linear mode
    bool m_headersAreVisible;           // Only valid in linear mode

    bool m_chordMode;
    bool m_tripletMode;
    bool m_graceMode;

    void locatePanner(bool vertical);

    /**
     * Widgets vertical positions inside the main QGridLayout
     */
    enum {
        CHORDNAMERULER_ROW,
        TEMPORULER_ROW,
        RAWNOTERULER_ROW,
        TOPRULER_ROW,
        PANNED_ROW,
        BOTTOMRULER_ROW,
        CONTROLS_ROW,
        HSLIDER_ROW,
        PANNER_ROW
    };

    /**
     * Widgets horizontal positions inside the main QGridLayout
     */
    enum {
        HEADER_COL,
        MAIN_COL,
        VPANNER_COL
    };

};

}

#endif

    
