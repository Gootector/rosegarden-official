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

#ifndef _ROSEGARDENTEMPODIALOG_H_
#define _ROSEGARDENTEMPODIALOG_H_

#include "Event.h"
#include "rosegardentempo.h"
#include "multiviewcommandhistory.h"

class RosegardenGUIDoc;

// Modifies the tempo at a position in time
//
//

namespace Rosegarden
{

class RosegardenTempoDialog : public RosegardenTempo
{
Q_OBJECT
public:
    RosegardenTempoDialog(RosegardenGUIDoc *doc,
                          QWidget *parent=0,
                          const char *name=0);
    ~RosegardenTempoDialog();

    void resetFont(QWidget *w);
    void resetFonts();

    void showTempo();
    void showPosition();

    MultiViewCommandHistory* getCommandHistory();
    void addCommandToHistory(Command *command);

public slots:
    void slotOK();
    void slotCancel();

    void slotCommandExecuted(Command *command);


private:
    RosegardenGUIDoc   *m_doc;


};

}
 


#endif // _ROSEGARDENTEMPODIALOG_H_
