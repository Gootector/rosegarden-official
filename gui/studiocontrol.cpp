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

#include <kapp.h>
#include <dcopclient.h>

#include "studiocontrol.h"
#include "rosedebug.h"
#include "rosegardendcop.h"
#include "Midi.h"

namespace Rosegarden
{

MappedObjectId
StudioControl::createStudioObject(MappedObject::MappedObjectType type)
{
    Rosegarden::MappedObjectId value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << type;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "createMappedObject(int)",
                                  data, replyType, replyData))
    {
        SEQMAN_DEBUG << "createStudioObject - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    return value;
}

bool
StudioControl::destroyStudioObject(MappedObjectId id)
{
    int value = 0;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << int(id);

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "destroyMappedObject(int)",
                                  data, replyType, replyData))
    {
        SEQMAN_DEBUG << "destroyStudioObject - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    if (value == 1)
        return true;
    else
        return false;
}

MappedObjectPropertyList
StudioControl::getStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << id;
    streamOut << QString(property);

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getPropertyList(int, QString)",
                                  data, replyType, replyData))
    {
        SEQMAN_DEBUG << "getStudioObjectProperty - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> list;
    }

    return list;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property,
                        MappedObjectValue value)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // Use new MappedEvent interface
    //
    streamOut << id;
    streamOut << property;
    streamOut << value;

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setMappedProperty(int, QString, float)",
                 data))
    {
        SEQMAN_DEBUG << "setStudioObjectProperty - "
                     << "failed to contact Rosegarden sequencer" << endl;
    }

    return true;
}

bool
StudioControl::setStudioObjectProperty(MappedObjectId id,
                        const MappedObjectProperty &property,
                        MappedObjectValueList value)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // Use new MappedEvent interface
    //
    streamOut << id;
    streamOut << property;
    streamOut << value;

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setMappedProperty(int, QString, std::vector<float>)",
                 data))
    {
        SEQMAN_DEBUG << "setStudioObjectProperty - "
                     << "failed to contact Rosegarden sequencer" << endl;
    }

    return true;
}

MappedObjectId
StudioControl::getStudioObjectByType(MappedObject::MappedObjectType type)
{
    Rosegarden::MappedObjectId value = -1;
    QByteArray data;
    QCString replyType;
    QByteArray replyData;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << type;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getMappedObjectId(int)",
                                  data, replyType, replyData))
    {
        SEQMAN_DEBUG << "getStudioObjectByType - "
                     << "failed to contact Rosegarden sequencer"
                     << endl;
    }
    else
    {
        QDataStream streamIn(replyData, IO_ReadOnly);
        streamIn >> value;
    }

    return value;
}

void
StudioControl::setStudioPluginPort(MappedObjectId pluginId,
                                   unsigned long portId,
                                   MappedObjectValue value)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    // Use new MappedEvent interface
    //
    streamOut << pluginId;
    streamOut << portId;
    streamOut << value;

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "setMappedPort(int, unsigned long int, float)",
                 data))
    {
        RG_DEBUG << "failed to contact RG sequencer" << endl;
    }
}


void
StudioControl::sendMappedEvent(Rosegarden::MappedEvent *mE)
{
    Rosegarden::MappedComposition mC;
    mC.insert(mE);
    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendMappedComposition(const Rosegarden::MappedComposition &mC)
{
    if (mC.size() == 0)
        return;

    QCString replyType;
    QByteArray replyData;

    MappedComposition::iterator it = mC.begin();

    for (; it != mC.end(); it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        streamOut << (*it);
        if (!kapp->dcopClient()->
                send(ROSEGARDEN_SEQUENCER_APP_NAME,
                     ROSEGARDEN_SEQUENCER_IFACE_NAME,
                     "processMappedEvent(Rosegarden::MappedEvent)", data))
        {
            RG_DEBUG << "failed to contact RG sequencer" << endl;
        }
    }
}

void
StudioControl::sendMappedInstrument(const Rosegarden::MappedInstrument &mI)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << mI.getType();
    streamOut << mI.getChannel();
    streamOut << mI.getId();

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
             "setMappedInstrument(int, unsigned char, unsigned int)", data))
    {
        RG_DEBUG << "failed to contact RG sequencer" << endl;
    }
}


void
StudioControl::sendQuarterNoteLength(const Rosegarden::RealTime &length)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << length.sec;
    streamOut << length.usec;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                           "setQuarterNoteLength(long int, long int)", data))
    {
        RG_DEBUG << "failed to contact RG sequencer" << endl;
    }
}


void
StudioControl::sendRPN(Rosegarden::InstrumentId instrumentId,
                       Rosegarden::MidiByte paramMSB,
                       Rosegarden::MidiByte paramLSB,
                       Rosegarden::MidiByte controller,
                       Rosegarden::MidiByte value)
{
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    paramMSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     paramLSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     6, // data value changed
                                     value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE =  new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    Rosegarden::MidiMaxValue); // null
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     Rosegarden::MidiMaxValue); // null
    mC.insert(mE);


    StudioControl::sendMappedComposition(mC);
}

void
StudioControl::sendNRPN(Rosegarden::InstrumentId instrumentId,
                        Rosegarden::MidiByte paramMSB,
                        Rosegarden::MidiByte paramLSB,
                        Rosegarden::MidiByte controller,
                        Rosegarden::MidiByte value)
{
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE =
        new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_NRPN_2,
                                    paramMSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_NRPN_1,
                                     paramLSB);
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     6, // data value changed
                                     value);
    mC.insert(mE);


    // Null the controller using - this is "best practice"
    //
    mE =  new Rosegarden::MappedEvent(instrumentId,
                                    Rosegarden::MappedEvent::MidiController,
                                    Rosegarden::MIDI_CONTROLLER_RPN_2,
                                    Rosegarden::MidiMaxValue); // null
    mC.insert(mE);

    mE = new Rosegarden::MappedEvent(instrumentId,
                                     Rosegarden::MappedEvent::MidiController,
                                     Rosegarden::MIDI_CONTROLLER_RPN_1,
                                     Rosegarden::MidiMaxValue); // null
    mC.insert(mE);
}


};

