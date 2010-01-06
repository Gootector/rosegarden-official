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

#include "WarningWidget.h"

#include "gui/general/IconLoader.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>

#include <iostream>
namespace Rosegarden
{

WarningWidget::WarningWidget(QWidget *parent) :
    QWidget(parent),
    m_text(""),
    m_informativeText(""),
    m_warningDialog(new WarningDialog(parent))
{
    setContentsMargins(0, 0, 0, 0);
    
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);
    layout->setMargin(0);
    layout->setSpacing(2);

    m_midiIcon = new QLabel();
    layout->addWidget(m_midiIcon);

    m_audioIcon = new QLabel();
    layout->addWidget(m_audioIcon);

    m_timerIcon = new QLabel();
    layout->addWidget(m_timerIcon);

    m_warningButton = new QToolButton();
    layout->addWidget(m_warningButton);
    m_warningButton->setIconSize(QSize(16, 16));
    m_warningButton->setIcon(IconLoader().loadPixmap("warning"));
    connect(m_warningButton,
            SIGNAL(clicked()),
            this,
            SLOT(displayMessageQueue()));
    m_warningButton->setToolTip(tr("<qt><p>Performance problems detected!</p><p>Click to display details</p></qt>"));
    m_warningButton->hide();

    m_graphicsButton = new QToolButton();
    layout->addWidget(m_graphicsButton);
    m_graphicsButton->setIconSize(QSize(16, 16));
    m_graphicsButton->setIcon(IconLoader().loadPixmap("safe-graphics"));
    connect(m_graphicsButton,
            SIGNAL(clicked()),
            this,
            SLOT(displayGraphicsAdvisory()));
    m_graphicsButton->hide();

    // Set these to false initially, assuming an all clear state.  When some
    // problem crops up, these will be set true as appropriate by
    // RosegardenMainWindow, which manages this widget
    setMidiWarning(false);
    setAudioWarning(false);
    setTimerWarning(false);
    setGraphicsAdvisory(false);
}

WarningWidget::~WarningWidget()
{
}

void
WarningWidget::setMidiWarning(const bool status)
{
    if (status) {
        m_midiIcon->hide();
    } else {
        m_midiIcon->setPixmap(IconLoader().loadPixmap("midi-ok"));
        m_midiIcon->show();
        m_midiIcon->setToolTip(tr("MIDI OK"));
    }
}

void
WarningWidget::setAudioWarning(const bool status)
{
    if (status) {
        m_audioIcon->hide();
    } else {
        m_audioIcon->setPixmap(IconLoader().loadPixmap("audio-ok"));
        m_audioIcon->show();
        m_audioIcon->setToolTip(tr("audio OK"));
    }
}

void
WarningWidget::setTimerWarning(const bool status)
{
    if (status) {
        m_timerIcon->hide();
    } else {
        m_timerIcon->setPixmap(IconLoader().loadPixmap("timer-ok"));
        m_timerIcon->show();
        m_timerIcon->setToolTip(tr("timer OK"));
    }
}

void
WarningWidget::setGraphicsAdvisory(const bool status)
{
    if (status) {
        m_graphicsButton->show();
        m_graphicsButton->setToolTip(tr("<qt>Safe graphics mode<br>Click for more information</qt>"));
    } else {
        m_graphicsButton->hide();
    }
}

void
WarningWidget::queueMessage(const QString text, const QString informativeText)
{
    RG_DEBUG << "WarningWidget::queueMessage(" << text
             << ", " << informativeText << ")" << endl;
    m_warningButton->show();

    Message message(text, informativeText);

    m_queue.enqueue(message);
}

void
WarningWidget::displayMessageQueue()
{
    while (!m_queue.isEmpty()) {
        std::cerr << " - emptying queue..." << std::endl;
        m_warningDialog->addWarning(m_queue.dequeue());
    }
    m_warningDialog->show();
}

void
WarningWidget::displayGraphicsAdvisory()
{
    QMessageBox::information(this, 
                             tr("Rosegarden"),
                             tr("<qt><p>Rosegarden is using safe graphics mode.  This provides the greatest stability, but graphics performance is very slow.</p><p>You may wish to visit <b>Edit -> Preferences -> Behavior -> Graphics performance</b> and try \"Normal\" or \"Fast\" for better performance.</p></qt>"));
}

// NOTES:
// Potential future warnings:
//
// * Chris's newly refactored autoconnect logic couldn't find a plausible looking
// thing to talk to, so you probably need to run QSynth now &c.
//
}
#include "WarningWidget.moc"
