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

#include <cassert>

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qwhatsthis.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qbitmap.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <kconfig.h>

#include "Colour.h"
#include "ColourMap.h"
#include "Composition.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "RulerScale.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "segmentcanvas.h"
#include "segmenttool.h"
#include "Segment.h"

#include "rosedebug.h"
#include "colours.h"

using Rosegarden::Segment;
using Rosegarden::SegmentSelection;
using Rosegarden::Note;
using Rosegarden::RulerScale;
using Rosegarden::SnapGrid;
using Rosegarden::TrackId;
using Rosegarden::timeT;
using Rosegarden::Colour;
using Rosegarden::ColourMap;
using Rosegarden::getCombinationColour;

class SegmentRepeatRectangle : public QCanvasRectangle
{
public:
    SegmentRepeatRectangle(QCanvas *,
			   Rosegarden::Segment *,
                           SnapGrid *,
               RosegardenGUIDoc *);

    void setRepeatInterval(unsigned int i) { m_repeatInterval = i; }

    // only stored to make it easy to identify this rectangle later
    Rosegarden::Segment *getSegment() const { return m_segment; }

    Rosegarden::timeT getRepeatStartTime(int x);

    virtual void drawShape(QPainter&);

protected:
    Rosegarden::Segment *m_segment;
    unsigned int m_repeatInterval;
    SnapGrid    *m_snapGrid;
    RosegardenGUIDoc *m_doc;
};

SegmentRepeatRectangle::SegmentRepeatRectangle(QCanvas *canvas,
					       Rosegarden::Segment *segment,
                                               SnapGrid *snapGrid, 
                           RosegardenGUIDoc    *doc)
    : QCanvasRectangle(canvas),
      m_segment(segment),
      m_repeatInterval(0),
      m_snapGrid(snapGrid),
      m_doc(doc)
{
    setBrush(RosegardenGUIColours::convertColour
	     (m_doc->getComposition().getSegmentColourMap().getColourByIndex
	      (m_segment->getColourIndex())).light(150));
    setPen(RosegardenGUIColours::RepeatSegmentBorder);
}

Rosegarden::timeT SegmentRepeatRectangle::getRepeatStartTime(int ex)
{
    int rWidth = int(m_snapGrid->getRulerScale()->
		     getXForTime(m_repeatInterval));

    int count = (ex - int(x())) / rWidth;
    
    // maybe this stuff should be in segmentitem after all...
    return m_segment->getEndMarkerTime() + count *
	(m_segment->getEndMarkerTime() - m_segment->getStartTime());
}


void SegmentRepeatRectangle::drawShape(QPainter& painter)
{
    QCanvasRectangle::drawShape(painter);

    int pos = int(x()); 
    int width = rect().width();
    int height = rect().height();

    int rWidth = int(m_snapGrid->getRulerScale()->
		     getXForTime(m_repeatInterval));

    setBrush(RosegardenGUIColours::convertColour
	     (m_doc->getComposition().getSegmentColourMap().getColourByIndex
	      (m_segment->getColourIndex())).light(150));
    painter.setPen(RosegardenGUIColours::RepeatSegmentBorder);

    while (pos < width + rWidth)
    {
        painter.drawRect(pos, int(y()), rWidth, height);
        pos += rWidth;
    }

}

//////////////////////////////////////////////////////////////////////
//                SegmentItemPreview
// (declared in segmenttool.h because tools need to set preview currency)
//////////////////////////////////////////////////////////////////////

SegmentItemPreview::SegmentItemPreview(SegmentItem& parent,
                                       Rosegarden::RulerScale* scale)
    : m_parent(parent),
      m_segment(parent.getSegment()),
      m_rulerScale(scale),
      m_previewIsCurrent(false)
{
}

SegmentItemPreview::~SegmentItemPreview()
{
}

QRect SegmentItemPreview::rect()
{
    return m_parent.rect();
}


class SegmentAudioPreview : public SegmentItemPreview
{
public:
    SegmentAudioPreview(SegmentItem& parent, Rosegarden::RulerScale* scale);

    virtual void drawShape(QPainter&);

protected:

    virtual void updatePreview(const QWMatrix &matrix);

    //--------------- Data members ---------------------------------

    std::vector<float>   m_values;
    bool                 m_showMinima;
    unsigned int         m_channels;
};

SegmentAudioPreview::SegmentAudioPreview(SegmentItem& parent,
                                         Rosegarden::RulerScale* scale)
    : SegmentItemPreview(parent, scale),
    m_showMinima(false),
    m_channels(0)
{
}

void SegmentAudioPreview::drawShape(QPainter& painter)
{
    // Fetch new set of values
    //
    updatePreview(painter.worldMatrix());

    // If there's nothing to draw then don't draw it
    //
    if (m_values.size() == 0)
        return;

    painter.save();
    //painter.translate(rect().x(), rect().y());

    // perhaps antialias this somehow at some point
    int height = rect().height()/2 - 3;
    int halfRectHeight = rect().height()/2;

    // Sometimes our audio file fails to get recorded properly and
    // the number of channels fails too.  Guard against this.
    //
    if (m_channels == 0)
    {
        std::cerr << "SegmentAudioPreview::drawShape - m_channels == 0 "
                  << "problem with audio file" <<std::endl;
	//painter.restore();
        return;
    }

    int samplePoints = m_values.size() / (m_channels * (m_showMinima ? 2 : 1));
    float h1, h2, l1 = 0, l2 = 0;

    /*
    RG_DEBUG << "SegmentAudioPreview::drawShape - "
             << "samplePoints = " << samplePoints << endl;
             */

    // The visible width of the rectangle
    //
    QWMatrix matrix = painter.worldMatrix();
    QRect tRect = painter.worldMatrix().map(rect());
    double sampleScaleFactor = samplePoints / double(tRect.width());
    double drawScaleFactor = double(rect().width())/double(tRect.width());

    bool state = painter.hasWorldXForm();
    painter.setWorldXForm(false);

    for (int i = 0; i < tRect.width(); ++i)
    {
        // For each i work get the sample starting point
        //
        int position = int(m_channels * i * sampleScaleFactor);

        if (m_channels == 1) {

            h1 = m_values[position++];
            h2 = h1;

            if (m_showMinima)
            {
                l1 = m_values[position++];
                l2 = l1;
            }
        }
        else {

            h1 = m_values[position++];
            if (m_showMinima) l1 = m_values[position++];

            h2 = m_values[position++];
            if (m_showMinima) l2 = m_values[position++];
            
        }

        /*
        std::cout << "PAINT at x = " << tRect.x() + i 
                  << ", y = " << rect().y() << std::endl;
                  */

        painter.setPen(RosegardenGUIColours::SegmentAudioPreview);
        painter.drawLine(tRect.x() + i,
                         tRect.y() + int(halfRectHeight - h1 * height),
                         tRect.x() + i,
                         tRect.y() + int(halfRectHeight + h2 * height));

        // For the moment draw it the same colour - the resolution on the
        // segmentcanvas doens't allow us to tell the difference between
        // minima and maxima anyway so we might as well ignore it.  Just
        // don't ask for minima yet.
        // 
        //painter.setPen(Qt::white);

        /*
        painter.drawLine(i,
                         static_cast<int>(halfRectHeight + l1 * height),
                         i,
                         static_cast<int>(halfRectHeight - l2 * height));
                         */
    }

    // perhaps draw an XOR'd label at some point
    /*
      painter.setPen(RosegardenGUIColours::SegmentLabel);
      painter.setFont(*m_font);
      QRect labelRect = rect();
      labelRect.setX(labelRect.x() + 3);
      painter.drawText(labelRect, Qt::AlignLeft|Qt::AlignVCenter, m_label);
    */

    painter.setWorldXForm(state);
    painter.restore();
}

void SegmentAudioPreview::updatePreview(const QWMatrix &matrix)
{
    if (isPreviewCurrent()) return;

    // Fetch vector of floats adjusted to our resolution
    //

    Rosegarden::AudioFileManager &aFM = m_parent.getDocument()->getAudioFileManager();

    Rosegarden::Composition &comp = m_parent.getDocument()->getComposition();

    // Get sample start and end times and work out duration
    //
    Rosegarden::RealTime audioStartTime = m_segment->getAudioStartTime();
    Rosegarden::RealTime audioEndTime = audioStartTime +
        comp.getElapsedRealTime(m_parent.getEndTime()) -
        comp.getElapsedRealTime(m_parent.getStartTime()) ;

    try
    {
        RG_DEBUG << "SegmentAudioPreview::updatePreview() - for file id "
                 << m_segment->getAudioFileId() << " fetching values" <<endl;

        QRect tRect = matrix.map(rect());
        QRect uRect = matrix.map(rect());

        RG_DEBUG << "SegmentAudioPreview::updatePreview "
                 << "rect().width() = " << rect().width()
                 << ", mapped width = " << tRect.width()
                 << ", inverse mapped width = " <<  uRect.width()
                 << endl;

        m_values =
            aFM.getPreview(m_segment->getAudioFileId(),
                           audioStartTime,
                           audioEndTime,
                           tRect.width(),
                           m_showMinima); // do we get and show the minima too?
    }
    catch (std::string e)
    {
        // empty all values out
        RG_DEBUG << "SegmentAudioPreview::updatePreview : "
                 << e.c_str() << endl;
        
        m_values.clear();
    }


    // If we haven't inserted the audio file yet we're probably
    // just still recording it.
    //
    if (m_values.size() == 0) {
        RG_DEBUG << "SegmentAudioPreview::updatePreview : no values\n";
        return;
    }

    m_channels = aFM.getAudioFile(m_segment->getAudioFileId())->getChannels();

    setPreviewCurrent(true);
}

//////////////////////////////////////////////////////////////////////
//                SegmentNotationPreview
//////////////////////////////////////////////////////////////////////

class SegmentNotationPreview : public SegmentItemPreview
{
public:
    SegmentNotationPreview(SegmentItem& parent,
			   Rosegarden::RulerScale* scale);

    virtual void drawShape(QPainter&);

protected:
    virtual void updatePreview(const QWMatrix &matrix);

    struct RectCompare {
	bool operator()(const QRect &r1, const QRect &r2) const {
	    return r1.x() < r2.x();
	}
//	bool operator()(const QRect *r1, const QRect *r2) const {
//	    return r1->x() < r2->x();
//	}
    };

    //--------------- Data members ---------------------------------
    bool m_haveSegmentRefreshStatus;
    unsigned int m_segmentRefreshStatus;
    typedef std::multiset<QRect, RectCompare> RectList;
    RectList m_previewInfo;
};

SegmentNotationPreview::SegmentNotationPreview(SegmentItem& parent,
                                               Rosegarden::RulerScale* scale)
    : SegmentItemPreview(parent, scale),
      m_haveSegmentRefreshStatus(false)
{
}

void SegmentNotationPreview::drawShape(QPainter& painter)
{
    updatePreview(painter.worldMatrix());
    painter.save();

    painter.translate(rect().x(), rect().y());
    painter.setPen(RosegardenGUIColours::SegmentInternalPreview);
    QRect viewportRect = painter.xFormDev(painter.viewport());

    double scaleFactor = m_rulerScale->getXForTime(Rosegarden::Note(Rosegarden::Note::Crotchet).
            getDuration()) / double(Rosegarden::Note(Rosegarden::Note::Crotchet).getDuration());

    for (RectList::iterator i = m_previewInfo.begin();
	 i != m_previewInfo.end(); ++i) {

        // draw rectangles, discarding those which are clipped
        //
        QRect p = *i;
        //int scaledX = int(m_rulerScale->getXForTime((*i).x()));
        //int scaledWidth = int(m_rulerScale->getXForTime((*i).width()));
        //if (!scaledWidth) scaledWidth = 1;

        p.setX(int((double((*i).x()) * scaleFactor)));
        p.setWidth(int((double((*i).width()) * scaleFactor)));

        if (p.width() == 0) p.setWidth(1);

	if (p.x() > (viewportRect.x() + viewportRect.width())) break;
        if ((p.x() + p.width()) >= viewportRect.x()) {

            painter.drawRect(p);

            /*
            RG_DEBUG << "SegmentNotationPreview::drawShape - "
                     << "p.x() = " << p.x()
                     << ", p.width() = " << p.width()
                     << ", p.y() = " << p.y()
                     << ", p.height() = " << p.height()
                     << endl;

            QRect tRect = painter.xFormDev(p);
            RG_DEBUG << "SegmentNotationPreview::drawShape -  transformed "
                     << "p.x() = " << tRect.x()
                     << ", p.width() = " << tRect.width()
                     << endl;
                     */
        }
    }

    painter.restore();
}

void SegmentNotationPreview::updatePreview(const QWMatrix & /*matrix*/)
{
    if (isPreviewCurrent()) return;

    //RG_DEBUG << "SegmentNotationItem::updatePreview() "
                         //<< this << endl;

    if (!m_haveSegmentRefreshStatus) {
	m_segmentRefreshStatus = m_segment->getNewRefreshStatusId();
	m_haveSegmentRefreshStatus = true;
    }

    Rosegarden::SegmentRefreshStatus &status =
	m_segment->getRefreshStatus(m_segmentRefreshStatus);
    if (!status.needsRefresh()) return;

    Rosegarden::timeT fromTime = status.from(), toTime = status.to();
    if (fromTime == toTime) {
	fromTime = m_segment->getStartTime();
	  toTime = m_segment->getEndMarkerTime();
    }

    int fromX = (int)m_rulerScale->getXForTime(fromTime) - rect().x();
    int   toX = (int)m_rulerScale->getXForTime(  toTime) - rect().x();

    QRect fromRect(fromX, 0, 10, 10), toRect(toX, 0, 10, 10);
    RectList::iterator fromi = m_previewInfo.lower_bound(fromRect);
    RectList::iterator   toi = m_previewInfo.lower_bound(  toRect);
    m_previewInfo.erase(fromi, toi);

    for (Segment::iterator i = m_segment->findTime(fromTime);
	 m_segment->isBeforeEndMarker(i) && i != m_segment->findTime(toTime);
	 ++i) {

        long pitch = 0;
        if (!(*i)->isa(Rosegarden::Note::EventType) ||
            !(*i)->get<Rosegarden::Int>
            (Rosegarden::BaseProperties::PITCH, pitch)) {
            continue;
        }

	timeT eventStart = (*i)->getAbsoluteTime();
	timeT eventEnd = eventStart + (*i)->getDuration();
	if (eventEnd > m_segment->getEndMarkerTime()) {
	    eventEnd = m_segment->getEndMarkerTime();
	}

        // FIX for 904300 - store the time rather than the position
        // and then scale with rulerScale.  Inefficient I'd imagine
        // but at least it works.
        //
        double x0 = eventStart - m_rulerScale->getTimeForX(rect().x());
        double x1 = eventEnd - m_rulerScale->getTimeForX(rect().x());

        /*
        RG_DEBUG << "SegmentNotationPreview::updatePreview - "
                 << "x0 = " << x0
                 << ", x1 = " << x1 << endl;
                 */

        int width = (int)(x1 - x0) - 2;
	if (width < 1) width = 1;

        double y0 = 0;
        double y1 = rect().height();
        double y = y1 + ((y0 - y1) * (pitch-16)) / 96;
        if (y < y0) y = y0;
        if (y > y1-1) y = y1-1;

        QRect r((int)x0, (int)y, width, 2);
        m_previewInfo.insert(r);
    }

    status.setNeedsRefresh(false);
    setPreviewCurrent(true);
}


//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

QFont *SegmentItem::m_font = 0;
QFontMetrics *SegmentItem::m_fontMetrics = 0;
int SegmentItem::m_fontHeight = 0;

SegmentItem::SegmentItem(TrackId trackPosition, timeT startTime, timeT endTime,
			 bool showPreview,
                         SnapGrid *snapGrid, QCanvas *canvas,
                         RosegardenGUIDoc *doc) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(0),
    m_doc(doc),
    m_trackPosition(trackPosition),
    m_startTime(startTime),
    m_endTime(endTime),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0),
    m_colour(RosegardenGUIColours::convertColour
	     (doc->getComposition().getSegmentColourMap().getColourByIndex(0))),
    m_overrideColour(false),
    m_preview(0),
    m_showPreview(showPreview)
{
    if (!m_font) makeFont();

    recalculateRectangle(true);
    setPreview();
}

SegmentItem::SegmentItem(Segment *segment,
			 bool showPreview,
                         SnapGrid *snapGrid,
                         QCanvas *canvas,
                         RosegardenGUIDoc *doc) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(segment),
    m_doc(doc),
    m_trackPosition(0),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0),
    m_colour(RosegardenGUIColours::convertColour
	     (doc->getComposition().getSegmentColourMap().getColourByIndex(0))),
    m_overrideColour(false),
    m_preview(0),
    m_showPreview(showPreview),
    m_suspendPreview(false),
    m_fontScale(1.0)

{
    if (!m_font) makeFont();

    recalculateRectangle(true);
    setPreview();
}

SegmentItem::~SegmentItem()
{
    delete m_preview;

    QCanvasItemList allItems = canvas()->allItems();

    if (m_repeatRectangle && allItems.find(m_repeatRectangle) != allItems.end())
        CanvasItemGC::mark(m_repeatRectangle);
    else
        RG_DEBUG << "~SegmentItem() - m_repeatRectangle not in items list - canvas is probably being deleted\n";

}

void SegmentItem::setShowPreview(bool preview)
{
    m_showPreview = preview;
}

void SegmentItem::setPreview()
{
    delete m_preview;
    m_preview = 0;
    if (!m_segment) return;

    if (m_segment->getType() == Rosegarden::Segment::Audio)
        m_preview = new SegmentAudioPreview(*this,
                                            m_snapGrid->getRulerScale());
    else if (m_segment->getType() == Rosegarden::Segment::Internal)
        m_preview = new SegmentNotationPreview(*this,
                                               m_snapGrid->getRulerScale());
}

void SegmentItem::setColour(QColor c)
{
    m_colour = c;
    m_overrideColour = true;
}

void SegmentItem::makeFont()
{
    m_font = new QFont();
    m_font->setPixelSize(m_snapGrid->getYSnap() / 2);
    m_font->setWeight(QFont::Bold);
    m_font->setItalic(false);
    m_fontMetrics = new QFontMetrics(*m_font);
    m_fontHeight = m_fontMetrics->boundingRect("|^M,g").height();
}

void SegmentItem::draw(QPainter& painter)
{
    m_fontScale = painter.worldMatrix().m11();
    //RG_DEBUG << "SegmentItem::draw - update font scale = " << m_fontScale << endl;
    QCanvasRectangle::draw(painter);
}

void SegmentItem::drawShape(QPainter& painter)
{
//    RG_DEBUG << "SegmentItem::drawShape: my width is " << width() << endl;

    QCanvasRectangle::drawShape(painter);

    // Can't use collisions() here : it's too slow. Go the simpler way.
    //
    QCanvasItemList items = canvas()->allItems();

    painter.save();

    bool colourset = false;

    for (QCanvasItemList::Iterator it=items.begin(); it!=items.end(); ++it) {

        if (!colourset)
        {
            if (m_segment) {
                painter.setBrush
		    (RosegardenGUIColours::convertColour
		     (m_doc->getComposition().getSegmentColourMap().getColourByIndex
		      (m_segment->getColourIndex())).dark(125));
		colourset = true;
	    }
        }

        if ((*it) == this) continue; // skip ourselves

        SegmentItem *item = dynamic_cast<SegmentItem*>(*it);

        if (!item) continue;
        if (item->getTrackPosition() != getTrackPosition()) continue;
        QRect intersection = rect() & item->rect();
        if (!intersection.isValid()) continue;

        painter.drawRect(intersection);
    }

    if (m_preview && m_showPreview) {
/*!!!
	Rosegarden::Colour col;

	if (m_segment && !m_overrideColour) {
	    col = (m_doc->getComposition().getSegmentColourMap().getColourByIndex
		   (m_segment->getColourIndex()));
	} else {
	    col = RosegardenGUIColours::convertColour(m_colour);
	}

	painter.setPen(RosegardenGUIColours::convertColour
		       (col.getContrastingColour()));
*/
	m_preview->drawShape(painter);
    }

    // draw label
    if (m_segment) 
    {

        //RG_DEBUG << "SegmentItem::drawShape - has world Xform = " << painter.hasWorldXForm() << endl;

        // Don't show label if we're showing the preview
        //
        if (m_showPreview && m_segment->getType() == Rosegarden::Segment::Audio) {
	    painter.restore();
            return;
	}

        // Store the current transform matrix in something local
        //
        QWMatrix matrix = painter.worldMatrix();

        // Unset the global transform so we don't bend the text
        //
        bool state = painter.hasWorldXForm();
        painter.setWorldXForm(false);
        painter.setFont(*m_font);

        QRect labelRect = matrix.mapRect(rect()); // map the rectangle to the transform
        int x = labelRect.x() + 3;
        int y = labelRect.y();

        // This will be useful over the top of audio previews once
        // we've got vertical zooming too - for the moment it's nice
        // to have it for audio segments too.  [rwb]
        //
        if(m_segment->getType() == Rosegarden::Segment::Audio)
        {
            painter.setPen(Qt::white);
            for (int wX = x - 2; wX < x + 2; wX++)
                for (int wY = y - 2; wY < y + 2; wY++)
                {
                    labelRect.setX(wX);
                    labelRect.setY(wY);

                    painter.drawText(labelRect,
                                     Qt::AlignLeft|Qt::AlignVCenter,
                                     m_label);
                }
        }

        labelRect.setX(x);
        labelRect.setY(y);
        painter.setPen(RosegardenGUIColours::SegmentLabel);
        painter.drawText(labelRect, Qt::AlignLeft|Qt::AlignVCenter, m_label);

        // Reenable the transform state
        //
        painter.setWorldXForm(state);
    }

    painter.restore();

}

void SegmentItem::recalculateRectangle(bool inheritFromSegment)
{
    // Ok, now draw as before
    //
    canvas()->setChanged(rect());

    // Get our segment colour
    QColor brush;
    if (m_segment && !m_overrideColour) {
        brush = RosegardenGUIColours::convertColour
	    (m_doc->getComposition().getSegmentColourMap().getColourByIndex
	     (m_segment->getColourIndex()));
    } else {
        brush = m_colour;
    }

    // Compute repeat rectangle if any
    //
    if (m_segment && inheritFromSegment) {

	m_trackPosition = m_doc->getComposition().
            getTrackById(m_segment->getTrack())->getPosition();
	m_startTime = m_segment->getStartTime();
	m_endTime = m_segment->getEndMarkerTime();
        m_label = strtoqstr(m_segment->getLabel());

	if (m_segment->isRepeating()) {
            if (!m_repeatRectangle)
                m_repeatRectangle = new SegmentRepeatRectangle(canvas(),
							       getSegment(),
                                                               m_snapGrid, m_doc);

	    // Set the colour for the repeat rectangle
	    m_repeatRectangle->setBrush(brush.light(150));

	    timeT repeatStart = m_endTime;
	    timeT repeatEnd = m_segment->getRepeatEndTime();

	    m_repeatRectangle->setX
		(int(m_snapGrid->getRulerScale()->getXForTime(repeatStart)) + 1);
	    m_repeatRectangle->setY
		(m_snapGrid->getYBinCoordinate(m_trackPosition));
	    m_repeatRectangle->setSize
		((int)m_snapGrid->getRulerScale()->getWidthForDuration
		 (repeatStart, repeatEnd - repeatStart) + 1,
		 m_snapGrid->getYSnap());

            // Let the repeat rectangle do the conversions
            m_repeatRectangle->setRepeatInterval(m_endTime - m_startTime);

            m_repeatRectangle->show();

	} else if (m_repeatRectangle) {
            m_repeatRectangle->hide();
        }
        
    }

    // Compute main rectangle
    //
    setX(m_snapGrid->getRulerScale()->getXForTime(m_startTime));
    setY(m_snapGrid->getYBinCoordinate(m_trackPosition));

    // Set our segment brush colour
    if (m_selected)
        setBrush(brush.dark(200));
    else
        setBrush(brush);

    int h = m_snapGrid->getYSnap();
    double w = m_snapGrid->getRulerScale()->getWidthForDuration
	(m_startTime, m_endTime - m_startTime);

//    RG_DEBUG << "SegmentItem::recalculateRectangle: start time " << m_startTime << ", end time " << m_endTime << ", width " << w << endl;
    
    setSize(int(w) + 1, h);

    // Compute label
    //
    bool dots = false;

    // How do we make this allow for a world transformation matrix?
    //
    while (m_label.length() > 0 &&
	   (double(m_fontMetrics->boundingRect(
            dots ? (m_label + "...") : m_label).width()) / m_fontScale) > double(width() - 5)) {
	if (!dots && m_label.length() > 6) {
	    m_label.truncate(m_label.length() - 4);
	    dots = true;
	} else if (dots && m_label.length() < 2) {
	    dots = false;
	} else {
	    m_label.truncate(m_label.length() - 1);
	}
    }

    if (dots) m_label += "...";
    canvas()->setChanged(rect());

    if (m_preview) {
	m_preview->setPreviewCurrent(false);
    }
}

Segment* SegmentItem::getSegment() const
{
    return m_segment;
}

void SegmentItem::setSegment(Segment *segment)
{
    m_segment = segment;
    recalculateRectangle(true);
}

void SegmentItem::setStartTime(timeT t)
{
    m_startTime = t;
    recalculateRectangle(false);
}

void SegmentItem::setEndTime(timeT t)
{
    m_endTime = t;
    recalculateRectangle(false);
}

void SegmentItem::setTrackPosition(TrackId trackPosition)
{
    m_trackPosition = trackPosition;
    recalculateRectangle(false);
}

void SegmentItem::normalize()
{
    if (m_endTime < m_startTime) {
	timeT temp = m_endTime;
	m_endTime = m_startTime;
	m_startTime = temp;
	recalculateRectangle(false);
    }
}

// Set this SegmentItem as selected/highlighted - we send
// in the QBrush we need at the same time
//
void SegmentItem::setSelected(bool select, const QBrush &brush)
{
    setBrush(brush);
    m_selected = select;
    setZ(select ? 2 : 1); // selected items come to the front
}

void SegmentItem::showRepeatRect(bool s)
{
    //!!! This doesn't really work terribly well... need a better fix
    // for the remaining preview trails when you move a segment
/*!!!
    m_suspendPreview = !s;
    if (m_showPreview) {
	if (m_suspendPreview) {
	    delete m_preview;
	    m_preview = 0;
	} else {
	    setPreview();
	}
    }
*/
    if (m_repeatRectangle) {
    
	if (s)
	    m_repeatRectangle->show();
	else
	    m_repeatRectangle->hide();
    }

    update();
}

//////////////////////////////////////////////////////////////////////
////             SegmentSplitLine
//////////////////////////////////////////////////////////////////////
SegmentSplitLine::SegmentSplitLine(int x, int y, int height,
                                   Rosegarden::RulerScale *rulerScale,
                                   QCanvas* canvas):
                                   QCanvasLine(canvas),
                                   m_rulerScale(rulerScale),
                                   m_height(height)
{
    setPen(RosegardenGUIColours::SegmentSplitLine);
    setBrush(RosegardenGUIColours::SegmentSplitLine);
    setZ(3);
    moveLine(x, y);
}

void SegmentSplitLine::moveLine(int x, int y)
{
    setPoints(x, y, x, y + m_height);
    show();
}

void SegmentSplitLine::hideLine()
{
    hide();
}



//////////////////////////////////////////////////////////////////////
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////

SegmentCanvas::SegmentCanvas(RosegardenGUIDoc *doc,
                             RulerScale *rulerScale,
                             int vStep,
			     QCanvas* c, QWidget* parent,
			     const char* name, WFlags f) :
    RosegardenCanvasView(c, parent, name, f),
    m_tool(0),
    m_grid(rulerScale, vStep),
    m_currentItem(0),
    m_recordingSegment(0),
    m_splitLine(0),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_fineGrain(false),
    m_showPreviews(true),
    m_doc(doc),
    m_toolBox(0),
    m_selectionRect(0)
{
    m_toolBox = new SegmentToolBox(this, m_doc);

    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

    // prepare selection rectangle
    m_selectionRect = new QCanvasRectangle(canvas());
    m_selectionRect->setPen(RosegardenGUIColours::SelectionRectangle);
    m_selectionRect->setZ(1000);  // Always in front
    m_selectionRect->hide();
}

SegmentCanvas::~SegmentCanvas()
{
    // canvas items are deleted by the Track Editor
    //
    // flush the Canvas Item GC to avoid double-deletion problems
    //
    CanvasItemGC::flush();
}

QCanvasRectangle*
SegmentCanvas::getSelectionRectangle()
{
    return m_selectionRect;
}

void SegmentCanvas::slotSetTool(const QString& toolName)
{
    RG_DEBUG << "SegmentCanvas::slotSetTool(" << toolName << ")"
                         << this << "\n";

    if (m_tool) m_tool->stow();

    m_tool = m_toolBox->getTool(toolName);

    if (m_tool) m_tool->ready();
    else {
        KMessageBox::error(0, QString("SegmentCanvas::slotSetTool() : unknown tool name %1").arg(toolName));
    }
}

void SegmentCanvas::updateAllSegmentItems()
{
    // delete repeat rects which need to be deleted
    //
    CanvasItemGC::gc();

    // store the segments we currently show here to speed up
    // determining if new segments were added to the composition
    // 
    std::vector<Segment*> currentSegments;
    bool foundOneSegmentDeleted = false;

    QCanvasItemList l = canvas()->allItems();
    
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
        SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
        if (item) {
	    if (item == m_recordingSegment) continue;
            Segment* segment = item->getSegment();

            if (segment) {
                if (!segment->getComposition()) {
                    // this segment has been deleted
                    delete item; // will automatically remove it from the selection
                    foundOneSegmentDeleted = true;
                }
                else {
                    item->recalculateRectangle(true);
                    currentSegments.push_back(segment);
                }
            }
        }
    }

    unsigned int nbSegmentsInComposition = m_doc->getComposition().getNbSegments();
    unsigned int nbCurrentSegments = currentSegments.size();
    
    if ((nbSegmentsInComposition != nbCurrentSegments) ||
        (foundOneSegmentDeleted))
        // if the composition and the segment canvas have the same
        // number of segments, but one of them got deleted they can't
        // be all the same segments, so we also have to look
    {
	using Rosegarden::Composition;

	// check all composition's segments if there's a SegmentItem for it
	//
	const Composition::segmentcontainer& compositionSegments =
	    m_doc->getComposition().getSegments();

	for (Composition::segmentcontainer::const_iterator i =
		 compositionSegments.begin();
	     i != compositionSegments.end(); ++i) {
	    
	    Segment* seg = (*i);
	    if (m_recordingSegment &&
		(seg == m_recordingSegment->getSegment())) continue;
	    
	    if (std::find(currentSegments.begin(),
			  currentSegments.end(), seg) ==
		currentSegments.end()) {
		// found one - update
		addSegmentItem(seg);
	    }
	}
    }

    slotUpdate();
}

void SegmentCanvas::updateSegmentItem(Segment *segment)
{
    RG_DEBUG << "SegmentCanvas::updateSegmentItem" << endl;

    SegmentItem *item = findSegmentItem(segment);
    if (!item) {
	addSegmentItem(segment);
    } else {
	item->recalculateRectangle(true);
    }

    slotUpdate();
}

void
SegmentCanvas::addToSelection(Segment *segment)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));
    if (!selTool) return;
    selTool->addToSelection(segment);
}


SegmentItem*
SegmentCanvas::findSegmentItem(Rosegarden::Segment *segment)
{
    // slow

    QCanvasItemList l = canvas()->allItems();
    
    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
	    if (item && (item->getSegment() == segment)) {
                return item;
	    }
        }
    }

    return 0;
}

SegmentItem*
SegmentCanvas::findSegmentClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                return item;
        }
    }

    return 0;
}

SegmentRepeatRectangle*
SegmentCanvas::findRepeatClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (SegmentRepeatRectangle *r = dynamic_cast<SegmentRepeatRectangle*>(*it))
                return r;
        }
    }

    return 0;
}

void SegmentCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    switch (e->button()) {
    case LeftButton:
    case MidButton:
        if (m_tool) m_tool->handleMouseButtonPress(e);
        else
            RG_DEBUG << "SegmentCanvas::contentsMousePressEvent() :"
                     << this << " no tool\n";
        break;
    case RightButton:
        if (m_tool) m_tool->handleRightButtonPress(e);
        else
            RG_DEBUG << "SegmentCanvas::contentsMousePressEvent() :"
                     << this << " no tool\n";
        break;
    default:
        break;
    }

    return;
}

void SegmentCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    SegmentItem *item = findSegmentClickedOn(
            inverseWorldMatrix().map(e->pos()));

    if (item) {
        m_currentItem = item;
	emit editSegment(item->getSegment());
    } else {
	SegmentRepeatRectangle *rect = findRepeatClickedOn(
                inverseWorldMatrix().map(e->pos()));
        if (rect) {
	    Rosegarden::timeT time = rect->getRepeatStartTime(e->x());

//	    RG_DEBUG << "editRepeat at time " << time << endl;
	
	    emit editRepeat(rect->getSegment(), time);
        }
    }
}

void SegmentCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_tool) return;

    if (e->button() == LeftButton ||
        e->button() == MidButton ) m_tool->handleMouseButtonRelease(e);
}

void SegmentCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (!m_tool) return;

    int follow = m_tool->handleMouseMove(e);
    
    if (follow != SegmentTool::NoFollow) {

        if (follow & SegmentTool::FollowHorizontal)
            slotScrollHorizSmallSteps(e->pos().x());

        if (follow & SegmentTool::FollowVertical)
            slotScrollVertSmallSteps(e->pos().y());
    }
}

// Show the split line. This is where we perform Segment splits.
//
void SegmentCanvas::slotShowSplitLine(int x, int y)
{
    if (m_splitLine == 0)
        m_splitLine = new SegmentSplitLine(x, y,
					   m_grid.getYSnap() - 1,
                                           m_grid.getRulerScale(),
                                           canvas());
    else
        m_splitLine->moveLine(x, y);

    slotUpdate();
}

// Hide the split line
//
void SegmentCanvas::slotHideSplitLine()
{
    if (m_splitLine) m_splitLine->hideLine();
    slotUpdate();
}

void SegmentCanvas::slotExternalWheelEvent(QWheelEvent* e)
{
    e->accept();
    wheelEvent(e);
}


SegmentItem *
SegmentCanvas::addSegmentItem(TrackId track, timeT startTime, timeT endTime)
{
    SegmentItem *newItem = new SegmentItem
	(track, startTime, endTime, m_showPreviews, &m_grid, canvas(), m_doc);

//    RG_DEBUG << "addSegmentItem from data: new item is " << newItem << endl;

    newItem->setPen(m_pen);
//    newItem->setBrush(RosegardenGUIColours::convertColour(m_doc->getComposition().getSegmentColourMap().getColourByIndex(0)));
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

SegmentItem *
SegmentCanvas::addSegmentItem(Segment *segment)
{
    SegmentItem *newItem = new SegmentItem
	(segment, m_showPreviews, &m_grid, canvas(), m_doc);

//    RG_DEBUG << "addSegmentItem from segment: new item is " << newItem << endl;

    newItem->setPen(m_pen);
//    newItem->setBrush(RosegardenGUIColours::convertColour(m_doc->getComposition().getSegmentColourMap().getColourByIndex(segment->getColourIndex())));
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

void SegmentCanvas::showRecordingSegmentItem(Segment *segment, timeT endTime)
{
//    RG_DEBUG << "SegmentCanvas::showRecordingSegmentItem(" << track << ", "
//	     << startTime << ", " << endTime << ") (seg now is " << m_recordingSegment << ")" << endl;

    if (m_recordingSegment) {

	m_recordingSegment->setStartTime(segment->getStartTime());
	m_recordingSegment->setEndTime(endTime);

	int trackPosition = m_doc->getComposition().
            getTrackById(segment->getTrack())->getPosition();

	m_recordingSegment->setTrackPosition(trackPosition);

    } else {
	
	m_recordingSegment = addSegmentItem(segment);
	m_recordingSegment->setEndTime(endTime);
	m_recordingSegment->setPen(RosegardenGUIColours::RecordingSegmentBorder);
        m_recordingSegment->setColour(RosegardenGUIColours::RecordingSegmentBlock);
	m_recordingSegment->setZ(2);

//	RG_DEBUG << "(recording segment now is " << m_recordingSegment << ")" << endl;
    }
}


void SegmentCanvas::deleteRecordingSegmentItem()
{
//    RG_DEBUG << "SegmentCanvas::deleteRecordingSegmentItem (seg is " << m_recordingSegment << ")" << endl;

    if (m_recordingSegment) {

	m_recordingSegment->setVisible(false);
	delete m_recordingSegment;
	m_recordingSegment = 0;
	updateAllSegmentItems();
        canvas()->update();
    }
}





// Select a SegmentItem on the canvas according to a
// passed Segment pointer
//
//
void SegmentCanvas::slotSelectSegments(const SegmentSelection &segments)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));

    if (!selTool) return;

    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    // clear any SegmentItems currently selected
    //
    selTool->clearSelected();

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
        
        if (segItem) { 

            for (SegmentSelection::const_iterator segIt = segments.begin();
		 segIt != segments.end(); ++segIt) {

                if (segItem->getSegment() == (*segIt)) {

                    selTool->slotSelectSegmentItem(segItem);
                }
            }
        }
    }
}

SegmentSelector*
SegmentCanvas::getSegmentSelectorTool()
{
    return dynamic_cast<SegmentSelector*>(getToolBox()->getTool(SegmentSelector::ToolName));
}

// Get a selected Segments if we're using a Selector tool
//
SegmentSelection
SegmentCanvas::getSelectedSegments()
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (selTool)
        return selTool->getSelectedSegments();

    return SegmentSelection();
}

bool
SegmentCanvas::haveSelection()
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool) return false;
    return (selTool->getSelectedSegments().size() > 0);
}

void
SegmentCanvas::clearSelected()
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (selTool)
        return selTool->clearSelected();
}



// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void SegmentCanvas::slotSetSelectAdd(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool) return;

    selTool->setSegmentAdd(value);
}


// enter/exit selection copy mode - this means that the CTRL key
// (or similar) has been depressed and if we're in Select mode we
// can copy the current selection with a click and drag (overrides
// the default movement behaviour for selection).
//
//
void SegmentCanvas::slotSetSelectCopy(bool value)
{
    SegmentSelector* selTool = getSegmentSelectorTool();

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}


void SegmentCanvas::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
	grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}


void SegmentCanvas::setShowPreviews(bool previews)
{
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
	if (segItem) {
	    segItem->setShowPreview(previews);
	    canvas()->setChanged(segItem->rect());
	}
    }

    m_showPreviews = previews;
    slotUpdate();
}


void SegmentCanvas::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}


// Find a SegmentItem for a given Segment
//
SegmentItem*
SegmentCanvas::getSegmentItem(Rosegarden::Segment *segment)
{
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it)
    {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
        
         if (segItem)
         { 
             if (segItem->getSegment() == segment)
                 return segItem;
         }
    }

    return 0;
}


// If we're changing zoom level then we need to regenerate the
// SegmentItem selection.
//
//
void
SegmentCanvas::updateSegmentItemSelection()
{
    RG_DEBUG << "SegmentCanvas::updateSegmentItemSelection" << endl;

    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>
        (getToolBox()->getTool(SegmentSelector::ToolName));

    SegmentItemList *list = selTool->getSegmentItemList();
    SegmentSelection selection;

    for (SegmentItemList::iterator it = list->begin();
            it != list->end(); ++it)
    {
        selection.insert(it->second->getSegment());
    }

    slotSelectSegments(selection);
}


