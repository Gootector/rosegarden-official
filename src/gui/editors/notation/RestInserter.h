/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifdef NOT_JUST_NOW //!!!

#ifndef _RG_RESTINSERTER_H_
#define _RG_RESTINSERTER_H_

#include "NoteInserter.h"
#include <QString>
#include "base/Event.h"




namespace Rosegarden
{

class Segment;
class Note;
class NotationView;
class Event;


/**
 * This tool will insert rests on mouse click events
 */
class RestInserter : public NoteInserter
{
    Q_OBJECT
    
    friend class NotationToolBox;

public:

    static const QString ToolName;

public slots:
    /// Set the nb of dots the inserted rest will have
    void slotSetDots(unsigned int dots);

protected:
    RestInserter(NotationView*);

    virtual Event *doAddCommand(Segment &,
                                            timeT time,
                                            timeT endTime,
                                            const Note &,
                                            int pitch, Accidental);
    virtual void showPreview();

protected slots:
    void slotToggleDot();
    void slotNotesSelected();
};


}

#endif
#endif
