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

#include "LADSPAPluginFactory.h"
#include <iostream>

#ifdef HAVE_LADSPA

#include <dlfcn.h>
#include <qdir.h>
#include <cmath>

#include "AudioPluginInstance.h"
#include "LADSPAPluginInstance.h"
#include "MappedStudio.h"
#include "PluginIdentifier.h"

#ifdef HAVE_LIBLRDF
#include "lrdf.h"
#endif // HAVE_LIBLRDF

namespace Rosegarden
{
 
LADSPAPluginFactory::LADSPAPluginFactory()
{
}
 
LADSPAPluginFactory::~LADSPAPluginFactory()
{
    for (std::set<RunnablePluginInstance *>::iterator i = m_instances.begin();
	 i != m_instances.end(); ++i) {
	(*i)->setFactory(0);
	delete *i;
    }
    m_instances.clear();
    unloadUnusedLibraries();
}

const std::vector<QString> &
LADSPAPluginFactory::getPluginIdentifiers() const
{
    return m_identifiers;
}

void
LADSPAPluginFactory::enumeratePlugins(MappedObjectPropertyList &list)
{
    for (std::vector<QString>::iterator i = m_identifiers.begin();
	 i != m_identifiers.end(); ++i) {

	const LADSPA_Descriptor *descriptor = getLADSPADescriptor(*i);

	if (!descriptor) {
	    std::cerr << "WARNING: LADSPAPluginFactory::enumeratePlugins: couldn't get descriptor for identifier " << *i << std::endl;
	    continue;
	}
	
	list.push_back(*i);
	list.push_back(descriptor->Name);
	list.push_back(QString("%1").arg(descriptor->UniqueID));
	list.push_back(descriptor->Label);
	list.push_back(descriptor->Maker);
	list.push_back(descriptor->Copyright);
	list.push_back("false"); // is synth
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
LADSPAPluginFactory::populatePluginSlot(QString identifier, MappedPluginSlot &slot)
{
    const LADSPA_Descriptor *descriptor = getLADSPADescriptor(identifier);

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
		    std::cerr << "WARNING: LADSPAPluginFactory::populatePluginSlot: can't find studio" << std::endl;
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

MappedObjectValue
LADSPAPluginFactory::getPortMinimum(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
	descriptor->PortRangeHints[port].HintDescriptor;

    MappedObjectValue minimum = 0.0;
    MappedObjectValue lb = descriptor->PortRangeHints[port].LowerBound;
    MappedObjectValue ub = descriptor->PortRangeHints[port].UpperBound;
		
    if (LADSPA_IS_HINT_BOUNDED_BELOW(d)) {
	minimum = lb;
    } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
	minimum = std::min(0.0, ub - 1.0);
    }
    
    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	minimum *= m_sampleRate;
    }

    return minimum;
}

MappedObjectValue
LADSPAPluginFactory::getPortMaximum(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
	descriptor->PortRangeHints[port].HintDescriptor;

    MappedObjectValue maximum = 0.0;
    MappedObjectValue lb = descriptor->PortRangeHints[port].LowerBound;
    MappedObjectValue ub = descriptor->PortRangeHints[port].UpperBound;
    
    if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
	maximum = ub;
    } else {
	maximum = lb + 1.0;
    }
    
    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	maximum *= m_sampleRate;
    }

    return maximum;
}

MappedObjectValue
LADSPAPluginFactory::getPortDefault(const LADSPA_Descriptor *descriptor, int port)
{
    if (m_portDefaults.find(descriptor->UniqueID) != 
	m_portDefaults.end()) {
	if (m_portDefaults[descriptor->UniqueID].find(port) !=
	    m_portDefaults[descriptor->UniqueID].end()) {
	    return m_portDefaults[descriptor->UniqueID][port];
	}
    }

    LADSPA_PortRangeHintDescriptor d =
	descriptor->PortRangeHints[port].HintDescriptor;
    MappedObjectValue deft;
    MappedObjectValue minimum = getPortMinimum(descriptor, port);
    MappedObjectValue maximum = getPortMaximum(descriptor, port);

    bool logarithmic = LADSPA_IS_HINT_LOGARITHMIC(d);
    
    if (!LADSPA_IS_HINT_HAS_DEFAULT(d)) {
	
	deft = minimum;
	
    } else if (LADSPA_IS_HINT_DEFAULT_MINIMUM(d)) {
	
	deft = minimum;
	
    } else if (LADSPA_IS_HINT_DEFAULT_LOW(d)) {
	
	if (logarithmic) {
	    deft = powf(10, log10(minimum) * 0.75 +
			    log10(maximum) * 0.25);
	} else {
	    deft = minimum * 0.75 + maximum * 0.25;
	}
	
    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(d)) {
	
	if (logarithmic) {
	    deft = powf(10, log10(minimum) * 0.5 +
		   	    log10(maximum) * 0.5);
	} else {
	    deft = minimum * 0.5 + maximum * 0.5;
	}
	
    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(d)) {
	
	if (logarithmic) {
	    deft = powf(10, log10(minimum) * 0.25 +
			    log10(maximum) * 0.75);
	} else {
	    deft = minimum * 0.25 + maximum * 0.75;
	}
	
    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(d)) {
	
	deft = maximum;
	
    } else if (LADSPA_IS_HINT_DEFAULT_0(d)) {
	
	deft = 0.0;
	
    } else if (LADSPA_IS_HINT_DEFAULT_1(d)) {
	
	deft = 1.0;
	
    } else if (LADSPA_IS_HINT_DEFAULT_100(d)) {
	
	deft = 100.0;
	
    } else if (LADSPA_IS_HINT_DEFAULT_440(d)) {
	
	deft = 440.0;
	
    } else {
	
	deft = minimum;
    }
    
    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
	deft *= m_sampleRate;
    }

    return deft;
}

int
LADSPAPluginFactory::getPortDisplayHint(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
	descriptor->PortRangeHints[port].HintDescriptor;
    int hint = PluginPort::NoHint;

    if (LADSPA_IS_HINT_TOGGLED(d)) hint |= PluginPort::Toggled;
    if (LADSPA_IS_HINT_INTEGER(d)) hint |= PluginPort::Integer;
    if (LADSPA_IS_HINT_LOGARITHMIC(d)) hint |= PluginPort::Logarithmic;

    return hint;
}


RunnablePluginInstance *
LADSPAPluginFactory::instantiatePlugin(QString identifier,
				       int instrument,
				       int position,
				       unsigned int sampleRate,
				       unsigned int blockSize,
				       unsigned int channels)
{
    const LADSPA_Descriptor *descriptor = getLADSPADescriptor(identifier);

    if (descriptor) {

	LADSPAPluginInstance *instance =
	    new LADSPAPluginInstance
	    (this, instrument, identifier, position, sampleRate, blockSize, channels,
	     descriptor);

	m_instances.insert(instance);

	return instance;
    }

    return 0;
}

void
LADSPAPluginFactory::releasePlugin(RunnablePluginInstance *instance,
				   QString identifier)
{
    
    if (m_instances.find(instance) == m_instances.end()) {
	std::cerr << "WARNING: LADSPAPluginFactory::releasePlugin: Not one of mine!"
		  << std::endl;
	return;
    }

    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    m_instances.erase(m_instances.find(instance));

    bool stillInUse = false;

    for (std::set<RunnablePluginInstance *>::iterator ii = m_instances.begin();
	 ii != m_instances.end(); ++ii) {
	QString itype, isoname, ilabel;
	PluginIdentifier::parseIdentifier((*ii)->getIdentifier(), itype, isoname, ilabel);
	if (isoname == soname) {
	    std::cerr << "LADSPAPluginFactory::releasePlugin: dll " << soname << " is still in use for plugin " << ilabel << std::endl;
	    stillInUse = true;
	    break;
	}
    }
    
    if (!stillInUse) {
	std::cerr << "LADSPAPluginFactory::releasePlugin: dll " << soname << " no longer in use, unloading" << std::endl;
	unloadLibrary(soname);
    }
}

const LADSPA_Descriptor *
LADSPAPluginFactory::getLADSPADescriptor(QString identifier)
{
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	loadLibrary(soname);
	if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
	    std::cerr << "WARNING: LADSPAPluginFactory::getLADSPADescriptor: loadLibrary failed for " << soname << std::endl;
	    return 0;
	}
    }

    void *libraryHandle = m_libraryHandles[soname];

    LADSPA_Descriptor_Function fn = (LADSPA_Descriptor_Function)
	dlsym(libraryHandle, "ladspa_descriptor");

    if (!fn) {
	std::cerr << "WARNING: LADSPAPluginFactory::getLADSPADescriptor: No descriptor function in library " << soname << std::endl;
	return 0;
    }

    const LADSPA_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {
	if (descriptor->Label == label) return descriptor;
	++index;
    }

    std::cerr << "WARNING: LADSPAPluginFactory::getLADSPADescriptor: No such plugin as " << label << " in library " << soname << std::endl;

    return 0;
}

void
LADSPAPluginFactory::loadLibrary(QString soName)
{
    void *libraryHandle = dlopen(soName.data(), RTLD_NOW);
    if (libraryHandle) m_libraryHandles[soName] = libraryHandle;
}

void
LADSPAPluginFactory::unloadLibrary(QString soName)
{
    LibraryHandleMap::iterator li = m_libraryHandles.find(soName);
    if (li != m_libraryHandles.end()) {
	std::cerr << "unloading " << soName << std::endl;
	dlclose(m_libraryHandles[soName]);
	m_libraryHandles.erase(li);
    }
}

void
LADSPAPluginFactory::unloadUnusedLibraries()
{
    std::vector<QString> toUnload;

    for (LibraryHandleMap::iterator i = m_libraryHandles.begin();
	 i != m_libraryHandles.end(); ++i) {

	bool stillInUse = false;

	for (std::set<RunnablePluginInstance *>::iterator ii = m_instances.begin();
	     ii != m_instances.end(); ++ii) {

	    QString itype, isoname, ilabel;
	    PluginIdentifier::parseIdentifier((*ii)->getIdentifier(), itype, isoname, ilabel);
	    if (isoname == i->first) {
		stillInUse = true;
		break;
	    }
	}

	if (!stillInUse) toUnload.push_back(i->first);
    }

    for (std::vector<QString>::iterator i = toUnload.begin();
	 i != toUnload.end(); ++i) {
	unloadLibrary(*i);
    }
}


// It is only later, after they've gone,
// I realize they have delivered a letter.
// It's a letter from my wife.  "What are you doing
// there?" my wife asks.  "Are you drinking?"
// I study the postmark for hours.  Then it, too, begins to fade.
// I hope someday to forget all this.


std::vector<QString>
LADSPAPluginFactory::getPluginPath()
{
    std::vector<QString> pathList;
    std::string path;

    char *cpath = getenv("LADSPA_PATH");
    if (cpath) path = cpath;

    if (path == "") {
	path = "/usr/local/lib/ladspa:/usr/lib/ladspa";
	char *home = getenv("HOME");
	if (home) path = std::string(home) + "/.ladspa:" + path;
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
LADSPAPluginFactory::getLRDFPath(QString &baseUri)
{
    std::vector<QString> pathList = getPluginPath();
    std::vector<QString> lrdfPaths;

    lrdfPaths.push_back("/usr/local/share/ladspa/rdf");
    lrdfPaths.push_back("/usr/share/ladspa/rdf");

    for (std::vector<QString>::iterator i = pathList.begin();
	 i != pathList.end(); ++i) {
	lrdfPaths.push_back(*i + "/rdf");
    }

    baseUri = LADSPA_BASE;
    return lrdfPaths;
}    
#endif

void
LADSPAPluginFactory::discoverPlugins()
{
    std::vector<QString> pathList = getPluginPath();

    std::cerr << "LADSPAPluginFactory::discoverPlugins - "
	      << "discovering plugins; path is ";
    for (std::vector<QString>::iterator i = pathList.begin();
	 i != pathList.end(); ++i) {
	std::cerr << "[" << *i << "] ";
    }
    std::cerr << std::endl;

#ifdef HAVE_LIBLRDF
    // Initialise liblrdf and read the description files 
    //
    lrdf_init();

    QString baseUri;
    std::vector<QString> lrdfPaths = getLRDFPath(baseUri);

    bool haveSomething = false;

    for (size_t i = 0; i < lrdfPaths.size(); ++i) {
	QDir dir(lrdfPaths[i], "*.rdf;*.rdfs");
	for (unsigned int j = 0; j < dir.count(); ++j) {
	    if (!lrdf_read_file(QString("file:" + lrdfPaths[i] + "/" + dir[j]).data())) {
		std::cerr << "LADSPAPluginFactory: read RDF file " << (lrdfPaths[i] + "/" + dir[j]) << std::endl;
		haveSomething = true;
	    }
	}
    }

    if (haveSomething) {
	generateTaxonomy(baseUri + "Plugin", "");
    }
#endif // HAVE_LIBLRDF

    for (std::vector<QString>::iterator i = pathList.begin();
	 i != pathList.end(); ++i) {

	QDir pluginDir(*i, "*.so");

	for (unsigned int j = 0; j < pluginDir.count(); ++j) {
	    discoverPlugins(QString("%1/%2").arg(*i).arg(pluginDir[j]));
	}
    }

#ifdef HAVE_LIBLRDF
    // Cleanup after the RDF library
    //
    lrdf_cleanup();
#endif // HAVE_LIBLRDF
}

void
LADSPAPluginFactory::discoverPlugins(QString soName)
{
    void *libraryHandle = dlopen(soName.data(), RTLD_LAZY);

    if (!libraryHandle) {
        std::cerr << "WARNING: LADSPAPluginFactory::discoverPlugins: couldn't dlopen "
                  << soName << " - " << dlerror() << std::endl;
        return;
    }

    LADSPA_Descriptor_Function fn = (LADSPA_Descriptor_Function)
	dlsym(libraryHandle, "ladspa_descriptor");

    if (!fn) {
	std::cerr << "WARNING: LADSPAPluginFactory::discoverPlugins: No descriptor function in " << soName << std::endl;
	return;
    }

    const LADSPA_Descriptor *descriptor = 0;
    
    int index = 0;
    while ((descriptor = fn(index))) {

#ifdef HAVE_LIBLRDF
	char *def_uri = 0;
	lrdf_defaults *defs = 0;
		
	QString category = m_taxonomy[descriptor->UniqueID];
	
	if (category == "" && descriptor->Name != 0) {
	    std::string name = descriptor->Name;
	    if (name.length() > 4 &&
		name.substr(name.length() - 4) == " VST") {
		category = "VST effects";
		m_taxonomy[descriptor->UniqueID] = category;
	    }
	}
	
	std::cerr << "Plugin id is " << descriptor->UniqueID
		  << ", category is \"" << (category ? category : QString("(none)"))
		  << "\", name is " << descriptor->Name
		  << ", label is " << descriptor->Label
		  << std::endl;
	
	def_uri = lrdf_get_default_uri(descriptor->UniqueID);
	if (def_uri) {
	    defs = lrdf_get_setting_values(def_uri);
	}

	int controlPortNumber = 1;
	
	for (unsigned long i = 0; i < descriptor->PortCount; i++) {
	    
	    if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i])) {
		
		if (def_uri && defs) {
		    
		    for (int j = 0; j < defs->count; j++) {
			if (defs->items[j].pid == controlPortNumber) {
			    std::cerr << "Default for this port (" << defs->items[j].pid << ", " << defs->items[j].label << ") is " << defs->items[j].value << "; applying this to port number " << i << " with name " << descriptor->PortNames[i] << std::endl;
			    m_portDefaults[descriptor->UniqueID][i] =
				defs->items[j].value;
			}
		    }
		}
		
		++controlPortNumber;
	    }
	}
#endif // HAVE_LIBLRDF

	QString identifier = PluginIdentifier::createIdentifier
	    ("ladspa", soName, descriptor->Label);
	m_identifiers.push_back(identifier);

	++index;
    }

    if (dlclose(libraryHandle) != 0) {
        std::cerr << "WARNING: LADSPAPluginFactory::discoverPlugins - can't unload " << libraryHandle << std::endl;
        return;
    }
}

void
LADSPAPluginFactory::generateTaxonomy(QString uri, QString base)
{
#ifdef HAVE_LIBLRDF
    lrdf_uris *uris = lrdf_get_instances(uri.data());

    if (uris != NULL) {
	for (int i = 0; i < uris->count; ++i) {
	    m_taxonomy[lrdf_get_uid(uris->items[i])] = base;
	}
	lrdf_free_uris(uris);
    }

    uris = lrdf_get_subclasses(uri.data());

    if (uris != NULL) {
	for (int i = 0; i < uris->count; ++i) {
	    char *label = lrdf_get_label(uris->items[i]);
	    generateTaxonomy(uris->items[i],
			     base + (base.length() > 0 ? " > " : "") + label);
	}
	lrdf_free_uris(uris);
    }
#endif
}
    
}

#endif // HAVE_LADSPA

