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


#include "EventFilterDialog.h"

#include "base/BaseProperties.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Quantizer.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include <kconfig.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qwidget.h>


namespace Rosegarden
{

EventFilterDialog::EventFilterDialog(QWidget* parent)
        : KDialogBase(parent, "eventfilerdialog", true, i18n("Event Filter"), Ok | Cancel, Ok),
        m_standardQuantizations(BasicQuantizer::getStandardQuantizations())
{
    cfg = kapp->config();
    initDialog();
}

EventFilterDialog::~EventFilterDialog()
{
    // nothing here
}

void
EventFilterDialog::initDialog()
{
    QVBox* mainWidget = makeVBoxMainWidget();


    //----------[ Note Filter Widgets ]-------------------------

    // Frame
    QGroupBox* noteFrame = new QGroupBox(i18n("Note Events"), mainWidget);
    QGridLayout* noteFrameLayout = new QGridLayout(noteFrame, 1, 1, 20, 6);

    // Master Checkbox
    m_noteCheckBox = new QCheckBox(i18n("Enable"), noteFrame);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteCheckBox->setChecked(cfg->readBoolEntry("notecheckbox", true));
    noteFrameLayout->addWidget(m_noteCheckBox, 0, 0);
    connect(m_noteCheckBox, SIGNAL(stateChanged(int)), this,
            SLOT(slotNoteCheckBoxToggle(int)));

    // Labels
    QLabel* pitchFromLabel = new QLabel(i18n("from:"), noteFrame);
    noteFrameLayout->addWidget(pitchFromLabel, 0, 2);

    QLabel* pitchToLabel = new QLabel(i18n("to:"), noteFrame);
    noteFrameLayout->addWidget(pitchToLabel, 0, 4);

    QLabel* pitchLabel = new QLabel(i18n("Pitch:"), noteFrame);
    noteFrameLayout->addWidget(pitchLabel, 1, 1);

    QLabel* velocityLabel = new QLabel(i18n("Velocity:"), noteFrame);
    noteFrameLayout->addWidget(velocityLabel, 2, 1);

    QLabel* durationLabel = new QLabel(i18n("Duration:"), noteFrame);
    noteFrameLayout->addWidget(durationLabel, 3, 1);

    // Include Boxes
    m_notePitchIncludeComboBox = new QComboBox(0, noteFrame);
    m_notePitchIncludeComboBox->insertItem(i18n("include"));
    m_notePitchIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_notePitchIncludeComboBox->setCurrentItem(cfg->readBoolEntry("pitchinclude", 0));
    noteFrameLayout->addWidget(m_notePitchIncludeComboBox, 1, 0);

    m_noteVelocityIncludeComboBox = new QComboBox(0, noteFrame);
    m_noteVelocityIncludeComboBox->insertItem(i18n("include"));
    m_noteVelocityIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteVelocityIncludeComboBox->setCurrentItem(cfg->readBoolEntry("velocityinclude", 0));
    noteFrameLayout->addWidget(m_noteVelocityIncludeComboBox, 2, 0);

    m_noteDurationIncludeComboBox = new QComboBox(0, noteFrame);
    m_noteDurationIncludeComboBox->insertItem(i18n("include"));
    m_noteDurationIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteDurationIncludeComboBox->setCurrentItem(cfg->readBoolEntry("durationinclude", 0));
    noteFrameLayout->addWidget(m_noteDurationIncludeComboBox, 3, 0);

    // Pitch From
    m_pitchFromSpinBox = new QSpinBox(noteFrame);
    m_pitchFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_pitchFromSpinBox->setValue(cfg->readUnsignedNumEntry("pitchfrom", 0));
    noteFrameLayout->addWidget(m_pitchFromSpinBox, 1, 2);
    connect(m_pitchFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchFromChanged(int)));

    m_pitchFromChooserButton = new QPushButton(i18n("..."), noteFrame);
    m_pitchFromChooserButton->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
                                            (QSizePolicy::SizeType)0, 0, 0, m_pitchFromChooserButton->
                                            sizePolicy().hasHeightForWidth()));
    QToolTip::add
        (m_pitchFromChooserButton, i18n("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchFromChooserButton, 1, 3);
    connect(m_pitchFromChooserButton, SIGNAL(clicked()),
            SLOT(slotPitchFromChooser()));

    // Pitch To
    m_pitchToSpinBox = new QSpinBox(noteFrame);
    m_pitchToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_pitchToSpinBox->setValue(cfg->readUnsignedNumEntry("pitchto", 127));
    noteFrameLayout->addWidget(m_pitchToSpinBox, 1, 4);
    connect(m_pitchToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchToChanged(int)));

    m_pitchToChooserButton = new QPushButton(i18n("..."), noteFrame);
    QToolTip::add
        (m_pitchToChooserButton, i18n("choose a pitch using a staff"));
    noteFrameLayout->addWidget(m_pitchToChooserButton, 1, 5);
    connect(m_pitchToChooserButton, SIGNAL(clicked()),
            SLOT(slotPitchToChooser()));

    // Velocity From/To
    m_velocityFromSpinBox = new QSpinBox(noteFrame);
    m_velocityFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_velocityFromSpinBox->setValue(cfg->readUnsignedNumEntry("velocityfrom", 0));
    noteFrameLayout->addWidget(m_velocityFromSpinBox, 2, 2);
    connect(m_velocityFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityFromChanged(int)));

    m_velocityToSpinBox = new QSpinBox(noteFrame);
    m_velocityToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_velocityToSpinBox->setValue(cfg->readUnsignedNumEntry("velocityto", 127));
    noteFrameLayout->addWidget( m_velocityToSpinBox, 2, 4 );
    connect(m_velocityToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityToChanged(int)));


    // Duration From/To
    m_noteDurationFromComboBox = new QComboBox(0, noteFrame);
    m_noteDurationFromComboBox->insertItem(i18n("unlimited"));
    noteFrameLayout->addWidget(m_noteDurationFromComboBox, 3, 2);
    connect(m_noteDurationFromComboBox, SIGNAL(activated(int)),
            SLOT(slotDurationFromChanged(int)));

    m_noteDurationToComboBox = new QComboBox(0, noteFrame);
    m_noteDurationToComboBox->insertItem(i18n("unlimited"));
    noteFrameLayout->addWidget(m_noteDurationToComboBox, 3, 4);
    connect(m_noteDurationToComboBox, SIGNAL(activated(int)),
            SLOT(slotDurationToChanged(int)));

    populateDurationCombos();

#ifdef COMPILE_DEPRECATED 
    // DEPRECATED

    //----------[ Controller Filter Widgets ]---------------------

    // Frame
    QGroupBox* controllerFrame = new QGroupBox(i18n("Controller Events"), mainWidget);
    QGridLayout* controllerFrameLayout = new QGridLayout(controllerFrame, 1, 1, 20, 6);

    // Master Checkbox
    m_controllerCheckBox = new QCheckBox(i18n("Enable"), controllerFrame);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerCheckBox->setChecked(cfg->readBoolEntry("controllercheckbox", false));
    controllerFrameLayout->addWidget(m_controllerCheckBox, 0, 0);
    connect(m_controllerCheckBox, SIGNAL(stateChanged(int)), this,
            SLOT(slotControllerCheckBoxToggle(int)));

    // Labels
    QLabel* controllerFromLabel = new QLabel(i18n("from:"), controllerFrame);
    controllerFrameLayout->addWidget(controllerFromLabel, 0, 2);

    QLabel* controllerToLabel = new QLabel(i18n("to:"), controllerFrame);
    controllerFrameLayout->addWidget(controllerToLabel, 0, 4);

    QLabel* numberLabel = new QLabel(i18n("Number:"), controllerFrame);
    controllerFrameLayout->addWidget(numberLabel, 1, 1);

    QLabel* valueLabel = new QLabel(i18n("Value:"), controllerFrame);
    controllerFrameLayout->addWidget(valueLabel, 2, 1);

    // Include Boxes
    m_controllerNumberIncludeComboBox = new QComboBox(0, controllerFrame);
    m_controllerNumberIncludeComboBox->insertItem(i18n("include"));
    m_controllerNumberIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerNumberIncludeComboBox->setCurrentItem(
        cfg->readUnsignedNumEntry("controllerinclude", 0));
    controllerFrameLayout->addWidget(m_controllerNumberIncludeComboBox, 1, 0);

    m_controllerValueIncludeComboBox = new QComboBox(0, controllerFrame);
    m_controllerValueIncludeComboBox->insertItem(i18n("include"));
    m_controllerValueIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerValueIncludeComboBox->setCurrentItem(
        cfg->readUnsignedNumEntry("valueinclude", 0));
    controllerFrameLayout->addWidget(m_controllerValueIncludeComboBox, 2, 0);

    // Spin Boxes From/To
    m_controllerNumberFromSpinBox = new QSpinBox(controllerFrame);
    m_controllerNumberFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerNumberFromSpinBox->setValue(cfg->readUnsignedNumEntry("controllerfrom", 0));
    controllerFrameLayout->addWidget(m_controllerNumberFromSpinBox, 1, 2);
    connect(m_controllerNumberFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotControllerFromChanged(int)));

    m_controllerNumberToSpinBox = new QSpinBox(controllerFrame);
    m_controllerNumberToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerNumberToSpinBox->setValue(cfg->readUnsignedNumEntry("controllerto", 127));
    controllerFrameLayout->addWidget( m_controllerNumberToSpinBox, 1, 4 );
    connect(m_controllerNumberToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotControllerToChanged(int)));

    m_controllerValueFromSpinBox = new QSpinBox(controllerFrame);
    m_controllerValueFromSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerValueFromSpinBox->setValue(cfg->readUnsignedNumEntry("valuefrom", 0));
    controllerFrameLayout->addWidget(m_controllerValueFromSpinBox, 2, 2);
    connect(m_controllerValueFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotValueFromChanged(int)));

    m_controllerValueToSpinBox = new QSpinBox(controllerFrame);
    m_controllerValueToSpinBox->setMaxValue(127);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_controllerValueToSpinBox->setValue(cfg->readUnsignedNumEntry("valueto", 127));
    controllerFrameLayout->addWidget( m_controllerValueToSpinBox, 2, 4 );
    connect(m_controllerValueToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotValueToChanged(int)));

    //----------[ Wheel Filter Widgets ]--------------------------

    // Frame
    QGroupBox* wheelFrame = new QGroupBox(i18n("Wheel Events"), mainWidget);
    QGridLayout* wheelFrameLayout = new QGridLayout(wheelFrame, 1, 1, 20, 6);

    // Master Checkbox
    m_wheelCheckBox = new QCheckBox(i18n("Enable"), wheelFrame);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_wheelCheckBox->setChecked(cfg->readBoolEntry("wheelcheckbox", false));
    wheelFrameLayout->addWidget(m_wheelCheckBox, 0, 0);
    connect(m_wheelCheckBox, SIGNAL(stateChanged(int)), this,
            SLOT(slotWheelCheckBoxToggle(int)));

    // Labels
    QLabel* wheelFromLabel = new QLabel(i18n("from:"), wheelFrame);
    wheelFrameLayout->addWidget(wheelFromLabel, 0, 2);

    QLabel* wheelToLabel = new QLabel(i18n("to:"), wheelFrame);
    wheelFrameLayout->addWidget(wheelToLabel, 0, 4);

    QLabel* wheelAmountLabel = new QLabel(i18n("Amount:"), wheelFrame);
    wheelFrameLayout->addMultiCellWidget(wheelAmountLabel, 1, 2, 1, 1);

    // Include Box
    m_wheelAmountIncludeComboBox = new QComboBox(0, wheelFrame);
    m_wheelAmountIncludeComboBox->insertItem(i18n("include"));
    m_wheelAmountIncludeComboBox->insertItem(i18n("exclude"));
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_wheelAmountIncludeComboBox->setCurrentItem(
        cfg->readUnsignedNumEntry("wheelinclude", 0));
    wheelFrameLayout->addMultiCellWidget(m_wheelAmountIncludeComboBox, 1, 2, 0, 0);

    // Spin Boxes From/To
    m_wheelAmountFromSpinBox = new QSpinBox(wheelFrame);
    m_wheelAmountFromSpinBox->setMaxValue(8191);
    m_wheelAmountFromSpinBox->setMinValue( -8192);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_wheelAmountFromSpinBox->setValue(cfg->readNumEntry("wheelfrom", -8192));
    wheelFrameLayout->addWidget(m_wheelAmountFromSpinBox, 2, 2);
    connect(m_wheelAmountFromSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotWheelFromChanged(int)));

    m_wheelAmountToSpinBox = new QSpinBox(wheelFrame);
    m_wheelAmountToSpinBox->setMaxValue(8191);
    m_wheelAmountToSpinBox->setMinValue( -8192);
    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_wheelAmountToSpinBox->setValue(cfg->readNumEntry("wheelto", 8191));
    wheelFrameLayout->addWidget(m_wheelAmountToSpinBox, 2, 4);
    connect(m_wheelAmountToSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotWheelToChanged(int)));

    // Force a sync with the checkboxes in the event that they were read false
    // from kconfig data.
    slotNoteCheckBoxToggle(0);
    slotControllerCheckBoxToggle(0);
    slotWheelCheckBoxToggle(0);

#endif // COMPILE_DEPRECATED

    //---------[ Buttons ]--------------------------------------
    QFrame* privateLayoutWidget = new QFrame(mainWidget);
    QGridLayout* buttonLayout = new QGridLayout(privateLayoutWidget, 1, 1, 20, 6);

    m_buttonAll = new QPushButton(i18n("Include all"), privateLayoutWidget);
    m_buttonAll->setAutoDefault(true);
    QToolTip::add
        (m_buttonAll, i18n("Include entire range of values"));
    buttonLayout->addWidget( m_buttonAll, 0, 0 );

    m_buttonNone = new QPushButton(i18n("Exclude all"), privateLayoutWidget);
    m_buttonNone->setAutoDefault(true);
    QToolTip::add
        (m_buttonNone, i18n("Exclude entire range of values"));
    buttonLayout->addWidget( m_buttonNone, 0, 1 );

    connect(m_buttonAll, SIGNAL(clicked()), this, SLOT(slotToggleAll()));
    connect(m_buttonNone, SIGNAL(clicked()), this, SLOT(slotToggleNone()));


}

void
EventFilterDialog::populateDurationCombos()
{
    QPixmap noMap = NotePixmapFactory::toQPixmap
                    (NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    for (unsigned int i = 0; i < m_standardQuantizations.size(); ++i) {
        timeT time = m_standardQuantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::toQPixmap
                       (NotePixmapFactory::makeNoteMenuPixmap(time, error));
        m_noteDurationFromComboBox->insertItem(error ? noMap : pmap, label);
        m_noteDurationToComboBox ->insertItem(error ? noMap : pmap, label);
    }
    m_noteDurationFromComboBox->insertItem(noMap, i18n("0"));
    m_noteDurationToComboBox->insertItem(noMap, i18n("0"));

    cfg->setGroup(EventFilterDialog::ConfigGroup);
    m_noteDurationFromComboBox->setCurrentItem(
        cfg->readUnsignedNumEntry("durationfrom", 0));
    m_noteDurationToComboBox->setCurrentItem(
        cfg->readUnsignedNumEntry("durationto", (m_noteDurationToComboBox->count() - 1)));
}

void
EventFilterDialog::slotToggleAll()
{
    RG_DEBUG << "EventFilterDialog::slotToggleAll()" << endl;
    m_pitchFromSpinBox ->setValue(0);
    m_pitchToSpinBox ->setValue(127);
    m_velocityFromSpinBox ->setValue(0);
    m_velocityToSpinBox ->setValue(127);
    m_noteDurationFromComboBox ->setCurrentItem(11); // hard coded; should be variable
    m_noteDurationToComboBox ->setCurrentItem(0);  // 0 = unlimited; 11 = 0
    m_controllerNumberFromSpinBox->setValue(0);
    m_controllerNumberToSpinBox ->setValue(127);
    m_controllerValueFromSpinBox ->setValue(0);
    m_controllerValueToSpinBox ->setValue(127);
    m_wheelAmountFromSpinBox ->setValue( -8192);
    m_wheelAmountToSpinBox ->setValue(8191);
}

void
EventFilterDialog::slotToggleNone()
{
    RG_DEBUG << "EventFilterDialog::slotToggleNone()" << endl;
    m_pitchFromSpinBox ->setValue(0);
    m_pitchToSpinBox ->setValue(0);
    m_velocityFromSpinBox ->setValue(0);
    m_velocityToSpinBox ->setValue(0);
    m_noteDurationFromComboBox ->setCurrentItem(11);
    m_noteDurationToComboBox ->setCurrentItem(11);
    m_controllerNumberFromSpinBox->setValue(0);
    m_controllerNumberToSpinBox ->setValue(0);
    m_controllerValueFromSpinBox ->setValue(0);
    m_controllerValueToSpinBox ->setValue(0);
    m_wheelAmountFromSpinBox ->setValue(0);
    m_wheelAmountToSpinBox ->setValue(0);
}

void
EventFilterDialog::slotOk()
{
    cfg->setGroup(EventFilterDialog::ConfigGroup);

    cfg->writeEntry("notecheckbox", m_noteCheckBox->isChecked());

    cfg->writeEntry("pitchinclude", m_notePitchIncludeComboBox->currentItem());
    cfg->writeEntry("pitchfrom", m_pitchFromSpinBox->value());
    cfg->writeEntry("pitchto", m_pitchToSpinBox->value());

    cfg->writeEntry("velocityinclude", m_noteVelocityIncludeComboBox->currentItem());
    cfg->writeEntry("velocityfrom", m_velocityFromSpinBox->value());
    cfg->writeEntry("velocityto", m_velocityToSpinBox->value());

    cfg->writeEntry("durationinclude", m_noteDurationIncludeComboBox->currentItem());
    cfg->writeEntry("durationfrom", m_noteDurationFromComboBox->currentItem());
    cfg->writeEntry("durationto", m_noteDurationToComboBox->currentItem());


#ifdef COMPILE_DEPRECATED

    cfg->writeEntry("controllercheckbox", m_controllerCheckBox->isChecked());

    cfg->writeEntry("controllerinclude", m_controllerNumberIncludeComboBox->currentItem());
    cfg->writeEntry("controllerfrom", m_controllerNumberFromSpinBox->value());
    cfg->writeEntry("controllerto", m_controllerNumberToSpinBox->value());

    cfg->writeEntry("valueinclude", m_controllerValueIncludeComboBox->currentItem());
    cfg->writeEntry("valuefrom", m_controllerValueFromSpinBox->value());
    cfg->writeEntry("valueto", m_controllerValueToSpinBox->value());


    cfg->writeEntry("wheelcheckbox", m_wheelCheckBox->isChecked());

    cfg->writeEntry("wheelinclude", m_wheelAmountIncludeComboBox->currentItem());
    cfg->writeEntry("wheelfrom", m_wheelAmountFromSpinBox->value());
    cfg->writeEntry("wheelto", m_wheelAmountToSpinBox->value());
#endif // DEPRECATED

    accept();
}

void
EventFilterDialog::slotNoteCheckBoxToggle(int)
{
    bool state = m_noteCheckBox->isChecked();
    m_notePitchIncludeComboBox ->setEnabled(state);
    m_noteVelocityIncludeComboBox->setEnabled(state);
    m_noteDurationIncludeComboBox->setEnabled(state);
    m_pitchToSpinBox ->setEnabled(state);
    m_pitchFromSpinBox ->setEnabled(state);
    m_velocityToSpinBox ->setEnabled(state);
    m_velocityFromSpinBox ->setEnabled(state);
    m_noteDurationToComboBox ->setEnabled(state);
    m_noteDurationFromComboBox ->setEnabled(state);
}

void
EventFilterDialog::slotControllerCheckBoxToggle(int)
{
    bool state = m_controllerCheckBox->isChecked();
    m_controllerNumberIncludeComboBox->setEnabled(state);
    m_controllerValueIncludeComboBox ->setEnabled(state);
    m_controllerNumberToSpinBox ->setEnabled(state);
    m_controllerNumberFromSpinBox ->setEnabled(state);
    m_controllerValueToSpinBox ->setEnabled(state);
    m_controllerValueFromSpinBox ->setEnabled(state);
}

void
EventFilterDialog::slotWheelCheckBoxToggle(int)
{
    bool state = m_wheelCheckBox->isChecked();
    m_wheelAmountIncludeComboBox->setEnabled(state);
    m_wheelAmountToSpinBox ->setEnabled(state);
    m_wheelAmountFromSpinBox ->setEnabled(state);
}

void
EventFilterDialog::slotPitchFromChanged(int pitch)
{
    if (pitch > m_pitchToSpinBox->value())
        m_pitchToSpinBox->setValue(pitch);
}

void
EventFilterDialog::slotPitchToChanged(int pitch)
{
    if (pitch < m_pitchFromSpinBox->value())
        m_pitchFromSpinBox->setValue(pitch);
}

void
EventFilterDialog::slotVelocityFromChanged(int velocity)
{
    if (velocity > m_velocityToSpinBox->value())
        m_velocityToSpinBox->setValue(velocity);
}

void
EventFilterDialog::slotVelocityToChanged(int velocity)
{
    if (velocity < m_velocityFromSpinBox->value())
        m_velocityFromSpinBox->setValue(velocity);
}

void
EventFilterDialog::slotDurationFromChanged(int index)
{
    if (index < m_noteDurationToComboBox->currentItem())
        m_noteDurationToComboBox->setCurrentItem(index);
}

void
EventFilterDialog::slotDurationToChanged(int index)
{
    if (index > m_noteDurationFromComboBox->currentItem())
        m_noteDurationFromComboBox->setCurrentItem(index);
}

void
EventFilterDialog::slotControllerFromChanged(int controller)
{
    if (controller > m_controllerNumberToSpinBox->value())
        m_controllerNumberToSpinBox->setValue(controller);
}

void
EventFilterDialog::slotControllerToChanged(int controller)
{
    if (controller < m_controllerNumberFromSpinBox->value())
        m_controllerNumberFromSpinBox->setValue(controller);
}

void
EventFilterDialog::slotValueFromChanged(int value)
{
    if (value > m_controllerValueToSpinBox->value())
        m_controllerValueToSpinBox->setValue(value);
}

void
EventFilterDialog::slotValueToChanged(int value)
{
    if (value < m_controllerValueFromSpinBox->value())
        m_controllerValueFromSpinBox->setValue(value);
}

void
EventFilterDialog::slotWheelFromChanged(int value)
{
    if (value > m_wheelAmountToSpinBox->value())
        m_wheelAmountToSpinBox->setValue(value);
}

void
EventFilterDialog::slotWheelToChanged(int value)
{
    if (value < m_wheelAmountFromSpinBox->value())
        m_wheelAmountFromSpinBox->setValue(value);
}

void
EventFilterDialog::slotPitchFromChooser()
{
    PitchPickerDialog dialog(this, m_pitchFromSpinBox->value(), i18n("Pitch from"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchFromSpinBox->setValue(dialog.getPitch());
    }
}

void
EventFilterDialog::slotPitchToChooser()
{
    PitchPickerDialog dialog(this, m_pitchToSpinBox->value(), i18n("Pitch to"));

    if (dialog.exec() == QDialog::Accepted) {
        m_pitchToSpinBox->setValue(dialog.getPitch());
    }
}

long
EventFilterDialog::getDurationFromIndex(int index)
{
    switch (index) {
        // 0
    case 11:
        return 0;
        // 1/96
    case 10:
        return long(Note(Note::SixtyFourthNote).getDuration() / 3);
        // 1/64
    case 9 :
        return long(Note(Note::SixtyFourthNote).getDuration());
        // 1/48
    case 8 :
        return long(Note(Note::ThirtySecondNote).getDuration() / 3);
        // 1/32
    case 7 :
        return long(Note(Note::ThirtySecondNote).getDuration());
        // 1/24
    case 6 :
        return long(Note(Note::SixteenthNote).getDuration() / 3);
        // 1/16
    case 5 :
        return long(Note(Note::SixteenthNote).getDuration());
        // 1/8
    case 4 :
        return long(Note(Note::EighthNote).getDuration());
        // 1/4
    case 3 :
        return long(Note(Note::QuarterNote).getDuration());
        // 1/2
    case 2 :
        return long(Note(Note::HalfNote).getDuration());
        // 1/1
    case 1 :
        return long(Note(Note::WholeNote).getDuration());
        // unlimited
    case 0 :
        return LONG_MAX;
    }
    // failsafe
    return LONG_MAX;
}

void
EventFilterDialog::invert(EventFilterDialog::filterRange &foo)
{
    long c = foo.first;
    foo.first = foo.second;
    foo.second = c;
}

EventFilterDialog::filterRange

EventFilterDialog::getPitch()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_pitchFromSpinBox->value();
    foo.second = m_pitchToSpinBox ->value();
    if (!pitchIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange

EventFilterDialog::getVelocity()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_velocityFromSpinBox->value();
    foo.second = m_velocityToSpinBox ->value();
    if (!velocityIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange

EventFilterDialog::getDuration()
{
    EventFilterDialog::filterRange foo;
    foo.first = getDurationFromIndex(m_noteDurationFromComboBox->currentItem());
    foo.second = getDurationFromIndex(m_noteDurationToComboBox ->currentItem());
    if (!durationIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange

EventFilterDialog::getController()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_controllerNumberFromSpinBox->value();
    foo.second = m_controllerNumberToSpinBox ->value();
    if (!controllerNumberIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange

EventFilterDialog::getValue()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_controllerValueFromSpinBox->value();
    foo.second = m_controllerValueToSpinBox ->value();
    if (!controllerValueIsInclusive())
        invert(foo);
    return foo;
}

EventFilterDialog::filterRange

EventFilterDialog::getWheel()
{
    EventFilterDialog::filterRange foo;
    foo.first = m_wheelAmountFromSpinBox->value();
    foo.second = m_wheelAmountToSpinBox ->value();
    if (!wheelIsInclusive())
        invert(foo);
    return foo;
}

bool
EventFilterDialog::keepEvent(Event* const &e)
{
    if ((*e).isa(Note::EventType)
#ifdef COMPILE_DEPRECATED
            ||
            (*e).isa(Controller::EventType) ||
            (*e).isa(PitchBend::EventType)
#endif
       ) {
        long property = 0;

        if ((*e).isa(Note::EventType) && filterNote()) {
            // pitch
            (*e).get<Int>(BaseProperties::PITCH, property);
            if (!eventInRange(getPitch(), property))
                return false;
            property = 0;

            // velocity
            (*e).get<Int>(BaseProperties::VELOCITY, property);
            if (!EventFilterDialog::eventInRange(getVelocity(), property))
                return false;
            property = 0;

            // duration
            property = (*e).getNotationDuration();
            if (!EventFilterDialog::eventInRange(getDuration(), property))
                return false;
            property = 0;
        }
#ifdef COMPILE_DEPRECATED
        else if ((*e).isa(Controller::EventType) && filterController()) {
            // controller number
            (*e).get<Int>(Controller::NUMBER, property);
            if (!EventFilterDialog::eventInRange(getController(), property))
                return false;
            property = 0;

            // controller value
            (*e).get<Int>(Controller::VALUE, property);
            if (!EventFilterDialog::eventInRange(getValue(), property))
                return false;

        } else if ((*e).isa(PitchBend::EventType) && filterWheel()) {
            // pitch bend amount
            (*e).get<Int>(PitchBend::MSB, property);
            unsigned char MSB = char(property);
            (*e).get<Int>(PitchBend::LSB, property);
            unsigned char LSB = char(property);

            property = LSB;
            property <<= 7;
            property += MSB;

            RG_DEBUG << "pitch bend was " << property << endl;

            if (!EventFilterDialog::eventInRange(getWheel(), property))
                return false;
        }
#endif
        return true;
    }
    return false;
}

}
#include "EventFilterDialog.moc"
