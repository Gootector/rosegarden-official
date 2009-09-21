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


#include "LatencyConfigurationPage.h"
#include <QLayout>

#include "misc/ConfigGroups.h"
#include "ConfigurationPage.h"
#include "document/RosegardenDocument.h"
#include "TabbedConfigurationPage.h"
#include <QSettings>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QTabWidget>
#include <QWidget>


namespace Rosegarden
{

LatencyConfigurationPage::LatencyConfigurationPage(RosegardenDocument *doc,
        QWidget *parent)
        : TabbedConfigurationPage(doc, parent)
{
#ifdef NOT_DEFINED
#ifdef HAVE_LIBJACK
    QSettings settings;
    settings.beginGroup( LatencyOptionsConfigGroup );

    frame = new QFrame(m_tabWidget, tr("JACK latency"));
    layout = new QGridLayout(frame, 6, 5, 10, 10);

    layout->addWidget(new QLabel(tr("Use the \"Fetch JACK latencies\" button to discover the latency values set at\nthe sequencer.  It's recommended that you use the returned values but it's also\npossible to override them manually using the sliders.  Note that if you change\nyour JACK server parameters you should always fetch the latency values again.\nThe latency values will be stored by Rosegarden for use next time."), frame),
                               0, 0,
                               0, 3);

    layout->addWidget(new QLabel(tr("JACK playback latency (in ms)"), frame), 1, 0);
    layout->addWidget(new QLabel(tr("JACK record latency (in ms)"), frame), 3, 0);

    m_fetchLatencyValues = new QPushButton(tr("Fetch JACK latencies"),
                                           frame);

    layout->addWidget(m_fetchLatencyValues, 1, 3);

    connect(m_fetchLatencyValues, SIGNAL(released()),
            SLOT(slotFetchLatencyValues()));

    int jackPlaybackValue = (settings.value(
                                 "jackplaybacklatencyusec", 0) / 1000).toInt() +
                            (settings.value(
                                 "jackplaybacklatencysec", 0) * 1000).toInt();

    m_jackPlayback = new QSlider(Horizontal, frame);
    m_jackPlayback->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_jackPlayback, 3, 2, 1, 3- 2+1);

    QLabel *jackPlaybackLabel = new QLabel(QString("%1").arg(jackPlaybackValue),
                                           frame);
    layout->addWidget(jackPlaybackLabel, 2, 2, Qt::AlignHCenter);
    connect(m_jackPlayback, SIGNAL(valueChanged(int)),
            jackPlaybackLabel, SLOT(setNum(int)));

    m_jackPlayback->setMinimum(0);
    layout->addWidget(new QLabel("0", frame), 3, 1, Qt::AlignRight);

    m_jackPlayback->setMaximum(500);
    layout->addWidget(new QLabel("500", frame), 3, 4, Qt::AlignLeft);

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = (settings.value(
                               "jackrecordlatencyusec", 0) / 1000).toInt() +
                          (settings.value(
                               "jackrecordlatencysec", 0) * 1000).toInt();

    m_jackRecord = new QSlider(Horizontal, frame);
    m_jackRecord->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_jackRecord, 5, 2, 1, 3- 2+1);

    QLabel *jackRecordLabel = new QLabel(QString("%1").arg(jackRecordValue),
                                         frame);
    layout->addWidget(jackRecordLabel, 4, 2, Qt::AlignHCenter);
    connect(m_jackRecord, SIGNAL(valueChanged(int)),
            jackRecordLabel, SLOT(setNum(int)));

    m_jackRecord->setMinimum(0);
    layout->addWidget(new QLabel("0", frame), 5, 1, Qt::AlignRight);

    m_jackRecord->setMaximum(500);
    m_jackRecord->setValue(jackRecordValue);
    layout->addWidget(new QLabel("500", frame), 5, 4, Qt::AlignLeft);

    addTab(frame, tr("JACK Latency"));

    settings.endGroup();
#endif  // HAVE_LIBJACK
#endif // NOT_DEFINED
}

void LatencyConfigurationPage::apply()
{
#ifdef HAVE_LIBJACK
    QSettings settings;
    settings.beginGroup( LatencyOptionsConfigGroup );

    int jackPlayback = getJACKPlaybackValue();
    settings.setValue("jackplaybacklatencysec", jackPlayback / 1000);
    settings.setValue("jackplaybacklatencyusec", jackPlayback * 1000);

    int jackRecord = getJACKRecordValue();
    settings.setValue("jackrecordlatencysec", jackRecord / 1000);
    settings.setValue("jackrecordlatencyusec", jackRecord * 1000);

    settings.endGroup();
#endif  // HAVE_LIBJACK
}

void LatencyConfigurationPage::slotFetchLatencyValues()
{
    int jackPlaybackValue = m_doc->getAudioPlayLatency().msec() +
                            m_doc->getAudioPlayLatency().sec * 1000;

    m_jackPlayback->setValue(jackPlaybackValue);

    int jackRecordValue = m_doc->getAudioRecordLatency().msec() +
                          m_doc->getAudioRecordLatency().sec * 1000;
    m_jackRecord->setValue(jackRecordValue);
}

}
#include "LatencyConfigurationPage.moc"
