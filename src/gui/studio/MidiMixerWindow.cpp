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


#include "MidiMixerWindow.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ActionFileClient.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/dialogs/AboutDialog.h"
#include "MidiMixerVUMeter.h"
#include "MixerWindow.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include "StudioControl.h"

#include <QAction>
#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>
#include <QShortcut>
#include <QDesktopServices>
#include <QToolButton>
#include <QToolBar>


namespace Rosegarden
{

MidiMixerWindow::MidiMixerWindow(QWidget *parent,
                                 RosegardenDocument *document):
    MixerWindow(parent, document),
    m_tabFrame(0)
{
    // Initial setup
    //
    setupTabs();

    createAction("file_close", SLOT(slotClose()));

    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("record", SIGNAL(record()));
    createAction("panic", SIGNAL(panic()));
    createAction("midimix_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createGUI("midimixer.rc");
    setRewFFwdToAutoRepeat();
}

void
MidiMixerWindow::setupTabs()
{
    DeviceListConstIterator it;
    MidiDevice *dev = 0;
    InstrumentList instruments;
    InstrumentList::const_iterator iIt;
    int faderCount = 0, deviceCount = 1;

    if (m_tabFrame)
        delete m_tabFrame;

    // Setup m_tabFrame
    //

    QWidget *blackWidget = new QWidget(this);
    setCentralWidget(blackWidget);
    QVBoxLayout *centralLayout = new QVBoxLayout;
    blackWidget->setLayout(centralLayout);

    m_tabWidget = new QTabWidget;
    centralLayout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(slotCurrentTabChanged(QWidget *)));
    m_tabWidget->setTabPosition(QTabWidget::Bottom);
    setWindowTitle(tr("MIDI Mixer"));
    setIcon(IconLoader().loadPixmap("window-midimixer"));


    for (it = m_studio->begin(); it != m_studio->end(); ++it) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev) {
            // Get the control parameters that are on the IPB (and hence can
            // be shown here too).
            //
            ControlList controls = getIPBForMidiMixer(dev);

            instruments = dev->getPresentationInstruments();

            // Don't add a frame for empty devices
            //
            if (!instruments.size())
                continue;

            m_tabFrame = new QFrame(m_tabWidget);
            m_tabFrame->setContentsMargins(10, 10, 10, 10);

            // m_tabFrame->setContentsMargins(5, 5, 5, 5); ???
            QGridLayout *mainLayout = new QGridLayout(m_tabFrame);

            // MIDI Mixer label
            QLabel *label = new QLabel("", m_tabFrame);
            mainLayout->addWidget(label, 0, 0, 0, 16, Qt::AlignCenter);

            // control labels
            for (size_t i = 0; i < controls.size(); ++i) {
                label = new QLabel(QObject::tr(
                            QString::fromStdString(
                                controls[i].getName())), m_tabFrame);
                mainLayout->addWidget(label, i + 1, 0, Qt::AlignCenter);
            }

            // meter label
            // (obsolete abandoned code deleted here)

            // volume label
            label = new QLabel(tr("Volume"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 2, 0,
                                  Qt::AlignCenter);

            // instrument label
            label = new QLabel(tr("Instrument"), m_tabFrame);
            label->setFixedWidth(80); //!!! this should come from metrics
            mainLayout->addWidget(label, controls.size() + 3, 0,
                                  Qt::AlignLeft);

            int posCount = 1;
            int firstInstrument = -1;

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt) {

                // Add new fader struct
                //
                m_faders.push_back(new FaderStruct());

                // Store the first ID
                //
                if (firstInstrument == -1)
                    firstInstrument = (*iIt)->getId();


                // Add the controls
                //
                for (size_t i = 0; i < controls.size(); ++i) {
                    QColor knobColour = QColor(Qt::white);

                    if (controls[i].getColourIndex() > 0) {
                        Colour c =
                            m_document->getComposition().getGeneralColourMap().
                            getColourByIndex(controls[i].getColourIndex());

                        knobColour = QColor(c.getRed(),
                                            c.getGreen(), c.getBlue());
                    }

                    Rotary *controller =
                        new Rotary(m_tabFrame,
                                   controls[i].getMin(),
                                   controls[i].getMax(),
                                   1.0,
                                   5.0,
                                   controls[i].getDefault(),
                                   20,
                                   Rotary::NoTicks,
                                   false,
                                   controls[i].getDefault() == 64); //!!! hacky

                    controller->setKnobColour(knobColour);

                    connect(controller, SIGNAL(valueChanged(float)),
                            this, SLOT(slotControllerChanged(float)));

                    mainLayout->addWidget(controller, i + 1, posCount,
                                          Qt::AlignCenter);

                    // Store the rotary
                    //
                    m_faders[faderCount]->m_controllerRotaries.push_back(
                        std::pair<MidiByte, Rotary*>
                        (controls[i].getControllerValue(), controller));
                }

                // VU meter
                //
                MidiMixerVUMeter *meter =
                    new MidiMixerVUMeter(m_tabFrame,
                                         VUMeter::FixedHeightVisiblePeakHold, 6, 30);
                mainLayout->addWidget(meter, controls.size() + 1,
                                      posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_vuMeter = meter;

                // Volume fader
                //
                Fader *fader =
                    new Fader(0, 127, 100, 20, 80, m_tabFrame);
                mainLayout->addWidget(fader, controls.size() + 2,
                                      posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_volumeFader = fader;

                // Label
                //
                QLabel *idLabel = new QLabel(QString("%1").
                                             arg((*iIt)->getId() - firstInstrument + 1),
                                             m_tabFrame);
                idLabel->setObjectName("idLabel");

                mainLayout->addWidget(idLabel, controls.size() + 3,
                                      posCount, Qt::AlignCenter);

                // store id in struct
                m_faders[faderCount]->m_id = (*iIt)->getId();

                // Connect them up
                //
                connect(fader, SIGNAL(faderChanged(float)),
                        this, SLOT(slotFaderLevelChanged(float)));

                // Update all the faders and controllers
                //
                slotUpdateInstrument((*iIt)->getId());

                // Increment counters
                //
                posCount++;
                faderCount++;
            }

            QString name = QString("%1 (%2)")
                           .arg(QObject::tr(QString::fromStdString(dev->getName())))
                           .arg(deviceCount++);

            addTab(m_tabFrame, name);
        }
    }
}

void
MidiMixerWindow::addTab(QWidget *tab, const QString &title)
{
    m_tabWidget->addTab(tab, title);
}

void
MidiMixerWindow::slotFaderLevelChanged(float value)
{
    const QObject *s = sender();

    for (FaderVector::const_iterator it = m_faders.begin();
            it != m_faders.end(); ++it) {
        if ((*it)->m_volumeFader == s) {
            Instrument *instr = m_studio->
                                getInstrumentById((*it)->m_id);

            if (instr) {

                instr->setControllerValue(MIDI_CONTROLLER_VOLUME, MidiByte(value));

                MappedEvent mE((*it)->m_id,
                               MappedEvent::MidiController,
                               MIDI_CONTROLLER_VOLUME,
                               MidiByte(value));
                StudioControl::sendMappedEvent(mE);

                // send out to external controllers as well.
                //!!! really want some notification of whether we have any!
                int tabIndex = m_tabWidget->currentPageIndex();
                if (tabIndex < 0)
                    tabIndex = 0;
                int i = 0;
                for (DeviceList::const_iterator dit = m_studio->begin();
                        dit != m_studio->end(); ++dit) {
                    RG_DEBUG << "slotFaderLevelChanged: i = " << i << ", tabIndex " << tabIndex << endl;
                    if (!dynamic_cast<MidiDevice*>(*dit))
                        continue;
                    if (i != tabIndex) {
                        ++i;
                        continue;
                    }
                    RG_DEBUG << "slotFaderLevelChanged: device id = " << instr->getDevice()->getId() << ", visible device id " << (*dit)->getId() << endl;
                    if (instr->getDevice()->getId() == (*dit)->getId()) {
                        RG_DEBUG << "slotFaderLevelChanged: sending control device mapped event for channel " << instr->getMidiChannel() << endl;
                        mE.setRecordedChannel(instr->getMidiChannel());
                        mE.setRecordedDevice(Device::CONTROL_DEVICE);
                        StudioControl::sendMappedEvent(mE);
                    }
                    break;
                }
            }

            emit instrumentParametersChanged((*it)->m_id);
            return ;
        }
    }
}

void
MidiMixerWindow::slotControllerChanged(float value)
{
    const QObject *s = sender();
    size_t i = 0, j = 0;

    for (i = 0; i < m_faders.size(); ++i) {
        for (j = 0; j < m_faders[i]->m_controllerRotaries.size(); ++j) {
            if (m_faders[i]->m_controllerRotaries[j].second == s)
                break;
        }

        // break out on match
        if (j != m_faders[i]->m_controllerRotaries.size())
            break;
    }

    // Don't do anything if we've not matched and got solid values
    // for i and j
    //
    if (i == m_faders.size() || j == m_faders[i]->m_controllerRotaries.size())
        return ;

    //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - found a controller"
    //<< endl;

    Instrument *instr = m_studio->getInstrumentById(
                            m_faders[i]->m_id);

    if (instr) {

        //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - "
        //<< "got instrument to change" << endl;

        instr->setControllerValue(m_faders[i]->
                                  m_controllerRotaries[j].first,
                                  MidiByte(value));

        MappedEvent mE(m_faders[i]->m_id,
                       MappedEvent::MidiController,
                       m_faders[i]->m_controllerRotaries[j].first,
                       MidiByte(value));
        StudioControl::sendMappedEvent(mE);

        int tabIndex = m_tabWidget->currentPageIndex();
        if (tabIndex < 0)
            tabIndex = 0;
        int k = 0;
        for (DeviceList::const_iterator dit = m_studio->begin();
                dit != m_studio->end(); ++dit) {
            RG_DEBUG << "slotControllerChanged: k = " << k << ", tabIndex " << tabIndex << endl;
            if (!dynamic_cast<MidiDevice*>(*dit))
                continue;
            if (k != tabIndex) {
                ++k;
                continue;
            }
            RG_DEBUG << "slotControllerChanged: device id = " << instr->getDevice()->getId() << ", visible device id " << (*dit)->getId() << endl;
            if (instr->getDevice()->getId() == (*dit)->getId()) {
                RG_DEBUG << "slotControllerChanged: sending control device mapped event for channel " << instr->getMidiChannel() << endl;
                // send out to external controllers as well.
                //!!! really want some notification of whether we have any!
                mE.setRecordedChannel(instr->getMidiChannel());
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }
        }

        emit instrumentParametersChanged(m_faders[i]->m_id);
    }
}

void
MidiMixerWindow::slotUpdateInstrument(InstrumentId id)
{
    //RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument - id = " << id << endl;

    DeviceListConstIterator it;
    MidiDevice *dev = 0;
    InstrumentList instruments;
    InstrumentList::const_iterator iIt;
    int count = 0;

    blockSignals(true);

    for (it = m_studio->begin(); it != m_studio->end(); ++it) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev) {
            instruments = dev->getPresentationInstruments();
            ControlList controls = getIPBForMidiMixer(dev);

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt) {
                // Match and set
                //
                if ((*iIt)->getId() == id) {
                    // Set Volume fader
                    //
                    m_faders[count]->m_volumeFader->blockSignals(true);

                    MidiByte volumeValue;
                    try {
                        volumeValue = (*iIt)->
                                getControllerValue(MIDI_CONTROLLER_VOLUME);
                    } catch (std::string s) {
                        // This should never get called.
                        volumeValue = (*iIt)->getVolume();
                    }
                    
                    m_faders[count]->m_volumeFader->setFader(float(volumeValue));
                    m_faders[count]->m_volumeFader->blockSignals(false);

                    /*
                    StaticControllers &staticControls = 
                        (*iIt)->getStaticControllers();
                    RG_DEBUG << "STATIC CONTROLS SIZE = " 
                             << staticControls.size() << endl;
                    */

                    // Set all controllers for this Instrument
                    //
                    for (size_t i = 0; i < controls.size(); ++i) {
                        float value = 0.0;

                        m_faders[count]->m_controllerRotaries[i].second->blockSignals(true);

                        // The ControllerValues might not yet be set on
                        // the actual Instrument so don't always expect
                        // to find one.  There might be a hole here for
                        // deleted Controllers to hang around on
                        // Instruments..
                        //
                        try {
                            value = float((*iIt)->getControllerValue
                                          (controls[i].getControllerValue()));
                        } catch (std::string s) {
                            /*
                            RG_DEBUG << 
                            "MidiMixerWindow::slotUpdateInstrument - "
                                     << "can't match controller " 
                                     << int(controls[i].
                                         getControllerValue()) << " - \""
                                     << s << "\"" << endl;
                                     */
                            continue;
                        }

                        /*
                        RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument"
                                 << " - MATCHED "
                                 << int(controls[i].getControllerValue())
                                 << endl;
                                 */

                        m_faders[count]->m_controllerRotaries[i].
                        second->setPosition(value);

                        m_faders[count]->m_controllerRotaries[i].second->blockSignals(false);
                    }
                }
                count++;
            }
        }
    }

    blockSignals(false);
}

void
MidiMixerWindow::updateMeters()
{
    for (size_t i = 0; i != m_faders.size(); ++i) {
        LevelInfo info;
        if (!SequencerDataBlock::getInstance()->
            getInstrumentLevelForMixer(m_faders[i]->m_id, info)) {
            continue;
        }
        if (m_faders[i]->m_vuMeter) {
            m_faders[i]->m_vuMeter->setLevel(double(info.level / 127.0));
            RG_DEBUG << "MidiMixerWindow::updateMeters - level  " << info.level << endl;
        } else {
            RG_DEBUG << "MidiMixerWindow::updateMeters - m_vuMeter for m_faders[" << i << "] is NULL!" << endl;
        }
    }
}

void
MidiMixerWindow::updateMonitorMeter()
{
    // none here
}

void
MidiMixerWindow::slotControllerDeviceEventReceived(MappedEvent *e,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return ;
    RG_DEBUG << "MidiMixerWindow::slotControllerDeviceEventReceived: this one's for me" << endl;
    raise();

    // get channel number n from event
    // get nth instrument on current tab

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    int tabIndex = m_tabWidget->currentPageIndex();

    int i = 0;

    for (DeviceList::const_iterator it = m_studio->begin();
            it != m_studio->end(); ++it) {

        MidiDevice *dev =
            dynamic_cast<MidiDevice*>(*it);

        if (!dev)
            continue;
        if (i != tabIndex) {
            ++i;
            continue;
        }

        InstrumentList instruments = dev->getPresentationInstruments();

        for (InstrumentList::const_iterator iIt =
                    instruments.begin(); iIt != instruments.end(); ++iIt) {

            Instrument *instrument = *iIt;

            if (instrument->getMidiChannel() != channel)
                continue;

            ControlList cl = dev->getIPBControlParameters();
            for (ControlList::const_iterator i = cl.begin();
                    i != cl.end(); ++i) {
                if ((*i).getControllerValue() == controller) {
                    RG_DEBUG << "Setting controller " << controller << " for instrument " << instrument->getId() << " to " << value << endl;
                    instrument->setControllerValue(controller, value);
                    break;
                }
            }

            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MidiByte(controller),
                           MidiByte(value));
            StudioControl::sendMappedEvent(mE);

            slotUpdateInstrument(instrument->getId());
            emit instrumentParametersChanged(instrument->getId());
        }

        break;
    }
}

void
MidiMixerWindow::slotCurrentTabChanged(QWidget *)
{
    sendControllerRefresh();
}

void
MidiMixerWindow::sendControllerRefresh()
{
    //!!! need to know if we have a current external controller device,
    // as this is expensive

    int tabIndex = m_tabWidget->currentPageIndex();
    RG_DEBUG << "MidiMixerWindow::slotCurrentTabChanged: current is " << tabIndex << endl;

    if (tabIndex < 0)
        return ;

    int i = 0;

    for (DeviceList::const_iterator dit = m_studio->begin();
            dit != m_studio->end(); ++dit) {

        MidiDevice *dev = dynamic_cast<MidiDevice*>(*dit);
        RG_DEBUG << "device is " << (*dit)->getId() << ", dev " << dev << endl;

        if (!dev)
            continue;
        if (i != tabIndex) {
            ++i;
            continue;
        }

        InstrumentList instruments = dev->getPresentationInstruments();
        ControlList controls = getIPBForMidiMixer(dev);

        RG_DEBUG << "device has " << instruments.size() << " presentation instruments, " << dev->getAllInstruments().size() << " instruments " << endl;

        for (InstrumentList::const_iterator iIt =
                    instruments.begin(); iIt != instruments.end(); ++iIt) {

            Instrument *instrument = *iIt;
            int channel = instrument->getMidiChannel();

            RG_DEBUG << "instrument is " << instrument->getId() << endl;

            for (ControlList::const_iterator cIt =
                        controls.begin(); cIt != controls.end(); ++cIt) {

                int controller = (*cIt).getControllerValue();
                int value;
                try {
                    value = instrument->getControllerValue(controller);
                } catch (std::string s) {
                    std::cerr << "Exception in MidiMixerWindow::currentChanged: " << s << " (controller " << controller << ", instrument " << instrument->getId() << ")" << std::endl;
                    value = 0;
                }

                MappedEvent mE(instrument->getId(),
                               MappedEvent::MidiController,
                               controller, value);
                mE.setRecordedChannel(channel);
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }

            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_VOLUME,
                           instrument->getVolume());
            mE.setRecordedChannel(channel);
            mE.setRecordedDevice(Device::CONTROL_DEVICE);
            RG_DEBUG << "sending controller mapped event for channel " << channel << ", volume " << instrument->getVolume() << endl;
            StudioControl::sendMappedEvent(mE);
        }

        break;
    }
}

void
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "MidiMixer::slotSynchronise" << endl;
    //setupTabs();
}


void
MidiMixerWindow::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:midiMixerWindow-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:midiMixerWindow-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
MidiMixerWindow::slotHelpAbout()
{
    new AboutDialog(this);
}

void
MidiMixerWindow::setRewFFwdToAutoRepeat()
{
    // This one didn't work in Classic either.  Looking at it as a fresh
    // problem, it was tricky.  The QAction has an objectName() of "rewind"
    // but the QToolButton associated with that action has no object name at
    // all.  We kind of have to go around our ass to get to our elbow on
    // this one.
    
    // get pointers to the actual actions    
    QAction *rewAction = findAction("playback_pointer_back_bar");    // rewind
    QAction *ffwAction = findAction("playback_pointer_forward_bar"); // fast forward

    QWidget* transportToolbar = this->findToolbar("Transport Toolbar");

    if (transportToolbar) {

        // get a list of all the toolbar's children (presumably they're
        // QToolButtons, but use this kind of thing with caution on customized
        // QToolBars!)
        QList<QToolButton *> widgets = transportToolbar->findChildren<QToolButton *>();

        // iterate through the entire list of children
        for (QList<QToolButton *>::iterator i = widgets.begin(); i != widgets.end(); ++i) {

            // get a pointer to the button's default action
            QAction *act = (*i)->defaultAction();

            // compare pointers, if they match, we've found the button
            // associated with that action
            //
            // we then have to not only setAutoRepeat() on it, but also connect
            // it up differently from what it got in createAction(), as
            // determined empirically (bleargh!!)
            if (act == rewAction) {
                connect((*i), SIGNAL(clicked()), this, SIGNAL(rewindPlayback()));

            } else if (act == ffwAction) {
                connect((*i), SIGNAL(clicked()), this, SIGNAL(fastForwardPlayback()));

            } else  {
                continue;
            }

            //  Must have found an button to update
            (*i)->removeAction(act);
            (*i)->setAutoRepeat(true);
        }
    }
}

// Code stolen From src/base/MidiDevice
ControlList
MidiMixerWindow::getIPBForMidiMixer(MidiDevice *dev) const
{
    ControlList controlList = dev->getIPBControlParameters();
    ControlList retList;

    Rosegarden::MidiByte MIDI_CONTROLLER_VOLUME = 0x07;

    for (ControlList::const_iterator it = controlList.begin();
         it != controlList.end(); ++it)
    {
        if (it->getIPBPosition() != -1 && 
            it->getControllerValue() != MIDI_CONTROLLER_VOLUME)
            retList.push_back(*it);
    }

    return retList;
}


}
#include "MidiMixerWindow.moc"
