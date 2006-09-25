// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef MATRIXTOOL_H
#define MATRIXTOOL_H

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

#include "NotationTypes.h"

#include "edittool.h"

class MatrixView;
class MatrixElement;
class MatrixStaff;

class MatrixToolBox : public EditToolBox
{
public:
    MatrixToolBox(MatrixView* parent);

protected:

    virtual EditTool* createTool(const QString& toolName);

    //--------------- Data members ---------------------------------

    MatrixView* m_mParentView;
};

//////////////////////////////////////////////////////////////////////
//                     MatrixTools
//////////////////////////////////////////////////////////////////////

class MatrixTool : public EditTool
{
    Q_OBJECT

public:
//     virtual void ready();

protected slots:

    // For switching between tools on RMB
    //
    void slotSelectSelected();
    void slotMoveSelected();
    void slotEraseSelected();
    void slotResizeSelected();
    void slotDrawSelected();

protected:
    MatrixTool(const QString& menuName, MatrixView*);

    //--------------- Data members ---------------------------------

    MatrixView* m_mParentView;
};

class MatrixPainter : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual int handleMouseMove(Rosegarden::timeT,
                                int height,
                                QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Rosegarden::Event *event);

    virtual void ready();
    virtual void stow();

protected slots:

    void slotMatrixScrolled(int x, int y);

protected:
    MatrixPainter(MatrixView*);
    MatrixPainter(QString name, MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff*   m_currentStaff;
};



class MatrixEraser : public MatrixTool
{
    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    static const QString ToolName;

protected:
    MatrixEraser(MatrixView*);

    MatrixStaff* m_currentStaff;
};

#include "Selection.h"
class QCanvasRectangle;

// Selector is a multipurpose tool and delegates often to other
// tools to speciliase its behaviour.
//
class MatrixSelector : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT time,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement *element);

    virtual void handleMidButtonPress(Rosegarden::timeT time,
                                      int height,
                                      int staffNo,
                                      QMouseEvent *event,
                                      Rosegarden::ViewElement *element);

    virtual int handleMouseMove(Rosegarden::timeT time,
                                int height,
                                QMouseEvent *event);

    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent *event);

    /**
     * Double-click: edit an event or make a whole-bar selection
     */
    virtual void handleMouseDoubleClick(Rosegarden::timeT time,
					int height,
					int staffNo,
					QMouseEvent* event,
					Rosegarden::ViewElement *element);

    /**
     * Triple-click: maybe make a whole-staff selection
     */
    virtual void handleMouseTripleClick(Rosegarden::timeT time,
					int height,
					int staffNo,
					QMouseEvent* event,
					Rosegarden::ViewElement *element);


    /**
     * Create the selection rect
     *
     * We need this because MatrixView deletes all QCanvasItems
     * along with it. This happens before the MatrixSelector is
     * deleted, so we can't delete the selection rect in
     * ~MatrixSelector because that leads to double deletion.
     */
    virtual void ready();

    /**
     * Delete the selection rect.
     */
    virtual void stow();

    /**
     * Returns the currently selected events
     *
     * The returned result is owned by the caller
     */
    Rosegarden::EventSelection* getSelection();

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Rosegarden::Event *event);

    static const QString ToolName;

public slots:
    /**
     * Hide the selection rectangle
     *
     * Should be called after a cut or a copy has been
     * performed
     */
    void slotHideSelection();

    void slotClickTimeout();

protected slots:

    void slotMatrixScrolled(int x, int y);

signals:
    void gotSelection(); // inform that we've got a new selection
    void editTriggerSegment(int);
    
protected:
    MatrixSelector(MatrixView*);

    void setViewCurrentSelection();
    
    //--------------- Data members ---------------------------------

    QCanvasRectangle* m_selectionRect;
    bool m_updateRect;

    int m_clickedStaff;
    MatrixStaff* m_currentStaff;

    MatrixElement* m_clickedElement;

    // tool to delegate to
    EditTool*    m_dispatchTool;

    bool m_justSelectedBar;

    MatrixView * m_matrixView;

    Rosegarden::EventSelection *m_selectionToMerge;
};


class MatrixMover : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual int handleMouseMove(Rosegarden::timeT,
                                int height,
                                QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Rosegarden::Event *event);

    virtual void ready();
    virtual void stow();

protected slots:

    void slotMatrixScrolled(int x, int y);

protected:
    MatrixMover(MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;

    // store MatrixElement's size
    int m_oldWidth;
    double m_oldX;
    double m_oldY;
};


class MatrixResizer : public MatrixTool
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual int handleMouseMove(Rosegarden::timeT,
                                int height,
                                QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Rosegarden::Event *event);

    virtual void ready();
    virtual void stow();

protected slots:

    void slotMatrixScrolled(int x, int y);

protected:
    MatrixResizer(MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;
};


#endif
