// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.2
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

#include <iostream>
#include <dlfcn.h>

#include <qdir.h>

#include "MappedStudio.h"
#include "Sequencer.h"

namespace Rosegarden
{

// Define our object properties - these can be queried and set.
//

// General things
//
const MappedObjectProperty MappedObject::Name = "name";
const MappedObjectProperty MappedObject::Instrument = "instrument";
const MappedObjectProperty MappedObject::Position = "position";

const MappedObjectProperty MappedAudioFader::FaderLevel = "faderLevel";

const MappedObjectProperty MappedAudioPluginManager::Plugins = "plugins";
const MappedObjectProperty MappedAudioPluginManager::PluginIds = "pluginids";

#ifdef HAVE_LADSPA

const MappedObjectProperty MappedLADSPAPlugin::UniqueId = "uniqueId";
const MappedObjectProperty MappedLADSPAPlugin::PluginName = "pluginname";
const MappedObjectProperty MappedLADSPAPlugin::Label = "label";
const MappedObjectProperty MappedLADSPAPlugin::Author = "author";
const MappedObjectProperty MappedLADSPAPlugin::Copyright = "copyright";
const MappedObjectProperty MappedLADSPAPlugin::PortCount = "portcount";
const MappedObjectProperty MappedLADSPAPlugin::Ports = "ports";

const MappedObjectProperty MappedLADSPAPort::Descriptor = "descriptor";
const MappedObjectProperty MappedLADSPAPort::RangeHint = "rangehint";
const MappedObjectProperty MappedLADSPAPort::RangeLower = "rangelower";
const MappedObjectProperty MappedLADSPAPort::RangeUpper = "rangeupper";
const MappedObjectProperty MappedLADSPAPort::Value = "value";
const MappedObjectProperty MappedLADSPAPort::PortNumber = "portNumber";

#endif // HAVE_LADSPA


// --------- MappedObject ---------
//

void
MappedObject::addChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        if ((*it) == object)
            return;

    m_children.push_back(object);
}

void
MappedObject::removeChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
    {
        if ((*it) == object)
        {
            m_children.erase(it);
            return;
        }
    }
}

// Return all child ids
//
MappedObjectPropertyList
MappedObject::getChildren()
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        list.push_back(QString("%1").arg((*it)->getId()));

    return list;
}


// Return all child ids of a certain type
//
MappedObjectPropertyList
MappedObject::getChildren(MappedObjectType type)
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
    {
        if ((*it)->getType() == type)
            list.push_back(QString("%1").arg((*it)->getId()));
    }

    return list;
}

void
MappedObject::clearChildren()
{
    // remove references from the studio as well as from the object
    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio*>(studioObject))
        studioObject = studioObject->getParent();

    MappedStudio *studio = dynamic_cast<MappedStudio*>(studioObject);

    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        studio->destroyObject(*it); // remove from studio and destroy

    m_children.erase(m_children.begin(), m_children.end());

}

void
MappedObject::clone(MappedObject *object)
{
    object->clearChildren();

    // If we have children then create new versions and clone then
    //
    if (m_children.size())
    {
        MappedObject *studio = getParent();
        while (!dynamic_cast<MappedStudio*>(studio))
            studio = studio->getParent();

        std::vector<MappedObject*>::iterator it = m_children.begin();
        for (; it != m_children.end(); it++)
        {
            MappedObject *child =
                dynamic_cast<MappedStudio*>(studio)
                    ->createObject((*it)->getType(), false);
            object->addChild(child);
            std::cout << "ADD CHILD" << endl;
            (*it)->clone(child);
        }
    }
    else
        std::cerr << "NO CHILDREN TO CLONE" << endl;
}



// ------- MappedStudio -------
//

MappedStudio::MappedStudio():MappedObject(0,
                                          "MappedStudio",
                                          Studio,
                                          0,
                                          true,
                                          true),
                             m_runningObjectId(1)
{
}

MappedStudio::~MappedStudio()
{
    std::cout << "MappedStudio::~MappedStudio" << std::endl;
    clear();
}


// Object factory
// 
MappedObject*
MappedStudio::createObject(MappedObjectType type,
                           bool readOnly)
{
    MappedObject *mO = 0;

    // Ensure we've got an empty slot
    //
    while(getObject(m_runningObjectId))
        m_runningObjectId++;

    mO = createObject(type, m_runningObjectId, readOnly);

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type,
                           MappedObjectId id,
                           bool readOnly)
{
    // fail if the object already exists and it's not zero
    if (id != 0 && getObject(id)) return 0;

    MappedObject *mO = 0;

    if (type == MappedObject::AudioPluginManager)
    {
        mO = new MappedAudioPluginManager(this, id, readOnly);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(this,
                                  id,
                                  readOnly,
                                  2); // channels

        // push to the studio's child stack
        addChild(mO);
    }
    
#ifdef HAVE_LADSPA

    else if (type == MappedObject::LADSPAPlugin)
    {
        // create plugins under the pluginmanager if it exists
        ///
        MappedObject *mLP =
            getObjectOfType(MappedObject::AudioPluginManager);

        mO = new MappedLADSPAPlugin(mLP, id, readOnly);

        // push to the plugin manager's child stack
        mLP->addChild(mO);
    }
    else if (type == MappedObject::LADSPAPort)
    {
        // reset the parent after creation outside this method
        mO = new MappedLADSPAPort(this, id, readOnly);
    }

#endif // HAVE_LADSPA

    // Insert
    if (mO)
    {
        m_objects.push_back(mO);
    }

    return mO;
}

MappedObject*
MappedStudio::getObjectOfType(MappedObjectType type)
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
        if ((*it)->getType() == type)
            return (*it);
    return 0;
}


bool
MappedStudio::destroyObject(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
    {
        if ((*it) == object)
        {
            delete (*it);
            m_objects.erase(it);
            return true;
        }
    }
   
    return false;
}

bool
MappedStudio::destroyObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            delete (*it);
            m_objects.erase(it);
            return true;
        }
    }
   
    return false;
}

bool
MappedStudio::connectInstrument(InstrumentId /*iId*/, MappedObjectId /*msId*/)
{
    return true;
}

bool
MappedStudio::connectObjects(MappedObjectId /*mId1*/, MappedObjectId /*mId2*/)
{
    return true;
}

// Clear down the whole studio
//
void
MappedStudio::clear()
{
    std::vector<MappedObject*>::iterator it;
    for (it = m_objects.begin(); it != m_objects.end(); it++)
        delete (*it);

    m_objects.erase(m_objects.begin(), m_objects.end());

}

MappedObjectPropertyList
MappedStudio::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        // something
    }

    return list;
}


MappedObject*
MappedStudio::getObject(MappedObjectId id)
{
    std::vector<MappedObject*>::iterator it;

    for (it = m_objects.begin(); it != m_objects.end(); it++)
        if ((*it)->getId() == id)
            return (*it);

    return 0;
}

MappedObject*
MappedStudio::getFirst(MappedObjectType type)
{
    return getObjectOfType(type);
}

MappedObject*
MappedStudio::getNext(MappedObject *object)
{
    MappedObjectType type = Studio;

    std::vector<MappedObject*>::iterator it = m_objects.begin();

    while (it != m_objects.end())
    {
        if (object->getId() == (*it)->getId())
        {
            type = (*it)->getType();
            it++;
            break;
        }
        it++;
    }

    for (; it != m_objects.end(); it++)
    {
        if (type == (*it)->getType())
            return (*it);
    }

    return 0;
}

void
MappedStudio::setProperty(const MappedObjectProperty &property,
                          MappedObjectValue /*value*/)
{
    if (property == "")
    {
    }

}

// Remove all non read-only objects from the studio
//
void
MappedStudio::clearTemporaries()
{
    std::vector<MappedObject*>::iterator it = m_objects.begin();
    std::vector<std::vector<MappedObject*>::iterator> dead;

    while (it != m_objects.end())
    {
        if (!(*it)->isReadOnly())
        {
            delete (*it);
            dead.push_back(it);
        }

        it++;
    }

    std::vector<std::vector<MappedObject*>::iterator>::iterator dIt;
    dIt = dead.end();

    if (dIt != dead.begin())
    {
        std::cout << "MappedStudio::clearTemporaries - "
                  << "clearing " << dead.size() << " temporary objects"
                  << std::endl;
        do
        {
            dIt--;
            m_objects.erase(*dIt);
    
        } while(dIt != dead.begin());
    }

}

MappedObject*
MappedStudio::getAudioFader(Rosegarden::InstrumentId id)
{
    std::vector<MappedObject*>::iterator it = m_objects.begin();
    std::vector<std::vector<MappedObject*>::iterator> dead;

    while (it != m_objects.end())
    {
        if ((*it)->getType() == MappedObject::AudioFader)
        {
            // compare InstrumentId
            //
            MappedObjectPropertyList list = (*it)->
                getPropertyList(MappedObject::Instrument);

            if (Rosegarden::InstrumentId(list[0].toInt()) == id)
                return (*it);
        }
        it++;
    }

    return 0;

}

MappedObject*
MappedStudio::getPluginInstance(Rosegarden::InstrumentId id,
                                int position)
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));

    if (pm)
        return pm->getPluginInstance(id, position);

    return 0;
}

#ifdef HAVE_LADSPA

const LADSPA_Descriptor*
MappedStudio::createPluginDescriptor(unsigned long uniqueId)
{
    std::cout << "MappedStudio::createPluginInstance of id = "
              << uniqueId << std::endl;

    // Get the pluginmanager
    //
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
    {
        std::cout << "MappedStudio::createPluginInstance - "
                  << "getting plugin descriptor" << std::endl;
        return pm->getPluginDescriptor(uniqueId);
    }
    else
    {
        std::cout << "MappedStudio::createPluginInstance - "
                  << "plugin manager not found" << std::endl;
        return 0;
    }
}

void
MappedStudio::unloadPlugin(unsigned long uniqueId)
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
        pm->unloadPlugin(uniqueId);
}

void
MappedStudio::unloadAllPluginLibraries()
{
    MappedAudioPluginManager *pm =
        dynamic_cast<MappedAudioPluginManager*>
            (getObjectOfType(AudioPluginManager));
    if (pm)
        pm->unloadAllPluginLibraries();
}


void
MappedStudio::setPluginInstancePort(InstrumentId id,
                                    int position,
                                    unsigned long portNumber,
                                    LADSPA_Data value)
{
    cout << "PORT NUMBER = " << portNumber << endl;

    if (m_sequencer)
    {
        m_sequencer->
            setPluginInstancePortValue(id, position, portNumber, value);
    }
} 




#endif // HAVE_LADSPA


// ------------ MappedAudioFader ----------------
//

/*
MappedObjectValue
MappedAudioFader::getLevel()
{
    return m_level;
}

void
MappedAudioFader::setLevel(MappedObjectValue param)
{
    m_level = param;
}
*/

MappedObjectPropertyList 
MappedAudioFader::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioFader::FaderLevel);
    }
    else if (property == MappedObject::Instrument)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_instrumentId));
    }
    else if (property == MappedAudioFader::FaderLevel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    }

    return list;
}

void
MappedAudioFader::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == MappedAudioFader::FaderLevel)
    {
        std::cout << "MappedAudioFader::setProperty - "
                  << "fader = " << value << std::endl;
        m_level = value;
    }
    else if (property == MappedObject::Instrument)
    {
        m_instrumentId = Rosegarden::InstrumentId(value);
    }
    else
    {
        std::cerr << "MappedAudioFader::setProperty - "
                  << "unsupported property" << std::endl;
    }


}

/*
QDataStream&
operator>>(QDataStream &dS, MappedStudio *mS)
{
    // not implemented
    mS->clear();

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, MappedStudio *mS)
{
    dS << mS->getObjects()->size();

    for (unsigned int i = 0; i < mS->getObjects()->size(); i++)
    {
        //dS << m_objects[i]->getId();
        //dS << m_objects[i]->getType();
    }

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedStudio &mS)
{
    mS.clear();

    unsigned int size = 0;
    dS >> size;
    
    //for (unsigned int i = 0; i < size; i++)

    return dS;
}


QDataStream&
operator<<(QDataStream &dS, const MappedStudio &mS)
{
    dS << mS.getObjects()->size();

    for (unsigned int i = 0; i < mS.getObjects()->size(); i++)
    {
        //dS << m_objects[i].getId();
        //dS << m_objects[i].getType();
    }

    return dS;
}
*/

// ----------------- MappedAudioPluginManager -----------------
//
MappedAudioPluginManager::MappedAudioPluginManager(
        MappedObject *parent,
        MappedObjectId id,
        bool readOnly)
    :MappedObject(parent,
                 "MappedAudioPluginManager",
                  AudioPluginManager,
                  id,
                  readOnly)
{
}

MappedAudioPluginManager::~MappedAudioPluginManager()
{
}

// If we pass no argument then return the list of plugins
//
MappedObjectPropertyList
MappedAudioPluginManager::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioPluginManager::Plugins);
        list.push_back(MappedAudioPluginManager::PluginIds);
    }

#ifdef HAVE_LADSPA

    else if (property == MappedAudioPluginManager::PluginIds)
    {


        // get the list of plugin ids
        //
        MappedStudio *studio = dynamic_cast<MappedStudio*>(m_parent);

        if (studio)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (studio->getFirst(MappedObject::LADSPAPlugin));

            while (plugin)
            {
                list.push_back(MappedObjectProperty
                            ("%1").arg(plugin->getId()));
                plugin = dynamic_cast<MappedLADSPAPlugin*>
                            (studio->getNext(plugin));
            }
        }
    }
    else if (property == MappedAudioPluginManager::Plugins)
    {

        // get a full list of plugin descriptions
        //
        MappedStudio *studio = dynamic_cast<MappedStudio*>(m_parent);

        if (studio)
        {

            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (studio->getFirst(MappedObject::LADSPAPlugin));

            while (plugin)
            {
                list.push_back(MappedObjectProperty
                            ("%1").arg(plugin->getId()));

                list.push_back(MappedObjectProperty
                        (plugin->getPluginName().c_str()));

                list.push_back(MappedObjectProperty
                        ("%1").arg(plugin->getUniqueId()));

                list.push_back(MappedObjectProperty
                        (plugin->getLabel().c_str()));

                list.push_back(MappedObjectProperty
                        (plugin->getAuthor().c_str()));

                list.push_back(MappedObjectProperty
                        (plugin->getCopyright().c_str()));

                list.push_back(MappedObjectProperty
                        ("%1").arg(plugin->getPortCount()));

                std::vector<MappedObject*> children =
                    plugin->getChildObjects();

                for (unsigned int i = 0; i < children.size(); i++)
                {
                    MappedLADSPAPort *port =
                        dynamic_cast<MappedLADSPAPort*>(children[i]);

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getId()));

                    list.push_back(MappedObjectProperty
                            (port->getPortName().c_str()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getDescriptor()));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getRangeHint().HintDescriptor));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getRangeHint().LowerBound));

                    list.push_back(MappedObjectProperty
                            ("%1").arg(port->getRangeHint().UpperBound));

                }

                plugin = dynamic_cast<MappedLADSPAPlugin*>
                            (studio->getNext(plugin));
            }
        }

    }

#endif // HAVE_LADSPA

    return list;
}

void
MappedAudioPluginManager::discoverPlugins(MappedStudio *studio)
{
    QDir dir(QString(m_path.c_str()), "*.so");
    clearPlugins(studio);

    for (unsigned int i = 0; i < dir.count(); i++ )
        enumeratePlugin(studio,
                m_path + std::string("/") + std::string(dir[i].data()));
    
}

void
MappedAudioPluginManager::clearPlugins(MappedStudio *studio)
{
    MappedObject *object;
    while ((object = studio->getObjectOfType(MappedObject::LADSPAPlugin)))
    {
        studio->destroyObject(object->getId());
    }
}


#ifdef HAVE_LADSPA

void
MappedAudioPluginManager::getenvLADSPAPath()
{
    char *path = getenv("LADSPA_PATH");

    if (!path) m_path = "";
    else m_path = std::string(path);

    // try a default value
    if (m_path == "")
        m_path = "/usr/lib/ladspa";

}

void
MappedAudioPluginManager::setLADSPAPath(const std::string &path)
{
    m_path = path;
}


void
MappedAudioPluginManager::addLADSPAPath(const std::string &path)
{
    m_path += path;
}


const LADSPA_Descriptor*
MappedAudioPluginManager::getDescriptorFromHandle(unsigned long uniqueId,
                                                  void *pluginHandle)
{
    LADSPA_Descriptor_Function descrFn = 0;
    descrFn = (LADSPA_Descriptor_Function)
                    dlsym(pluginHandle, "ladspa_descriptor");
    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;

        int index = 0;

        do
        {
            descriptor = descrFn(index);
                                                                                            if (descriptor)
            {
                if (descriptor->UniqueID == uniqueId)
                    return descriptor;
            }

            index++;

        } while (descriptor);
    }

    return 0;
}


// The LADSPAPluginHandles vector holds all the libraries we've loaded
// and their handles for re-use by plugins from the same library.  When
// we've got no plugins left in use from a library we can automatically
// unload it.
//
const LADSPA_Descriptor*
MappedAudioPluginManager::getPluginDescriptor(unsigned long uniqueId)
{
    // Find the plugin
    //
    MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(getReadOnlyPlugin(uniqueId));

    if (plugin == 0)
    {
        std::cerr << "MappedAudioPluginManager::getPluginDescriptor - "
                  << "can't find read-only plugin" << std::endl;
        return 0;
    }

    // Check if we have the handle
    //
    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
    {
        if (it->first == plugin->getLibraryName())
            return getDescriptorFromHandle(uniqueId, it->second);
    }
    

    // Now create the handle and store it
    //
    void *pluginHandle = dlopen(plugin->getLibraryName().c_str(), RTLD_LAZY);

    std::pair<std::string, void*> pluginHandlePair(plugin->getLibraryName(),
                                                   pluginHandle);
    m_pluginHandles.push_back(pluginHandlePair);

    // now generate the descriptor
    return getDescriptorFromHandle(uniqueId, pluginHandle);
}


// Potentially unload the library if all plugins within it are
// not being used.
//
void
MappedAudioPluginManager::unloadPlugin(unsigned long uniqueId)
{
    std::cout << "MappedAudioPluginManager::unloadPlugin - "
              << "unloading plugin " << uniqueId << std::endl;

    // Find the plugin
    //
    MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(getReadOnlyPlugin(uniqueId));

    if (plugin == 0)
    {
        std::cerr << "MappedAudioPluginManager::unloadPlugin - "
                  << "can't find plugin to unload" << std::endl;
        return;
    }

    void *pluginHandle = 0;

    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
    {
        if (it->first == plugin->getLibraryName())
        {
            // library is loaded (as it should be)
            pluginHandle = it->second;
            break;
        }
    }
    
    if (pluginHandle == 0)
    {
        std::cout << "MappedAudioPluginManager::unloadPlugin - "
                  << "can't find plugin library to unload" << std::endl;
        return;
    }

    // check if any of these plugins are still loaded
    //
    std::vector<unsigned long> list = getPluginsInLibrary(pluginHandle);
    std::vector<unsigned long>::iterator pIt = list.begin();

    //std::cout << list.size() << " plugins in library" << std::endl;

    for (; pIt != list.end(); pIt++)
    {
        if (getPluginInstance(*pIt, false))
            return;
    }

    /*
    std::cout << "HERE!!! - MappedAudioPluginManager::unloadPlugin - "
              << "unloading library" << std::endl;
              */

    dlclose(pluginHandle);
    m_pluginHandles.erase(it);

}

void
MappedAudioPluginManager::unloadAllPluginLibraries()
{
    LADSPAIterator it = m_pluginHandles.begin();
    for (; it != m_pluginHandles.end(); it++)
        dlclose(it->second);

    m_pluginHandles.erase(m_pluginHandles.begin(), m_pluginHandles.end());
}

std::vector<unsigned long>
MappedAudioPluginManager::getPluginsInLibrary(void *pluginHandle)
{
    std::vector<unsigned long> list;
    LADSPA_Descriptor_Function descrFn = 0;
    descrFn = (LADSPA_Descriptor_Function)
                    dlsym(pluginHandle, "ladspa_descriptor");
    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;
        int index = 0;

        do
        {
            descriptor = descrFn(index);

            if (descriptor)
            {
                list.push_back(descriptor->UniqueID);
            }
            index++;
        } while(descriptor);
    }

    return list;
}

#endif // HAVE_LADSPA


void
MappedAudioPluginManager::enumeratePlugin(MappedStudio *studio,
                                          const std::string& path)
{
#ifdef HAVE_LADSPA
    LADSPA_Descriptor_Function descrFn = 0;
    void *pluginHandle = 0;

    pluginHandle = dlopen(path.c_str(), RTLD_LAZY);

    descrFn = (LADSPA_Descriptor_Function)dlsym(pluginHandle,
                                                "ladspa_descriptor");

    if (descrFn)
    {
        const LADSPA_Descriptor *descriptor;

        int index = 0;

        do
        {
            descriptor = descrFn(index);

            if (descriptor)
            {
                // The sequencer is only interested in plugins that 
                // will be able to run in real time.
                //
                if (LADSPA_IS_HARD_RT_CAPABLE(descriptor->Properties))
                {
                    MappedLADSPAPlugin *plugin =
                        dynamic_cast<MappedLADSPAPlugin*>
                            (studio->createObject
                                 (MappedObject::LADSPAPlugin, true)); // RO

                    plugin->setLibraryName(path);
                    plugin->populate(descriptor);

                    for (unsigned long i = 0; i < descriptor->PortCount; i++)
                    {
                        MappedLADSPAPort *port = 
                            dynamic_cast<MappedLADSPAPort*>
                            (studio->createObject
                             (MappedObject::LADSPAPort, true)); // read-only

                        // tie up relationships
                        plugin->addChild(port);
                        port->setParent(plugin);

                        port->setPortName(descriptor->PortNames[i]);
                        port->setDescriptor(descriptor->PortDescriptors[i]);

                        port->setRangeHint(
                                descriptor->PortRangeHints[i].HintDescriptor,
                                descriptor->PortRangeHints[i].LowerBound,
                                descriptor->PortRangeHints[i].UpperBound);

                        port->setPortNumber(i);
                    }

                    
                    //plugin->setDescriptor(descriptor);
                }
                index++;
            }
        }
        while(descriptor);
    }

    if(dlclose(pluginHandle) != 0)
        std::cerr << "PluginManager::loadPlugin - can't unload plugin"
                  << std::endl;

#endif // HAVE_LADSPA
}

void
MappedAudioPluginManager::setProperty(const MappedObjectProperty &property,
                                      MappedObjectValue /*value*/)
{
    if (property == "")
    {
    }
}

MappedObject*
MappedAudioPluginManager::getReadOnlyPlugin(unsigned long uniqueId)
{
    return getPluginInstance(uniqueId, true);
}

MappedObject*
MappedAudioPluginManager::getPluginInstance(unsigned long uniqueId,
                                            bool readOnly)
{
#ifdef HAVE_LADSPA
    std::vector<MappedObject*>::iterator it = m_children.begin();

    for(; it != m_children.end(); it++)
    {
        if ((*it)->getType() == LADSPAPlugin &&
            (*it)->isReadOnly() == readOnly)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(*it);

            if (plugin->getUniqueId() == uniqueId)
                return *it;
        }
    }

#endif // HAVE_LADSPA

    return 0;
}

MappedObject*
MappedAudioPluginManager::getPluginInstance(InstrumentId instrument,
                                            int position)
{
#ifdef HAVE_LADSPA
    std::vector<MappedObject*>::iterator it = m_children.begin();

    for(; it != m_children.end(); it++)
    {
        if ((*it)->getType() == LADSPAPlugin)
        {
            MappedLADSPAPlugin *plugin =
                dynamic_cast<MappedLADSPAPlugin*>(*it);

            if(plugin->getInstrument() == instrument &&
               plugin->getPosition() == position)
                return *it;
        }
    }

#endif // HAVE_LADSPA

    return 0;
}





//  ------------------ MappedLADSPAPlugin ------------------
//
//

#ifdef HAVE_LADSPA
void
MappedLADSPAPlugin::populate(const LADSPA_Descriptor *descriptor)
{
    if (descriptor)
    {
        m_uniqueId = descriptor->UniqueID;
        m_label = descriptor->Label;
        m_author = descriptor->Maker;
        m_copyright = descriptor->Copyright;
        m_portCount = descriptor->PortCount;
        m_pluginName = descriptor->Name;
    }

}
MappedObjectPropertyList
MappedLADSPAPlugin::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;
    
    if (property == "")
    {
        // our LADSPA properties
        list.push_back(UniqueId);
        list.push_back(PluginName);
        list.push_back(Label);
        list.push_back(Author);
        list.push_back(Copyright);
        list.push_back(PortCount);
    }
    else
    {
        if (property == MappedLADSPAPlugin::UniqueId)
            list.push_back(MappedObjectProperty("%1").arg(m_uniqueId));
        else if (property == MappedLADSPAPlugin::Label)
            list.push_back(MappedObjectProperty(m_label.c_str()));
        else if (property == MappedLADSPAPlugin::PluginName)
            list.push_back(MappedObjectProperty(m_pluginName.c_str()));
        else if (property == MappedLADSPAPlugin::Author)
            list.push_back(MappedObjectProperty(m_author.c_str()));
        else if (property == MappedLADSPAPlugin::Copyright)
            list.push_back(MappedObjectProperty(m_copyright.c_str()));
        else if (property == MappedLADSPAPlugin::PortCount)
            list.push_back(MappedObjectProperty("%1").arg(m_portCount));
        else if (property == MappedObject::Instrument)
            list.push_back(MappedObjectProperty("%1").arg(m_instrument));
        else if (property == MappedObject::Position)
            list.push_back(MappedObjectProperty("%1").arg(m_position));
        else if (property == MappedLADSPAPlugin::Ports)
        {
            // list the port object ids
            /*
            MappedObjectPropertyList list = 
                getChildren(MappedObject::LADSPAPort);
            return list;
            cout << "GOT " << list.size() << " CHILDREN" << endl;
            */

            return getChildren(MappedObject::LADSPAPort);

        }
    }

    return list;

}

void
MappedLADSPAPlugin::setProperty(const MappedObjectProperty &property,
                                MappedObjectValue value)
{

    if (property == MappedLADSPAPlugin::UniqueId)
    {
        // we've already got a plugin of this type
        //
        if (m_uniqueId == ((unsigned long)value))
            return;

        m_uniqueId = (unsigned long)value;

        // Get the studio and the sequencer
        //
        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent()->getParent());
        Rosegarden::Sequencer *seq = studio->getSequencer();

        // shut down and remove the plugin instance we have running
        seq->removePluginInstance(m_instrument, m_position);

        // manufacture the ports for this plugin type
        MappedAudioPluginManager *pluginManager =
            dynamic_cast<MappedAudioPluginManager*>(getParent());

        MappedLADSPAPlugin *roPlugin =
                dynamic_cast<MappedLADSPAPlugin*>
                    (pluginManager->getReadOnlyPlugin(((unsigned long)value)));

        if (roPlugin == 0)
        {
            std::cerr << "MappedLADSPAPlugin::setProperty - "
                      << "can't get read-only copy to clone" << std::endl;
            return;
        }

        // clone the ports
        //
        roPlugin->clone(this);

        // now create the new instance
        seq->setPluginInstance(m_instrument,
                               ((unsigned long)(value)),
                               m_position);

    }
    else if (property == MappedLADSPAPlugin::Instrument)
    {
        m_instrument = Rosegarden::InstrumentId(value);
        std::cout << "MappedLADSPAPlugin::setProperty - "
                  << "setting instrument id to " << m_instrument << std::endl;
    }
    else if (property == Rosegarden::MappedObject::Position)
    {
        m_position = int(value);
        std::cout << "MappedLADSPAPlugin::setProperty - "
                  << "setting position to " << m_position << std::endl;

    }



}

void
MappedLADSPAPlugin::clone(MappedObject *object)
{
    object->clearChildren();

    // If we have children then create new versions and clone then
    //
    if (m_children.size())
    {
        std::cout << "MappedLADSPAPlugin::clone - cloning "
                  << m_children.size() << " children" << std::endl;

        MappedObject *studio = getParent();
        while(!(dynamic_cast<MappedStudio*>(studio)))
            studio = studio->getParent();

        std::vector<MappedObject*>::iterator it = m_children.begin();
        for (; it != m_children.end(); it++)
        {
            MappedObject *child =
                dynamic_cast<MappedStudio*>(studio)
                    ->createObject(LADSPAPort, false);
            object->addChild(child);
            child->setParent(object);

            (*it)->clone(child);
        }
    }
    else
        std::cerr << "MappedLADSPAPlugin::clone - " 
                  << "no children to clone" << std::endl;
}
#endif // HAVE_LADSPA


// ------------------ MappedLADSPAPort --------------------
//
//

#ifdef HAVE_LADSPA

MappedLADSPAPort::MappedLADSPAPort(MappedObject *parent,
                                   MappedObjectId id,
                                   bool readOnly):
    MappedObject(parent, "MappedAudioPluginPort", LADSPAPort, id, readOnly),
    m_value(0.0),
    m_portNumber(0)
{
}

MappedObjectPropertyList
MappedLADSPAPort::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back("name");
        list.push_back("descriptor");
        list.push_back("rangehint");
        list.push_back("rangelower");
        list.push_back("rangeupper");
    }
    else
    {
        if (property == MappedLADSPAPort::Value)
        {
            cout << "GETTING PORT VALUE" << endl;
        }

    }


    return list;
}

void
MappedLADSPAPort::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == MappedLADSPAPort::Value)
    {
        std::cout << "MappedLADSPAPort::setProperty value = "
                  << value << std::endl;
        m_value = value;

        // Gather information and tell the plugin instance that we've
        // changed
        MappedObject *studio = getParent();
        while(!(dynamic_cast<MappedStudio*>(studio)))
            studio = studio->getParent();

        MappedStudio *studioObject = dynamic_cast<MappedStudio*>(studio);
        MappedLADSPAPlugin *plugin =
            dynamic_cast<MappedLADSPAPlugin*>(getParent());

        studioObject->setPluginInstancePort(plugin->getInstrument(),
                                            plugin->getPosition(),
                                            m_portNumber,
                                            value);
    }
    else if (property == MappedLADSPAPort::PortNumber)
    {
        m_portNumber =  ((unsigned long)value);
    }

}

void
MappedLADSPAPort::clone(MappedObject *object)
{
    // Set the port number first - see the above method for an
    // explanation of why..
    //
    object->setProperty(MappedLADSPAPort::PortNumber, m_portNumber);
    object->setProperty(MappedLADSPAPort::Value, m_value);
}


#endif // HAVE_LADSPA



}


