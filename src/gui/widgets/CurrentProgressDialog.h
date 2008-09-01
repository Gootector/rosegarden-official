
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

#ifndef _RG_CURRENTPROGRESSDIALOG_H_
#define _RG_CURRENTPROGRESSDIALOG_H_

#include <QObject>




namespace Rosegarden
{

class ProgressDialog;


class CurrentProgressDialog : public QObject
{
    Q_OBJECT
public:
    static CurrentProgressDialog* getInstance();

    static ProgressDialog* get();
    static void set(ProgressDialog*);

    /**
     * Block the current progress so that it won't appear
     * regardless of passing time and occurring events.
     * This is useful when you want to show another dialog
     * and you want to make sure the progress dialog is out of the way
     */
    static void freeze();

    /**
     * Restores the progress dialog to its normal state atfer a freeze()
     */
    static void thaw();

public slots:
    /// Called then the current progress dialog is being destroyed
    void slotCurrentProgressDialogDestroyed();

protected:
    CurrentProgressDialog(QObject* parent, const char* name = 0)
        : QObject(parent, name) {}
    
    //--------------- Data members ---------------------------------
    static CurrentProgressDialog* m_instance;
    static ProgressDialog* m_currentProgressDialog;
};


// A Text popup - a tooltip we can control.
//

}

#endif
