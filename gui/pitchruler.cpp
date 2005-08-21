// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include "pitchruler.h"


PitchRuler::
PitchRuler(QWidget *parent) :
    QWidget(parent)
{
    // nothing else
}

QSize
PitchRuler::sizeHint() const
{
    return QWidget::sizeHint();
}

QSize
PitchRuler::minimumSizeHint() const
{
    return QWidget::minimumSizeHint();
}

#include "pitchruler.moc"
