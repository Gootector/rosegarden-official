// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <vector>

#include <qcanvas.h>
#include <kmainwindow.h>

#include "Staff.h"
#include "LayoutEngine.h"
#include "FastVector.h"

#include "editview.h"
#include "linedstaff.h"
#include "matrixelement.h"

namespace Rosegarden { class Segment; }

class RosegardenGUIDoc;
class MatrixStaff;
class QMouseEvent;

class MatrixCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    MatrixCanvasView(MatrixStaff&, QCanvas *viewing=0, QWidget *parent=0,
                     const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

signals:

    /**
     * Emitted when the user clicks on a QCanvasItem which is active
     *
     * @see QCanvasItem#setActive
     */
    void activeItemPressed(QMouseEvent*,
                           QCanvasItem* item);

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

    void mousePressed(Rosegarden::timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void mouseMoved(Rosegarden::timeT time, QMouseEvent*);

    void mouseReleased(Rosegarden::timeT time, QMouseEvent*);

protected:
    /**
     * Callback for a mouse button press event in the canvas
     */
    virtual void contentsMousePressEvent(QMouseEvent*);

    /**
     * Callback for a mouse move event in the canvas
     */
    virtual void contentsMouseMoveEvent(QMouseEvent*);

    /**
     * Callback for a mouse button release event in the canvas
     */
    virtual void contentsMouseReleaseEvent(QMouseEvent*);

    /**
     * Callback for a mouse double-click event in the canvas
     *
     * NOTE: a double click event is always preceded by a mouse press
     * event
     */
    virtual void contentsMouseDoubleClickEvent(QMouseEvent*);

    //--------------- Data members ---------------------------------

    MatrixStaff& m_staff;

    Rosegarden::timeT m_previousEvTime;
    int m_previousEvPitch;

    bool m_mouseWasPressed;
    bool m_ignoreClick;
};

//------------------------------------------------------------
//                       Layouts
//------------------------------------------------------------

class MatrixVLayout : public Rosegarden::VerticalLayoutEngine<MatrixElement>
{
public:
    MatrixVLayout();

    virtual ~MatrixVLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType &staff);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout();

    static const unsigned int maxMIDIPitch;

protected:
    //--------------- Data members ---------------------------------


};

//------------------------------

class MatrixHLayout : public Rosegarden::HorizontalLayoutEngine<MatrixElement>
{
public:
    MatrixHLayout();
    virtual ~MatrixHLayout();

    /**
     * Resets internal data stores for all staffs
     */
    virtual void reset();

    /**
     * Resets internal data stores for a specific staff
     */
    virtual void resetStaff(StaffType &staff);

    /**
     * Returns the total length of all elements once layout is done.
     * This is the x-coord of the end of the last element on the
     * longest staff
     */
    virtual double getTotalWidth();

    /**
     * Returns the total number of bar lines on the given staff
     */
    virtual unsigned int getBarLineCount(StaffType &staff);

    /**
     * Returns the x-coordinate of the given bar number (zero-based)
     * on the given staff
     */
    virtual double getBarLineX(StaffType &staff, unsigned int barNo);

    /**
     * Precomputes layout data for a single staff, updating any
     * internal data stores associated with that staff and updating
     * any layout-related properties in the events on the staff's
     * segment.
     */
    virtual void scanStaff(StaffType&);

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one staff.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     */
    virtual void finishLayout();

protected:

    //--------------- Data members ---------------------------------

    // pair of layout-x and time-signature event if there is one
    typedef std::pair<double, Rosegarden::Event *> BarData;
    typedef FastVector<BarData> BarDataList;
    BarDataList m_barData;
    double m_totalWidth;
};

//------------------------------------------------------------

typedef Rosegarden::ViewElementList<MatrixElement> MatrixElementList;

class MatrixStaff : public LinedStaff<MatrixElement>
{
public:
    MatrixStaff(QCanvas *, Rosegarden::Segment *, int id, int vResolution);
    virtual ~MatrixStaff();

protected:
    virtual int getLineCount() const;
    virtual int getLegerLineCount() const;
    virtual int getBottomLineHeight() const;
    virtual int getHeightPerLine() const;
    virtual bool elementsInSpaces() const;
    virtual bool showBeatLines() const;

    /**
     * Override from Rosegarden::Staff<T>
     * Wrap only notes 
     */
    virtual bool wrapEvent(Rosegarden::Event*);

public:
    LinedStaff<MatrixElement>::setResolution;

    double getTimeScaleFactor() const { return m_scaleFactor; }
    void setTimeScaleFactor(double f) { m_scaleFactor = f; }

    Rosegarden::timeT getTimeForCanvasX(double x); // assuming one row only

    int getElementHeight() { return m_resolution; }

    virtual void positionElements(Rosegarden::timeT from = -1,
				  Rosegarden::timeT to = -1);

    virtual void positionElement(MatrixElement*);

    /**
     * Override from Rosegarden::Staff<T>
     * Check a flag before wrapping event
     */
    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *);

    QString getNoteNameForPitch(unsigned int pitch);

    void setWrapAddedEvents(bool wrap = true) { m_wrapAddedEvents = wrap; }

private:
    double m_scaleFactor;

    bool m_wrapAddedEvents;
};


//------------------------------------------------------------

/**
 * Matrix ("Piano Roll") View
 *
 * Note: we currently display only one staff
 */
class MatrixView : public EditView
{
    Q_OBJECT
public:
    MatrixView(RosegardenGUIDoc *doc,
               std::vector<Rosegarden::Segment *> segments,
               QWidget *parent);

    virtual ~MatrixView();

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime,
				Rosegarden::timeT endTime);

    QCanvas* canvas() { return getCanvasView()->canvas(); }

    MatrixStaff* getStaff(int) { return m_staffs[0]; } // deal with 1 staff only
    virtual void update();

public slots:

    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut();

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste();

    /// edition tools
    void slotPaintSelected();
    void slotEraseSelected();
    void slotSelectSelected();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a matrix element
     * or somewhere on the staff
     */
    void mousePressed(Rosegarden::timeT time, int pitch,
                      QMouseEvent*, MatrixElement*);

    void mouseMoved(Rosegarden::timeT time, QMouseEvent*);
    void mouseReleased(Rosegarden::timeT time, QMouseEvent*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverNoteChanged(const QString&);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int);

protected:

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void saveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    /**
     * Return the size of the MatrixCanvasView
     */
    virtual QSize getViewSize();

    /**
     * Set the size of the MatrixCanvasView
     */
    virtual void setViewSize(QSize);

    //--------------- Data members ---------------------------------

    std::vector<MatrixStaff*> m_staffs;
    
    MatrixHLayout* m_hlayout;
    MatrixVLayout* m_vlayout;

    // Status bar elements
    QLabel* m_hoveredOverAbsoluteTime;
    QLabel* m_hoveredOverNoteName;

    virtual MatrixCanvasView *getCanvasView() {
	return static_cast<MatrixCanvasView *>(m_canvasView);
    }
};

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

#include "edittool.h"

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
public:
//     virtual void ready();

protected:
    MatrixTool(const QString& menuName, MatrixView*);

    //--------------- Data members ---------------------------------

    MatrixView* m_mParentView;
};

class MatrixPainter : public MatrixTool
{
    Q_OBJECT

    friend MatrixToolBox;

public:

    virtual void handleLeftButtonPress(Rosegarden::timeT,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*);

    /**
     * Set the duration of the element
     */
    virtual void handleMouseMove(Rosegarden::timeT,
                                 int height,
                                 QMouseEvent*);

    /**
     * Actually insert the new element
     */
    virtual void handleMouseRelease(Rosegarden::timeT,
                                    int height,
                                    QMouseEvent*);

    static const QString ToolName;

public slots:
    /**
     * Set the shortest note which can be "painted"
     * on the matrix
     */
    void setResolution(Rosegarden::Note::Type);

protected:
    MatrixPainter(MatrixView*);

    MatrixElement* m_currentElement;
    MatrixStaff* m_currentStaff;

    Rosegarden::Note::Type m_resolution;
    Rosegarden::timeT m_basicDuration;
};



class MatrixEraser : public MatrixTool
{
    Q_OBJECT

    friend MatrixToolBox;

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

#endif
