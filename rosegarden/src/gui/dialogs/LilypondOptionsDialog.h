
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

#ifndef _RG_LILYPONDOPTIONSDIALOG_H_
#define _RG_LILYPONDOPTIONSDIALOG_H_

#include <kdialogbase.h>
#include <qstring.h>


class QWidget;
class QComboBox;
class QCheckBox;


namespace Rosegarden
{



class LilypondOptionsDialog : public KDialogBase
{
    Q_OBJECT

public:
    LilypondOptionsDialog(QWidget *parent,
                          QString windowCaption = "",
                          QString heading = "");

public slots:
    void slotOk();

protected:
    QComboBox *m_lilyLanguage;
    QComboBox *m_lilyPaperSize;
    QComboBox *m_lilyFontSize;
    QComboBox *m_lilyTempoMarks;
    QComboBox *m_lilyExportSelection;
    QCheckBox *m_lilyPaperLandscape;
    QCheckBox *m_lilyExportHeaders;
    QCheckBox *m_lilyExportLyrics;
    QCheckBox *m_lilyExportMidi;
    QCheckBox *m_lilyExportPointAndClick;
    QCheckBox *m_lilyExportBeams;
    QCheckBox *m_lilyExportStaffGroup;
    QCheckBox *m_lilyExportStaffMerge;
};



}

#endif
