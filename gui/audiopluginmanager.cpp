// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include "rgapplication.h"
#include "audiopluginmanager.h"
#include "rosegardendcop.h"
#include "rosestrings.h"

#include "PluginIdentifier.h"

namespace Rosegarden
{

// ---------------- AudioPluginManager -------------------
//
//
AudioPluginManager::AudioPluginManager():m_sampleRate(0)
{
    // fetch from sequencer
    fetchSampleRate();

    // Clear the plugin clipboard
    //
    m_pluginClipboard.m_pluginNumber = -1;
    m_pluginClipboard.m_program = "";
    m_pluginClipboard.m_controlValues.clear();

}

AudioPlugin*
AudioPluginManager::addPlugin(const QString &identifier,
                              const QString &name,
                              unsigned long uniqueId,
                              const QString &label,
                              const QString &author,
                              const QString &copyright,
			      bool isSynth,
			      bool isGrouped,
			      const QString &category)
{
    AudioPlugin *newPlugin = new AudioPlugin(identifier,
                                             name,
                                             uniqueId,
                                             label,
                                             author,
                                             copyright,
					     isSynth,
					     isGrouped,
					     category);
    m_plugins.push_back(newPlugin);

    return newPlugin;
}

bool
AudioPluginManager::removePlugin(const QString &identifier)
{
    std::vector<AudioPlugin*>::iterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getIdentifier() == identifier)
        {
            delete *it;
            m_plugins.erase(it);
            return true;
        }
    }

    return false;
}

std::vector<QString>
AudioPluginManager::getPluginNames()
{
    std::vector<QString> names;

    PluginIterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
	names.push_back((*it)->getName());

    return names;
}

AudioPlugin* 
AudioPluginManager::getPlugin(int number)
{
    if (number < 0 || number > (int(m_plugins.size()) - 1))
        return 0;

    return m_plugins[number];
}

int
AudioPluginManager::getPositionByIdentifier(QString identifier)
{
    int pos = 0;
    PluginIterator it = m_plugins.begin();

    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getIdentifier() == identifier)
            return pos;

        pos++;
    }

    pos = 0;
    it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if (PluginIdentifier::areIdentifiersSimilar((*it)->getIdentifier(), identifier))
            return pos;

        pos++;
    }

    return -1;
}

AudioPlugin*
AudioPluginManager::getPluginByIdentifier(QString identifier)
{
    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getIdentifier() == identifier)
            return (*it);
    }

    it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if (PluginIdentifier::areIdentifiersSimilar((*it)->getIdentifier(), identifier))
            return (*it);
    }

    return 0;
}

AudioPlugin*
AudioPluginManager::getPluginByUniqueId(unsigned long uniqueId)
{
    PluginIterator it = m_plugins.begin();
    for (; it != m_plugins.end(); ++it)
    {
        if ((*it)->getUniqueId() == uniqueId)
            return (*it);
    }

    return 0;
}


void
AudioPluginManager::fetchSampleRate()
{
    QCString replyType;
    QByteArray replyData;

    if (rgapp->sequencerCall("getSampleRate()", replyType, replyData)) {

        QDataStream streamIn(replyData, IO_ReadOnly);
        unsigned int result;
        streamIn >> result;
        m_sampleRate = result;
    }
}




// ----------------- AudioPlugin ---------------------
//
//
AudioPlugin::AudioPlugin(const QString &identifier,
                         const QString &name,
                         unsigned long uniqueId,
                         const QString &label,
                         const QString &author,
                         const QString &copyright,
			 bool isSynth,
			 bool isGrouped,
			 const QString &category):
    m_identifier(identifier),
    m_name(name),
    m_uniqueId(uniqueId),
    m_label(label),
    m_author(author),
    m_copyright(copyright),
    m_isSynth(isSynth),
    m_isGrouped(isGrouped),
    m_category(category),
    m_colour(Qt::darkRed)
{
}

void
AudioPlugin::addPort(int number,
		     const QString &name,
                     PluginPort::PortType type,
                     PluginPort::PortDisplayHint hint,
                     PortData lowerBound,
                     PortData upperBound,
		     PortData defaultValue)
{
    PluginPort *port = new PluginPort(number,
                                      qstrtostr(name),
                                      type,
                                      hint,
                                      lowerBound,
                                      upperBound,
				      defaultValue);
    m_ports.push_back(port);

}


}


