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


#ifndef _TRACKBUTTONS_H_
#define _TRACKBUTTONS_H_

#include <vector>

#include <qframe.h>
#include <qpopupmenu.h>

class QVBoxLayout;
class QButtonGroup;
class TrackVUMeter;
class TrackLabel;
class RosegardenGUIDoc;
class InstrumentLabel;

// This class creates a list of mute and record buttons
// based on the rosegarden document and a specialisation
// of the Vertical Box widget.
//
//
// 

class TrackButtons : public QFrame
{
    Q_OBJECT
public:

    typedef enum
    {
        ShowTrack,
        ShowInstrument,
        ShowBoth
    } InstrumentTrackLabels;

    TrackButtons(RosegardenGUIDoc* doc,
                 unsigned int trackCellHeight,
                 unsigned int trackLabelWidth,
                 bool showTrackLabels,
                 QWidget* parent = 0,
                 const char* name = 0,
                 WFlags f=0);

    ~TrackButtons();

    // Return the track selected for recording
    //
    int selectedRecordTrack();

    // Return a vector of muted tracks
    //
    std::vector<int> mutedTracks();

    // Return a vector of highlighted tracks
    //
    std::vector<int> getHighlightedTracks();

    void changeTrackInstrumentLabels(InstrumentTrackLabels label);

signals:
    // to emit what Track has been selected
    //
    void trackSelected(int);
    void widthChanged();

public slots:

    // We use these to set the Track and Composition
    // objects with our returned values
    //
    void slotSetRecordTrack(int id);
    void slotToggleMutedTrack(int mutedTrack);
    void slotLabelSelected(int id);
    void slotRenameTrack(QString newName, int trackNumber);
    void slotSetTrackMeter(double value, int position);

    void slotInstrumentSelection(int);
    void slotInstrumentPopupActivated(int);
    void slotInstrumentPopupHiding();

private:

    void drawButtons();

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc *m_doc;

    QButtonGroup *m_recordButtonGroup;
    QButtonGroup *m_muteButtonGroup;
    QVBoxLayout *m_layout;

    std::vector<TrackLabel *> m_trackLabels;
    std::vector<TrackVUMeter *> m_trackMeters;
    std::vector<InstrumentLabel *> m_instrumentLabels;

    QPopupMenu *m_instrumentPopup;

    // Number of tracks on our view
    //
    int m_tracks;

    // The pixel offset from the top - just to overcome
    // the borders
    int m_offset;

    // The height of the cells
    //
    int m_cellSize;
    int m_lastID;
    int m_trackLabelWidth;
    int m_popupItem;

    InstrumentTrackLabels m_trackInstrumentLabels;

};


#endif // _TRACKBUTTONS_H_


