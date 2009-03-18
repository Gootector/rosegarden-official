/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TRACKPARAMETERBOX_H_
#define _RG_TRACKPARAMETERBOX_H_

#include "base/MidiProgram.h"
#include "base/Track.h"
#include "gui/widgets/ColourTable.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include <map>
#include <vector>
#include "Composition.h"

#include <QString>
#include <QCheckBox> // #include <QCheckBox> in QT4, thinking ahead



class QWidget;
class QPushButton;
class QLabel;
class QFrame;
class QComboBox;
class QCheckBox;
//class SqueezedLabel;

namespace Rosegarden
{

class RosegardenGUIDoc;


class TrackParameterBox : public RosegardenParameterBox,
                          public CompositionObserver
{
Q_OBJECT
        
public:
    TrackParameterBox( RosegardenGUIDoc *doc,
                       QWidget *parent=0);
    ~TrackParameterBox();
    
    void setDocument( RosegardenGUIDoc *doc );
    void populateDeviceLists();
    virtual void showAdditionalControls(bool showThem);

    virtual QString getPreviousBox(RosegardenParameterArea::Arrangement) const;

    // CompositionObserver interface
    //
    virtual void trackDeleted(const Composition *, TrackId);

public slots:
    void slotSelectedTrackChanged();
    void slotSelectedTrackNameChanged();
    void slotPlaybackDeviceChanged(int index);
    void slotInstrumentChanged(int index);
    void slotRecordingDeviceChanged(int index);
    void slotRecordingChannelChanged(int index);
    void slotUpdateControls(int);
    void slotInstrumentLabelChanged(InstrumentId id, QString label);

    void slotClefChanged(int clef);
    void slotTransposeChanged(int transpose);
    void slotTransposeIndexChanged(int index);
    void slotTransposeTextChanged(QString text);
    void slotDocColoursChanged();
    void slotColorChanged(int index);
    void slotHighestPressed();
    void slotLowestPressed();
    void slotPresetPressed();

    void slotStaffSizeChanged(int index);
    void slotStaffBracketChanged(int index);

signals:
    void instrumentSelected(TrackId, int);

protected:
    void populatePlaybackDeviceList();
    void populateRecordingDeviceList();
    void updateHighLow();

private:
    RosegardenGUIDoc    *m_doc;

    QComboBox           *m_playDevice;
    QComboBox           *m_instrument;
    QComboBox           *m_recDevice;
    QComboBox           *m_recChannel;

    QPushButton         *m_presetButton;
    QPushButton         *m_highButton;
    QPushButton         *m_lowButton;

    QComboBox           *m_defClef;
    QComboBox           *m_defColor;
    QComboBox           *m_defTranspose;
    QComboBox           *m_staffSizeCombo;
    QComboBox           *m_staffBracketCombo;


    int                 m_addColourPos;
    int                 m_highestPlayable;
    int                 m_lowestPlayable;
    ColourTable::ColourList  m_colourList;
    
    QLabel              *m_trackLabel;
    
    typedef std::vector<DeviceId> IdsVector;
    
    IdsVector           m_playDeviceIds;    
    IdsVector           m_recDeviceIds;

    std::map<DeviceId, IdsVector>   m_instrumentIds;
    std::map<DeviceId, QStringList> m_instrumentNames;
    
    int                 m_selectedTrackId;
    
    char                m_lastInstrumentType;

    // Additional elements that may be hidden in vertical stacked mode
    //QFrame              *m_separator2;
    QFrame              *m_playbackGroup;
    QFrame              *m_recordGroup;
    QFrame              *m_defaultsGroup;
    QFrame              *m_staffGroup;
    QLabel              *m_segHeader;
    QLabel              *m_presetLbl;
    QLabel              *m_staffGrpLbl;
    QLabel              *m_grandStaffLbl;
    QLabel              *m_clefLbl;
    QLabel              *m_transpLbl;
    QLabel              *m_colorLbl;
    QLabel              *m_rangeLbl;
    QLabel              *m_psetLbl;
};


}

#endif
