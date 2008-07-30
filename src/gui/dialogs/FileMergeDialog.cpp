/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "FileMergeDialog.h"

#include <klocale.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include "document/RosegardenGUIDoc.h"


namespace Rosegarden
{

FileMergeDialog::FileMergeDialog(QWidget *parent,
                                 QString /*fileName*/,
                                 bool timingsDiffer) :
        KDialogBase(parent, 0, true, i18n("Merge File"), Ok | Cancel | Help)
{
    setHelp("file-merge");

    QVBox *vbox = makeVBoxMainWidget();

    QHBox *hbox = new QHBox(vbox);
    new QLabel(i18n("Merge new file  "), hbox);

    m_choice = new KComboBox(hbox);
    m_choice->insertItem(i18n("At start of existing composition"));
    m_choice->insertItem(i18n("From end of existing composition"));
    m_useTimings = 0;

    if (timingsDiffer) {
        new QLabel(i18n("The file has different time signatures or tempos."), vbox);
        m_useTimings = new QCheckBox(i18n("Import these as well"), vbox);
        m_useTimings->setChecked(false);
    }
}

int
FileMergeDialog::getMergeOptions()
{
    int options = MERGE_KEEP_OLD_TIMINGS | MERGE_IN_NEW_TRACKS;

    if (m_choice->currentItem() == 1) {
        options |= MERGE_AT_END;
    }

    if (m_useTimings && m_useTimings->isChecked()) {
        options |= MERGE_KEEP_NEW_TIMINGS;
    }

    return options;
}

}
#include "FileMergeDialog.moc"
