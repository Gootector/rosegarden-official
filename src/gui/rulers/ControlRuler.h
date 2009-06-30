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

#ifndef _RG_CONTROLRULER_H_
#define _RG_CONTROLRULER_H_

#include <QWidget>
//#include <Q3Canvas>
//#include <Q3CanvasItemList>
//#include <Q3CanvasRectangle>
#include "base/Segment.h"
#include "base/Selection.h"
//#include "gui/general/RosegardenCanvasView.h"
#include "gui/editors/matrix/MatrixViewSegment.h"
#include <QColor>
#include <QPoint>
#include <QString>
#include <utility>

#include "ControlItem.h"

//class QWidget;
class QMenu;
class QWheelEvent;
class QScrollBar;
class QMouseEvent;
class QContextMenuEvent;
//class Q3CanvasRectangle;
//class Q3Canvas;


namespace Rosegarden
{

class ControlTool;
class ControlSelector;
//class ControlItem;
//class ControlItemList;
class Segment;
class RulerScale;
class EventSelection;
class EditViewBase;

//typedef std::list<ControlItem*> ControlItemList;
//typedef std::list<ControlItem*>::iterator ControlItemListIterator;

/**
 * ControlRuler : base class for Control Rulers
 */
//class ControlRuler : public RosegardenCanvasView, public SegmentObserver, public EventSelectionObserver
//class ControlRuler : public QWidget, public SegmentObserver, public EventSelectionObserver
class ControlRuler : public QWidget, public ViewSegmentObserver
{
    Q_OBJECT

    friend class ControlItem;

public:
    ControlRuler(MatrixViewSegment*,
                 RulerScale*,
//                 EditViewBase* parentView,
//                 Q3Canvas*,
                 QWidget* parent=0); //###  const char name is obsolete, and I'm almost sure WFlags is obsolete too
    virtual ~ControlRuler();

    virtual QString getName() = 0;

    virtual QSize sizeHint() { return QSize(1,100); }

    virtual void paintEvent(QPaintEvent *);

    int getMaxItemValue() { return m_maxItemValue; }
    void setMaxItemValue(int val) { m_maxItemValue = val; }

    void clear();

    void setControlTool(ControlTool*);

    int applyTool(double x, int val);

//    Q3CanvasRectangle* getSelectionRectangle() { return m_selectionRect; }
    QRect* getSelectionRectangle() { return m_selectionRect; }

    virtual void setSegment(Segment *);
    virtual void setViewSegment(MatrixViewSegment *);

    virtual void setRulerScale(RulerScale *rulerscale) { m_rulerScale = rulerscale; }
    RulerScale* getRulerScale() { return m_rulerScale; }

    /// EventSelectionObserver
//    virtual void eventSelected(EventSelection *,Event *);
//    virtual void eventDeselected(EventSelection *,Event *);
//    virtual void eventSelectionDestroyed(EventSelection *);

//    void assignEventSelection(EventSelection *);

    // SegmentObserver interface
    virtual void viewSegmentDeleted(const ViewSegment *);

    static const int DefaultRulerHeight;
    static const int MinItemHeight;
    static const int MaxItemHeight;
    static const int ItemHeightRange;

    void flipForwards();
    void flipBackwards();

//    void setMainHorizontalScrollBar(QScrollBar* s ) { m_mainHorizontalScrollBar = s; }

signals:
    void stateChange(const QString&, bool);

public slots:
    /// override RosegardenCanvasView - we don't want to change the main hscrollbar
    virtual void slotUpdate();
//    virtual void slotUpdateElementsHPos();
    virtual void slotScrollHorizSmallSteps(int);
    virtual void slotSetPannedRect(QRectF);
//    virtual void slotSetScale(double);
    virtual void slotSetToolName(const QString&);

protected:
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void contextMenuEvent(QContextMenuEvent*);
    virtual void wheelEvent(QWheelEvent*);

//    virtual QScrollBar* getMainHorizontalScrollBar();

//    virtual void computeViewSegmentOffset() {};

//    virtual void layoutItem(ControlItem*);



    // Stacking of the SegmentItems on the canvas
    //
    std::pair<int, int> getZMinMax();

//    virtual void init();
//virtual void drawBackground() = 0;

    int xMapToWidget(double x) {return (x-m_pannedRect.left())*width()/m_pannedRect.width();};
    QPolygon mapItemToWidget(QPolygonF*);
    QPointF mapWidgetToItem(QPoint*);

    int valueToHeight(long val);
    long heightToValue(int height);
    QColor valueToColour(int max, int val);

    void clearSelectedItems();
    void updateSelection();

    void setMenuName(QString menuName) { m_menuName = menuName; }
    void createMenu();

    //--------------- Data members ---------------------------------

//    EditViewBase*               m_parentEditView;
//    QScrollBar*                 m_mainHorizontalScrollBar;
    RulerScale*     m_rulerScale;
    EventSelection* m_eventSelection; //,*m_assignedEventSelection;

    MatrixScene *m_scene;

    MatrixViewSegment *m_viewSegment;
    Segment *m_segment;

    ControlItemList m_controlItemList;

    ControlItem* m_currentIndex;
//    Q3CanvasItemList m_selectedItems;
    ControlItemList m_selectedItems;

    ControlTool *m_tool;
    QString m_currentToolName;

    QRectF m_pannedRect;
    double m_scale;

    int m_maxItemValue;

    double m_viewSegmentOffset;

    double m_currentX;

    QPoint m_lastEventPos;
    bool m_itemMoved;

    bool m_overItem;
    bool m_selecting;
    ControlSelector* m_selector;
//    Q3CanvasRectangle* m_selectionRect;
    QRect* m_selectionRect;

    QString m_menuName;
    QMenu* 	m_menu;

    bool m_hposUpdatePending;

    typedef std::list<Event *> SelectionSet;
    SelectionSet m_selectedEvents;
};


}

#endif
