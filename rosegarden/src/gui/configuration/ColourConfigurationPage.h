
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

#ifndef _RG_COLOURCONFIGURATIONPAGE_H_
#define _RG_COLOURCONFIGURATIONPAGE_H_

#include "base/ColourMap.h"
#include "gui/widgets/ColourTable.h"
#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <klocale.h>


class QWidget;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * Colour Configuration Page
 *
 * (document-wide settings)
 */
class ColourConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    ColourConfigurationPage(RosegardenGUIDoc *doc,
                            QWidget *parent=0, const char *name=0);
    virtual void apply();

    void populate_table();

    static QString iconLabel() { return i18n("Color"); }
    static QString title()     { return i18n("Color Settings"); }
    static QString iconName()  { return "colorize"; }

signals:
    void docColoursChanged();

protected slots:
    void slotAddNew();
    void slotDelete();
    void slotTextChanged(unsigned int, QString);
    void slotColourChanged(unsigned int, QColor);

protected:
    ColourTable *m_colourtable;

    ColourMap m_map;
    ColourTable::ColourList m_listmap;

};

// -----------  SequencerConfigurationage -----------------
//


}

#endif
