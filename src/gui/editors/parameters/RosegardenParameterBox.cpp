/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenParameterBox.h"

#include "RosegardenParameterArea.h"
#include <ktabwidget.h>
#include <qfont.h>
#include <qframe.h>
#include <qscrollview.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qwidgetstack.h>


namespace Rosegarden
{

RosegardenParameterBox::RosegardenParameterBox(const QString &shortLabel,
        const QString &longLabel,
        QWidget *parent,
        const char *name) :
        QFrame(parent, name),
        m_shortLabel(shortLabel),
        m_longLabel(longLabel),
        m_mode(LANDSCAPE_MODE)
{
    init();
}

void RosegardenParameterBox::init()
{
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 95 / 100);
    if (plainFont.pixelSize() > 14)
        plainFont.setPixelSize(14);
    plainFont.setBold(false);
    m_font = plainFont;

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    if (boldFont.pixelSize() > 14)
        boldFont.setPixelSize(14);
    boldFont.setBold(true);

    setFont(boldFont);
}

QString RosegardenParameterBox::getShortLabel() const
{
    return m_shortLabel;
}

QString RosegardenParameterBox::getLongLabel() const
{
    return m_longLabel;
}

QString RosegardenParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement) const
{
    // No ordering known -- depends on subclasses
    return "";
}

}
#include "RosegardenParameterBox.moc"
