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

#ifndef SEGMENTSEDITOR_H
#define SEGMENTSEDITOR_H

#include <list>
#include <qwidget.h>
#include <qheader.h>
#include <qcanvas.h>
#include "trackheader.h"

#include "trackseditoriface.h"

#include "Event.h" // for timeT

namespace Rosegarden { class Segment; }
class SegmentItem;
class SegmentCanvas;
class RosegardenGUIDoc;
class RulerScale;

/**
 * Global widget for segment edition.
 *
 * Shows a global overview of the composition, and lets the user
 * manipulate the segments
 *
 * @see SegmentCanvas
 */
class TrackEditor : public QWidget, virtual public TrackEditorIface
{
    Q_OBJECT
public:
    /**
     * Create a new TrackEditor representing the document \a doc
     */
    TrackEditor(RosegardenGUIDoc* doc,
		RulerScale *rulerScale,
		QWidget* parent = 0, const char* name = 0,
		WFlags f=0);

    /// Clear the SegmentCanvas
    void clear();

    /**
     * Reset all the segments Y coordinates after the order of the
     * instruments has been changed
     */
    void updateSegmentOrder();

    SegmentCanvas*       canvas()       { return m_segmentCanvas; }
    const SegmentCanvas* canvas() const { return m_segmentCanvas; }

    /**
     * Must be called after construction and signal connection
     * if a document was passed to ctor, otherwise segments will
     * be created but not registered in the main doc
     */
    void setupSegments();

    /**
     * Add a new segment - DCOP interface
     */
    virtual void addSegment(int track, int start, unsigned int nbTimeSteps);

    /**
     *  Return the track header pointer for scrutiny
     */
    Rosegarden::TrackHeader* getVHeader() { return m_vHeader; }
    QHeader *getHHeader() { return m_hHeader; }


public slots:
    /**
     * Set the position pointer during playback
     */
    void setPointerPosition(Rosegarden::timeT position);

    /**
     * Create a Segment Item from a Segment (after recording)
     *
     */
    void addSegmentItem(Rosegarden::Segment *segment);

    /**
     * c.f. what we have in rosegardenguiview.h
     * These are instrumental in passing through
     * key presses from GUI front panel down to
     * the SegmentCanvas.
     *
     */
    void setSelectAdd(bool value);
    void setSelectCopy(bool value);


protected slots:
    void segmentOrderChanged(int section, int fromIdx, int toIdx);
    void addSegment(SegmentItem*);
    void deleteSegment(Rosegarden::Segment*);
    void updateSegmentDuration(SegmentItem*);
    void updateSegmentTrackAndStartIndex(SegmentItem*);

signals:
    /**
     * Emitted when the represented data changed and the SegmentCanvas
     * needs to update itself
     *
     * @see SegmentCanvas::update()
     */
    void needUpdate();

    /**
     * Emitted when the SegmentCanvas needs to be scrolled 
     * horizontally to a position
     *
     * @see 
     */
    void scrollHorizTo(int);

    /**
     * Emitted when a new segment is created by the user
     * \a item is the SegmentItem representing the segment on the SegmentCanvas
     * \a instrument is the instrument for the segment
     *
     * It is up to the slot to create the new Segment, to insert it in the
     * Composition and to set its instrument.
     *
     * @see RosegardenGUIDoc::createNewSegment()
     */
    void createNewSegment(SegmentItem* item, int instrument);

protected:

    void init(unsigned int nbTracks, int firstBar, int lastBar);
    void setupHorizontalHeader(int firstBar, int lastBar);

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_document;
    RulerScale *m_rulerScale;

    SegmentCanvas *m_segmentCanvas;
    QHeader *m_hHeader;
    Rosegarden::TrackHeader *m_vHeader;

    QCanvasLine *m_pointer;
};

#endif
