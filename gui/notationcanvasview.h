
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

#ifndef NOTATIONCANVASVIEW_H
#define NOTATIONCANVASVIEW_H

#include <qcanvas.h>

// #include "notepixmapfactory.h"

class StaffLine;
class NotationElement;

/**
 * Central widget for the NotationView window
 *
 *@author Guillaume Laurent, Chris Cannam, Rich Bown
 *@see NotationView
 */

class NotationCanvasView : public QCanvasView
{
    Q_OBJECT

public:
    NotationCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                       const char *name=0, WFlags f=0);

    ~NotationCanvasView();

    /** Callback for a mouse button press event in the canvas */
    virtual void contentsMousePressEvent(QMouseEvent *e);
    /** Callback for a mouse button release event in the canvas */
    virtual void contentsMouseReleaseEvent(QMouseEvent *e);
    /** Callback for a mouse move event in the canvas */
    virtual void contentsMouseMoveEvent(QMouseEvent *e);

// Used for a note-shaped cursor - leaving around just in case
//     void setCurrentNotePixmap(QCanvasPixmap note);

public slots:

//     void currentNoteChanged(Note::Type);

signals:
    void itemClicked(int pitch, const QPoint&, NotationElement*);

    void hoveredOverNoteChange(const QString &noteName);
    void hoveredOverAbsoluteTimeChange(unsigned int);
    
protected:

    void handleClick(const StaffLine*, const QPoint&, NotationElement* = 0);

    bool posIsTooFarFromStaff(const QPoint &pos);

    /// returns the pitch the staff line is associated with
/*!    int getPitchForLine(const StaffLine *line); */

    /// returns the note name (C4, Bb3) the staff line is associated with
    QString getNoteNameForLine(const StaffLine *line);

    /// the staff line over which the mouse cursor is
    StaffLine* m_currentHighlightedLine;

    // Used for a note-shaped cursor - leaving around just in case
//     QCanvasSprite *m_currentNotePixmap;
//     NotePixmapFactory m_notePixmapFactory;

    int m_lastYPosNearStaff;
};


#endif
