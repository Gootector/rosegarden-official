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


NotationVLayout::NotationVLayout(Rosegarden::Composition *c, NotePixmapFactory *npf,
				 const NotationProperties &properties,
                                 QObject* parent, const char* name) :
    ProgressReporter(parent, name),
    m_composition(c),
    m_npf(npf),
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

	// Displaced Y will only be used for certain events -- in
	// particular not for notes, whose y-coord is obviously kind
	// of meaningful.
	double displacedY = 0.0;
	long dyRaw = 0;
	el->event()->get<Int>(DISPLACED_Y, dyRaw);
	displacedY = double(dyRaw * m_npf->getLineSpacing()) / 1000.0;

        el->setLayoutY(staff.getLayoutYForHeight(-9) + displacedY);

        if (el->isRest()) {

            // rests for notes longer than the minim have hotspots
            // aligned with the line above the middle line; the rest
            // are aligned with the middle line

            int noteType = el->event()->get<Int>(NOTE_TYPE);
            if (noteType > Note::Minim) {
                el->setLayoutY(staff.getLayoutYForHeight(6) + displacedY);
            } else {
                el->setLayoutY(staff.getLayoutYForHeight(4) + displacedY);
            }

        } else if (el->isNote()) {

            NotationChord chord(*notes, i, m_notationQuantizer, m_properties);
            if (chord.size() == 0) continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
		long height = 0;
		if (!(*chord[j])->event()->get<Int>
		    (m_properties.HEIGHT_ON_STAFF, height)) {
		    std::cerr << QString(i18n("ERROR: Event in chord at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*chord[j])->getViewAbsoluteTime())) << std::endl;
		    (*chord[j])->event()->dump(std::cerr);
		}
		h.push_back(height);
            }
	    bool stemmed = chord.hasStem();
            bool stemUp = chord.hasStemUp();
            bool hasNoteHeadShifted = chord.hasNoteHeadShifted();

            unsigned int flaggedNote = (stemUp ? chord.size() - 1 : 0);

	    bool hasShifted = chord.hasNoteHeadShifted();

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

		if (primary) {
		    el->event()->setMaybe<Int>
			(m_properties.CHORD_MARK_COUNT, chord.getMarkCountForChord());
		}

		bool shifted = chord.isNoteHeadShifted(chord[j]);
                el->event()->setMaybe<Bool>
		    (m_properties.NOTE_HEAD_SHIFTED, shifted);

		el->event()->setMaybe<Bool>
		    (m_properties.NOTE_DOT_SHIFTED, false);
		if (hasShifted && stemUp) {
		    long dots = 0;
		    (void)el->event()->get<Int>(NOTE_DOTS, dots);
		    if (dots > 0) {
			el->event()->setMaybe<Bool>
			    (m_properties.NOTE_DOT_SHIFTED, true);
		    }
		}

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


	    // #938545 (Broken notation: Duplicated note can float
	    // outside stave) -- Need to cope with the case where a
	    // note that's not a member of a chord (different stem
	    // direction &c) falls between notes that are members.
	    // Not optimal, as we can end up scanning the chord
	    // multiple times (we'll return to it after scanning the
	    // contained note).  [We can't just iterate over all
	    // elements within the chord (as we can in hlayout)
	    // because we need them in height order.]

	    i = chord.getFirstElementNotInChord();
	    if (i == notes->end()) i = chord.getFinalElement();
	    else --i;
            
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
		    el->setLayoutY(staff.getLayoutYForHeight(-7) + displacedY);
		} else if (type == Text::Lyric ||
			   type == Text::Annotation) {
		    el->setLayoutY(staff.getLayoutYForHeight(-13) + displacedY);
		} else {
		    el->setLayoutY(staff.getLayoutYForHeight(22) + displacedY);
		}

            } else if (el->event()->isa(Indication::EventType)) {

		std::string indicationType =
		    el->event()->get<String>(Indication::IndicationTypePropertyName);

		if (indicationType == Indication::Slur ||
		    indicationType == Indication::PhrasingSlur) {
		    getSlurList(staff).push_back(i);
		}

		if (indicationType == Indication::OttavaUp ||
		    indicationType == Indication::QuindicesimaUp) {
		    el->setLayoutY(staff.getLayoutYForHeight(15) + displacedY);
		} else {
		    el->setLayoutY(staff.getLayoutYForHeight(-9) + displacedY);
		}
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
    bool phrasing = ((*i)->event()->get<String>(Indication::IndicationTypePropertyName)
		     == Indication::PhrasingSlur);

    NotationElementList::iterator scooter = i;

    timeT slurDuration = (*i)->event()->getDuration();
    if (slurDuration == 0) {
	slurDuration = (*i)->event()->get<Int>("indicationduration"); // obs property
    }
    timeT endTime = (*i)->getViewAbsoluteTime() + slurDuration;

    bool haveStart = false;

    int startTopHeight = 4, endTopHeight = 4,
	startBottomHeight = 4, endBottomHeight = 4,
	maxTopHeight = 4, minBottomHeight = 4,
	maxCount = 0, minCount = 0;

    int startX = (int)(*i)->getLayoutX(), endX = startX + 10;
    bool startStemUp = false, endStemUp = false;
    long startMarks = 0, endMarks = 0;
    bool startTied = false, endTied = false;
    bool beamAbove = false, beamBelow = false;
    bool dynamic = false;

    std::vector<Event *> stemUpNotes, stemDownNotes;

    // Scan the notes spanned by the slur, recording the top and
    // bottom heights of the first and last chords, plus the presence
    // of any troublesome beams and high or low notes in the body.

    while (scooter != staff.getViewElementList()->end()) {

	if ((*scooter)->getViewAbsoluteTime() >= endTime) break;
	Event *event = (*scooter)->event();

	if (event->isa(Rosegarden::Note::EventType)) {

	    long h = 0;
	    if (!event->get<Int>(m_properties.HEIGHT_ON_STAFF, h)) {
		KMessageBox::sorry
		    ((QWidget *)parent(), QString(i18n("Spanned note at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*scooter)->getViewAbsoluteTime())));
		event->dump(std::cerr);
	    }

	    bool stemUp = (h <= 4);
	    event->get<Bool>(m_properties.VIEW_LOCAL_STEM_UP, stemUp);
	    
	    bool beamed = false;
	    event->get<Bool>(m_properties.BEAMED, beamed);
	    
	    bool primary = false;

	    if (event->get<Bool>
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
		    minBottomHeight = startBottomHeight;
		    maxTopHeight = startTopHeight;

		    startX = (int)(*scooter)->getLayoutX();
		    startStemUp = stemUp;
		    startMarks = chord.getMarkCountForChord();

		    bool tied = false;
		    if ((event->get<Bool>(TIED_FORWARD, tied) && tied) ||
			(event->get<Bool>(TIED_BACKWARD, tied) && tied)) {
			startTied = true;
		    }

		    haveStart = true;

		} else {
		    if (chord.getLowestNoteHeight() < minBottomHeight) {
			minBottomHeight = chord.getLowestNoteHeight();
			++minCount;
		    }
		    if (chord.getHighestNoteHeight() > maxTopHeight) {
			maxTopHeight = chord.getHighestNoteHeight();
			++maxCount;
		    }
		}

		endBottomHeight = chord.getLowestNoteHeight();
		endTopHeight = chord.getHighestNoteHeight();
		endX = (int)(*scooter)->getLayoutX();
		endStemUp = stemUp;
		endMarks = chord.getMarkCountForChord();

		bool tied = false;
		if ((event->get<Bool>(TIED_FORWARD, tied) && tied) ||
		    (event->get<Bool>(TIED_BACKWARD, tied) && tied)) {
		    endTied = true;
		}
	    }

	    if (!beamed) {
		if (stemUp) stemUpNotes.push_back(event);
		else stemDownNotes.push_back(event);
	    }

	} else if (event->isa(Indication::EventType)) {
	    
	    std::string indicationType =
		event->get<String>(Indication::IndicationTypePropertyName);

	    if (indicationType == Indication::Crescendo ||
		indicationType == Indication::Decrescendo) dynamic = true;
	}

	++scooter;
    }

    bool above = true;

    if ((*i)->event()->has(NotationProperties::SLUR_ABOVE) &&
	(*i)->event()->isPersistent<Bool>(NotationProperties::SLUR_ABOVE)) {

	(*i)->event()->get<Bool>(NotationProperties::SLUR_ABOVE, above);

    } else if (phrasing) {

	int score = 0; // for "above"

	if (dynamic) score += 2;

	if (startStemUp == endStemUp) {
	    if (startStemUp) score -= 2;
	    else score += 2;
	} else if (beamBelow != beamAbove) {
	    if (beamAbove) score -= 2;
	    else score += 2;
	}

	if (maxTopHeight < 6) score += 1;
	else if (minBottomHeight > 2) score -= 1;

	if (stemUpNotes.size() != stemDownNotes.size()) {
	    if (stemUpNotes.size() < stemDownNotes.size()) score += 1;
	    else score -= 1;
	}

	above = (score >= 0);

    } else {

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
    }

    // now choose the actual y-coord of the slur based on the side
    // we've decided to put it on

    int startHeight, endHeight;
    int startOffset = 2, endOffset = 2;

    if (above) {

	if (!startStemUp) startOffset += startMarks * 2;
	else startOffset += 5;

	if (!endStemUp) endOffset += startMarks * 2;
	else endOffset += 5;

	startHeight = startTopHeight + startOffset;
	endHeight = endTopHeight + endOffset;

	bool maxRelevant = ((maxTopHeight != endTopHeight) || (maxCount > 1));
	if (maxRelevant) {
	    int midHeight = (startHeight + endHeight)/2;
	    if (maxTopHeight > midHeight - 1) {
		startHeight += maxTopHeight - midHeight + 1;
		endHeight   += maxTopHeight - midHeight + 1;
	    }
	}

    } else {

	if (startStemUp) startOffset += startMarks * 2;
	else startOffset += 5;

	if (endStemUp) endOffset += startMarks * 2;
	else endOffset += 5;

	startHeight = startBottomHeight - startOffset;
	endHeight = endBottomHeight - endOffset;

	bool minRelevant = ((minBottomHeight != endBottomHeight) || (minCount > 1));
	if (minRelevant) {
	    int midHeight = (startHeight + endHeight)/2;
	    if (minBottomHeight < midHeight + 1) {
		startHeight -= midHeight - minBottomHeight + 1;
		endHeight   -= midHeight - minBottomHeight + 1;
	    }
	}
    }

    int y0 = staff.getLayoutYForHeight(startHeight),
	y1 = staff.getLayoutYForHeight(endHeight);

    int dy = y1 - y0;
    int length = endX - startX;
    int diff = staff.getLayoutYForHeight(0) - staff.getLayoutYForHeight(3);
    if (length < diff*10) diff /= 2;
    if (length > diff*3) length -= diff/2;
    startX += diff;

    (*i)->event()->setMaybe<Bool>(NotationProperties::SLUR_ABOVE, above);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_Y_DELTA, dy);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_LENGTH, length);

    double displacedX = 0.0, displacedY = 0.0;

    long dxRaw = 0;
    (*i)->event()->get<Int>(DISPLACED_X, dxRaw);
    displacedX = double(dxRaw * m_npf->getNoteBodyWidth()) / 1000.0;

    long dyRaw = 0;
    (*i)->event()->get<Int>(DISPLACED_Y, dyRaw);
    displacedY = double(dyRaw * m_npf->getLineSpacing()) / 1000.0;

    (*i)->setLayoutX(startX + displacedX);
    (*i)->setLayoutY(y0 + displacedY);
}

