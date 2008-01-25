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


#include "SplitByPitchDialog.h"

#include <klocale.h>
#include "commands/segment/SegmentSplitByPitchCommand.h"
#include "gui/general/ClefIndex.h"
#include "gui/widgets/PitchChooser.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qlayout.h>


namespace Rosegarden
{

SplitByPitchDialog::SplitByPitchDialog(QWidget *parent) :
        KDialogBase(parent, 0, true, i18n("Split by Pitch"), Ok | Cancel)
{
    QVBox *vBox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    m_pitch = new PitchChooser(i18n("Starting split pitch"), frame, 60);
    layout->addMultiCellWidget(m_pitch, 0, 0, 0, 2, Qt::AlignHCenter);

    m_range = new QCheckBox(i18n("Range up and down to follow music"), frame);
    layout->addMultiCellWidget(m_range,
                               1, 1,  // fromRow, toRow
                               0, 2  // fromCol, toCol
                              );

    m_duplicate = new QCheckBox(i18n("Duplicate non-note events"), frame);
    layout->addMultiCellWidget(m_duplicate, 2, 2, 0, 2);

    layout->addWidget(new QLabel(i18n("Clef handling:"), frame), 3, 0);

    m_clefs = new KComboBox(frame);
    m_clefs->insertItem(i18n("Leave clefs alone"));
    m_clefs->insertItem(i18n("Guess new clefs"));
    m_clefs->insertItem(i18n("Use treble and bass clefs"));
    layout->addMultiCellWidget(m_clefs, 3, 3, 1, 2);

    m_range->setChecked(true);
    m_duplicate->setChecked(true);
    m_clefs->setCurrentItem(2);
}

int
SplitByPitchDialog::getPitch()
{
    return m_pitch->getPitch();
}

bool
SplitByPitchDialog::getShouldRange()
{
    return m_range->isChecked();
}

bool
SplitByPitchDialog::getShouldDuplicateNonNoteEvents()
{
    return m_duplicate->isChecked();
}

int
SplitByPitchDialog::getClefHandling()
{
    switch (m_clefs->currentItem()) {
    case 0:
        return (int)SegmentSplitByPitchCommand::LeaveClefs;
    case 1:
        return (int)SegmentSplitByPitchCommand::RecalculateClefs;
    default:
        return (int)SegmentSplitByPitchCommand::UseTrebleAndBassClefs;
    }
}

}
#include "SplitByPitchDialog.moc"
