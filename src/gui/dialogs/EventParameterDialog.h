
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_EVENTPARAMETERDIALOG_H_
#define _RG_EVENTPARAMETERDIALOG_H_

#include "base/PropertyName.h"
#include <QDialog>
#include "commands/edit/SelectionPropertyCommand.h"

class QWidget;
class QString;
class QLabel;
class QComboBox;


namespace Rosegarden
{



class EventParameterDialog : public QDialog
{
    Q_OBJECT

public:
    EventParameterDialog(QWidget *parent,
                         const QString &name,                      // name
                         const PropertyName &property, // property
                         int startValue);                          // start

    int getValue1();
    int getValue2();
    PropertyPattern getPattern();

public slots:
    void slotPatternSelected(int value);

protected:
    //--------------- Data members ---------------------------------
    PropertyName    m_property;
    PropertyPattern m_pattern;

    QComboBox         *m_value1Combo;
    QComboBox         *m_value2Combo;
    QComboBox         *m_patternCombo;

    QLabel                     *m_value1Label;
    QLabel                     *m_value2Label;

};


// ---------------- CompositionLengthDialog -----------

}

#endif
