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

#include "trackeditor.h"
#include "segmentcanvas.h"
#include "rosegardenguidoc.h"
#include "rulerscale.h"
#include "Track.h"
#include "colours.h"
#include "NotationTypes.h"

#include "rosedebug.h"

#include <qlayout.h>
#include <qcanvas.h>

#include <kmessagebox.h>

using Rosegarden::Composition;

TrackEditor::TrackEditor(RosegardenGUIDoc* doc,
			 RulerScale *rulerScale,
			 QWidget* parent, const char* name,
			 WFlags) :
    QWidget(parent, name),
    DCOPObject("TrackEditorIface"),
    m_document(doc),
    m_rulerScale(rulerScale),
    m_segmentCanvas(0),
    m_hHeader(0),
    m_vHeader(0)
{
    Composition &comp = doc->getComposition();

    int tracks = doc->getNbTracks();

    // If we have no Track then create a default document with 10 of them
    //
    if (tracks == 0)
    {
        // default number of Tracks
        //
        tracks = 10;

        // Create the Tracks on the Composition
        //
        Rosegarden::Track *track;
        for (int i = 0; i < tracks; i++)
        {
            track = new Rosegarden::Track(i, false, Rosegarden::Track::Midi,
                                          std::string("untitled"), i, 0);

            comp.addTrack(track);
        }

        // Add a default Instrument
        //
        Rosegarden::Instrument *instr = new Rosegarden::Instrument(0,
                          Rosegarden::Instrument::Midi, std::string("Instrument 1"));
        comp.addInstrument(instr);
    }

    init(tracks,
	 comp.getBarNumber(comp.getStartMarker(), false),
	 comp.getBarNumber(comp.getEndMarker(), false));
}


void
TrackEditor::init(unsigned int nbTracks, int firstBar, int lastBar)
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::init(nbTracks = "
                         << nbTracks << ", firstBar = " << firstBar
                         << ", lastBar = " << lastBar << ")" << endl;

    QGridLayout *grid = new QGridLayout(this, 2, 2);

    setupHorizontalHeader(firstBar, lastBar);

    grid->addWidget(m_hHeader, 0, 1);
    grid->addWidget(m_vHeader =
         new Rosegarden::TrackHeader(nbTracks, this), 1, 0);
    m_vHeader->setOrientation(Qt::Vertical);

    // set up vert. header
    for (int i = 0; i < m_vHeader->count(); ++i) {
        m_vHeader->resizeSection(i, 25);
        m_vHeader->setLabel(i, QString("Track %1").arg(i));
    }

    m_vHeader->setMinimumWidth(100);
    m_vHeader->setResizeEnabled(false);

    QObject::connect(m_vHeader, SIGNAL(indexChange(int,int,int)),
                     this, SLOT(segmentOrderChanged(int,int,int)));

    QCanvas *canvas = new QCanvas(this);

    int canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
			    m_rulerScale->getBarWidth(lastBar));

    canvas->resize(canvasWidth, m_vHeader->sectionSize(0) * nbTracks);
    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    m_segmentCanvas = new SegmentCanvas(m_hHeader->sectionSize(0),
					m_vHeader->sectionSize(0),
					*canvas, this);

    grid->addWidget(m_segmentCanvas, 1, 1);

    // Hide both headers - we use these for measurement and not show!
    //
    m_vHeader->hide();
    m_hHeader->hide();

    connect(this, SIGNAL(needUpdate()),
            m_segmentCanvas, SLOT(update()));

    QObject::connect(m_segmentCanvas, SIGNAL(addSegment(SegmentItem*)),
                     this,           SLOT(addSegment(SegmentItem*)));

    QObject::connect(m_segmentCanvas, SIGNAL(deleteSegment(Rosegarden::Segment*)),
                     this,           SLOT(deleteSegment(Rosegarden::Segment*)));

    QObject::connect(m_segmentCanvas, SIGNAL(updateSegmentDuration(SegmentItem*)),
                     this,           SLOT(updateSegmentDuration(SegmentItem*)));

    QObject::connect(m_segmentCanvas, SIGNAL(updateSegmentTrackAndStartIndex(SegmentItem*)),
                     this,           SLOT(updateSegmentTrackAndStartIndex(SegmentItem*)));

    // create the position pointer
    m_pointer = new QCanvasLine(canvas);
    m_pointer->setPen(RosegardenGUIColours::TimePointer);
    m_pointer->setPoints(0, 0, 0, canvas->height());
    m_pointer->setZ(10);
    m_pointer->show();
}

void
TrackEditor::setupSegments()
{
    kdDebug(KDEBUG_AREA) << "TrackEditor::setupSegments() begin" << endl;

    if (!m_document) return; // sanity check
    
    Composition &comp = m_document->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

        if (!(*i)) continue;

	kdDebug(KDEBUG_AREA) << "TrackEditor::setupSegments() add segment"
			     << " - start idx : " << (*i)->getStartIndex()
			     << " - nb time steps : " << (*i)->getDuration()
			     << " - track : " << (*i)->getTrack()
			     << endl;
	
	int startBar = comp.getBarNumber((*i)->getStartIndex(), true);
	int barCount = comp.getBarNumber((*i)->getEndIndex(), true)
	    - startBar +1;
	
	int y = m_vHeader->sectionPos((*i)->getTrack());
	int x = m_hHeader->sectionPos(startBar);
	
	SegmentItem *newItem = m_segmentCanvas->addPartItem(x, y, barCount);
	newItem->setSegment(*i);
    }
}


void TrackEditor::addSegment(int track, int start,
                            unsigned int nbTimeSteps)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();

    Rosegarden::Segment* segment =
        new Rosegarden::Segment(Rosegarden::Segment::Internal, start);

    segment->setTrack(track);
    comp.addSegment(segment);
    segment->setDuration(nbTimeSteps);

    int startBar = comp.getBarNumber(start, true);
    int barCount = comp.getBarNumber(start + nbTimeSteps, true) - startBar + 1;

    int y = m_vHeader->sectionPos(track);
    int x = m_hHeader->sectionPos(startBar);

    SegmentItem *newItem = m_segmentCanvas->addPartItem(x, y, barCount);
    newItem->setSegment(segment);

    emit needUpdate();
}


void TrackEditor::segmentOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    updateSegmentOrder();
    emit needUpdate();
}


void
TrackEditor::addSegment(SegmentItem *p)
{
    // first find track for part, as it is used for indexing
    //
    int track = m_vHeader->sectionAt(static_cast<int>(p->y()));

    emit createNewSegment(p, track);

    kdDebug(KDEBUG_AREA) << QString("TrackEditor::addSegment() : segment track is %1 at y=%2")
        .arg(track).arg(p->y())
                         << ", start bar = " << p->getStartBar()
                         << ", p = " << p << endl;

}


void TrackEditor::deleteSegment(Rosegarden::Segment *p)
{
    Composition& composition = m_document->getComposition();

    if (!composition.deleteSegment(p)) {
        KMessageBox::error(0, QString("TrackEditor::deleteSegment() : part %1 not found").arg(long(p), 0, 16));
        
        kdDebug(KDEBUG_AREA) << "TrackEditor::deleteSegment() : segment "
                             << p << " not found" << endl;
    }
}


void TrackEditor::updateSegmentDuration(SegmentItem *i)
{
    Composition& composition = m_document->getComposition();

    int startBar = m_hHeader->sectionAt(int(i->x()));
    int barCount = i->getItemNbBars();

    timeT startIndex = composition.getBarRange(startBar, false).first;
    timeT duration = composition.getBarRange(startBar + barCount, false).first -
	startIndex;

    kdDebug(KDEBUG_AREA) << "TrackEditor::updateSegmentDuration() : set duration to "
                         << duration << endl;

    i->getSegment()->setDuration(duration);
}


void TrackEditor::updateSegmentTrackAndStartIndex(SegmentItem *i)
{
    Composition& composition = m_document->getComposition();

    int track = m_vHeader->sectionAt(int(i->y()));
    int startBar = m_hHeader->sectionAt(int(i->x()));
    timeT startIndex = composition.getBarRange(startBar, false).first;

    kdDebug(KDEBUG_AREA) << "TrackEditor::updateSegmentTrackAndStartIndex() : set track to "
                         << track
                         << " - start Index to : " << startIndex << endl;

    composition.setSegmentStartIndexAndTrack(i->getSegment(), startIndex, track);

    m_document->documentModified();

}


void TrackEditor::updateSegmentOrder()
{
    QCanvasItemList itemList = canvas()->canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        QCanvasItem *item = *it;
        SegmentItem *segmentItem = dynamic_cast<SegmentItem*>(item);
        
        if (segmentItem) {
            segmentItem->setY(m_vHeader->sectionPos(segmentItem->getTrack()));
        }
    }
}


void TrackEditor::clear()
{
    m_segmentCanvas->clear();
}


void TrackEditor::setupHorizontalHeader(int firstBar, int lastBar)
{
    QString num;
    Composition &comp = m_document->getComposition();
    m_hHeader = new QHeader(lastBar - firstBar + 1, this);

    int x = 0;

    for (int i = firstBar; i <= lastBar; ++i) {

	// The (i < lastBar) case resynchronises against the absolute
	// bar position at each stage so as to avoid gradually increasing
	// error through integer rounding

	int width;
	if (i < lastBar) {
	    width = (int)(m_rulerScale->getBarPosition(i+1) - (double)x);
	    x += width;
	} else {
	    width = (int)(m_rulerScale->getBarWidth(i));
	}

	m_hHeader->resizeSection(i - firstBar, width);
        m_hHeader->setLabel(i - firstBar, num.setNum(i));
    }
}


// Move the position pointer
void
TrackEditor::setPointerPosition(Rosegarden::timeT position)
{

//    kdDebug(KDEBUG_AREA) << "TrackEditor::setPointerPosition: time is " << position << endl;
    if (!m_pointer) return;

    double canvasPosition = m_rulerScale->getXForTime(position);
    double distance = (double)canvasPosition - m_pointer->x();

    if (distance < 0.0) distance = -distance;
    if (distance >= 1.0) {

	m_pointer->setX(canvasPosition);
        emit scrollHorizTo((int)canvasPosition);
	emit needUpdate();
    }
}

void
TrackEditor::setSelectAdd(bool value)
{
    m_segmentCanvas->setSelectAdd(value);
}

void
TrackEditor::setSelectCopy(bool value)
{
     m_segmentCanvas->setSelectCopy(value);
}


// Just like setupSegments() this creates a SegmentItem
// on the SegmentCanvas after we've recorded a Segment.
//
//
void
TrackEditor::addSegmentItem(Rosegarden::Segment *segment)
{
    if (!m_document) return; // sanity check

    Composition &comp = m_document->getComposition();

    int startBar = comp.getBarNumber(segment->getStartIndex(), true);
    int barCount = comp.getBarNumber(segment->getEndIndex(), true)
                        - startBar +1;

    int y = m_vHeader->sectionPos(segment->getTrack());
    int x = m_hHeader->sectionPos(startBar);

    SegmentItem *newItem = m_segmentCanvas->addPartItem(x, y, barCount);
    newItem->setSegment(segment);

    emit needUpdate();

}

// Show a Segment as its being recorded
//
void
TrackEditor::updateRecordingSegmentItem(Rosegarden::Segment *segment)
{
    Composition &comp = m_document->getComposition();

    int y = m_vHeader->sectionPos(segment->getTrack());

    int startBar = comp.getBarNumber(segment->getStartIndex(), false);
    std::pair<timeT, timeT> bar = comp.getBarRange(startBar, false);
     
    int xAdj = (segment->getStartIndex() - bar.first) *
                m_hHeader->sectionSize(startBar)/
                (bar.second - bar.first);

    int x = m_hHeader->sectionPos(startBar) + xAdj;

    int endBar = comp.getBarNumber(comp.getPosition(), false);
    bar = comp.getBarRange(endBar, false);

    int wAdj = (comp.getPosition() - bar.first) *
                m_hHeader->sectionSize(endBar)/
                (bar.second - bar.first);

    int width = m_hHeader->sectionPos(endBar) + wAdj - x;

    m_segmentCanvas->showRecordingSegmentItem(x, y, width);
    emit needUpdate();
}

void
TrackEditor::destroyRecordingSegmentItem()
{
    m_segmentCanvas->destroyRecordingSegmentItem();
}




