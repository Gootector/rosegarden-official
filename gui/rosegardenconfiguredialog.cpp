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

/*
 * Code borrowed from KDE KMail : configuredialog*.h
 * Copyright (C) 2000 The KMail Development Team
 */

#include <qspinbox.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <kfiledialog.h>
#include <qpushbutton.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kiconloader.h>

#include "rosestrings.h"
#include "rosegardenconfiguredialog.h"
#include "rosegardenconfigurationpage.h"
#include "rosegardenguidoc.h"
#include "Composition.h"
#include "Configuration.h"
#include "RealTime.h"
#include "rosedebug.h"

namespace Rosegarden
{

TabbedConfigurationPage::TabbedConfigurationPage(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(doc, parent, name)
{
    init();  
}

TabbedConfigurationPage::TabbedConfigurationPage(KConfig *cfg,
                                                 QWidget *parent,
                                                 const char *name)
  : ConfigurationPage(cfg, parent, name)
{
    init();
}

void TabbedConfigurationPage::init()
{
    QVBoxLayout *vlay = new QVBoxLayout(this, 0, KDialog::spacingHint());
    m_tabWidget = new QTabWidget(this);
    vlay->addWidget(m_tabWidget);
}


void TabbedConfigurationPage::addTab(QWidget *tab, const QString &title)
{
  m_tabWidget->addTab(tab, title);
}

//------------------------------------------------------------

GeneralConfigurationPage::GeneralConfigurationPage(KConfig *cfg,
                                                   QWidget *parent, const char *name)
    : TabbedConfigurationPage(cfg, parent, name),
      m_client(0),
      m_countIn(0)
{
//     Rosegarden::Composition &comp = doc->getComposition();
//     Rosegarden::Configuration &config = doc->getConfiguration();
    m_cfg->setGroup("General Options");

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame,
                                          2, 2, // nbrow, nbcol
                                          10, 5);

    layout->addWidget(new QLabel(i18n("Double click on segment opens..."),
                                 frame), 0, 0);
    layout->addWidget(new QLabel(i18n("Number of count-in bars when recording"),
                                 frame), 1, 0);
    layout->addWidget(new QLabel(i18n("MIDI pitch to string offset"),
                                 frame), 2, 0);


    m_client = new QComboBox(frame);
    m_client->insertItem(i18n("Notation"));
    m_client->insertItem(i18n("Matrix"));
    m_client->setCurrentItem(m_cfg->readUnsignedNumEntry("doubleclickclient", NotationView));
    
    layout->addWidget(m_client, 0, 1);

    m_countIn = new QSpinBox(frame);
    m_countIn->setValue(m_cfg->readUnsignedNumEntry("countinbars", 2));
    m_countIn->setMaxValue(10);
    m_countIn->setMinValue(0);

    layout->addWidget(m_countIn, 1, 1);

    m_midiPitchOffset = new QSpinBox(frame);
    m_midiPitchOffset->setValue(m_cfg->readUnsignedNumEntry("midipitchoffset", 4));
    m_midiPitchOffset->setMaxValue(10);
    m_midiPitchOffset->setMinValue(0);

    layout->addWidget(m_midiPitchOffset, 2, 1);

    addTab(frame, i18n("General"));
}

void GeneralConfigurationPage::apply()
{
//     Rosegarden::Composition &comp = m_doc->getComposition();
//     Rosegarden::Configuration &config = m_doc->getConfiguration();
//     comp.setCountInBars(countIn);
//     config.setDoubleClickClient(
//                                 (Rosegarden::Configuration::DoubleClickClient)client);
    m_cfg->setGroup("General Options");

    int countIn = getCountInSpin();
    m_cfg->writeEntry("countinbars", countIn);

    int client = getDblClickClient();
    m_cfg->writeEntry("doubleclickclient", client);

    int offset = getMIDIPitch2StringOffset();
    m_cfg->writeEntry("midipitchoffset", offset);


}


NotationConfigurationPage::NotationConfigurationPage(KConfig *cfg,
                                                     QWidget *parent,
                                                     const char *name) :
    TabbedConfigurationPage(cfg, parent, name)
{
    m_cfg->setGroup("Notation Options");
}

void
NotationConfigurationPage::apply()
{
    m_cfg->setGroup("Notation Options");

}



PlaybackConfigurationPage::PlaybackConfigurationPage(KConfig *cfg,
                                                     QWidget *parent,
                                                     const char *name)
    : TabbedConfigurationPage(cfg, parent, name),
      m_readAhead(0),
      m_playback(0)
{
//     Rosegarden::Configuration &config = doc->getConfiguration();
    m_cfg->setGroup("Playback Options");

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 3, 4,
                                          10, 5);

    layout->addMultiCellWidget(new QLabel(i18n("Higher latency improves playback quality on slower systems\nbut reduces overall sequencer response."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(i18n("Read ahead (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(i18n("Playback (in ms)"), frame), 2, 0);

    m_readAhead = new QSlider(Horizontal, frame);

    m_readAhead->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 1, 1);

    m_readAhead->setMaxValue(120);
    layout->addWidget(new QLabel("120", frame), 1, 3);

    m_readAhead->setValue(m_cfg->readLongNumEntry
			  ("readaheadusec", 40000) / 1000);
    m_readAhead->setTickmarks(QSlider::Below);
    layout->addWidget(m_readAhead, 1, 2);

    m_playback = new QSlider(Horizontal, frame);

    m_playback->setMinValue(20);
    layout->addWidget(new QLabel("20", frame), 2, 1);

    m_playback->setMaxValue(500);
    layout->addWidget(new QLabel("500", frame), 2, 3);

    m_playback->setValue(m_cfg->readLongNumEntry
			 ("playbacklatencyusec", 100000) / 1000);
    m_playback->setTickmarks(QSlider::Below);
    layout->addWidget(m_playback, 2, 2);

    addTab(frame, i18n("Latency"));
}

void PlaybackConfigurationPage::apply()
{
    m_cfg->setGroup("Playback Options");

//     Rosegarden::Configuration &config = m_doc->getConfiguration();
//     config.setReadAhead((RealTime(0, (readAhead * 1000))));
//     config.setPlaybackLatency((RealTime(0, (playback * 1000))));

    int readAhead = getReadAheadValue();
    m_cfg->writeEntry("readaheadusec", readAhead * 1000);
    m_cfg->writeEntry("readaheadsec", 0L);
    
    int playback = getPlaybackValue();
    m_cfg->writeEntry("playbacklatencyusec", playback * 1000);
    m_cfg->writeEntry("playbacklatencysec", 0L);
}


AudioConfigurationPage::AudioConfigurationPage(RosegardenGUIDoc *doc,
                                               QWidget *parent,
                                               const char *name)
    : TabbedConfigurationPage(doc, parent, name),
      m_path(0),
      m_changePathButton(0)
{
    Rosegarden::AudioFileManager &afm = doc->getAudioFileManager();

    QFrame *frame = new QFrame(m_tabWidget);
    QGridLayout *layout = new QGridLayout(frame, 1, 3,
                                          10, 5);
    layout->addWidget(new QLabel(i18n("Audio file path:"), frame), 0, 0);
    m_path = new QLineEdit(QString(afm.getAudioPath().c_str()), frame);
    layout->addWidget(m_path, 0, 1);
    
    m_changePathButton =
        new QPushButton(i18n("Choose..."), frame);

    layout->addWidget(m_changePathButton, 0, 2);

    connect(m_changePathButton, SIGNAL(released()),
            SLOT(slotFileDialog()));

    addTab(frame, i18n("Modify audio path"));
}

void
AudioConfigurationPage::slotFileDialog()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();

    KFileDialog *fileDialog = new KFileDialog(QString(afm.getAudioPath().c_str()),
                                              QString::null,
                                              this, "file dialog", true);
    fileDialog->setMode(KFile::Directory);

    connect(fileDialog, SIGNAL(fileSelected(const QString&)),
            SLOT(slotFileSelected(const QString&)));

    connect(fileDialog, SIGNAL(destroyed()),
            SLOT(slotDirectoryDialogClosed()));

    if (fileDialog->exec() == QDialog::Accepted)
    {
        m_path->setText(fileDialog->selectedFile());
    }
    delete fileDialog;
}

void
AudioConfigurationPage::apply()
{
    Rosegarden::AudioFileManager &afm = m_doc->getAudioFileManager();
    QString newDir = m_path->text();
    
    if (!newDir.isNull())
    {
        afm.setAudioPath(std::string(newDir.latin1()));
    }
}


//------------------------------------------------------------

static inline QPixmap loadIcon(const char *name)
{
  return KGlobal::instance()->iconLoader()
    ->loadIcon(QString::fromLatin1(name), KIcon::NoGroup, KIcon::SizeMedium);
}

ConfigureDialogBase::ConfigureDialogBase(QWidget *parent,
                                         const char *name):
    KDialogBase(IconList, i18n("Configure"), Help|Apply|Ok|Cancel,
                Ok, parent, name, true) // modal
{
}

ConfigureDialogBase::~ConfigureDialogBase()
{
}

void
ConfigureDialogBase::slotApply()
{
    for(configurationpages::iterator i = m_configurationPages.begin();
        i != m_configurationPages.end(); ++i)
        (*i)->apply();
}


void
ConfigureDialogBase::slotActivateApply()
{
//     ApplyButton->setDisabled(false);
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
}

ConfigureDialog::ConfigureDialog(KConfig* cfg,
                                 QWidget *parent,
                                 const char *name)
    : ConfigureDialogBase(parent, name)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // General Page
    //
    pageWidget = addPage(GeneralConfigurationPage::iconLabel(),
                         GeneralConfigurationPage::title(),
                         loadIcon(GeneralConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new GeneralConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Notation Page
    pageWidget = addPage(NotationConfigurationPage::iconLabel(),
			 NotationConfigurationPage::title(),
			 loadIcon(NotationConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new NotationConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);

    // Playback Page
    //
    pageWidget = addPage(PlaybackConfigurationPage::iconLabel(),
                         PlaybackConfigurationPage::title(),
                         loadIcon(PlaybackConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new PlaybackConfigurationPage(cfg, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
}

//------------------------------------------------------------

DocumentConfigureDialog::DocumentConfigureDialog(RosegardenGUIDoc *doc,
                                                 QWidget *parent,
                                                 const char *name)
    : ConfigureDialogBase(parent, name),
      m_doc(doc)
{
    QWidget *pageWidget = 0;
    QVBoxLayout *vlay = 0;
    ConfigurationPage* page = 0;

    // Audio Page
    //
    pageWidget = addPage(AudioConfigurationPage::iconLabel(),
                         AudioConfigurationPage::title(),
                         loadIcon(AudioConfigurationPage::iconName()));
    vlay = new QVBoxLayout(pageWidget, 0, spacingHint());
    page = new AudioConfigurationPage(doc, pageWidget);
    vlay->addWidget(page);
    page->setPageIndex(pageIndex(pageWidget));
    m_configurationPages.push_back(page);
}

}
