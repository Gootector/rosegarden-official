/***************************************************************************
                          staff.h  -  description
                             -------------------
    begin                : Mon Jun 19 2000
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

#ifndef STAFF_H
#define STAFF_H

#include <vector>

#include "qcanvasitemgroup.h"

/**A Staff (treble and bass clef + lines) as displayed on screen.

  *@author Guillaume Laurent
  */

class Staff : public QCanvasItemGroup  {
public:
    enum Clef { Treble, Bass, Alto, Tenor };
    
    Staff(QCanvas*, Clef clef = Treble);

    /**
     * Returns the Y offset at which a note with pitch 'pitch'
     * should be displayed on this staff
     */
    int pitchYOffset(unsigned int pitch) const;

    static const unsigned int noteHeight;
    static const unsigned int noteWidth;
    static const unsigned int lineWidth;
    static const unsigned int stalkLen;
    static const unsigned int nbLines;
    static const unsigned int linesOffset;
protected:

    Clef m_currentKey;

    vector<int> m_pitchToHeight;
};

#endif
