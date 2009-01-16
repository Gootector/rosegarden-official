
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

#ifndef _RG_GENERALCONFIGURATIONPAGE_H_
#define _RG_GENERALCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include "gui/editors/eventlist/EventView.h"
#include <QString>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>

#include <QObject>

class QWidget;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * General Rosegarden Configuration page
 *
 * (application-wide settings)
 */
class GeneralConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    enum DoubleClickClient
    {
        NotationView,
        MatrixView,
        EventView
    };

    enum NoteNameStyle
    { 
        American,
        Local
    };

    GeneralConfigurationPage(RosegardenGUIDoc *doc,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return QObject::tr("General"); }
    static QString title()     { return QObject::tr("General Configuration"); }
    static QString iconName()  { return "configure-general"; }

signals:
    void updateAutoSaveInterval(unsigned int);
    void updateSidebarStyle(unsigned int);

protected slots:
    void slotShowStatus();

protected:
    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentIndex(); }
    int getNoteNameStyle()          { return m_nameStyle->currentIndex(); }
    int getAppendLabel()            { return m_appendLabel->isChecked(); }
    
    //--------------- Data members ---------------------------------
    RosegardenGUIDoc* m_doc;

    QComboBox* m_client;
    QSpinBox*  m_countIn;
    QCheckBox* m_toolContextHelp;
    QCheckBox* m_backgroundTextures;
    QCheckBox* m_notationBackgroundTextures;
    QCheckBox* m_matrixBackgroundTextures;
    QComboBox *m_autoSave;
    QComboBox* m_nameStyle;
    QComboBox* m_sidebarStyle;
    QComboBox* m_globalStyle;
    QCheckBox* m_appendLabel;
    QCheckBox *m_jackTransport;

};




}

#endif
