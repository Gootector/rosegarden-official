
/*
    Rosegarden-4 v0.1
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

#include "rulerscale.h"
#include "Composition.h"

using Rosegarden::Composition;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;


SimpleRulerScale::SimpleRulerScale(Composition *composition,
				   double origin, double ratio) :
    m_composition(composition),
    m_origin(origin),
    m_ratio(ratio)
{
    // nothing
}

SimpleRulerScale::~SimpleRulerScale()
{
    // nothing
}

double
SimpleRulerScale::getBarPosition(int n)
{
    timeT barStart = m_composition->getBarRange(n, false).first;
    return getXForTime(barStart);
}

double
SimpleRulerScale::getBarWidth(int n)
{
    std::pair<timeT, timeT> range = m_composition->getBarRange(n, false);
    return (double)(range.second - range.first) / m_ratio;
}

double
SimpleRulerScale::getBeatWidth(int n)
{
    bool isNew;
    TimeSignature timeSig(m_composition->getTimeSignatureInBar(n, isNew));
    return (double)(timeSig.getBeatDuration()) / m_ratio;
}

int
SimpleRulerScale::getBarForX(double x)
{
    return m_composition->getBarNumber(getTimeForX(x), false);
}

timeT
SimpleRulerScale::getTimeForX(double x)
{
    return (timeT)((x - m_origin) * m_ratio);
}

double
SimpleRulerScale::getXForTime(timeT time)
{
    return m_origin + (double)time / m_ratio;
}

