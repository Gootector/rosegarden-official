
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_ROSEGARDENPARAMETERBOX_H_
#define _RG_ROSEGARDENPARAMETERBOX_H_

#include "RosegardenParameterArea.h"
#include <qfont.h>
#include <qframe.h>
#include <qstring.h>
#include <klocale.h>


class QWidget;


namespace Rosegarden
{



/**
 * A flat QFrame, in which a group of parameters can be laid out.
 * Virtual method functions are defined for for requesting a layout
 * style, and returning the single-word to use for labelling the
 * box.
 */

class RosegardenParameterBox : public QFrame
{
    Q_OBJECT
public:
    RosegardenParameterBox(const QString &shortLabel, // e.g. i18n("Track")
                           const QString &longLabel,  // e.g. i18n("Track Parameters")
                           QWidget *parent = 0,
                           const char *name = 0);

    // Ask for a one-word string that can be used to label the widget.
    QString getShortLabel() const;

    // Ask for the full label (e.g. short-label "Parameters")
    QString getLongLabel() const;

    // Get the short label of the prior parameter box (to establish an ordering)
    virtual QString getPreviousBox(RosegardenParameterArea::Arrangement) const;

    virtual void showAdditionalControls(bool) = 0; 

protected:
    void init();

    // List the layout styles that may be requested via a call to setStyle().

    enum LayoutMode {
        LANDSCAPE_MODE,  // Optimize the layout for a tall and narrow parent.
        PORTRAIT_MODE    // Optimize the layout for a short and wide parent.
    };

    void setLayoutMode(LayoutMode mode);

    QFont m_font;
    QString m_shortLabel;    // The string that containers can use for labelling and identification
    QString m_longLabel;    // The full title
    LayoutMode m_mode;  // The current layout mode.
};


}

#endif
