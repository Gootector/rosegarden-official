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

#include "DSSIPluginFactory.h"
#include <iostream>

#ifdef HAVE_DSSI

#include <dlfcn.h>
#include "AudioPluginInstance.h"
#include "DSSIPluginInstance.h"
#include "MappedStudio.h"
#include "PluginIdentifier.h"

#ifdef HAVE_LIBLRDF
#include "lrdf.h"
#endif // HAVE_LIBLRDF

namespace Rosegarden
{

DSSIPluginFactory::DSSIPluginFactory() :
    LADSPAPluginFactory()
{
    // nothing else to do
}

DSSIPluginFactory::~DSSIPluginFactory()
{
    // nothing else to do here either
}

void
DSSIPluginFactory::enumeratePlugins(MappedObjectPropertyList &list)
{
    for (std::vector<QString>::iterator i = m_identifiers.begin();
	 i != m_identifiers.end(); ++i) {

	const LADSPA_Descriptor *descriptor = getLADSPADescriptor(*i);
	if (!descriptor) continue;

	if (!descriptor) {
	    std::cerr << "WARNING: DSSIPluginFactory::enumeratePlugins: couldn't get descriptor for identifier " << *i << std::endl;
	    continue;
	}
	
	list.push_back(*i);
	list.push_back(descriptor->Name);
	list.push_back(QString("%1").arg(descriptor->UniqueID));
	list.push_back(descriptor->Label);
	list.push_back(descriptor->Maker);
	list.push_back(descriptor->Copyright);
	list.push_back("true"); // is synth
	list.push_back(m_taxonomy[descriptor->UniqueID]);
	list.push_back(QString("%1").arg(descriptor->PortCount));

	for (unsigned long p = 0; p < descriptor->PortCount; ++p) {

	    int type = 0;
	    if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[p])) {
		type |= PluginPort::Control;
	    } else {
		type |= PluginPort::Audio;
	    }
	    if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[p])) {
		type |= PluginPort::Input;
	    } else {
		type |= PluginPort::Output;
	    }

	    list.push_back(QString("%1").arg(p));
	    list.push_back(descriptor->PortNames[p]);
	    list.push_back(QString("%1").arg(type));
	    list.push_back(QString("%1").arg(getPortDisplayHint(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortMinimum(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortMaximum(descriptor, p)));
	    list.push_back(QString("%1").arg(getPortDefault(descriptor, p)));
	}
    }

    unloadUnusedLibraries();
}
	

void
DSSIPluginFactory::populatePluginSlot(QString identifier, MappedPluginSlot &slot)
{
    const LADSPA_Descriptor *descriptor = getLADSPADescriptor(identifier);
    if (!descriptor) return;

    if (descriptor) {
	
	slot.setProperty(MappedPluginSlot::Label, descriptor->Label);
	slot.setProperty(MappedPluginSlot::PluginName, descriptor->Name);
	slot.setProperty(MappedPluginSlot::Author, descriptor->Maker);
	slot.setProperty(MappedPluginSlot::Copyright, descriptor->Copyright);
	slot.setProperty(MappedPluginSlot::PortCount, descriptor->PortCount);
	slot.setProperty(MappedPluginSlot::Category, m_taxonomy[descriptor->UniqueID]);

	slot.destroyChildren();
	
	for (unsigned long i = 0; i < descriptor->PortCount; i++) {

	    if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i]) &&
		LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {

		MappedStudio *studio = dynamic_cast<MappedStudio *>(slot.getParent());
		if (!studio) {
		    std::cerr << "WARNING: DSSIPluginFactory::populatePluginSlot: can't find studio" << std::endl;
		    return;
		}

		MappedPluginPort *port = 
		    dynamic_cast<MappedPluginPort *>
		    (studio->createObject(MappedObject::PluginPort));
		    
		slot.addChild(port);
		port->setParent(&slot);
		
		port->setProperty(MappedPluginPort::PortNumber, i);
		port->setProperty(MappedPluginPort::Name,
				  descriptor->PortNames[i]);
		port->setProperty(MappedPluginPort::Maximum,
				  getPortMaximum(descriptor, i));
		port->setProperty(MappedPluginPort::Minimum,
				  getPortMinimum(descriptor, i));
		port->setProperty(MappedPluginPort::Default,
				  getPortDefault(descriptor, i));
		port->setProperty(MappedPluginPort::DisplayHint,
				  getPortDisplayHint(descriptor, i));
	    }
	}
    }
    
    //!!! leak here if the plugin is not instantiated too...?
}

RunnablePluginInstance *
DSSIPluginFactory::instantiatePlugin(QString identifier,
				     int instrument,
				     int position,
				     unsigned int sampleRate,
				     unsigned int blockSize,
				     unsigned int channels)
{
    const DSSI_Descriptor *descriptor = getDSSIDescriptor(identifier);

    if (descriptor) {

	DSSIPluginInstance *instance =
	    new DSSIPluginInstance
	    (this, instrument, identifier, position, sampleRate, blockSize, channels,
	     descriptor);

	m_instances.insert(instance);

	return instance;
    }

    return 0;
}


const DSSI_Descriptor *
DSSIPluginFactory::getDSSIDescriptor(QString identifier)
{
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	loadLibrary(soname);
	if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	    std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: loadLibrary failed for " << soname << std::endl;
	    return 0;
	}
    }

    void *libraryHandle = m_libraryHandles[soname];

    DSSI_Descriptor_Function fn = (DSSI_Descriptor_Function)
	dlsym(libraryHandle, "dssi_descriptor");

    if (!fn) {
	std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: No descriptor function in library " << soname << std::endl;
	return 0;
    }

    const DSSI_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {
	if (descriptor->LADSPA_Plugin->Label == label) return descriptor;
	++index;
    }

    std::cerr << "WARNING: DSSIPluginFactory::getDSSIDescriptor: No such plugin as " << label << " in library " << soname << std::endl;

    return 0;
}

const LADSPA_Descriptor *
DSSIPluginFactory::getLADSPADescriptor(QString identifier)
{
    const DSSI_Descriptor *dssiDescriptor = getDSSIDescriptor(identifier);
    if (dssiDescriptor) return dssiDescriptor->LADSPA_Plugin;
    else return 0;
}


std::vector<QString>
DSSIPluginFactory::getPluginPath()
{
    std::vector<QString> pathList;
    std::string path;

    char *cpath = getenv("DSSI_PATH");
    if (cpath) path = cpath;

    if (path == "") {
	path = "/usr/local/lib/dssi:/usr/lib/dssi";
	char *home = getenv("HOME");
	if (home) path = std::string(home) + "/.dssi:" + path;
    }

    std::string::size_type index = 0, newindex = 0;

    while ((newindex = path.find(':', index)) < path.size()) {
	pathList.push_back(path.substr(index, newindex - index).c_str());
	index = newindex + 1;
    }
    
    pathList.push_back(path.substr(index).c_str());

    return pathList;
}


#ifdef HAVE_LIBLRDF
std::vector<QString>
DSSIPluginFactory::getLRDFPath(QString &baseUri)
{
    std::vector<QString> pathList = getPluginPath();
    std::vector<QString> lrdfPaths;

    lrdfPaths.push_back("/usr/local/share/dssi/rdf");
    lrdfPaths.push_back("/usr/share/dssi/rdf");

    lrdfPaths.push_back("/usr/local/share/ladspa/rdf");
    lrdfPaths.push_back("/usr/share/ladspa/rdf");

    for (std::vector<QString>::iterator i = pathList.begin();
	 i != pathList.end(); ++i) {
	lrdfPaths.push_back(*i + "/rdf");
    }

#ifdef DSSI_BASE
    baseUri = DSSI_BASE;
#else
    baseUri = "http://dssi.sourceforge.net/ontology#";
#endif

    return lrdfPaths;
}    
#endif


void
DSSIPluginFactory::discoverPlugins(QString soName)
{
    void *libraryHandle = dlopen(soName.data(), RTLD_LAZY);

    if (!libraryHandle) {
        std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: couldn't dlopen "
                  << soName << " - " << dlerror() << std::endl;
        return;
    }

    DSSI_Descriptor_Function fn = (DSSI_Descriptor_Function)
	dlsym(libraryHandle, "dssi_descriptor");

    if (!fn) {
	std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: No descriptor function in " << soName << std::endl;
	return;
    }

    const DSSI_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {

	const LADSPA_Descriptor *ladspaDescriptor = descriptor->LADSPA_Plugin;
	if (!ladspaDescriptor) {
	    std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins: No LADSPA descriptor for plugin " << index << " in " << soName << std::endl;
	    ++index;
	    continue;
	}

#ifdef HAVE_LIBLRDF
	char *def_uri = 0;
	lrdf_defaults *defs = 0;
		
	if (!m_taxonomy.empty()) {

	    QString category = m_taxonomy[ladspaDescriptor->UniqueID];

	    std::cerr << "Plugin id is " << ladspaDescriptor->UniqueID
		      << ", category is \"" << (category ? category : QString("(none)"))
		      << "\", name is " << ladspaDescriptor->Name
		      << ", label is " << ladspaDescriptor->Label
		      << std::endl;

	    def_uri = lrdf_get_default_uri(ladspaDescriptor->UniqueID);
	    if (def_uri) {
		defs = lrdf_get_setting_values(def_uri);
	    }

	    int controlPortNumber = 1;
		
	    for (unsigned long i = 0; i < ladspaDescriptor->PortCount; i++) {

		if (LADSPA_IS_PORT_CONTROL(ladspaDescriptor->PortDescriptors[i])) {

		    if (def_uri) {

			for (int j = 0; j < defs->count; j++) {
			    if (defs->items[j].pid == controlPortNumber) {
				    std::cerr << "Default for this port (" << defs->items[j].pid << ", " << defs->items[j].label << ") is " << defs->items[j].value << "; applying this to port number " << i << " with name " << ladspaDescriptor->PortNames[i] << std::endl;
				m_portDefaults[ladspaDescriptor->UniqueID][i] =
				    defs->items[j].value;
			    }
			}
		    }

		    ++controlPortNumber;
		}
	    }
	}
#endif // HAVE_LIBLRDF

	QString identifier = PluginIdentifier::createIdentifier
	    ("dssi", soName, ladspaDescriptor->Label);
	m_identifiers.push_back(identifier);

	++index;
    }

    if (dlclose(libraryHandle) != 0) {
        std::cerr << "WARNING: DSSIPluginFactory::discoverPlugins - can't unload " << libraryHandle << std::endl;
        return;
    }
}

    
}

#endif // HAVE_DSSI

