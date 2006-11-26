
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

#ifndef _RG_MANAGEMETRONOMEDIALOG_H_
#define _RG_MANAGEMETRONOMEDIALOG_H_

#include "base/MidiProgram.h"
#include <kdialogbase.h>


class QWidget;
class QSpinBox;
class QCheckBox;
class KComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;
class PitchChooser;
class InstrumentParameterBox;


class ManageMetronomeDialog : public KDialogBase
{
    Q_OBJECT

public:
    ManageMetronomeDialog(QWidget *parent, RosegardenGUIDoc *doc);

    void setModified(bool value);

public slots:
    void slotOk();
    void slotApply();
    void slotSetModified();
    void slotResolutionChanged(int);
    void slotPreviewPitch(int);
    void slotInstrumentChanged(int);
    void slotPitchSelectorChanged(int);
    void slotPitchChanged(int);
    void populate(int dev);

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc       *m_doc;

    KComboBox              *m_metronomeDevice;
    KComboBox              *m_metronomeInstrument;
    KComboBox              *m_metronomeResolution;
    KComboBox              *m_metronomePitchSelector;
    PitchChooser *m_metronomePitch;
    QSpinBox               *m_metronomeBarVely;
    QSpinBox               *m_metronomeBeatVely;
    QSpinBox               *m_metronomeSubBeatVely;
    InstrumentParameterBox *m_instrumentParameterBox;
    QCheckBox              *m_playEnabled;
    QCheckBox              *m_recordEnabled;
        
    bool                   m_modified;
    MidiByte   m_barPitch;
    MidiByte   m_beatPitch;
    MidiByte   m_subBeatPitch;
};


}

#endif
