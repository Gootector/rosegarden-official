
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

#ifndef _RG_QCANVASMATRIXRECTANGLE_H_
#define _RG_QCANVASMATRIXRECTANGLE_H_

#include <qcanvas.h>


namespace Rosegarden
{

class MatrixElement;


/**
 * A QCanvasRectangle referencing a MatrixElement
 */
class QCanvasMatrixRectangle : public QCanvasRectangle
{
public:
    QCanvasMatrixRectangle(MatrixElement&, QCanvas*);

    virtual ~QCanvasMatrixRectangle();
    
    MatrixElement& getMatrixElement() { return m_matrixElement; }
    
protected:
    //--------------- Data members ---------------------------------

    MatrixElement& m_matrixElement;

};


}

#endif
