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

#include "PluginFactory.h"
#include "PluginIdentifier.h"

#ifdef HAVE_LADSPA
#include "LADSPAPluginFactory.h"
#endif

#ifdef HAVE_DSSI
#include "DSSIPluginFactory.h"
#endif

#include <iostream>

namespace Rosegarden
{

int PluginFactory::m_sampleRate = 48000;

#ifdef HAVE_LADSPA
static LADSPAPluginFactory *_ladspaInstance = 0;
#endif

#ifdef HAVE_DSSI
static LADSPAPluginFactory *_dssiInstance = 0;
#endif

PluginFactory *
PluginFactory::instance(QString pluginType)
{
    if (pluginType == "ladspa") {
#ifdef HAVE_LADSPA
	if (!_ladspaInstance) {
	    std::cerr << "PluginFactory::instance(" << pluginType
		      << "): creating new LADSPAPluginFactory" << std::endl;
	    _ladspaInstance = new LADSPAPluginFactory();
	    _ladspaInstance->discoverPlugins();
	}
	return _ladspaInstance;
#else
	return 0;
#endif
    } else if (pluginType == "dssi") {
#ifdef HAVE_DSSI
	if (!_dssiInstance) {
	    std::cerr << "PluginFactory::instance(" << pluginType
		      << "): creating new DSSIPluginFactory" << std::endl;
	    _dssiInstance = new DSSIPluginFactory();
	    _dssiInstance->discoverPlugins();
	}
	return _dssiInstance;
#else
	return 0;
#endif
    }
	
    else return 0;
}

PluginFactory *
PluginFactory::instanceFor(QString identifier)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    return instance(type);
}

void
PluginFactory::enumerateAllPlugins(MappedObjectPropertyList &list)
{
    PluginFactory *factory;

    factory = instance("ladspa");
    if (factory) factory->enumeratePlugins(list);

    factory = instance("dssi");
    if (factory) factory->enumeratePlugins(list);
}


}

