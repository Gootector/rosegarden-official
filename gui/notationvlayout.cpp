
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "notationvlayout.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"

using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::TimeSignature;

NotationVLayout::NotationVLayout(Staff &staff, NotationElementList &elements) :
    m_staff(staff), m_notationElements(elements)
{
    // empty
}

NotationVLayout::~NotationVLayout() { }

void NotationVLayout::reset() { }

void
NotationVLayout::layout(NotationElementList::iterator from,
                        NotationElementList::iterator to)
{ 
    NotationElementList::iterator i;
    const NotePixmapFactory &npf(m_staff.getNotePixmapFactory());

    for (i = from; i != to; ++i) {

        NotationElement *el = (*i);
        el->setLayoutY(0);

        if (el->isRest()) {

            // rest pixmaps are sized so that they will be correctly
            // displayed when set to align on the top staff line
            el->setLayoutY(m_staff.yCoordOfHeight(8));

        } else if (el->isNote()) {

            Chord chord(m_notationElements, i);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
                h.push_back((*chord[j])->event()->get<Int>(P_HEIGHT_ON_STAFF));
            }
            int top = h.size()-1;

	    bool stalkUp = true;
	    if (h[top] > 4) {
		if (h[0] > 4) stalkUp = false;
		else stalkUp = (h[top] - 4) < (5 - h[0]);
	    }

	    for (unsigned int j = 0; j < chord.size(); ++j) {
		el = *chord[j];
		el->setLayoutY(m_staff.yCoordOfHeight(h[j]));

		if (!el->event()->has(P_STALK_UP))
		    el->event()->setMaybe<Bool>(P_STALK_UP, stalkUp);

		if (!el->event()->has(P_DRAW_TAIL))
		    el->event()->setMaybe<Bool>
			(P_DRAW_TAIL,
			 ((stalkUp && j == chord.size()-1) ||
			  (!stalkUp && j == 0)));

		bool beamed = false;
		(void)el->event()->get<Bool>(P_BEAMED, beamed);

		if (beamed && el->event()->get<Bool>(P_BEAM_PRIMARY_NOTE)) {

		    kdDebug(KDEBUG_AREA) << "NotationVLayout::layout: Note is a primary beamed note" << endl;

		    double gradient =
			(double)el->event()->get<Int>(P_BEAM_GRADIENT) / 100.0;

		    int myX = el->event()->get<Int>(P_BEAM_RELATIVE_X);

		    long nextX;
		    if (el->event()->get<Int>(P_BEAM_SECTION_WIDTH, nextX)) {
			nextX += myX;
		    } else {
			el->event()->setMaybe<Int>(P_BEAM_SECTION_WIDTH, 0);
			nextX = myX;
		    }

		    int startY = m_staff.yCoordOfHeight
			(el->event()->get<Int>(P_BEAM_START_HEIGHT));

		    int myY   = (int)(gradient *   myX) + startY;
		    int nextY = (int)(gradient * nextX) + startY;

		    kdDebug(KDEBUG_AREA) << "NotationVLayout::layout: myY is " << myY << ", nextY is " << nextY << endl;
		    
		    el->event()->setMaybe<Int>(P_BEAM_MY_Y, myY);
		    el->event()->setMaybe<Int>(P_BEAM_NEXT_Y, nextY);
		}
            }

            i = chord.getFinalElement();
            
        } else {

            if (el->event()->isa(Clef::EventType)) {

                // clef pixmaps are sized so that they will be
                // correctly displayed when set to align one leger
                // line above the top staff line... well, almost
                el->setLayoutY(m_staff.yCoordOfHeight(10) + 1);

            } else if (el->event()->isa(Key::EventType)) {

                el->setLayoutY(m_staff.yCoordOfHeight(12));

            } else if (el->event()->isa(TimeSignature::EventType)) {

                el->setLayoutY(m_staff.yCoordOfHeight(8) + 2);
            }
        }
    }
}

