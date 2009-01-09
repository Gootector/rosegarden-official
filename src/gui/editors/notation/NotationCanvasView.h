
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

#ifndef _RG_NOTATIONCANVASVIEW_H_
#define _RG_NOTATIONCANVASVIEW_H_

#include <Q3Canvas>
#include <Q3CanvasItem>
#include <Q3CanvasItemList>
#include <Q3CanvasText>
#include "gui/general/RosegardenCanvasView.h"
#include <QRect>
#include <vector>


class QWidget;
class QString;
class QPoint;
class QPaintEvent;
class QPainter;
class QMouseEvent;
class QCanvasLineGroupable;
class QCanvasItemGroup;
class Q3CanvasItem;
class Q3Canvas;


namespace Rosegarden
{

class NotationStaff;
class NotationElement;
class LinedStaffManager;


/**
 * Central widget for the NotationView window
 *
 * This class only takes care of the event handling
 * (see the various signals).
 *
 * It does not deal with any canvas element. All elements are added by
 * the NotationView.
 *
 *@see NotationView
 */

class NotationCanvasView : public RosegardenCanvasView
{
    Q_OBJECT

public:
    NotationCanvasView(const LinedStaffManager &staffmgr,
                       Q3Canvas *viewing, QWidget *parent=0,
                       const char *name=0);	//, WFlags f=0);

    ~NotationCanvasView();

    void setHeightTracking(bool t);

signals:

    /**
     * Emitted when the user clicks on a staff (e.g. mouse button press)
     * \a pitch is set to the MIDI pitch on which the click occurred
     * \a staffNo is set to the staff on which the click occurred
     * \a point is set to the coordinates of the click event
     * \a el points to the NotationElement which was clicked on, if any
     */
    void itemPressed(int pitch, int staffNo,
                     QMouseEvent*,
                     NotationElement* el);

    /**
     * Emitted when the user clicks on a Q3CanvasItem which is active
     *
     * @see Q3CanvasItem#setActive
     */
    void activeItemPressed(QMouseEvent*,
                           Q3CanvasItem* item);

    /**
     * Emitted when the user clicks on a Q3CanvasItem which is neither
     * active nor a notation element
     */
    void nonNotationItemPressed(QMouseEvent *,
                                Q3CanvasItem *);

    /**
     * Emitted when the user clicks on a Q3CanvasItem which is a
     * plain Q3CanvasText
     */
    void textItemPressed(QMouseEvent *,
                         Q3CanvasItem *);

    /**
     * Emitted when the mouse cursor moves to a different height
     * on the staff
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
    void hoveredOverNoteChanged(const QString &noteName);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int time);

    /**
     * Emitted when the mouse cursor moves (used by the selection tool)
     */
    void mouseMoved(QMouseEvent*);

    /**
     * Emitted when the mouse button is released
     */
    void mouseReleased(QMouseEvent*);

    /**
     * Emitted when a region is about to be drawn by the canvas view.
     * Indicates that any on-demand rendering for that region should
     * be carried out.
     */
    void renderRequired(double cx0, double cx1);

public slots:
     void slotRenderComplete();

    void slotExternalWheelEvent(QWheelEvent* e);

protected:

    virtual void viewportPaintEvent(QPaintEvent *e);
    virtual void drawContents(QPainter *p, int cx, int cy, int cw, int ch);

    const LinedStaffManager &m_linedStaffManager;

    /**
     * Callback for a mouse button press event in the canvas
     */
    virtual void contentsMousePressEvent(QMouseEvent*);

    /**
     * Callback for a mouse button release event in the canvas
     */
    virtual void contentsMouseReleaseEvent(QMouseEvent*);

    /**
     * Callback for a mouse move event in the canvas
     */
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    /**
     * Callback for a mouse double click event in the canvas
     */
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    void processActiveItems(QMouseEvent*, Q3CanvasItemList);

    void handleMousePress(int height, int staffNo,
                          QMouseEvent*,
                          NotationElement* pressedNotationElement = 0);

    bool posIsTooFarFromStaff(const QPoint &pos);

    int getLegerLineCount(int height, bool &offset);

    void setHeightMarkerHeight(QMouseEvent *e);

    NotationElement *getElementAtXCoord(QMouseEvent *e);

    //--------------- Data members ---------------------------------

    int m_lastYPosNearStaff;

    unsigned int m_staffLineThreshold; 

    NotationStaff *m_currentStaff;
    int m_currentHeight;

    QCanvasItemGroup *m_heightMarker;
    QCanvasLineGroupable *m_vert1;
    QCanvasLineGroupable *m_vert2;
    std::vector<QCanvasLineGroupable *> m_legerLines;
    bool m_legerLineOffset;

    bool m_heightTracking;

    QRect m_lastRender;
};



}

#endif
