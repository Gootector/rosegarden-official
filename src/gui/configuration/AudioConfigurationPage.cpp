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


#include "AudioConfigurationPage.h"

#include "sound/Midi.h"
#include "sound/SoundDriver.h"
#include "misc/ConfigGroups.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/ShowSequencerStatusDialog.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedEvent.h"
#include "TabbedConfigurationPage.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"

#include <QComboBox>
#include <QSettings>
#include <QFileDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QByteArray>
#include <QDataStream>
#include <QFrame>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QLayout>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QMessageBox>


namespace Rosegarden
{

AudioConfigurationPage::AudioConfigurationPage(
    RosegardenDocument *doc,
    QWidget *parent,
    const char *name):
    TabbedConfigurationPage(parent, name),
    m_externalAudioEditorPath(0)
{
    // set the document in the super class
    m_doc = doc;

    QSettings settings;
    //@@@ next line not needed. Commented out.
    //@@@ settings.beginGroup( SequencerOptionsConfigGroup );

    QFrame *frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    QLabel *label = 0;

    int row = 0;

    settings.beginGroup( GeneralOptionsConfigGroup );

    layout->setRowMinimumHeight(row, 15);
    ++row;

    layout->addWidget(new QLabel(tr("Audio preview scale"),
                                 frame), row, 0);

    m_previewStyle = new QComboBox(frame);
    connect(m_previewStyle, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_previewStyle->addItem(tr("Linear - easier to see loud peaks"));
    m_previewStyle->addItem(tr("Meter scaling - easier to see quiet activity"));
    m_previewStyle->setCurrentIndex( settings.value("audiopreviewstyle", 1).toUInt() );
    layout->addWidget(m_previewStyle, row, 1, row- row+1, 2);
    ++row;

    settings.endGroup();

#ifdef HAVE_LIBJACK
    settings.beginGroup( SequencerOptionsConfigGroup );

    label = new QLabel(tr("Record audio files as"), frame);
    m_audioRecFormat = new QComboBox(frame);
    connect(m_audioRecFormat, SIGNAL(activated(int)), this, SLOT(slotModified()));
    m_audioRecFormat->addItem(tr("16-bit PCM WAV format (smaller files)"));
    m_audioRecFormat->addItem(tr("32-bit float WAV format (higher quality)"));
    m_audioRecFormat->setCurrentIndex( settings.value("audiorecordfileformat", 1).toUInt() );
    layout->addWidget(label, row, 0);
    layout->addWidget(m_audioRecFormat, row, 1, row- row+1, 2);
    ++row;

    settings.endGroup();

#endif
    settings.beginGroup( GeneralOptionsConfigGroup );

    layout->addWidget(new QLabel(tr("External audio editor"), frame),
                      row, 0);

    QString defaultAudioEditor = getBestAvailableAudioEditor();

    std::cerr << "defaultAudioEditor = " << defaultAudioEditor << std::endl;

    QString externalAudioEditor = settings.value("externalaudioeditor",
                                  defaultAudioEditor).toString() ;

    if (externalAudioEditor == "") {
        externalAudioEditor = defaultAudioEditor;
        settings.setValue("externalaudioeditor", externalAudioEditor);
    }

    m_externalAudioEditorPath = new LineEdit(externalAudioEditor, frame);
    connect(m_externalAudioEditorPath, SIGNAL(textChanged(const QString &)), this, SLOT(slotModified()));
//    m_externalAudioEditorPath->setMinimumWidth(150);
    layout->addWidget(m_externalAudioEditorPath, row, 1);
    
    QPushButton *changePathButton =
        new QPushButton(tr("Choose..."), frame);

    layout->addWidget(changePathButton, row, 2);
    connect(changePathButton, SIGNAL(clicked()), SLOT(slotFileDialog()));
    ++row;

    settings.endGroup();

    layout->addWidget(new QLabel(tr("Create JACK outputs"), frame),
                      row, 0);
//    ++row;

#ifdef HAVE_LIBJACK
    settings.beginGroup( SequencerOptionsConfigGroup );

    m_createFaderOuts = new QCheckBox(tr("for individual audio instruments"), frame);
    connect(m_createFaderOuts, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_createFaderOuts->setChecked( qStrToBool( settings.value("audiofaderouts", "false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createFaderOuts, row, 1);
    ++row;

    m_createSubmasterOuts = new QCheckBox(tr("for submasters"), frame);
    connect(m_createSubmasterOuts, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_createSubmasterOuts->setChecked( qStrToBool( settings.value("audiosubmasterouts", "                                      false" ) ) );

//    layout->addWidget(label, row, 0, Qt::AlignRight);
    layout->addWidget(m_createSubmasterOuts, row, 1);
    ++row;

    settings.endGroup();
#endif

    layout->setRowStretch(row, 10);

    addTab(frame, tr("General"));

    // --------------------- Startup control ----------------------
    //
#ifdef HAVE_LIBJACK
#define OFFER_JACK_START_OPTION 1
#ifdef OFFER_JACK_START_OPTION

    frame = new QFrame(m_tabWidget);
    frame->setContentsMargins(10, 10, 10, 10);
    layout = new QGridLayout(frame);
    layout->setSpacing(5);

    row = 0;

    layout->setRowMinimumHeight(row, 15);
    ++row;

    label = new QLabel(tr("Rosegarden can start the JACK audio daemon (jackd) for you automatically if it isn't already running when Rosegarden starts.\n\nThis is recommended for beginners and those who use Rosegarden as their main audio application, but it might not be to the liking of advanced users.\n\nIf you want to start JACK automatically, make sure the command includes a full path where necessary as well as any command-line arguments you want to use.\n\nFor example: /usr/local/bin/jackd -d alsa -d hw -r44100 -p 2048 -n 2\n\n"), frame);
    label->setWordWrap(true);

    layout->addWidget(label, row, 0, row- row+1, 3- 0+1);
    ++row;

    settings.beginGroup( SequencerOptionsConfigGroup );

    // JACK control things
    //
    bool startJack = qStrToBool( settings.value("jackstart", "false" ) ) ;
    m_startJack = new QCheckBox(frame);
    connect(m_startJack, SIGNAL(stateChanged(int)), this, SLOT(slotModified()));
    m_startJack->setChecked(startJack);

    layout->addWidget(new QLabel(tr("Start JACK when Rosegarden starts"), frame), 2, 0);

    layout->addWidget(m_startJack, row, 1);
    ++row;

    layout->addWidget(new QLabel(tr("JACK command"), frame),
                      row, 0);

    QString jackPath = settings.value("jackcommand",
                                        // "/usr/local/bin/jackd -d alsa -d hw -r 44100 -p 2048 -n 2") ;
                                        "/usr/bin/qjackctl -s").toString();
    m_jackPath = new LineEdit(jackPath, frame);
    connect(m_jackPath, SIGNAL(textChanged(const QString &)), this, SLOT(slotModified()));

    layout->addWidget(m_jackPath, row, 1, row- row+1, 3);
    ++row;

    layout->setRowStretch(row, 10);

    addTab(frame, tr("JACK Startup"));

    settings.endGroup();

#endif // OFFER_JACK_START_OPTION
#endif // HAVE_LIBJACK

}

void
AudioConfigurationPage::slotFileDialog()
{
    QString path = QFileDialog::getOpenFileName(this, tr("External audio editor path"), QDir::currentPath() );

    m_externalAudioEditorPath->setText(path);
}

void
AudioConfigurationPage::apply()
{
    QSettings settings;
    settings.beginGroup( SequencerOptionsConfigGroup );

#ifdef HAVE_LIBJACK
#ifdef OFFER_JACK_START_OPTION
    // Jack control
    //
    settings.setValue("jackstart", m_startJack->isChecked());
    settings.setValue("jackcommand", m_jackPath->text());
#endif // OFFER_JACK_START_OPTION

    // Jack audio inputs
    //
    settings.setValue("audiofaderouts", m_createFaderOuts->isChecked());
    settings.setValue("audiosubmasterouts", m_createSubmasterOuts->isChecked());
    settings.setValue("audiorecordfileformat", m_audioRecFormat->currentIndex());
#endif

    settings.endGroup();
    settings.beginGroup( GeneralOptionsConfigGroup );

    int previewstyle = m_previewStyle->currentIndex();
    settings.setValue("audiopreviewstyle", previewstyle);

    QString externalAudioEditor = getExternalAudioEditor();

    QStringList extlist = externalAudioEditor.split(" ", QString::SkipEmptyParts);
    QString extpath = "";
    if (extlist.size() > 0) extpath = extlist[0];

    if (extpath != "") {
        QFileInfo info(extpath);
        if (!info.exists() || !info.isExecutable()) {
            QMessageBox::critical(0, "", tr("External audio editor \"%1\" not found or not executable").arg(extpath));
            settings.setValue("externalaudioeditor", "");
        } else {
            settings.setValue("externalaudioeditor", externalAudioEditor);
        }
    } else {
        settings.setValue("externalaudioeditor", "");
    }
    settings.endGroup();
}

QString
AudioConfigurationPage::getBestAvailableAudioEditor()
{
    static QString result = "";
    static bool haveResult = false;

    if (haveResult) return result;

    QString path;
    const char *cpath = getenv("PATH");
    if (cpath) path = cpath;
    else path = "/usr/bin:/bin";

    QStringList pathList = path.split(":", QString::SkipEmptyParts);

    const char *candidates[] = {
        "mhwaveedit",
        "rezound",
        "audacity"
    };

    for (unsigned int i = 0;
         i < sizeof(candidates)/sizeof(candidates[0]) && result == "";
         i++) {

        QString n(candidates[i]);

        for (int j = 0;
             j < pathList.size() && result == "";
             j++) {

            QDir dir(pathList[j]);
            QString fp(dir.filePath(n));
            QFileInfo fi(fp);

            if (fi.exists() && fi.isExecutable()) {
                if (n == "rezound") {
                    result = QString("%1 --audio-method=jack").arg(fp);
                } else {
                    result = fp;
                }
            }
        }
    }

    haveResult = true;
    return result;
}

}
#include "AudioConfigurationPage.moc"

