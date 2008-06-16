// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef QCANVASSIMPLESPRITE_H
#define QCANVASSIMPLESPRITE_H

#include <qwmatrix.h>
#include <qcanvas.h>

#include "gui/editors/notation/NotePixmapFactory.h"

namespace Rosegarden {

/**
 * A QCanvasSprite with 1 frame only
 */
class QCanvasSimpleSprite : public QCanvasSprite
{
public:
    QCanvasSimpleSprite(QPixmap*, QCanvas*);
    QCanvasSimpleSprite(QCanvasPixmap*, QCanvas*);
    QCanvasSimpleSprite(const QString &pixmapfile, QCanvas*);

    // For lazy pixmap rendering, when we get around looking at it
    QCanvasSimpleSprite(QCanvas*);

    virtual ~QCanvasSimpleSprite();

protected:
    static QCanvasPixmapArray* makePixmapArray(QPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(QCanvasPixmap *pixmap);

    static QCanvasPixmapArray* makePixmapArray(const QString &pixmapfile);

    //--------------- Data members ---------------------------------

    QCanvasPixmapArray* m_pixmapArray;
};

class NotationElement;

/**
 * A QCanvasSprite referencing a NotationElement
 */
class QCanvasNotationSprite : public QCanvasSimpleSprite
{
public:
    QCanvasNotationSprite(Rosegarden::NotationElement&, QPixmap*, QCanvas*);
    QCanvasNotationSprite(Rosegarden::NotationElement&, QCanvasPixmap*, QCanvas*);

    virtual ~QCanvasNotationSprite();
    
    Rosegarden::NotationElement& getNotationElement() { return m_notationElement; }

protected:
    //--------------- Data members ---------------------------------

    Rosegarden::NotationElement& m_notationElement;
};

class QCanvasNonElementSprite : public QCanvasSimpleSprite
{
public:
    QCanvasNonElementSprite(QPixmap *, QCanvas *);
    QCanvasNonElementSprite(QCanvasPixmap *, QCanvas *);
    virtual ~QCanvasNonElementSprite();
};

/**
 * A QCanvasSprite used for a time signature
 */
class QCanvasTimeSigSprite : public QCanvasNonElementSprite
{
public:
    QCanvasTimeSigSprite(double layoutX, QPixmap *, QCanvas *);
    QCanvasTimeSigSprite(double layoutX, QCanvasPixmap *, QCanvas *);
    virtual ~QCanvasTimeSigSprite();

    void setLayoutX(double layoutX) { m_layoutX = layoutX; }
    double getLayoutX() const { return m_layoutX; }

protected:
    double m_layoutX;
};

/**
 * A QCanvasSprite used for a staff name
 */
class QCanvasStaffNameSprite : public QCanvasNonElementSprite
{
public:
    QCanvasStaffNameSprite(QPixmap *, QCanvas *);
    QCanvasStaffNameSprite(QCanvasPixmap *, QCanvas *);
    virtual ~QCanvasStaffNameSprite();
};

/**
 * A GC for QCanvasPixmapArray which have to be deleted seperatly
 */
class PixmapArrayGC
{
public:
    static void registerForDeletion(QCanvasPixmapArray*);
    static void deleteAll();
    
protected:
    //--------------- Data members ---------------------------------

    static std::vector<QCanvasPixmapArray*> m_pixmapArrays;
};

}

#endif
