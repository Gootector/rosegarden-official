// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _INSTRUMENTPARAMETERBOX_H_
#define _INSTRUMENTPARAMETERBOX_H_

#include <qgroupbox.h>

// Display and allow modification of Instrument parameters
//

namespace Rosegarden { class Instrument; }

class RosegardenComboBox;
class QCheckBox;

class InstrumentParameterBox : public QGroupBox
{
Q_OBJECT

public:
    InstrumentParameterBox(QWidget *parent=0, const char *name=0, WFlags f=0);
    ~InstrumentParameterBox();

    void useInstrument(Rosegarden::Instrument *instrument);

public slots:
    void slotSelectProgram(int index);
    void slotSelectPan(int index);
    void slotSelectVelocity(int index);
    void slotSelectBank(int index);

    void slotActivateProgramChange(bool value);
    void slotActivateVelocity(bool value);
    void slotActivatePan(bool value);
    void slotActivateBank(bool value);

protected:
    void populateProgramList();
    void initBox();

    //--------------- Data members ---------------------------------

    RosegardenComboBox *m_bankValue;
    RosegardenComboBox *m_channelValue;
    RosegardenComboBox *m_programValue;
    RosegardenComboBox *m_panValue;
    RosegardenComboBox *m_velocityValue;

    QCheckBox          *m_bankCheckBox;
    QCheckBox          *m_programCheckBox;
    QCheckBox          *m_panCheckBox;
    QCheckBox          *m_velocityCheckBox;

    Rosegarden::Instrument *m_selectedInstrument;

};


#endif // _INSTRUMENTPARAMETERBOX_H_
