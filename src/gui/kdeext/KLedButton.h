
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    This file taken from KMix
    Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_KLEDBUTTON_H_
#define _RG_KLEDBUTTON_H_

#include <kled.h>


class QWidget;
class QMouseEvent;
class QColor;


namespace Rosegarden
{

/**
 * @author Stefan Schimanski
 * Taken from KMix code,
 * Copyright (C) 2000 Stefan Schimanski <1Stein@gmx.de>
 */
class KLedButton : public KLed  {
   Q_OBJECT
  public: 
   KLedButton(const QColor &col=QColor(Qt::green), QWidget *parent=0, const char *name=0);
   KLedButton(const QColor& col, KLed::State st, KLed::Look look, KLed::Shape shape,
              QWidget *parent=0, const char *name=0);
   ~KLedButton();       

  signals:
   void stateChanged( bool newState );

  protected:    
   void mousePressEvent ( QMouseEvent *e );
};

}

#endif
