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

#ifndef _ROSEGARDENTRANSPORTDIALOG_H_
#define _ROSEGARDENTRANSPORTDIALOG_H_

#include "rosegardentransport.h"
#include "Composition.h" // for RealTime
#include "MappedEvent.h"
#include <qtimer.h>
#include <map>
#include <qpixmap.h>

namespace Rosegarden
{

class RosegardenTransportDialog : public RosegardenTransport
{
Q_OBJECT
public:
    RosegardenTransportDialog(QWidget *parent=0,
                              const char *name=0);
    ~RosegardenTransportDialog();

    void displayTime(const Rosegarden::RealTime &rt);
    void displayBarTime(int bar, int beat, int unit);

    bool isShowingTimeToEnd();
    bool isShowingBarTime();

    void setTempo(const double &tempo);
    void setTimeSignature(const Rosegarden::TimeSignature &timeSig);

    // Called indirectly from the sequencer and from the GUI to
    // show incoming and outgoing MIDI events on the Transport
    //
    void setMidiInLabel(const Rosegarden::MappedEvent *mE);
    void setMidiOutLabel(const Rosegarden::MappedEvent *mE);

protected:
    virtual void closeEvent(QCloseEvent * e);

public slots:

    // These two slots are activated by QTimers
    //
    void slotClearMidiInLabel();
    void slotClearMidiOutLabel();

    // These just change the little labels that say what
    // mode we're in, nothing else
    //
    void slotChangeTimeDisplay();
    void slotChangeToEnd();

    void slotLoopButtonReleased();

    void slotPanelOpenButtonReleased();
    void slotPanelCloseButtonReleased();

signals:
    void closed();

    // Set and unset the loop at the RosegardenGUIApp
    //
    void setLoop();
    void unsetLoop();

private:
    void loadPixmaps();
    void resetFonts();
    void resetFont(QWidget *);

    //--------------- Data members ---------------------------------

    std::map<int, QPixmap> m_lcdList;
    QPixmap m_lcdNegative;

    int m_lastTenHours;
    int m_lastUnitHours;
    int m_lastTenMinutes;
    int m_lastUnitMinutes;
    int m_lastTenSeconds;
    int m_lastUnitSeconds;
    int m_lastTenths;
    int m_lastHundreths;
    int m_lastThousandths;
    int m_lastTenThousandths;

    bool m_lastNegative;
    bool m_lastBarTime;

    int m_tenHours;
    int m_unitHours;
    int m_tenMinutes;
    int m_unitMinutes;
    int m_tenSeconds;
    int m_unitSeconds;
    int m_tenths;
    int m_hundreths;
    int m_thousandths;
    int m_tenThousandths;

    double m_tempo;
    int m_numerator;
    int m_denominator;

    QTimer *m_midiInTimer;
    QTimer *m_midiOutTimer;

    QPixmap m_panelOpen;
    QPixmap m_panelClosed;

    void updateTimeDisplay();
};

}
 


#endif // _ROSEGARDENTRANSPORTDIALOG_H_
