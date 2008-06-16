/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
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


#include "KLedButton.h"

#include <kled.h>
#include <qcolor.h>
#include <qwidget.h>


namespace Rosegarden
{

KLedButton::KLedButton(const QColor &col, QWidget *parent, const char *name)
        : KLed( col, parent, name )
{}

KLedButton::KLedButton(const QColor& col, KLed::State st, KLed::Look look,
                       KLed::Shape shape, QWidget *parent, const char *name)
        : KLed( col, st, look, shape, parent, name )
{}

KLedButton::~KLedButton()
{}

void KLedButton::mousePressEvent( QMouseEvent *e )
{
    if (e->button() == LeftButton) {
        toggle();
        emit stateChanged( state() );
    }
}

}
#include "KLedButton.moc"
