
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_DOCUMENTMETACONFIGURATIONPAGE_H_
#define _RG_DOCUMENTMETACONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <klocale.h>


class QWidget;
class KListView;


namespace Rosegarden
{

class RosegardenGUIDoc;
class HeadersConfigurationPage;

/**
 * Document Meta-information page
 *
 * (document-wide settings)
 */
class DocumentMetaConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
                                  QWidget *parent = 0, const char *name = 0);
    virtual void apply();

    static QString iconLabel() { return i18n("About"); }
    static QString title() { return i18n("About"); }
    static QString iconName()  { return "contents"; }

/* hjj: WHAT TO DO WITH THIS ?
    void selectMetadata(QString name);
*/

protected:

    //--------------- Data members ---------------------------------

    HeadersConfigurationPage *m_headersPage;
};



}

#endif
