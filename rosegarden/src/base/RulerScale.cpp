
/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include <cmath>
#include "RulerScale.h"
#include "Composition.h"

namespace Rosegarden {


//////////////////////////////////////////////////////////////////////
//                 RulerScale
//////////////////////////////////////////////////////////////////////

RulerScale::RulerScale(Composition *c) :
    m_composition(c)
{ 
    // nothing
}

RulerScale::~RulerScale()
{
    // nothing
}

int
RulerScale::getFirstVisibleBar() const
{
    return m_composition->getBarNumber(m_composition->getStartMarker());
}

int
RulerScale::getLastVisibleBar() const
{
    return m_composition->getBarNumber(m_composition->getEndMarker());
}

double
RulerScale::getBarWidth(int n) const
{
    return getBarPosition(n + 1) - getBarPosition(n);
}

double
RulerScale::getBeatWidth(int n) const
{
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;
    if (barDuration == 0) return 0;

    bool isNew;
    TimeSignature timeSig = m_composition->getTimeSignatureInBar(n, isNew);

    // cope with partial bars
    double theoreticalWidth =
	(getBarWidth(n) * timeSig.getBarDuration()) / barDuration;

    return theoreticalWidth / timeSig.getBeatsPerBar();
}

int
RulerScale::getBarForX(double x) const
{
    // binary search

    int minBar = getFirstVisibleBar(),
	maxBar = getLastVisibleBar();
    
    while (maxBar > minBar) {
	int middle = minBar + (maxBar - minBar) / 2;
	if (x > getBarPosition(middle)) minBar = middle + 1;
	else maxBar = middle;
    }

    // we've just done equivalent of lower_bound -- we're one bar too
    // far into the list

    if (minBar > getFirstVisibleBar()) return minBar - 1;
    else return minBar;
}

timeT
RulerScale::getTimeForX(double x) const
{
    int n = getBarForX(x);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);

    if (barWidth < 1.0) {

	return barRange.first;

    } else {

	timeT barDuration = barRange.second - barRange.first;
	x -= getBarPosition(n);

	return barRange.first + (timeT)nearbyint(((double)(x * barDuration) / barWidth));
    }
}

double
RulerScale::getXForTime(timeT time) const
{
    int n = m_composition->getBarNumber(time);

    double barWidth = getBarWidth(n);
    std::pair<timeT, timeT> barRange = m_composition->getBarRange(n);
    timeT barDuration = barRange.second - barRange.first;

    if (barDuration == 0) {

	return getBarPosition(n);

    } else {

	time -= barRange.first;
	return getBarPosition(n) + (double)(time * barWidth) / barDuration;
    }
}

timeT
RulerScale::getDurationForWidth(double x, double width) const
{
    return getTimeForX(x + width) - getTimeForX(x);
}

double
RulerScale::getWidthForDuration(timeT startTime, timeT duration) const
{
    return getXForTime(startTime + duration) - getXForTime(startTime);
}

double
RulerScale::getTotalWidth() const
{
    int n = getLastVisibleBar();
    return getBarPosition(n) + getBarWidth(n);
}




//////////////////////////////////////////////////////////////////////
//                 SimpleRulerScale
//////////////////////////////////////////////////////////////////////


SimpleRulerScale::SimpleRulerScale(Composition *composition,
				   double origin, double ratio) :
    RulerScale(composition),
    m_origin(origin),
    m_ratio(ratio),
    m_firstBar(0)
{
    // nothing
}

SimpleRulerScale::SimpleRulerScale(const SimpleRulerScale &ruler):
    RulerScale(ruler.getComposition()),
    m_origin(ruler.getOrigin()),
    m_ratio(ruler.getUnitsPerPixel()),
    m_firstBar(ruler.getFirstVisibleBar())
{
    // nothing
}


SimpleRulerScale::~SimpleRulerScale()
{
    // nothing
}

double
SimpleRulerScale::getBarPosition(int n) const
{
    timeT barStart = m_composition->getBarRange(n).first;
    return getXForTime(barStart);
}

double
SimpleRulerScale::getBarWidth(int n) const
{
    std::pair<timeT, timeT> range = m_composition->getBarRange(n);
    return (double)(range.second - range.first) / m_ratio;
}

double
SimpleRulerScale::getBeatWidth(int n) const
{
    bool isNew;
    TimeSignature timeSig(m_composition->getTimeSignatureInBar(n, isNew));
    return (double)(timeSig.getBeatDuration()) / m_ratio;
}

int
SimpleRulerScale::getBarForX(double x) const
{
    return m_composition->getBarNumber(getTimeForX(x));
}

timeT
SimpleRulerScale::getTimeForX(double x) const
{
    timeT t = (timeT)(nearbyint((double)(x - m_origin) * m_ratio));

    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	t += m_composition->getBarRange(firstBar).first;
    }

    return t;
}

double
SimpleRulerScale::getXForTime(timeT time) const
{
    int firstBar = getFirstVisibleBar();
    if (firstBar != 0) {
	time -= m_composition->getBarRange(firstBar).first;
    }

    return m_origin + (double)time / m_ratio;
}


}
