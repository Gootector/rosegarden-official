/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <sstream>

#include "ControlParameter.h"
#include "base/MidiTypes.h"

namespace Rosegarden
{

ControlParameter::ControlParameter():
    m_name("<unnamed>"),
    m_type(Rosegarden::Controller::EventType),
    m_description("<none>"),
    m_min(0),
    m_max(127),
    m_default(0),
    m_controllerValue(0),
    m_colourIndex(0),
    m_ipbPosition(-1)  // doesn't appear on IPB by default
{
}


ControlParameter::ControlParameter(const std::string &name,
                                   const std::string &type,
                                   const std::string &description,
                                   int min,
                                   int max,
                                   int def,
                                   MidiByte controllerValue,
                                   unsigned int colour,
                                   int ipbPosition):
        m_name(name),
        m_type(type),
        m_description(description),
        m_min(min),
        m_max(max),
        m_default(def),
        m_controllerValue(controllerValue),
        m_colourIndex(colour),
        m_ipbPosition(ipbPosition)
{
}


ControlParameter::ControlParameter(const ControlParameter &control):
        XmlExportable(),
        m_name(control.getName()),
        m_type(control.getType()),
        m_description(control.getDescription()),
        m_min(control.getMin()),
        m_max(control.getMax()),
        m_default(control.getDefault()),
        m_controllerValue(control.getControllerValue()),
        m_colourIndex(control.getColourIndex()),
        m_ipbPosition(control.getIPBPosition())
{
}

ControlParameter& 
ControlParameter::operator=(const ControlParameter &control)
{
    m_name = control.getName();
    m_type = control.getType();
    m_description = control.getDescription();
    m_min = control.getMin();
    m_max = control.getMax();
    m_default = control.getDefault();
    m_controllerValue = control.getControllerValue();
    m_colourIndex = control.getColourIndex();
    m_ipbPosition = control.getIPBPosition();

    return *this;
}

bool ControlParameter::operator==(const ControlParameter &control)
{
    return m_type == control.getType() &&
        m_controllerValue == control.getControllerValue() &&
        m_min == control.getMin() &&
        m_max == control.getMax();
}

bool operator<(const ControlParameter &a, const ControlParameter &b)
{
    if (a.m_type != b.m_type)
        return a.m_type < b.m_type;
    else if (a.m_controllerValue != b.m_controllerValue)
        return a.m_controllerValue < b.m_controllerValue;
    else
	return false;
}


std::string
ControlParameter::toXmlString()
{ 
    std::stringstream control;

    control << "            <control name=\"" << encode(m_name)
            << "\" type=\"" << encode(m_type)
            << "\" description=\"" << encode(m_description)
            << "\" min=\"" << m_min
            << "\" max=\"" << m_max
            << "\" default=\"" << m_default
            << "\" controllervalue=\"" << int(m_controllerValue)
            << "\" colourindex=\"" << m_colourIndex
            << "\" ipbposition=\"" << m_ipbPosition;

    control << "\"/>" << std::endl;

    return control.str();
}

}
