
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef TRACKSCANVAS_H
#define TRACKSCANVAS_H

#include <qwidget.h>
#include <qcanvas.h>

namespace Rosegarden { class Track; }

class TrackPartItem : public QCanvasRectangle
{
public:
    TrackPartItem(QCanvas* canvas);
    TrackPartItem(const QRect &, QCanvas* canvas);
    TrackPartItem(int x, int y, int width, int height, QCanvas* canvas);

    unsigned int getLength() const;
    unsigned int getStartIndex() const;

    int  getInstrument() const { return m_instrument; }
    void setInstrument(int i)  { m_instrument = i; }

    void setTrack(Rosegarden::Track *p)  { m_track = p; }
    Rosegarden::Track* getTrack() const  { return m_track; }

    static void setWidthToLengthRatio(unsigned int);

protected:
    int m_instrument;

    Rosegarden::Track* m_track;

    static unsigned int m_widthToLengthRatio;

};

class TrackTool;

/**
 * A class to visualize and edit track parts
 *
 * @author Guillaume Laurent, Chris Cannam, Rich Bown
 */

class TracksCanvas : public QCanvasView
{
    Q_OBJECT

public:
    enum ToolType { Pencil, Eraser, Mover, Resizer };
    
    TracksCanvas(int gridH, int gridV,
                 QCanvas&,
                 QWidget* parent=0, const char* name=0, WFlags f=0);
    ~TracksCanvas();

    void clear();
    unsigned int gridHStep() const { return m_grid.hstep(); }

    class SnapGrid
    {
    public:
        SnapGrid(unsigned int hstep, unsigned int vstep)
            : m_hstep(hstep), m_vstep(vstep)
        {}

        int snapX(int x) const { return x / m_hstep * m_hstep; }
        int snapY(int y) const { return y / m_vstep * m_vstep; }

        unsigned int hstep() const { return m_hstep; }
        unsigned int vstep() const { return m_vstep; }

    protected:
        unsigned int m_hstep;
        unsigned int m_vstep;
    };

    const SnapGrid& grid() const { return m_grid; }
    const QBrush& brush()  const { return m_brush; }
    const QPen& pen()      const { return m_pen; }

    TrackPartItem* addPartItem(int x, int y, unsigned int nbBars);
    TrackPartItem* findPartClickedOn(QPoint);

public slots:
    void setTool(TracksCanvas::ToolType);
    virtual void update();

protected:
    void contentsMousePressEvent(QMouseEvent*);
    void contentsMouseReleaseEvent(QMouseEvent*);
    void contentsMouseMoveEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);

protected slots:
/**
 * connected to the 'Edit' item of the popup menu - re-emits
 * editTrackPart(TrackPart*)
 */
    void onEdit();
    void onEditSmall();

signals:
    void addTrack(TrackPartItem*);
    void deleteTrack(Rosegarden::Track*);
    void resizeTrack(Rosegarden::Track*);
    void editTrack(Rosegarden::Track*);
    void editTrackSmall(Rosegarden::Track*);

private:
    ToolType m_toolType;
    TrackTool *m_tool;

    SnapGrid m_grid;

    TrackPartItem* m_currentItem;

    QCanvasItem* m_moving;

    QBrush m_brush;
    QPen m_pen;

    QPopupMenu *m_editMenu;
    
};

//////////////////////////////////////////////////////////////////////
//                 Track Tools
//////////////////////////////////////////////////////////////////////

class TrackTool : public QObject
{
public:
    TrackTool(TracksCanvas*);
    virtual ~TrackTool();

    virtual void handleMouseButtonPress(QMouseEvent*)  = 0;
    virtual void handleMouseButtonRelase(QMouseEvent*) = 0;
    virtual void handleMouseMove(QMouseEvent*)         = 0;

protected:
    TracksCanvas*  m_canvas;
    TrackPartItem* m_currentItem;
};

//////////////////////////////
// TrackPencil
//////////////////////////////

class TrackPencil : public TrackTool
{
    Q_OBJECT
public:
    TrackPencil(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void addTrack(TrackPartItem*);
    void resizeTrack(Rosegarden::Track*);
    void deleteTrack(Rosegarden::Track*);

protected:
    bool m_newRect;
};

class TrackEraser : public TrackTool
{
    Q_OBJECT
public:
    TrackEraser(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteTrack(Rosegarden::Track*);
};

class TrackMover : public TrackTool
{
public:
    TrackMover(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);
};

/**
 * Track Resizer tool. Allows resizing only at the end of the track part
 */
class TrackResizer : public TrackTool
{
    Q_OBJECT
public:
    TrackResizer(TracksCanvas*);

    virtual void handleMouseButtonPress(QMouseEvent*);
    virtual void handleMouseButtonRelase(QMouseEvent*);
    virtual void handleMouseMove(QMouseEvent*);

signals:
    void deleteTrack(Rosegarden::Track*);
    void resizeTrack(Rosegarden::Track*);

protected:
    bool cursorIsCloseEnoughToEdge(TrackPartItem*, QMouseEvent*);

    unsigned int m_edgeThreshold;
};

#endif
