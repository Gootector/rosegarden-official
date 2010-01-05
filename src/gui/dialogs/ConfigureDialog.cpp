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


#include "ConfigureDialog.h"
#include "ConfigureDialogBase.h"

#include "document/RosegardenDocument.h"
#include "gui/configuration/ConfigurationPage.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/configuration/NotationConfigurationPage.h"
#include "gui/configuration/AudioConfigurationPage.h"
#include "gui/configuration/MIDIConfigurationPage.h"
#include "gui/general/IconLoader.h"

#include <QLayout>
#include <QSettings>
#include <QString>
#include <QWidget>
#include <QTabWidget>

#include <QDir>


namespace Rosegarden
{



ConfigureDialog::ConfigureDialog(RosegardenDocument *doc,
                                 QWidget *parent,
                                 const char *name)
    : ConfigureDialogBase(parent, tr("Rosegarden - Preferences"), name )
{
    
    QWidget* page = 0;
    
    // General Page
    //
    IconLoader il;
    
    
    page = new GeneralConfigurationPage(doc, this);
    connect(page,SIGNAL(modified()),this,SLOT(slotActivateApply()));
    addPage(GeneralConfigurationPage::iconLabel(),GeneralConfigurationPage::title(),il.loadPixmap(GeneralConfigurationPage::iconName()),page);    
    m_configurationPages.push_back((ConfigurationPage*)page);

    connect(page, SIGNAL(updateAutoSaveInterval(unsigned int)),
            this, SIGNAL(updateAutoSaveInterval(unsigned int)));
    connect(page, SIGNAL(updateSidebarStyle(unsigned int)),
            this, SIGNAL(updateSidebarStyle(unsigned int)));

    page = new MIDIConfigurationPage(doc, this);
    connect(page,SIGNAL(modified()),this,SLOT(slotActivateApply()));
    addPage(MIDIConfigurationPage::iconLabel(),MIDIConfigurationPage::title(),il.loadPixmap( MIDIConfigurationPage::iconName()),page);
    m_configurationPages.push_back((ConfigurationPage*)page);

    page = new AudioConfigurationPage(doc, this);
    connect(page,SIGNAL(modified()),this,SLOT(slotActivateApply()));
    addPage(AudioConfigurationPage::iconLabel(),AudioConfigurationPage::title(),il.loadPixmap(AudioConfigurationPage::iconName()),page);
    m_configurationPages.push_back((ConfigurationPage*)page);

    // Notation Page
    page = new NotationConfigurationPage(this);
    connect(page,SIGNAL(modified()),this,SLOT(slotActivateApply()));
    addPage(NotationConfigurationPage::iconLabel(),NotationConfigurationPage::title(),il.loadPixmap(NotationConfigurationPage::iconName()),page);
    m_configurationPages.push_back((ConfigurationPage*)page);
}

}
#include "ConfigureDialog.moc"
