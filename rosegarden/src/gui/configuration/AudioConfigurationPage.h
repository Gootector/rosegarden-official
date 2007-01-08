
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

#ifndef _RG_AUDIOCONFIGURATIONPAGE_H_
#define _RG_AUDIOCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <qstring.h>
#include <klocale.h>


class QWidget;
class QPushButton;
class QLabel;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * Audio Configuration page
 *
 * (document-wide settings)
 */
class AudioConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioConfigurationPage(RosegardenGUIDoc *doc,
                           QWidget *parent=0, const char *name=0);
    virtual void apply();

    static QString iconLabel() { return i18n("Audio"); }
    static QString title()     { return i18n("Audio Settings"); }
    static QString iconName()  { return "folder"; }

protected slots:
    void slotFileDialog();

    // Work out and display remaining disk space and time left 
    // at current path.
    //
    void calculateStats();

    void slotFoundMountPoint(const QString&,
                             unsigned long kBSize,
                             unsigned long kBUsed,
                             unsigned long kBAvail);
    
protected:

    //--------------- Data members ---------------------------------

    QLabel           *m_path;
    QLabel           *m_diskSpace;
    QLabel           *m_minutesAtStereo;

    QPushButton      *m_changePathButton;
};


}

#endif
