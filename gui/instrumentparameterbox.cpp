// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <iostream>
#include <cstdio>

#include <qdial.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>
#include <qwidgetstack.h>

#include <kcombobox.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>

#include "Midi.h"
#include "Instrument.h"
#include "MidiDevice.h"
#include "MappedStudio.h"

#include "audioplugindialog.h"
#include "instrumentparameterbox.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "studiocontrol.h"
#include "rosegardenguidoc.h"
#include "trackvumeter.h"
#include "rosegardengui.h"

#include "rosestrings.h"
#include "rosedebug.h"

#include "studiocontrol.h"
#include "studiowidgets.h"

InstrumentParameterBox::InstrumentParameterBox(RosegardenGUIDoc *doc,
                                               QWidget *parent)
    : RosegardenParameterBox(1, Qt::Horizontal, i18n("Instrument Parameters"), parent),
//      m_widgetStack(new QWidgetStack(this)),
      m_widgetStack(0), //!!!
      m_noInstrumentParameters(new QVBox(this)),
      m_midiInstrumentParameters(new MIDIInstrumentParameterPanel(doc, this)),
      m_audioInstrumentParameters(new AudioInstrumentParameterPanel(doc, this)),
      m_selectedInstrument(0),
      m_doc(doc)
{
//!!!    m_widgetStack->setFont(getFont());
    m_noInstrumentParameters->setFont(getFont());
    m_midiInstrumentParameters->setFont(getFont());
    m_audioInstrumentParameters->setFont(getFont());

    bool contains = false;

    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
        if ((*it) == this)
            contains = true;

    if (!contains)
        instrumentParamBoxes.push_back(this);

    QLabel *label = new QLabel(i18n("<no instrument>"), m_noInstrumentParameters);
    label->setAlignment(label->alignment() | Qt::AlignHCenter);

//!!!    m_widgetStack->addWidget(m_midiInstrumentParameters);
//    m_widgetStack->addWidget(m_audioInstrumentParameters);
//    m_widgetStack->addWidget(m_noInstrumentParameters);

    m_midiInstrumentParameters->adjustSize();
    m_audioInstrumentParameters->adjustSize();
    m_noInstrumentParameters->adjustSize();

    connect(m_audioInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_audioInstrumentParameters,
            SIGNAL(muteButton(Rosegarden::InstrumentId, bool)),
            this, 
            SIGNAL(setMute(Rosegarden::InstrumentId, bool)));
    
    connect(m_audioInstrumentParameters,
            SIGNAL(soloButton(Rosegarden::InstrumentId, bool)),
            this,
            SIGNAL(setSolo(Rosegarden::InstrumentId, bool)));
    
    connect(m_audioInstrumentParameters,
            SIGNAL(recordButton(Rosegarden::InstrumentId, bool)),
            this,
            SIGNAL(setRecord(Rosegarden::InstrumentId, bool)));
    
    connect(m_midiInstrumentParameters, SIGNAL(updateAllBoxes()),
            this, SLOT(slotUpdateAllBoxes()));

    connect(m_midiInstrumentParameters, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this, SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)));
}


InstrumentParameterBox::~InstrumentParameterBox()
{
    // deregister this paramter box
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
    {
        if ((*it) == this)
        {
            instrumentParamBoxes.erase(it);
            break;
        }
    }
}

void
InstrumentParameterBox::setAudioMeter(double ch1, double ch2)
{
    m_audioInstrumentParameters->setAudioMeter(ch1, ch2);
}

void
InstrumentParameterBox::useInstrument(Rosegarden::Instrument *instrument)
{
    RG_DEBUG << "useInstrument() - populate Instrument\n";

    m_noInstrumentParameters->hide();
    m_midiInstrumentParameters->hide();
    m_audioInstrumentParameters->hide();

    if (instrument == 0)
    {
//!!!        m_widgetStack->raiseWidget(m_noInstrumentParameters);
	m_noInstrumentParameters->show();
        return;
    } 

    // ok
    m_selectedInstrument = instrument;

    // Hide or Show according to Instrumen type
    //
    if (instrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->setupForInstrument(m_selectedInstrument);
//!!!        m_widgetStack->raiseWidget(m_audioInstrumentParameters);
	m_audioInstrumentParameters->show();

    } else { // Midi

        m_midiInstrumentParameters->setupForInstrument(m_selectedInstrument);
//!!!        m_widgetStack->raiseWidget(m_midiInstrumentParameters);
	m_midiInstrumentParameters->show();
    }
    
}

void
InstrumentParameterBox::setMute(bool value)
{
    if (m_selectedInstrument && 
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetMute(value);
    }
}

/*
 * Set the record state of the audio instrument parameter panel
 */
void
InstrumentParameterBox::setRecord(bool value)
{
    if (m_selectedInstrument &&
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetRecord(value);
    }
}

void
InstrumentParameterBox::setSolo(bool value)
{
    if (m_selectedInstrument &&
            m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        m_audioInstrumentParameters->slotSetSolo(value);
    }
}


void
MIDIInstrumentParameterPanel::slotActivateProgramChange(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_programCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_selectedInstrument->setSendProgramChange(value);
    m_programValue->setDisabled(!value);
    populateProgramList();

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(),
                                 Rosegarden::MappedEvent::MidiProgramChange,
                                 m_selectedInstrument->getProgramChange(),
                                 (Rosegarden::MidiByte)0);
    // Send the controller change
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);
    emit updateAllBoxes();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotActivateBank(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_bankCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_bankValue->setDisabled(!value);
    value |= m_variationCheckBox->isChecked();
    m_selectedInstrument->setSendBankSelect(value);

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotActivateVariation(bool value)
{
    if (m_selectedInstrument == 0)
    {
        m_variationCheckBox->setChecked(false);
        emit updateAllBoxes();
        return;
    }

    m_variationValue->setDisabled(!value);
    value |= m_bankCheckBox->isChecked();
    m_selectedInstrument->setSendBankSelect(value);

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectBank(int index)
{
    if (m_selectedInstrument == 0)
        return;

    Rosegarden::MidiBank *bank = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getBankByIndex(index);

    m_selectedInstrument->setMSB(bank->getMSB());
    m_selectedInstrument->setLSB(bank->getLSB());

    // repopulate program list
    populateProgramList();

    Rosegarden::MappedEvent *mE = 
        new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                    bank->getMSB());
    Rosegarden::StudioControl::sendMappedEvent(mE);

    mE = new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                    bank->getLSB());
    // Send the lsb
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);

    // also need to resend Program change to activate new program
    slotSelectProgram(m_selectedInstrument->getProgramChange());
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectProgram(int index)
{
    if (m_selectedInstrument == 0)
        return;

    Rosegarden::MidiProgram *prg;

    if (m_selectedInstrument->sendsBankSelect())
    {
	Rosegarden::MidiBank bank(m_selectedInstrument->getProgram().getBank());
        prg =
            dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgram(bank, index);

        // Send the bank select message before any PC message
        //
        Rosegarden::MappedEvent *mE = 
            new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_MSB,
                                    bank.getMSB());
        Rosegarden::StudioControl::sendMappedEvent(mE);

        mE = new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_BANK_LSB,
                                    bank.getLSB());
        Rosegarden::StudioControl::sendMappedEvent(mE);

        RG_DEBUG << "sending bank select "
		 << bank.getMSB() << " : "
		 << bank.getLSB() << endl;
    }
    else
    {
        prg = dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramByIndex(index);
    }

    if (prg == 0)
    {
        RG_DEBUG << "program change not found in bank" << endl;
        return;
    }

    m_selectedInstrument->setProgramChange(prg->getProgram());

    RG_DEBUG << "sending program change " << prg->getProgram() << endl;

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiProgramChange,
                                 prg->getProgram(),
                                 (Rosegarden::MidiByte)0);
    // Send the controller change
    //
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
			       strtoqstr(m_selectedInstrument->
					 getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectVariation(int index)
{
//!!! implement
}

void
MIDIInstrumentParameterPanel::slotSelectPan(float value)
{
    if (m_selectedInstrument == 0)
        return;

    // For audio instruments we pan from -100 to +100 but storage
    // within an unsigned char is 0 - 200 - so we adjust by 100
    //
    float adjValue = value;
    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
        value += 100;

    m_selectedInstrument->setPan(Rosegarden::MidiByte(adjValue));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_PAN,
                                 (Rosegarden::MidiByte)value);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectVolume(float value)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setVolume(Rosegarden::MidiByte(value));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_VOLUME,
                                 (Rosegarden::MidiByte)value);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
}

void
AudioInstrumentParameterPanel::slotSelectAudioLevel(int value)
{
    if (m_selectedInstrument == 0)
        return;

    if (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio)
    {
        // If this is the record track then we store and send the record level
        //
        if (m_audioFader->m_recordButton->isOn())
        {
            //cout << "SETTING STORED RECORD LEVEL = " << value << endl;
            m_selectedInstrument->setRecordLevel(Rosegarden::MidiByte(value));

            Rosegarden::StudioControl::setStudioObjectProperty
              (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
               Rosegarden::MappedAudioFader::FaderRecordLevel,
               Rosegarden::MappedObjectValue(value));
        }
        else
        {
            //cout << "SETTING STORED LEVEL = " << value << endl;
            m_selectedInstrument->setVolume(Rosegarden::MidiByte(value));

            Rosegarden::StudioControl::setStudioObjectProperty
              (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
               Rosegarden::MappedAudioFader::FaderLevel,
               Rosegarden::MappedObjectValue(value));
        }
    }

    emit updateAllBoxes();
}

void 
AudioInstrumentParameterPanel::slotSetMute(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetMute - "
             << "value = " << value << endl;
    m_audioFader->m_muteButton->setOn(value);
}

void
AudioInstrumentParameterPanel::slotSetSolo(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetSolo - "
             << "value = " << value << endl;
    m_audioFader->m_soloButton->setOn(value);
}

void 
AudioInstrumentParameterPanel::slotSetRecord(bool value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetRecord - "
             << "value = " << value << endl;

    //if (m_selectedInstrument)
        //cout << "INSTRUMENT NAME = " 
               //<< m_selectedInstrument->getName() << endl;

    // Set the background colour for the button
    //
    if (value)
    {
        m_audioFader->m_recordButton->
            setPalette(QPalette(RosegardenGUIColours::ActiveRecordTrack));

        if (m_selectedInstrument &&
            (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio))
        {
            // set the fader value to the record value

            disconnect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                       this, SLOT(slotSelectAudioLevel(int)));

            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getRecordLevel());
            //cout << "SETTING VISIBLE FADER RECORD LEVEL = " << 
                    //int(m_selectedInstrument->getRecordLevel()) << endl;

            connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                    this, SLOT(slotSelectAudioLevel(int)));

            // Set the prepend text on the audio fader
            m_audioFader->m_fader->setPrependText(i18n("Record level = "));
        }
    }
    else
    {
        m_audioFader->m_recordButton->unsetPalette();

        if (m_selectedInstrument &&
            (m_selectedInstrument->getType() == Rosegarden::Instrument::Audio))
        {
            disconnect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                       this, SLOT(slotSelectAudioLevel(int)));

            // set the fader value to the playback value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getVolume());

            //cout << "SETTING VISIBLE FADER LEVEL = " << 
                    //int(m_selectedInstrument->getVolume()) << endl;

            connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
                    this, SLOT(slotSelectAudioLevel(int)));

            // Set the prepend text on the audio fader
            m_audioFader->m_fader->setPrependText(i18n("Playback level = "));
        }
    }

    m_audioFader->m_recordButton->setOn(value);
}


void
MIDIInstrumentParameterPanel::slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setMidiChannel(index);

    // don't use the emit - use this method instead
    Rosegarden::StudioControl::sendMappedInstrument(
            Rosegarden::MappedInstrument(m_selectedInstrument));
    emit updateAllBoxes();
}





// Populate program list by bank context
//
void
MIDIInstrumentParameterPanel::populateProgramList()
{
    if (m_selectedInstrument == 0)
        return;

    // The program list
    m_programValue->clear();


    Rosegarden::MidiByte msb = 0;
    Rosegarden::MidiByte lsb = 0;

    if (m_selectedInstrument->sendsBankSelect())
    {
        msb = m_selectedInstrument->getMSB();
        lsb = m_selectedInstrument->getLSB();
    }

    Rosegarden::StringList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (m_selectedInstrument->getDevice())->getProgramList
	(Rosegarden::MidiBank(m_selectedInstrument->isPercussion(), msb, lsb));

    Rosegarden::StringList::iterator it;

    for (it = list.begin(); it != list.end(); it++) {

        m_programValue->insertItem(strtoqstr(*it));

	const Rosegarden::MidiProgram *program = 
	    dynamic_cast<Rosegarden::MidiDevice*>
	    (m_selectedInstrument->getDevice())->
	    getProgramByIndex(m_programValue->count() - 1);

	if (m_selectedInstrument->getProgramChange() == program->getProgram()) {
	    m_programValue->setCurrentItem(m_programValue->count() - 1);
	}
    }

//    m_programValue->setCurrentItem(
//            (int)m_selectedInstrument->getProgramChange());
}

void
InstrumentParameterBox::slotUpdateAllBoxes()
{
    std::vector<InstrumentParameterBox*>::iterator it =
        instrumentParamBoxes.begin();

    for (; it != instrumentParamBoxes.end(); it++)
    {
        if ((*it) != this && m_selectedInstrument &&
            (*it)->getSelectedInstrument() == m_selectedInstrument)
            (*it)->useInstrument(m_selectedInstrument);
    }

}


void
AudioInstrumentParameterPanel::slotSelectPlugin(int index)
{
    int key = (index << 24) + m_selectedInstrument->getId();

    if (m_pluginDialogs[key] == 0)
    {
        // only create a dialog if we've got a plugin instance
        Rosegarden::AudioPluginInstance *inst = 
            m_selectedInstrument->getPlugin(index);

        if (inst)
        {
            // Create the plugin dialog
            //
            m_pluginDialogs[key] = 
                new Rosegarden::AudioPluginDialog(this,
                                              m_doc->getPluginManager(),
                                              m_selectedInstrument,
                                              index);

            // Get the App pointer and plug the new dialog into the 
            // standard keyboard accelerators so that we can use them
            // still while the plugin has focus.
            //
            QWidget *par = parentWidget();
            while (!par->isTopLevel()) par = par->parentWidget();

            RosegardenGUIApp *app = dynamic_cast<RosegardenGUIApp*>(par);

            app->plugAccelerators(m_pluginDialogs[key],
                                  m_pluginDialogs[key]->getAccelerators());

            connect(m_pluginDialogs[key], SIGNAL(pluginSelected(int, int)),
                    this, SLOT(slotPluginSelected(int, int)));

            connect(m_pluginDialogs[key],
		    SIGNAL(pluginPortChanged(int, int, float)),
                    this, SLOT(slotPluginPortChanged(int, int, float)));

            connect(m_pluginDialogs[key], SIGNAL(bypassed(int, bool)),
                    this, SLOT(slotBypassed(int, bool)));

	    connect(m_pluginDialogs[key], SIGNAL(destroyed(int)),
		    this, SLOT(slotPluginDialogDestroyed(int)));

            m_pluginDialogs[key]->show();

            // Set modified
            m_doc->slotDocumentModified();
        }
        else
        {
	    std::cerr << "AudioInstrumentParameterPanel::slotSelectPlugin - "
		      << "no AudioPluginInstance found for index "
		      << index << std::endl;
        }
    }
    else
    {
	m_pluginDialogs[key]->raise();
    }
}

void
AudioInstrumentParameterPanel::slotPluginSelected(int index, int plugin)
{

    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(index);

    if (inst)
    {
        if (plugin == -1)
        {
            std:: cout << "InstrumentParameterBox::slotPluginSelected - "
                       << "no plugin selected" << std::endl;

            // Destroy plugin instance
            if (Rosegarden::StudioControl::
                    destroyStudioObject(inst->getMappedId()))
            {
                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << "cannot destroy Studio object "
                         << inst->getMappedId() << endl;
            }

            inst->setAssigned(false);
            m_audioFader->m_plugins[index]->setText(i18n("<no plugin>"));

        }
        else
        {
            Rosegarden::AudioPlugin *plgn = 
                m_doc->getPluginManager()->getPlugin(plugin);

            // If unassigned then create a sequencer instance of this
            // AudioPluginInstance.
            //
            if (inst->isAssigned())
            {
                // unassign, destory and recreate
                std::cout << "MAPPED ID = " << inst->getMappedId() 
			  << " for Instrument " << inst->getId() << std::endl;

                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << "MappedObjectId = "
                         << inst->getMappedId()
                         << " - UniqueId = " << plgn->getUniqueId()
                         << endl;


#ifdef HAVE_LADSPA
                Rosegarden::StudioControl::setStudioObjectProperty
                    (inst->getMappedId(),
                     Rosegarden::MappedLADSPAPlugin::UniqueId,
                     plgn->getUniqueId());
#endif

            }
            else
            {
                // create a studio object at the sequencer
                Rosegarden::MappedObjectId newId =
                    Rosegarden::StudioControl::createStudioObject
                        (Rosegarden::MappedObject::LADSPAPlugin);

                RG_DEBUG << "InstrumentParameterBox::slotPluginSelected - "
                         << " new MappedObjectId = " << newId << endl;

                // set the new Mapped ID and that this instance
                // is assigned
                inst->setMappedId(newId);
                inst->setAssigned(true);

#ifdef HAVE_LADSPA
                // set the instrument id
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedObject::Instrument,
                     Rosegarden::MappedObjectValue(
                         m_selectedInstrument->getId()));

                // set the position
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedObject::Position,
                     Rosegarden::MappedObjectValue(index));

                // set the plugin id
                Rosegarden::StudioControl::setStudioObjectProperty
                    (newId,
                     Rosegarden::MappedLADSPAPlugin::UniqueId,
                     Rosegarden::MappedObjectValue(
                         plgn->getUniqueId()));
#endif
            }

            Rosegarden::AudioPlugin *pluginClass 
                = m_doc->getPluginManager()->getPlugin(plugin);

            if (pluginClass)
                m_audioFader->m_plugins[index]->
                    setText(pluginClass->getLabel());
        }

        // Set modified
        m_doc->slotDocumentModified();
    }
    else
        std::cerr << "InstrumentParameterBox::slotPluginSelected - "
                  << "got index of unknown plugin!" << std::endl;
}

void
AudioInstrumentParameterPanel::slotPluginPortChanged(int pluginIndex,
                                                     int portIndex,
                                                     float value)
{
    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(pluginIndex);

    if (inst)
    {

#ifdef HAVE_LADSPA

        Rosegarden::StudioControl::
            setStudioPluginPort(inst->getMappedId(),
                                portIndex,
                                value);
                                
        RG_DEBUG << "InstrumentParameterBox::slotPluginPortChanged - "
                 << "setting plugin port (" << portIndex << ") to "
                 << value << endl;

        // Set modified
        m_doc->slotDocumentModified();

#endif // HAVE_LADSPA
    }

}


void
AudioInstrumentParameterPanel::slotPluginDialogDestroyed(int index)
{
    int key = (index << 24) + m_selectedInstrument->getId();
    m_pluginDialogs[key] = 0;
}

void
AudioInstrumentParameterPanel::slotBypassed(int pluginIndex, bool bp)
{
    Rosegarden::AudioPluginInstance *inst = 
        m_selectedInstrument->getPlugin(pluginIndex);

    if (inst)
    {
#ifdef HAVE_LADSPA
        Rosegarden::StudioControl::setStudioObjectProperty
            (inst->getMappedId(),
             Rosegarden::MappedLADSPAPlugin::Bypassed,
             Rosegarden::MappedObjectValue(bp));
#endif // HAVE_LADSPA


        /// Set the colour on the button
        //
        setBypassButtonColour(pluginIndex, bp);

        // Set the bypass on the instance
        //
        inst->setBypass(bp);

        // Set modified
        m_doc->slotDocumentModified();
    }
}

// Set the button colour
//
void
AudioInstrumentParameterPanel::setBypassButtonColour(int pluginIndex,
                                                     bool bypassState)
{
    // Set the bypass colour on the plugin button
    if (bypassState)
    {
        m_audioFader->m_plugins[pluginIndex]->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));

        m_audioFader->m_plugins[pluginIndex]->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));
    }
    else
    {
        m_audioFader->m_plugins[pluginIndex]->
            setPaletteForegroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::ButtonText));

        m_audioFader->m_plugins[pluginIndex]->
            setPaletteBackgroundColor(kapp->palette().
                    color(QPalette::Active, QColorGroup::Button));
    }
}


void
MIDIInstrumentParameterPanel::slotSelectChorus(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setChorus(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_CHORUS,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectReverb(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setReverb(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_REVERB,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectHighPass(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setFilter(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_FILTER,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectResonance(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setResonance(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_RESONANCE,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    // Send the controller change
    //
    /*

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MidiByte(99),
                                 (Rosegarden::MidiByte)0);
    // Send the controller change
    //
    emit Rosegarden::StudioControl::sendMappedEvent(mE);
    mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MidiByte(98),
                                 (Rosegarden::MidiByte)33);
    // Send the controller change
    //
    emit Rosegarden::StudioControl::sendMappedEvent(mE);
    mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MidiByte(6),
                                 (Rosegarden::MidiByte)index);
    // Send the controller change
    //
    emit Rosegarden::StudioControl::sendMappedEvent(mE);
    */

    updateAllBoxes();
}


void
MIDIInstrumentParameterPanel::slotSelectAttack(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setAttack(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_ATTACK,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectRelease(float index)
{
    if (m_selectedInstrument == 0)
        return;

    m_selectedInstrument->setRelease(Rosegarden::MidiByte(index));

    Rosegarden::MappedEvent *mE = 
     new Rosegarden::MappedEvent(m_selectedInstrument->getId(), 
                                 Rosegarden::MappedEvent::MidiController,
                                 Rosegarden::MIDI_CONTROLLER_RELEASE,
                                 (Rosegarden::MidiByte)index);
    Rosegarden::StudioControl::sendMappedEvent(mE);

    updateAllBoxes();
}


InstrumentParameterPanel::InstrumentParameterPanel(RosegardenGUIDoc *doc, 
                                                   QWidget* parent)
    : QFrame(parent),
      m_instrumentLabel(new QLabel(this)),
      m_selectedInstrument(0),
      m_doc(doc)
{
}



AudioInstrumentParameterPanel::AudioInstrumentParameterPanel(RosegardenGUIDoc* doc, QWidget* parent)
    : InstrumentParameterPanel(doc, parent),
      m_audioFader(new AudioFaderWidget(this, "instrumentAudioFader", false))
{
    QGridLayout *gridLayout = new QGridLayout(this, 10, 6, 5, 5);

    // Instrument label : first row, all cols
    gridLayout->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 5, AlignCenter);

    // fader and connect it
    gridLayout->addMultiCellWidget(m_audioFader, 1, 9, 2, 3);

    connect(m_audioFader, SIGNAL(audioChannelsChanged(int)),
            this, SLOT(slotAudioChannels(int)));

    connect(m_audioFader->m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(slotSelectPlugin(int)));

    connect(m_audioFader->m_fader, SIGNAL(faderChanged(int)),
            this, SLOT(slotSelectAudioLevel(int)));

    connect(m_audioFader->m_muteButton, SIGNAL(clicked()),
            this, SLOT(slotMute()));

    connect(m_audioFader->m_soloButton, SIGNAL(clicked()),
            this, SLOT(slotSolo()));

    connect(m_audioFader->m_recordButton, SIGNAL(clicked()),
            this, SLOT(slotRecord()));

    connect(m_audioFader->m_pan, SIGNAL(valueChanged(float)),
            this, SLOT(slotSetPan(float)));

    connect(m_audioFader->m_audioInput, SIGNAL(activated(int)),
            this, SLOT(slotSelectAudioInput(int)));

}

void
AudioInstrumentParameterPanel::slotMute()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotMute" << endl;
    emit muteButton(m_selectedInstrument->getId(),
                    m_audioFader->m_muteButton->isOn());
}

void
AudioInstrumentParameterPanel::slotSolo()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSolo" << endl;
    emit soloButton(m_selectedInstrument->getId(),
                    m_audioFader->m_soloButton->isOn());
}

void
AudioInstrumentParameterPanel::slotRecord()
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotRecord - " 
             << " isOn = " <<  m_audioFader->m_recordButton->isOn() << endl;

    // At the moment we can't turn a recording button off
    //
    if (m_audioFader->m_recordButton->isOn())
    {
        m_audioFader->m_recordButton->setOn(true);

        if (m_selectedInstrument)
        {
            //std::cout << "SETTING FADER RECORD LEVEL = " 
                 //<< int(m_selectedInstrument->getRecordLevel()) << endl;

            // set the fader value to the record value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getRecordLevel());
        }

        emit recordButton(m_selectedInstrument->getId(),
                          m_audioFader->m_recordButton->isOn());
    }
    else
    {
        m_audioFader->m_recordButton->setOn(true);
    }

    /*
    else
    {
        if (m_selectedInstrument)
        {
            // set the fader value to the record value
            m_audioFader->m_fader->
                setFader(m_selectedInstrument->getVolume());

            cout << "SETTING FADER LEVEL = " 
                 << int(m_selectedInstrument->getVolume()) << endl;
        }
    }
    */
}

void
AudioInstrumentParameterPanel::slotSetPan(float pan)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSetPan - "
             << "pan = " << pan << endl;

    Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioFader::Pan,
         Rosegarden::MappedObjectValue(pan));

    m_selectedInstrument->setPan(Rosegarden::MidiByte(pan + 100.0));
}

void
AudioInstrumentParameterPanel::setAudioMeter(double ch1, double ch2)
{
    if (m_selectedInstrument)
    {
        if (m_selectedInstrument->getAudioChannels() == 1)
            m_audioFader->m_vuMeter->setLevel(ch1);
        else
            m_audioFader->m_vuMeter->setLevel(ch1, ch2);
    }
}


void
AudioInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument* instrument)
{
    m_selectedInstrument = instrument;

    m_instrumentLabel->setText(strtoqstr(instrument->getName()));

    /*
    if (m_audioFader->m_recordButton->isOn())
        m_audioFader->m_fader->setFader(instrument->getRecordLevel());
    else
        m_audioFader->m_fader->setFader(instrument->getVolume());
        */


    for (unsigned int i = 0; i < m_audioFader->m_plugins.size(); i++)
    {
        m_audioFader->m_plugins[i]->show();

        Rosegarden::AudioPluginInstance *inst = instrument->getPlugin(i);

        if (inst && inst->isAssigned())
        {
            Rosegarden::AudioPlugin *pluginClass 
                = m_doc->getPluginManager()->getPlugin(
                        m_doc->getPluginManager()->
                            getPositionByUniqueId(inst->getId()));

            if (pluginClass)
                m_audioFader->m_plugins[i]->
                    setText(pluginClass->getLabel());

            setBypassButtonColour(i, inst->isBypassed());

        }
        else
        {
            m_audioFader->m_plugins[i]->setText(i18n("<no plugin>"));

            if (inst)
                setBypassButtonColour(i, inst->isBypassed());
            else
                setBypassButtonColour(i, false);
        }
    }

    // Set the number of channels on the fader widget
    //
    m_audioFader->setAudioChannels(instrument->getAudioChannels());

    // Pan - adjusted backwards
    //
    m_audioFader->m_pan->setPosition(instrument->getPan() - 100);
}

void
AudioInstrumentParameterPanel::slotAudioChannels(int channels)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotAudioChannels - "
             << "channels = " << channels << endl;

    m_selectedInstrument->setAudioChannels(channels);
    /*
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(
                m_selectedInstrument->getId(),
                Rosegarden::MappedEvent::AudioChannels,
                Rosegarden::MidiByte(m_selectedInstrument->getAudioChannels()));

    Rosegarden::StudioControl::sendMappedEvent(mE);
    */

    Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioObject::Channels,
         Rosegarden::MappedObjectValue(channels));

}

void
AudioInstrumentParameterPanel::slotSelectAudioInput(int value)
{
    RG_DEBUG << "AudioInstrumentParameterPanel::slotSelectAudioInput - "
             << value << endl;

    /*
   Rosegarden::MappedObjectValueList connectionList;
   connectionList.push_back(value);
   */

   Rosegarden::StudioControl::setStudioObjectProperty
        (Rosegarden::MappedObjectId(m_selectedInstrument->getMappedId()),
         Rosegarden::MappedAudioObject::ConnectionsIn,
         Rosegarden::MappedObjectValue(value));
}




MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent)
    : InstrumentParameterPanel(doc, parent)
{
    QGridLayout *gridLayout = new QGridLayout(this, 11, 6, 8, 1);

//    QFrame *rotaryFrame = new QFrame(this);
//    rotaryFrame->setFrameStyle(QFrame::NoFrame);
//    comboLayout->addMultiCellWidget(rotaryFrame, 5, 5, 0, 2, AlignCenter);
//    QGridLayout *rotaryLayout = new QGridLayout(rotaryFrame, 4, 4, 8, 1);
        
    m_connectionLabel = new QLabel(this);
    m_bankValue = new KComboBox(false, this);
    m_channelValue = new RosegardenComboBox(true, false, this);
    m_programValue = new KComboBox(this);
    m_variationValue = new KComboBox(false, this);
    m_bankCheckBox = new QCheckBox(this);
    m_programCheckBox = new QCheckBox(this);
    m_variationCheckBox = new QCheckBox(this);
    m_percussionCheckBox = new QCheckBox(this);

    m_bankLabel = new QLabel(i18n("Bank"), this);
    m_variationLabel = new QLabel(i18n("Variation"), this);
    QLabel* programLabel = new QLabel(i18n("Program"), this);
    QLabel* channelLabel = new QLabel(i18n("Channel"), this);
    QLabel *percussionLabel = new QLabel(i18n("Percussion"), this);

    // Ensure a reasonable amount of space in the program dropdowns even
    // if no instrument initially selected
    QFontMetrics metrics(m_programValue->font());
    int width = metrics.width("Acoustic Grand Piano 123");
    m_bankValue->setMinimumWidth(width);
    m_programValue->setMinimumWidth(width);

    int   allMinCol = 0,   allMaxCol = 5;
    int comboMinCol = 2, comboMaxCol = 5;
    int  leftMinCol = 0,  leftMaxCol = 2;
    int rightMinCol = 3, rightMaxCol = 5;

    gridLayout->addMultiCellWidget
	(m_instrumentLabel, 0, 0, allMinCol, allMaxCol, AlignCenter);
    gridLayout->addMultiCellWidget
	(m_connectionLabel, 1, 1, allMinCol, allMaxCol, AlignCenter);

    gridLayout->addMultiCellWidget
	(channelLabel, 2, 2, 0, 1, AlignLeft);
    gridLayout->addMultiCellWidget
	(m_channelValue, 2, 2, comboMinCol, comboMaxCol, AlignRight);

    gridLayout->addWidget(percussionLabel, 3, 0, AlignLeft);
    gridLayout->addWidget(m_percussionCheckBox, 3, 5, AlignRight);

    gridLayout->addWidget(m_bankLabel,      4, 0, AlignLeft);
    gridLayout->addWidget(m_bankCheckBox, 4, 1);
    gridLayout->addMultiCellWidget
	(m_bankValue, 4, 4, comboMinCol, comboMaxCol, AlignRight);

    gridLayout->addWidget(programLabel,      5, 0);
    gridLayout->addWidget(m_programCheckBox, 5, 1);
    gridLayout->addMultiCellWidget
	(m_programValue, 5, 5, comboMinCol, comboMaxCol, AlignRight);

    gridLayout->addWidget(m_variationLabel, 6, 0);
    gridLayout->addWidget(m_variationCheckBox, 6, 1);
    gridLayout->addMultiCellWidget
	(m_variationValue, 6, 6, comboMinCol, comboMaxCol, AlignRight);

    QHBox *hbox;

    int row = 7;
    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_panRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 64.0, 20);
    QLabel* panLabel = new QLabel(i18n("Pan"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, leftMinCol, leftMaxCol, AlignLeft);

    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_chorusRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* chorusLabel = new QLabel(i18n("Chorus"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, rightMinCol, rightMaxCol, AlignLeft);

    ++row;
    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_volumeRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 64.0, 20);
    QLabel* volumeLabel= new QLabel(i18n("Volume"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, leftMinCol, leftMaxCol, AlignLeft);

    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_reverbRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* reverbLabel = new QLabel(i18n("Reverb"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, rightMinCol, rightMaxCol, AlignLeft);

    ++row;
    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_attackRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* attackLabel = new QLabel(i18n("Attack"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, leftMinCol, leftMaxCol, AlignLeft);

    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_highPassRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* highPassLabel = new QLabel(i18n("Filter"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, rightMinCol, rightMaxCol, AlignLeft);

    ++row;
    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_releaseRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* releaseLabel = new QLabel(i18n("Release"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, leftMinCol, leftMaxCol, AlignLeft);

    hbox = new QHBox(this);
    hbox->setSpacing(8);
    m_resonanceRotary = new RosegardenRotary(hbox, 0.0, 127.0, 1.0, 5.0, 0.0, 20);
    QLabel* resonanceLabel = new QLabel(i18n("Resonance"), hbox);
    gridLayout->addMultiCellWidget
	(hbox, row, row, rightMinCol, rightMaxCol, AlignLeft);

    // Some top space
    gridLayout->addRowSpacing(0, 8);
    gridLayout->addRowSpacing(1, 8);
//    gridLayout->addRowSpacing(1, 30);

    // Set some nice pastel knob colours
    //
    // light blue
    m_volumeRotary->setKnobColour(RosegardenGUIColours::RotaryPastelBlue);

    // light red
    m_panRotary->setKnobColour(RosegardenGUIColours::RotaryPastelRed);

    // light green
    m_chorusRotary->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);
    m_reverbRotary->setKnobColour(RosegardenGUIColours::RotaryPastelGreen);

    // light orange
    m_highPassRotary->setKnobColour(RosegardenGUIColours::RotaryPastelOrange);
    m_resonanceRotary->setKnobColour(RosegardenGUIColours::RotaryPastelOrange);

    // light yellow
    m_attackRotary->setKnobColour(RosegardenGUIColours::RotaryPastelYellow);
    m_releaseRotary->setKnobColour(RosegardenGUIColours::RotaryPastelYellow);

    // Populate channel list
    for (int i = 0; i < 16; i++)
        m_channelValue->insertItem(QString("%1").arg(i+1));

    // Disable these by default - they are activate by their
    // checkboxes
    //
    m_programValue->setDisabled(true);
    m_bankValue->setDisabled(true);
    m_variationValue->setDisabled(true);

    // Only active if we have an Instrument selected
    //
    m_programCheckBox->setDisabled(true);
    m_bankCheckBox->setDisabled(true);
    m_variationCheckBox->setDisabled(true);

    // Connect up the toggle boxes
    //
    connect(m_programCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateProgramChange(bool)));

    connect(m_bankCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateBank(bool)));

    connect(m_variationCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotActivateVariation(bool)));


    // Connect activations
    //
    connect(m_bankValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_variationValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectVariation(int)));

    connect(m_panRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectPan(float)));
    
    connect(m_volumeRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectVolume(float)));

    connect(m_programValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));

    // connect the advanced MIDI controls
    connect(m_chorusRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectChorus(float)));

    connect(m_reverbRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectReverb(float)));

    connect(m_highPassRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectHighPass(float)));

    connect(m_resonanceRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectResonance(float)));

    connect(m_attackRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectAttack(float)));

    connect(m_releaseRotary, SIGNAL(valueChanged(float)),
            this, SLOT(slotSelectRelease(float)));


    // don't select any of the options in any dropdown
    m_programValue->setCurrentItem(-1);
    m_bankValue->setCurrentItem(-1);
    m_channelValue->setCurrentItem(-1);
    m_variationValue->setCurrentItem(-1);
}

//!!! dup with trackbuttons.cpp
static QString
getPresentationName(Rosegarden::Instrument *instr)
{
    if (!instr) {
	return i18n("<no instrument>");
    } else if (instr->getType() == Rosegarden::Instrument::Audio) {
	return strtoqstr(instr->getName());
    } else {
	return strtoqstr(instr->getDevice()->getName() + " " + 
			 instr->getName());
    }
}


void
MIDIInstrumentParameterPanel::setupForInstrument(Rosegarden::Instrument *instrument)
{
    m_selectedInstrument = instrument;

    // Set instrument name
    //
    m_instrumentLabel->setText(getPresentationName(instrument));

    // Set Studio Device name
    //
    if (instrument->getDevice()) {
	QString connection(strtoqstr(instrument->getDevice()->getConnection()));
	if (connection == "") {
	    m_connectionLabel->setText(i18n("[ %1 ]").arg(i18n("No connection")));
	} else {

	    // remove trailing "(duplex)", "(read only)", "(write only)" etc
	    connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");

	    QString text = i18n("[ %1 ]").arg(connection);
	    QString origText(text);

	    QFontMetrics metrics(m_connectionLabel->fontMetrics());
	    int maxwidth = metrics.width
		("Program: [X]   Acoustic Grand Piano 123");// kind of arbitrary!

	    int hlen = text.length() / 2;
	    while (metrics.width(text) > maxwidth && text.length() > 10) {
		--hlen;
		text = origText.left(hlen) + "..." + origText.right(hlen);
	    }

	    if (text.length() > origText.length() - 7) text = origText;
	    m_connectionLabel->setText(text);
	}
    } else {
	m_connectionLabel->setText("");
    }

    // Enable all check boxes
    //
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());

    // Basic parameters
    //
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());
    m_panRotary->setPosition((float)instrument->getPan());
    m_volumeRotary->setPosition((float)instrument->getVolume());

    // Check for program change
    //
    if (instrument->sendsProgramChange())
    {
        m_programValue->setDisabled(false);
        populateProgramList();
    }
    else
    {
        m_programValue->setDisabled(true);
	m_programValue->clear();
        m_programValue->setCurrentItem(-1);
    }

    // clear bank list
    m_bankValue->clear();

    // create bank list
    Rosegarden::StringList list = 
        dynamic_cast<Rosegarden::MidiDevice*>
            (instrument->getDevice())->getBankList();

    Rosegarden::StringList::iterator it;

    for (it = list.begin(); it != list.end(); it++) {

        m_bankValue->insertItem(strtoqstr(*it));

	// Select 
	if (instrument->sendsBankSelect())
	{
	    const Rosegarden::MidiBank *bank = 
		dynamic_cast<Rosegarden::MidiDevice*>(instrument->getDevice())
		->getBankByIndex(m_bankValue->count() - 1);

	    if (instrument->getProgram().getBank() == *bank) {
		m_bankValue->setCurrentItem(m_bankValue->count() - 1);
	    }
	}
    }

    if (!instrument->sendsBankSelect())
    {
        m_bankValue->setDisabled(true);
        m_bankValue->setCurrentItem(-1);
    }

    // Advanced MIDI controllers
    //
    m_chorusRotary->setPosition(float(instrument->getChorus()));
    m_reverbRotary->setPosition(float(instrument->getReverb()));
    m_highPassRotary->setPosition(float(instrument->getFilter()));
    m_resonanceRotary->setPosition(float(instrument->getResonance()));
    m_attackRotary->setPosition(float(instrument->getAttack()));
    m_releaseRotary->setPosition(float(instrument->getRelease()));


}



