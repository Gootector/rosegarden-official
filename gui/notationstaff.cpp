// -*- c-basic-offset: 4 -*-

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

#include <algorithm>

#include "notationstaff.h"
#include "qcanvassimplesprite.h"
#include "notationproperties.h"
#include "Selection.h"
#include "Composition.h"
#include "BaseProperties.h"

#include "rosedebug.h"
#include "colours.h"

#include "Event.h"
#include "Segment.h"
#include "Quantizer.h"
#include "NotationTypes.h"

using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Segment;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Accidental;
using Rosegarden::Accidentals::NoAccidental;
using Rosegarden::TimeSignature;
using Rosegarden::PropertyName;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

using std::string;

NotationStaff::NotationStaff(QCanvas *canvas, Segment *segment,
                             int id, bool pageMode, double pageWidth, 
                             string fontName, int resolution) :
    LinedStaff<NotationElement>(canvas, segment, id, resolution,
				1, //!!!
				pageMode, pageWidth,
				0 //!!!
	),
    m_npf(0)
{
    changeFont(fontName, resolution);
}

NotationStaff::~NotationStaff()
{
    deleteTimeSignatures();
}

void
NotationStaff::changeFont(string fontName, int resolution) 
{
    setResolution(resolution);

    delete m_npf;
    m_npf = new NotePixmapFactory(fontName, resolution);
}

void
NotationStaff::insertTimeSignature(double layoutX,
				   const TimeSignature &timeSig)
{
    m_npf->setSelected(false);
    QCanvasPixmap *pixmap =
	new QCanvasPixmap(m_npf->makeTimeSigPixmap(timeSig));
    QCanvasSimpleSprite *sprite = new QCanvasSimpleSprite(pixmap, m_canvas);

    LinedStaffCoords sigCoords =
	getCanvasCoordsForLayoutCoords(layoutX, getLayoutYForHeight(4));

    sprite->move(sigCoords.first, (double)sigCoords.second);
    sprite->show();
    m_timeSigs.insert(sprite);
}

void
NotationStaff::deleteTimeSignatures()
{
    kdDebug(KDEBUG_AREA) << "NotationStaff::deleteTimeSignatures()\n";
    
    for (SpriteSet::iterator i = m_timeSigs.begin();
	 i != m_timeSigs.end(); ++i) {
        delete (*i);
    }

    m_timeSigs.clear();
}

void
NotationStaff::getClefAndKeyAtCanvasCoords(double cx, int cy,
					   Clef &clef, 
					   Rosegarden::Key &key) const
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);
    int i;

    for (i = 0; i < m_clefChanges.size(); ++i) {
	if (m_clefChanges[i].first > layoutCoords.first) break;
	clef = m_clefChanges[i].second;
    }

    for (i = 0; i < m_keyChanges.size(); ++i) {
	if (m_keyChanges[i].first > layoutCoords.first) break;
	key = m_keyChanges[i].second;
    }
}

NotationElementList::iterator
NotationStaff::getClosestElementToCanvasCoords(double cx, int cy,
					       Rosegarden::Event *&clef,
					       Rosegarden::Event *&key,
					       bool notesAndRestsOnly,
					       int proximityThreshold)
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);

    kdDebug(KDEBUG_AREA) << "My coords: canvas (" << cx << "," << cy
			 << "), layout (" << layoutCoords.first << ","
			 << layoutCoords.second << ")" << endl;

    return getClosestElementToLayoutX
	(layoutCoords.first, clef, key, notesAndRestsOnly, proximityThreshold);
}


NotationElementList::iterator
NotationStaff::getClosestElementToLayoutX(double x,
					  Rosegarden::Event *&clef,
					  Rosegarden::Event *&key,
					  bool notesAndRestsOnly,
					  int proximityThreshold)
{
    START_TIMING;

    double minDist = 10e9, prevDist = 10e9;

    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it, result;

    //!!! Doesn't handle time signature correctly -- needs to get 
    // from composition

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {

	bool before = ((*it)->getLayoutX() < x);
	
	if (!(*it)->isNote() && !(*it)->isRest()) {
	    if (before) {
		if ((*it)->event()->isa(Clef::EventType)) {
		    clef = (*it)->event();
		} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
		    key = (*it)->event();
		}
	    }
	    if (notesAndRestsOnly) continue;
	}

	double dx = x - (*it)->getLayoutX();
	if (dx < 0) dx = -dx;

	if (dx < minDist) {
	    minDist = dx;
	    result = it;
	} else if (!before) {
	    break;
	}

	prevDist = dx;
    }

    if (proximityThreshold > 0 && minDist > proximityThreshold) {
        kdDebug(KDEBUG_AREA) << "NotationStaff::getClosestElementToLayoutX() : element is too far away : "
                             << minDist << endl;
        return notes->end();
    }
        
    kdDebug(KDEBUG_AREA) << "NotationStaff::getClosestElementToLayoutX: found element at layout " << (*result)->getLayoutX() << " (" << (*result)->getCanvasX() << "," << (*result)->getCanvasY() << ") - we're at layout " << x << endl;

    PRINT_ELAPSED("NotationStaff::getClosestElementToLayoutX");

    return result;
}


NotationElementList::iterator
NotationStaff::getElementUnderCanvasCoords(double cx, int cy,
					   Rosegarden::Event *&clef,
					   Rosegarden::Event *&key)
{
    LinedStaffCoords layoutCoords = getLayoutCoordsForCanvasCoords(cx, cy);

    kdDebug(KDEBUG_AREA) << "My coords: canvas (" << cx << "," << cy
			 << "), layout (" << layoutCoords.first << ","
			 << layoutCoords.second << ")" << endl;

    return getElementUnderLayoutX(layoutCoords.first, clef, key);
}

NotationElementList::iterator
NotationStaff::getElementUnderLayoutX(double x,
				      Rosegarden::Event *&clef,
				      Rosegarden::Event *&key)
{
    NotationElementList *notes = getViewElementList();
    NotationElementList::iterator it;

    // TODO: this is grossly inefficient

    for (it = notes->begin(); it != notes->end(); ++it) {

	bool before = ((*it)->getLayoutX() < x);
	
	if (!(*it)->isNote() && !(*it)->isRest()) {
	    if (before) {
		if ((*it)->event()->isa(Clef::EventType)) {
		    clef = (*it)->event();
		} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
		    key = (*it)->event();
		}
	    }
	}

	double airX, airWidth;
	(*it)->getLayoutAirspace(airX, airWidth);
	if (x >= airX && x < airX + airWidth) {
	    return it;
	} else if (!before) {
	    break;
	}
    }

    return notes->end();
}

 
string
NotationStaff::getNoteNameAtCanvasCoords(double x, int y,
					 Rosegarden::Accidental acc) const
{
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    getClefAndKeyAtCanvasCoords(x, y, clef, key);

    return
	Rosegarden::NotationDisplayPitch
	(getHeightAtCanvasY(y), acc).getAsString(clef, key);
}

void
NotationStaff::renderElements(NotationElementList::iterator from,
			      NotationElementList::iterator to)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff " << this << "::renderElements()" << endl;
    START_TIMING;

    Clef currentClef; // default is okay to start with

    int elementCount = 0; // purely diagnostic

    for (NotationElementList::iterator it = from; it != to; ++it) {

	if ((*it)->event()->isa(Clef::EventType)) {
	    currentClef = Clef(*(*it)->event());
	}

	bool selected = false;
	(void)((*it)->event()->get<Bool>(SELECTED, selected));

//	kdDebug(KDEBUG_AREA) << "Rendering at " << (*it)->getAbsoluteTime()
//			     << " (selected = " << selected << ")" << endl;

	renderSingleElement(*it, currentClef, selected);

	++elementCount;
    }

    kdDebug(KDEBUG_AREA) << "NotationStaff::renderElements: "
			 << elementCount << " elements rendered" << endl;

    PRINT_ELAPSED("NotationStaff::renderElements");
}	

void
NotationStaff::positionElements(timeT from, timeT to)
{
    kdDebug(KDEBUG_AREA) << "NotationStaff " << this << "::positionElements()"
                         << from << " -> " << to << "\n";
    START_TIMING;

    int elementsPositioned = 0, elementsRendered = 0; // diagnostic
    
    timeT nextBarTime;
    NotationElementList::iterator beginAt = findUnchangedBarStart(from);
    NotationElementList::iterator endAt = findUnchangedBarEnd(to, nextBarTime);
    if (beginAt == getViewElementList()->end()) return;

    truncateClefsAndKeysAt((*beginAt)->getLayoutX());
    Clef currentClef; // default is okay to start with
    Rosegarden::Key currentKey; // likewise

    for (NotationElementList::iterator it = beginAt; it != endAt; ++it) {

	if ((*it)->event()->isa(Clef::EventType)) {

	    currentClef = Clef(*(*it)->event());
	    m_clefChanges.push_back(ClefChange(int((*it)->getLayoutX()),
					       currentClef));

	} else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {

	    currentKey = Rosegarden::Key(*(*it)->event());
	    m_keyChanges.push_back(KeyChange(int((*it)->getLayoutX()),
					     currentKey));
	}

	bool selected = false;
	(void)((*it)->event()->get<Bool>(SELECTED, selected));

	bool needNewSprite = (selected != (*it)->isSelected());

	if (!(*it)->getCanvasItem()) {

	    needNewSprite = true;

	} else if ((*it)->isNote() && !(*it)->isRecentlyRegenerated()) {

	    // If the note's y-coordinate has changed, we should
	    // redraw it -- its stem direction may have changed, or it
	    // may need leger lines.  This will happen e.g. if the
	    // user inserts a new clef; unfortunately this means
	    // inserting clefs is rather slow.
	    
	    needNewSprite = needNewSprite || !elementNotMovedInY(*it);
	    
	    if (!needNewSprite) {

		// If the event is a beamed or tied-forward note, then
		// we might need a new sprite if the distance from
		// this note to the next has changed (because the beam
		// or tie is part of the note's sprite).

		bool spanning = false;
		(void)((*it)->event()->get<Bool>(BEAMED, spanning));
		if (!spanning)
		    (void)((*it)->event()->get<Bool>(TIED_FORWARD, spanning));
	    
		if (spanning) {
		    needNewSprite =
			((*it)->getAbsoluteTime() < nextBarTime ||
			 !elementShiftedOnly(it));
		}
	    }

	} else if ((*it)->event()->isa(Indication::EventType) &&
		   !(*it)->isRecentlyRegenerated()) {
	    needNewSprite = true;
	}

	if (needNewSprite) {
	    renderSingleElement(*it, currentClef, selected);
	    ++elementsRendered;
	}

	LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
	    ((*it)->getLayoutX(), (int)(*it)->getLayoutY());
	(*it)->reposition(coords.first, (double)coords.second);
	(*it)->setSelected(selected);
	++elementsPositioned;
    }

    kdDebug(KDEBUG_AREA) << "NotationStaff::positionElements: "
			 << elementsPositioned << " elements positioned, "
			 << elementsRendered << " elements re-rendered"
			 << endl;

    PRINT_ELAPSED("NotationStaff::positionElements");
    NotePixmapFactory::dumpStats(std::cerr);
}

void
NotationStaff::truncateClefsAndKeysAt(int x)
{
    for (FastVector<ClefChange>::iterator i = m_clefChanges.begin();
	 i != m_clefChanges.end(); ++i) {
	if (i->first >= x) {
	    m_clefChanges.erase(i, m_clefChanges.end());
	    break;
	}
    }
    
    for (FastVector<KeyChange>::iterator i = m_keyChanges.begin();
	 i != m_keyChanges.end(); ++i) {
	if (i->first >= x) {
	    m_keyChanges.erase(i, m_keyChanges.end());
	    break;
	}
    }
}

NotationElementList::iterator
NotationStaff::findUnchangedBarStart(timeT from)
{
    NotationElementList *nel = (NotationElementList *)getViewElementList();

    // Track back bar-by-bar until we find one whose start position
    // hasn't changed

    NotationElementList::iterator beginAt = nel->begin();
    do {
	from = getSegment().getBarStartForTime(from - 1);
	beginAt = nel->findTime(from);
    } while (beginAt != nel->begin() &&
	     (beginAt == nel->end() || !elementNotMoved(*beginAt)));

    return beginAt;
}

NotationElementList::iterator
NotationStaff::findUnchangedBarEnd(timeT to, timeT &nextBarTime)
{
    NotationElementList *nel = (NotationElementList *)getViewElementList();

    // Track forward to the end, similarly.  Here however it's very
    // common for all the positions to have changed right up to the
    // end of the piece; so we save time by assuming that to be the
    // case if we get more than (arbitrary) 3 changed bars.

    // We also record the start of the bar following the changed
    // section, for later use.

    NotationElementList::iterator endAt = nel->end();
    nextBarTime = -1;

    int changedBarCount = 0;
    NotationElementList::iterator candidate = nel->end();
    do {
	candidate = nel->findTime(getSegment().getBarEndForTime(to));
	if (candidate != nel->end()) {
	    to = (*candidate)->getAbsoluteTime();
	    if (nextBarTime < 0) nextBarTime = to;
	} else {
	    nextBarTime = getSegment().getEndTime();
	}
	++changedBarCount;
    } while (changedBarCount < 4 &&
	     candidate != nel->end() && !elementNotMoved(*candidate));

    if (changedBarCount < 4) return candidate;
    else return endAt;
}


bool
NotationStaff::elementNotMoved(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());

    bool ok =
	(int)(elt->getCanvasX()) == (int)(coords.first) &&
	(int)(elt->getCanvasY()) == (int)(coords.second);


//    if (!ok) {
//	kdDebug(KDEBUG_AREA)
//	    << "elementNotMoved: elt at " << elt->getAbsoluteTime() <<
//	    ", ok is " << ok << endl;
//	std::cerr << "(cf " << (int)(elt->getCanvasX()) << " vs "
//		  << (int)(coords.first) << ", "
//		  << (int)(elt->getCanvasY()) << " vs "
//		  << (int)(coords.second) << ")" << std::endl;
//    }
    return ok;
}

bool
NotationStaff::elementNotMovedInY(NotationElement *elt)
{
    if (!elt->getCanvasItem()) return false;

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(elt->getLayoutX(), (int)elt->getLayoutY());

    bool ok = (int)(elt->getCanvasY()) == (int)(coords.second);

    if (!ok) {
	kdDebug(KDEBUG_AREA)
	    << "elementNotMovedInY: elt at " << elt->getAbsoluteTime() <<
	    ", ok is " << ok << endl;
	std::cerr << "(cf " << (int)(elt->getCanvasY()) << " vs "
		  << (int)(coords.second) << ")" << std::endl;
    }
    return ok;
}

bool
NotationStaff::elementShiftedOnly(NotationElementList::iterator i)
{
    int shift = 0;
    bool ok = false;

    for (NotationElementList::iterator j = i;
	 j != getViewElementList()->end(); ++j) {

	NotationElement *elt = *j;
	if (!elt->getCanvasItem()) break;

	LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	    (elt->getLayoutX(), (int)elt->getLayoutY());
	
	// regard any shift in y as suspicious
	if ((int)(elt->getCanvasY()) != (int)(coords.second)) break;

	int myShift = (int)(elt->getCanvasX()) - (int)(coords.first);
	if (j == i) shift = myShift;
	else if (myShift != shift) break;
	
	if (elt->getAbsoluteTime() > (*i)->getAbsoluteTime()) {
	    // all events up to and including this one have passed
	    ok = true;
	    break;
	}
    }

    if (!ok) {
	kdDebug(KDEBUG_AREA) 
	    << "elementShiftedOnly: elt at " << (*i)->getAbsoluteTime()
	    << ", ok is " << ok << endl;
    }

    return ok;
}

void
NotationStaff::renderSingleElement(NotationElement *elt,
				   const Rosegarden::Clef &currentClef,
				   bool selected)
{
    try {

	QCanvasPixmap *pixmap = 0;
	QCanvasItem *canvasItem = 0;

	m_npf->setSelected(selected);

	if (elt->isNote()) {

	    canvasItem = makeNoteSprite(elt);

	} else if (elt->isRest()) {

//    kdDebug(KDEBUG_AREA) << "NotationStaff::renderSingleElement: about to query legato duration property" << endl;
//	    timeT duration = elt->event()->get<Int>
//		(Rosegarden::Quantizer::LegatoDurationProperty);
//kdDebug(KDEBUG_AREA) << "done" <<endl;

//	    if (duration > 0) {

		Note::Type note = elt->event()->get<Int>(NOTE_TYPE);
		int dots = elt->event()->get<Int>(NOTE_DOTS);
		NotePixmapParameters params(note, dots);
		setTuplingParameters(elt, params);
		pixmap = new QCanvasPixmap(m_npf->makeRestPixmap(params));
//	    } else {
//		kdDebug(KDEBUG_AREA) << "Omitting too-short rest" << endl;
//	    }

	} else if (elt->event()->isa(Clef::EventType)) {

	    pixmap = new QCanvasPixmap
		(m_npf->makeClefPixmap(Rosegarden::Clef(*elt->event())));

	} else if (elt->event()->isa(Rosegarden::Key::EventType)) {

	    pixmap = new QCanvasPixmap
		(m_npf->makeKeyPixmap
		 (Rosegarden::Key(*elt->event()), currentClef));

	} else if (elt->event()->isa(Rosegarden::Text::EventType)) {

	    pixmap = new QCanvasPixmap
		(m_npf->makeTextPixmap(Rosegarden::Text(*elt->event())));

	} else if (elt->event()->isa(Indication::EventType)) {

	    timeT indicationDuration =
		elt->event()->get<Int>(Indication::IndicationDurationPropertyName);
	    NotationElementList::iterator indicationEnd =
		getViewElementList()->findTime(elt->getAbsoluteTime() +
					       indicationDuration);

	    string indicationType = 
		elt->event()->get<String>(Indication::IndicationTypePropertyName);

	    int length, y1;

	    if (indicationType == Indication::Slur &&
		indicationEnd != getViewElementList()->begin()) {
		--indicationEnd;
	    }

	    if (indicationEnd != getViewElementList()->end()) {
		length = (int)((*indicationEnd)->getLayoutX() -
			       elt->getLayoutX());
		y1 = (int)(*indicationEnd)->getLayoutY();
	    } else {
		//!!! imperfect
		--indicationEnd;
		length = (int)((*indicationEnd)->getLayoutX() +
			       m_npf->getNoteBodyWidth() * 3 -
			       elt->getLayoutX());
		y1 = (int)(*indicationEnd)->getLayoutY();
	    }

	    if (length < m_npf->getNoteBodyWidth()) {
		length = m_npf->getNoteBodyWidth();
	    }

	    if (indicationType == Indication::Crescendo) {

		pixmap = new QCanvasPixmap
		    (m_npf->makeHairpinPixmap(length, true));

	    } else if (indicationType == Indication::Decrescendo) {

		pixmap = new QCanvasPixmap
		    (m_npf->makeHairpinPixmap(length, false));

	    } else if (indicationType == Indication::Slur) {

		bool above = true;
		long dy = 0;
		long length = 10;
		
		elt->event()->get<Bool>(SLUR_ABOVE, above);
		elt->event()->get<Int>(SLUR_Y_DELTA, dy);
		elt->event()->get<Int>(SLUR_LENGTH, length);
		
		pixmap = new QCanvasPixmap
		    (m_npf->makeSlurPixmap(length, dy, above));
		    
	    } else {
		//...
	    }

	} else {
                    
	    kdDebug(KDEBUG_AREA)
		<< "NotationElement of unrecognised type "
		<< elt->event()->getType() << endl;
	    pixmap = new QCanvasPixmap(m_npf->makeUnknownPixmap());
	}

	if (!canvasItem && pixmap) {
	    canvasItem = new QCanvasNotationSprite(*elt, pixmap, m_canvas);
	}

	// Show the sprite
	//
	if (canvasItem) {
	    LinedStaffCoords coords = getCanvasOffsetsForLayoutCoords
		(elt->getLayoutX(), (int)elt->getLayoutY());
	    elt->setCanvasItem
		(canvasItem, coords.first, (double)coords.second);
	    canvasItem->setZ(selected ? 2 : 0);
	    canvasItem->show();
	} else {
	    elt->removeCanvasItem();
	}
	
//	kdDebug(KDEBUG_AREA) << "NotationStaff::renderSingleElement: Setting selected at " << elt->getAbsoluteTime() << " to " << selected << endl;
            
    } catch (...) {
	kdDebug(KDEBUG_AREA) << "Event lacks the proper properties: "
			     << (*elt->event())
			     << endl;
    }
}

QCanvasSimpleSprite *
NotationStaff::makeNoteSprite(NotationElement *elt)
{
    static NotePixmapParameters params(Note::Crotchet, 0);

    Note::Type note = elt->event()->get<Int>(NOTE_TYPE);
    int dots = elt->event()->get<Int>(NOTE_DOTS);

    Accidental accidental = NoAccidental;
    (void)elt->event()->get<String>(DISPLAY_ACCIDENTAL, accidental);

    bool up = true;
    (void)(elt->event()->get<Bool>(STEM_UP, up));

    bool flag = true;
    (void)(elt->event()->get<Bool>(DRAW_FLAG, flag));

    bool beamed = false;
    (void)(elt->event()->get<Bool>(BEAMED, beamed));

    bool shifted = false;
    (void)(elt->event()->get<Bool>(NOTE_HEAD_SHIFTED, shifted));

    long stemLength = m_npf->getNoteBodyHeight();
    (void)(elt->event()->get<Int>(UNBEAMED_STEM_LENGTH, stemLength));
    
    long heightOnStaff = 0;
    int legerLines = 0;

    (void)(elt->event()->get<Int>(HEIGHT_ON_STAFF, heightOnStaff));
    if (heightOnStaff < 0) {
        legerLines = heightOnStaff;
    } else if (heightOnStaff > 8) {
        legerLines = heightOnStaff - 8;
    }

    params.setNoteType(note);
    params.setDots(dots);
    params.setAccidental(accidental);
    params.setNoteHeadShifted(shifted);
    params.setDrawFlag(flag);
    params.setDrawStem(true);
    params.setStemGoesUp(up);
    params.setLegerLines(legerLines);
    params.setBeamed(false);
    params.setIsOnLine(heightOnStaff % 2 == 0);
    params.removeMarks();

    if (elt->event()->get<Bool>(CHORD_PRIMARY_NOTE)) {
	long markCount = 0;
	(void)(elt->event()->get<Int>(MARK_COUNT, markCount));
	if (markCount == 0) {
	} else {
	    std::vector<Rosegarden::Mark> marks;
	    for (int i = 0; i < markCount; ++i) {
		Rosegarden::Mark mark;
		if (elt->event()->get<String>(getMarkPropertyName(i), mark)) {
		    marks.push_back(mark);
		}
	    }
	    params.setMarks(marks);
	}
    }

    long tieLength = 0;
    (void)(elt->event()->get<Int>(TIE_LENGTH, tieLength));
    if (tieLength > 0) {
        params.setTied(true);
        params.setTieLength(tieLength);
    } else {
        params.setTied(false);
    }

    if (beamed) {

        if (elt->event()->get<Bool>(CHORD_PRIMARY_NOTE)) {

            int myY = elt->event()->get<Int>(BEAM_MY_Y);

            stemLength = myY - (int)elt->getLayoutY();
            if (stemLength < 0) stemLength = -stemLength;

            int nextBeamCount =
                elt->event()->get<Int>(BEAM_NEXT_BEAM_COUNT);
            int width =
                elt->event()->get<Int>(BEAM_SECTION_WIDTH);
            int gradient =
                elt->event()->get<Int>(BEAM_GRADIENT);

            bool thisPartialBeams(false), nextPartialBeams(false);
            (void)elt->event()->get<Bool>
                (BEAM_THIS_PART_BEAMS, thisPartialBeams);
            (void)elt->event()->get<Bool>
                (BEAM_NEXT_PART_BEAMS, nextPartialBeams);

	    params.setBeamed(true);
            params.setNextBeamCount(nextBeamCount);
            params.setThisPartialBeams(thisPartialBeams);
            params.setNextPartialBeams(nextPartialBeams);
            params.setWidth(width);
            params.setGradient((double)gradient / 100.0);

        } else {
            params.setBeamed(false);
            params.setDrawStem(false);
        }
    }
    
    params.setStemLength(stemLength);
    setTuplingParameters(elt, params);

    QCanvasPixmap notePixmap(m_npf->makeNotePixmap(params));
    return new QCanvasNotationSprite(*elt,
                                     new QCanvasPixmap(notePixmap), m_canvas);
}

void
NotationStaff::setTuplingParameters(NotationElement *elt,
				    NotePixmapParameters &params)
{
    params.setTupletCount(0);
    long tuplingLineY = 0;
    bool tupled = (elt->event()->get<Int>(TUPLING_LINE_MY_Y, tuplingLineY));

    if (tupled) {
	int tuplingLineWidth =
	    elt->event()->get<Int>(TUPLING_LINE_WIDTH);
	double tuplingLineGradient =
	    (double)(elt->event()->get<Int>(TUPLING_LINE_GRADIENT)) / 100.0;

	long tupletCount;
	if (elt->event()->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT, tupletCount)) {
	    params.setTupletCount(tupletCount);
	    params.setTuplingLineY(tuplingLineY - (int)elt->getLayoutY());
	    params.setTuplingLineWidth(tuplingLineWidth);
	    params.setTuplingLineGradient(tuplingLineGradient);
	}
    }
}

bool
NotationStaff::wrapEvent(Rosegarden::Event *e)
{
    if (e->isa(Note::EventRestType)) {
	const Rosegarden::Quantizer *q =
	    getSegment().getComposition()->getLegatoQuantizer();
//	q->quantize(e);//!!!
	if (q->getQuantizedDuration(e) == 0) return false;
    }
    return true;
}

