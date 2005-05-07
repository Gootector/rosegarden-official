// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <qcanvas.h>

#include "matrixstaff.h"
#include "matrixvlayout.h"
#include "velocitycolour.h"
#include "matrixview.h"
#include "rosegardenguidoc.h"

#include "Segment.h"
#include "BaseProperties.h"
#include "Composition.h"
#include "Selection.h"

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::SnapGrid;
using Rosegarden::timeT;

MatrixStaff::MatrixStaff(QCanvas *canvas,
                         Segment *segment,
                         SnapGrid *snapGrid,
			 int id,
                         int vResolution,
                         MatrixView *view) :
    LinedStaff(canvas, segment, snapGrid, id, vResolution, 1),
    m_scaleFactor(2.0/
                  Rosegarden::Note(Rosegarden::Note::Shortest).getDuration()),
    m_view(view)
{
}

MatrixStaff::~MatrixStaff()
{
    // nothing
}

int MatrixStaff::getLineCount() const
{
    MATRIX_DEBUG << "MatrixStaff::getLineCount: isDrumMode " << m_view->isDrumMode() << ", key mapping " << (getKeyMapping() ? getKeyMapping()->getName() : "<none>") << endl;

    if (m_view->isDrumMode()) {
	const Rosegarden::MidiKeyMapping *km = getKeyMapping();
	if (km) return km->getMap().size() + 1;
    }
    return MatrixVLayout::maxMIDIPitch + 1;
}

int  MatrixStaff::getLegerLineCount()   const { return 0; }
int  MatrixStaff::getBottomLineHeight() const { return 0; }
int  MatrixStaff::getHeightPerLine()    const { return 1; }
bool MatrixStaff::elementsInSpaces()    const { return true; }
bool MatrixStaff::showBeatLines()       const { return true; }

bool MatrixStaff::wrapEvent(Rosegarden::Event* e)
{
    // Changed from "Note or Time signature" to just "Note" because 
    // there should be no time signature events in any ordinary
    // segments, they're only in the composition's ref segment

    return e->isa(Rosegarden::Note::EventType) &&
	Rosegarden::Staff::wrapEvent(e);
}

void
MatrixStaff::positionElements(timeT from, timeT to)
{
    MatrixElementList *mel = getViewElementList();

    MatrixElementList::iterator beginAt = mel->findTime(from);
    if (beginAt != mel->begin()) --beginAt;

    MatrixElementList::iterator endAt = mel->findTime(to);

    for (MatrixElementList::iterator i = beginAt; i != endAt; ++i) {
	positionElement(*i);
    }
}

void MatrixStaff::positionElement(Rosegarden::ViewElement* vel)
{
    MatrixElement* el = dynamic_cast<MatrixElement*>(vel);

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(el->getLayoutX(), int(el->getLayoutY()));

    // Get velocity for colouring
    //
    using Rosegarden::BaseProperties::VELOCITY;
    long velocity = 127;
    if (el->event()->has(VELOCITY))
        el->event()->get<Rosegarden::Int>(VELOCITY, velocity);

    el->setCanvas(m_canvas);

    // Is the event currently selected?  Colour accordingly.
    //
    Rosegarden::EventSelection *selection = m_view->getCurrentSelection();

    if (selection && selection->contains(el->event()))
        el->setColour(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::SelectedElement));
    else if (el->event()->has(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID))
	el->setColour(Qt::gray);
    else 
        el->setColour(DefaultVelocityColour::getInstance()->getColour(velocity));

    el->setCanvasX(coords.first);
    el->setCanvasY((double)coords.second);
}


MatrixElement*
MatrixStaff::getElement(Rosegarden::Event *event)
{
    return dynamic_cast<MatrixElement*>((*findEvent(event)));
}


void
MatrixStaff::eventRemoved(const Rosegarden::Segment *segment,
			  Rosegarden::Event *event)
{
    LinedStaff::eventRemoved(segment, event);
    m_view->handleEventRemoved(event);
}

Rosegarden::ViewElement*
MatrixStaff::makeViewElement(Rosegarden::Event* e)
{
    return new MatrixElement(e, m_view->isDrumMode());
}

const Rosegarden::MidiKeyMapping *
MatrixStaff::getKeyMapping() const
{
    Rosegarden::Composition *comp = getSegment().getComposition();
    if (!comp) return 0;
    Rosegarden::TrackId trackId = getSegment().getTrack();
    Rosegarden::Track *track = comp->getTrackById(trackId);
    Rosegarden::Instrument *instr = m_view->getDocument()->getStudio().
	getInstrumentById(track->getInstrument());
    if (!instr) return 0;
    return instr->getKeyMapping();
}

