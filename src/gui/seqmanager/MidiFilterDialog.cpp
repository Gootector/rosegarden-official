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


#include "MidiFilterDialog.h"

#include <klocale.h>
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/seqmanager/SequenceManager.h"
#include "sound/MappedEvent.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QWidget>
#include <QHBoxLayout>


namespace Rosegarden
{

MidiFilterDialog::MidiFilterDialog(QDialogButtonBox::QWidget *parent,
                                   RosegardenGUIDoc *doc):
        QDialog(parent),
        m_doc(doc),
        m_modified(true)
{
    setHelp("studio-midi-filters");

    setModal(true);
    setWindowTitle(i18n("Modify MIDI filters..."));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *hBox = new QWidget(this);
    QHBoxLayout hBoxLayout = new QHBoxLayout;
    metagrid->addWidget(hBox, 0, 0);


    m_thruBox = new QGroupBox(
                         i18n("THRU events to ignore"), hBox );
    hBoxLayout->addWidget(m_thruBox);

    QCheckBox *noteThru = new QCheckBox(i18n("Note"), m_thruBox);
    QCheckBox *progThru = new QCheckBox(i18n("Program Change"), m_thruBox);
    QCheckBox *keyThru = new QCheckBox(i18n("Key Pressure"), m_thruBox);
    QCheckBox *chanThru = new QCheckBox(i18n("Channel Pressure"), m_thruBox);
    QCheckBox *pitchThru = new QCheckBox(i18n("Pitch Bend"), m_thruBox);
    QCheckBox *contThru = new QCheckBox(i18n("Controller"), m_thruBox);
    QCheckBox *sysThru = new QCheckBox(i18n("System Exclusive"), m_thruBox);

    MidiFilter thruFilter = m_doc->getStudio().getMIDIThruFilter();

    if (thruFilter & MappedEvent::MidiNote)
        noteThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiProgramChange)
        progThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiKeyPressure)
        keyThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiChannelPressure)
        chanThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiPitchBend)
        pitchThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiController)
        contThru->setChecked(true);

    if (thruFilter & MappedEvent::MidiSystemMessage)
        sysThru->setChecked(true);

    m_recordBox = new QGroupBox(
                         i18n("RECORD events to ignore"), hBox );
    hBoxLayout->addWidget(m_recordBox);
    hBox->setLayout(hBoxLayout);

    QCheckBox *noteRecord = new QCheckBox(i18n("Note"), m_recordBox);
    QCheckBox *progRecord = new QCheckBox(i18n("Program Change"), m_recordBox);
    QCheckBox *keyRecord = new QCheckBox(i18n("Key Pressure"), m_recordBox);
    QCheckBox *chanRecord = new QCheckBox(i18n("Channel Pressure"), m_recordBox);
    QCheckBox *pitchRecord = new QCheckBox(i18n("Pitch Bend"), m_recordBox);
    QCheckBox *contRecord = new QCheckBox(i18n("Controller"), m_recordBox);
    QCheckBox *sysRecord = new QCheckBox(i18n("System Exclusive"), m_recordBox);

    MidiFilter recordFilter =
        m_doc->getStudio().getMIDIRecordFilter();

    if (recordFilter & MappedEvent::MidiNote)
        noteRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiProgramChange)
        progRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiKeyPressure)
        keyRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiChannelPressure)
        chanRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiPitchBend)
        pitchRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiController)
        contRecord->setChecked(true);

    if (recordFilter & MappedEvent::MidiSystemMessage)
        sysRecord->setChecked(true);


    connect(m_thruBox, SIGNAL(released(int)),
            this, SLOT(slotSetModified()));

    connect(m_recordBox, SIGNAL(released(int)),
            this, SLOT(slotSetModified()));

    setModified(false);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
MidiFilterDialog::slotApply()
{
    MidiFilter thruFilter = 0,
                            recordFilter = 0;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(0))->isChecked())
        thruFilter |= MappedEvent::MidiNote;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(1))->isChecked())
        thruFilter |= MappedEvent::MidiProgramChange;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(2))->isChecked())
        thruFilter |= MappedEvent::MidiKeyPressure;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(3))->isChecked())
        thruFilter |= MappedEvent::MidiChannelPressure;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(4))->isChecked())
        thruFilter |= MappedEvent::MidiPitchBend;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(5))->isChecked())
        thruFilter |= MappedEvent::MidiController;

    if (dynamic_cast<QCheckBox*>(m_thruBox->find(6))->isChecked())
        thruFilter |= MappedEvent::MidiSystemMessage;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(0))->isChecked())
        recordFilter |= MappedEvent::MidiNote;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(1))->isChecked())
        recordFilter |= MappedEvent::MidiProgramChange;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(2))->isChecked())
        recordFilter |= MappedEvent::MidiKeyPressure;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(3))->isChecked())
        recordFilter |= MappedEvent::MidiChannelPressure;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(4))->isChecked())
        recordFilter |= MappedEvent::MidiPitchBend;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(5))->isChecked())
        recordFilter |= MappedEvent::MidiController;

    if (dynamic_cast<QCheckBox*>(m_recordBox->find(6))->isChecked())
        recordFilter |= MappedEvent::MidiSystemMessage;


    //if (m_thruBox->

    m_doc->getStudio().setMIDIThruFilter(thruFilter);
    m_doc->getStudio().setMIDIRecordFilter(recordFilter);

    if (m_doc->getSequenceManager()) {
        m_doc->getSequenceManager()->filtersChanged(thruFilter, recordFilter);
    }

    setModified(false);
}

void
MidiFilterDialog::slotOk()
{
    slotApply();
    accept();
}

void
MidiFilterDialog::slotSetModified()
{
    setModified(true);
}

void
MidiFilterDialog::setModified(bool value)
{
    if (m_modified == value)
        return ;

    if (value) {
        enableButtonApply(true);
    } else {
        enableButtonApply(false);
    }

    m_modified = value;

}

}
#include "MidiFilterDialog.moc"
