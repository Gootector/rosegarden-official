// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "notationstaff.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "Composition.h"
#include "BaseProperties.h"
#include "Quantizer.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"

// for debugging purposes:
#include <kmessagebox.h>
#include <klocale.h>
#include <qstring.h>
#include <qwidget.h>


// I woke up on the last day of the year
// with the sudden realisation
// that people have brought terrible ills upon themselves by
// trying to cover the earth with fields in shapes such as squares
// which don't tesselate when mapped onto curved surfaces.
// War and famine would cease, if only we could all
// move at once onto a system of triangles.

using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::TimeSignature;
using Rosegarden::Note;
using Rosegarden::Text;
using Rosegarden::Indication;
using Rosegarden::Quantizer;
using Rosegarden::timeT;
using Rosegarden::Staff;

using namespace Rosegarden::BaseProperties;


NotationVLayout::NotationVLayout(Rosegarden::Composition *c,
				 const NotationProperties &properties,
                                 QObject* parent, const char* name) :
    ProgressReporter(parent, name),
    m_composition(c),
    m_notationQuantizer(c->getNotationQuantizer()),
    m_properties(properties)
{
    // empty
}

NotationVLayout::~NotationVLayout()
{
    // empty
}

NotationVLayout::SlurList &
NotationVLayout::getSlurList(Staff &staff)
{
    SlurListMap::iterator i = m_slurs.find(&staff);
    if (i == m_slurs.end()) {
	m_slurs[&staff] = SlurList();
    }

    return m_slurs[&staff];
}

void
NotationVLayout::reset()
{
    m_slurs.clear();
}

void
NotationVLayout::resetStaff(Staff &staff, timeT, timeT)
{
    getSlurList(staff).clear();
}

void
NotationVLayout::scanStaff(Staff &staffBase, timeT, timeT)
{
    START_TIMING;

    NotationStaff &staff = dynamic_cast<NotationStaff &>(staffBase);
    NotationElementList *notes = staff.getViewElementList();

    NotationElementList::iterator from = notes->begin();
    NotationElementList::iterator to = notes->end();
    NotationElementList::iterator i;

    for (i = from; i != to; ++i) {

        NotationElement *el = static_cast<NotationElement*>(*i);
        el->setLayoutY(0);

        if (el->isRest()) {

            // rests for notes longer than the minim have hotspots
            // aligned with the line above the middle line; the rest
            // are aligned with the middle line

            int noteType = el->event()->get<Int>(NOTE_TYPE);
            if (noteType > Note::Minim) {
                el->setLayoutY(staff.getLayoutYForHeight(6));
            } else {
                el->setLayoutY(staff.getLayoutYForHeight(4));
            }

        } else if (el->isNote()) {

            NotationChord chord(*notes, i, m_notationQuantizer, m_properties);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
		long height = 0;
		if (!(*chord[j])->event()->get<Int>
		    (m_properties.HEIGHT_ON_STAFF, height)) {
		    KMessageBox::sorry
			((QWidget *)parent(), QString(i18n("Event in chord at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*chord[j])->getViewAbsoluteTime())));
		    (*chord[j])->event()->dump(std::cerr);
		}
		h.push_back(height);
            }
	    bool stemmed = chord.hasStem();
            bool stemUp = chord.hasStemUp();
            bool hasNoteHeadShifted = chord.hasNoteHeadShifted();

            unsigned int flaggedNote = (stemUp ? chord.size() - 1 : 0);

	    for (unsigned int j = 0; j < chord.size(); ++j) {
		el = static_cast<NotationElement*>(*chord[j]);
		el->setLayoutY(staff.getLayoutYForHeight(h[j]));

		// These calculations and assignments are pretty much final
		// if the chord is not in a beamed group, but if it is then
		// they will be reworked by NotationGroup::applyBeam, which
		// is called from NotationHLayout::layout, which is called
		// after this.  Any inaccuracies here for beamed groups
		// should be stamped out there.

//                el->event()->setMaybe<Bool>(STEM_UP, stemUp);
		el->event()->setMaybe<Bool>(m_properties.VIEW_LOCAL_STEM_UP, stemUp);

		bool primary =
		    ((stemmed && stemUp) ? (j == 0) : (j == chord.size()-1));
                el->event()->setMaybe<Bool>
		    (m_properties.CHORD_PRIMARY_NOTE, primary);

                el->event()->setMaybe<Bool>
		    (m_properties.NOTE_HEAD_SHIFTED,
		     chord.isNoteHeadShifted(chord[j]));

                el->event()->setMaybe<Bool>
		    (m_properties.NEEDS_EXTRA_SHIFT_SPACE,
		     hasNoteHeadShifted && !stemUp);

                el->event()->setMaybe<Bool>
		    (m_properties.DRAW_FLAG, j == flaggedNote);

                int stemLength = -1;
                if (j != flaggedNote) {
                    stemLength = staff.getLayoutYForHeight(h[flaggedNote]) -
                        staff.getLayoutYForHeight(h[j]);
                    if (stemLength < 0) stemLength = -stemLength;
//                    NOTATION_DEBUG << "Setting stem length to "
//                                         << stemLength << endl;
                } else {
		    int minStemLength = stemLength;
		    if (h[j] < -2 && stemUp) {
			//!!! needs tuning, & applying for beamed stems too
			minStemLength = staff.getLayoutYForHeight(h[j]) -
			    staff.getLayoutYForHeight(4);
		    } else if (h[j] > 10 && !stemUp) {
			minStemLength = staff.getLayoutYForHeight(4) -
			    staff.getLayoutYForHeight(h[j]);
		    }
		    stemLength = std::max(minStemLength, stemLength);
		}

                el->event()->setMaybe<Int>
		    (m_properties.UNBEAMED_STEM_LENGTH, stemLength);
            }

            i = chord.getFinalElement();
            
        } else {

            if (el->event()->isa(Clef::EventType)) {

                // clef pixmaps have the hotspot placed to coincide
                // with the pitch of the clef -- so the alto clef
                // should be "on" the middle line, the treble clef
                // "on" the line below the middle, etc

                el->setLayoutY(staff.getLayoutYForHeight
			       (Clef(*el->event()).getAxisHeight()));

            } else if (el->event()->isa(Rosegarden::Key::EventType)) {

                el->setLayoutY(staff.getLayoutYForHeight(12));

	    } else if (el->event()->isa(Text::EventType)) {

		std::string type =
		    el->event()->get<String>(Text::TextTypePropertyName);

		if (type == Text::Dynamic ||
		    type == Text::LocalDirection ||
		    type == Text::UnspecifiedType) {
		    el->setLayoutY(staff.getLayoutYForHeight(-7));
		} else if (type == Text::Lyric ||
			   type == Text::Annotation) {
		    el->setLayoutY(staff.getLayoutYForHeight(-13));
		} else {
		    el->setLayoutY(staff.getLayoutYForHeight(22));
		}

            } else if (el->event()->isa(Indication::EventType)) {

		std::string indicationType =
		    el->event()->get<String>(Indication::IndicationTypePropertyName);

		if (indicationType == Indication::Slur) {
		    getSlurList(staff).push_back(i);
		}

		el->setLayoutY(staff.getLayoutYForHeight(-9));
	    }
        }
    }

    PRINT_ELAPSED("NotationVLayout::scanStaff");
}

void
NotationVLayout::finishLayout(timeT, timeT)
{
    START_TIMING;

    for (SlurListMap::iterator mi = m_slurs.begin();
	 mi != m_slurs.end(); ++mi) {

	for (SlurList::iterator si = mi->second.begin();
	     si != mi->second.end(); ++si) {

	    NotationElementList::iterator i = *si;
	    NotationStaff &staff = dynamic_cast<NotationStaff &>(*(mi->first));

	    positionSlur(staff, i);
	}
    }

    PRINT_ELAPSED("NotationVLayout::finishLayout");
}


void
NotationVLayout::positionSlur(NotationStaff &staff,
			      NotationElementList::iterator i)
{

    NotationElementList::iterator scooter = i;

    timeT slurDuration =
	(*i)->event()->get<Int>(Indication::IndicationDurationPropertyName);
    timeT endTime = (*i)->getViewAbsoluteTime() + slurDuration;

    bool haveStart = false;

    int startTopHeight = 4, endTopHeight = 4,
	startBottomHeight = 4, endBottomHeight = 4;

    int startX = (int)(*i)->getLayoutX(), endX = startX + 10;
    bool startStemUp = false, endStemUp = false;
    bool beamAbove = false, beamBelow = false;

    std::vector<Event *> stemUpNotes, stemDownNotes;

    // Scan the notes spanned by the slur, recording the top and
    // bottom heights of the first and last chords, plus the presence
    // of any troublesome beams.  (We should also be recording the
    // presence of notes that are high or low enough in the body to
    // get in the way of our slur -- not implemented yet.)

    while (scooter != staff.getViewElementList()->end()) {

	if ((*scooter)->getViewAbsoluteTime() >= endTime) break;

	if (static_cast<NotationElement*>(*scooter)->isNote()) {

	    long h = 0;
	    if (!(*scooter)->event()->get<Int>(m_properties.HEIGHT_ON_STAFF, h)) {
		KMessageBox::sorry
		    ((QWidget *)parent(), QString(i18n("Spanned note at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*scooter)->getViewAbsoluteTime())));
		(*scooter)->event()->dump(std::cerr);
	    }

	    bool stemUp = (h <= 4);
	    (*scooter)->event()->get<Bool>(m_properties.VIEW_LOCAL_STEM_UP, stemUp);
	    
	    bool beamed = false;
	    (*scooter)->event()->get<Bool>(m_properties.BEAMED, beamed);
	    
	    bool primary = false;

	    if ((*scooter)->event()->get<Bool>
		(m_properties.CHORD_PRIMARY_NOTE, primary) && primary) {

		NotationChord chord(*(staff.getViewElementList()), scooter,
				    m_notationQuantizer, m_properties);

		if (beamed) {
		    if (stemUp) beamAbove = true;
		    else beamBelow = true;
		} 

		if (!haveStart) {
		    startBottomHeight = chord.getLowestNoteHeight();
		    startTopHeight = chord.getHighestNoteHeight();
		    startX = (int)(*scooter)->getLayoutX();
		    startStemUp = stemUp;
		    haveStart = true;
		}

		endBottomHeight = chord.getLowestNoteHeight();
		endTopHeight = chord.getHighestNoteHeight();
		endX = (int)(*scooter)->getLayoutX();
		endStemUp = stemUp;
	    }

	    if (!beamed) {
		if (stemUp) stemUpNotes.push_back((*scooter)->event());
		else stemDownNotes.push_back((*scooter)->event());
	    }
	}

	++scooter;
    }

    bool above = true;
    if (startStemUp == endStemUp) {
	above = !startStemUp;
    } else if (beamBelow) {
	above = true;
    } else if (beamAbove) {
	above = false;
    } else if (stemUpNotes.size() != stemDownNotes.size()) {
	above = (stemUpNotes.size() < stemDownNotes.size());
    } else {
	above = ((startTopHeight - 4) + (endTopHeight - 4) +
		 (4 - startBottomHeight) + (4 - endBottomHeight) <= 8);
    }

    // re-point the stems of any notes that will otherwise interfere
    // with our slur (n.b. the only notes in the stemUpNotes and
    // stemDownNotes arrays are the ones that are not beamed, so this
    // shouldn't cause any trouble)

    std::vector<Event *> *wrongStemNotes =
	(above ? &stemUpNotes : &stemDownNotes);

    for (unsigned int wsi = 0; wsi < wrongStemNotes->size(); ++wsi) {
	(*wrongStemNotes)[wsi]->setMaybe<Bool>(m_properties.VIEW_LOCAL_STEM_UP, !above);
    }

    // now choose the actual y-coord of the slur based on the side
    // we've decided to put it on

    int startHeight, endHeight;

    if (above) {
	startHeight = startTopHeight + 3;
	endHeight = endTopHeight + 3;
    } else {
	startHeight = startBottomHeight - 3;
	endHeight = endBottomHeight - 3;
    }

    int y0 = staff.getLayoutYForHeight(startHeight),
	y1 = staff.getLayoutYForHeight(endHeight);

    // in some circumstances it pays to make the slur a bit less
    // slopey -- we don't always need to reach all the way to the
    // target note

    int dy = y1 - y0;
    if (above) {
	if (dy < 0) y0 -= dy / 5;
	dy = dy * 4 / 5;
    } else {
	if (dy > 0) y0 += dy / 5;
	dy = dy * 4 / 5;
    }

    int length = endX - startX;
    int diff = staff.getLayoutYForHeight(0) - staff.getLayoutYForHeight(2);
    if (length < diff*10) diff /= 2;
    if (length > diff*3) length -= diff/2;
    startX += diff;

    (*i)->event()->setMaybe<Bool>(m_properties.SLUR_ABOVE, above);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_Y_DELTA, dy);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_LENGTH, length);
    (*i)->setLayoutX(startX);
    (*i)->setLayoutY(y0);
}

