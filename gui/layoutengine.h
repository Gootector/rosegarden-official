/***************************************************************************
                          layoutengine.h  -  description
                             -------------------
    begin                : Thu Aug 3 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LAYOUTENGINE_H
#define LAYOUTENGINE_H

// Layout stuff
#include <algorithm>

#include "Event.h"
#include "notationelement.h"

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class LayoutEngine
{
public: 
    LayoutEngine();
    virtual ~LayoutEngine();

    unsigned int status() const { return m_status; }

protected:
    unsigned int m_status;
};

class NotationLayout : public LayoutEngine, public unary_function<NotationElement*, void>
{
public:

    void operator() (NotationElement *el) { layout(el); }

protected:
    virtual void layout(NotationElement*) = 0;
};

#endif
