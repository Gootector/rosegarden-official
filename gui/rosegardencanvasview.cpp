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

#include <qlayout.h>

#include "rosestrings.h"
#include "rosegardencanvasview.h"

#include "rosedebug.h"

RosegardenCanvasView::RosegardenCanvasView(QCanvas* canvas,
                                           QWidget* parent,
                                           const char* name, WFlags f)
    : QCanvasView(canvas, parent, name, f),
      m_bottomWidget(0),
      m_currentBottomWidgetHeight(-1),
      m_smoothScroll(true)
{
    m_hScrollTimer.start();
    m_vScrollTimer.start();
}

void RosegardenCanvasView::fitWidthToContents()
{
    QRect allItemsBoundingRect;

    QCanvasItemList items = canvas()->allItems();

    QCanvasItemList::Iterator it;

    for (it = items.begin(); it != items.end(); ++it) {
        allItemsBoundingRect |= (*it)->boundingRect();
    }

    QSize currentSize = canvas()->size();
    
    resizeContents(allItemsBoundingRect.width(), currentSize.height());
}

void RosegardenCanvasView::setBottomFixedWidget(QWidget* w)
{
    m_bottomWidget = w;
    if (m_bottomWidget) {
        m_bottomWidget->reparent(this, 0, QPoint(0,0));
        m_bottomWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        setMargins(0, 0, 0, m_bottomWidget->sizeHint().height());
    }
}

void RosegardenCanvasView::slotUpdate()
{
    CanvasItemGC::gc();
    canvas()->update();
}

// This scrolling model pages the CanvasView across the screen
//
//
void RosegardenCanvasView::slotScrollHoriz(int hpos)
{
    QScrollBar* hbar = getMainHorizontalScrollBar();

    /* Lots of performance hitting debug
    RG_DEBUG << "RosegardenCanvasView::slotScrollHoriz: hpos is " << hpos
	     << ", contentsX is " << contentsX() << ", visibleWidth is "
	     << visibleWidth() << endl;
             */

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if (hpos > (contentsX() +
		       visibleWidth() * 1.6) ||
	       hpos < (contentsX() -
		       visibleWidth() * 0.7)) {
	
	// miles off one side or the other
	hbar->setValue(hpos - int(visibleWidth() * 0.4));

    } else if (hpos > (contentsX() + 
		       visibleWidth() * 0.9)) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + int(visibleWidth() * 0.6));

    } else if (hpos < (contentsX() +
		       visibleWidth() * 0.1)) {

	// moving off the left
	hbar->setValue(hbar->value() - int(visibleWidth() * 0.6));
    }
}


void RosegardenCanvasView::slotScrollHorizSmallSteps(int hpos)
{
    if (m_smoothScroll) {
	// #988164: Matrix: Auto-scrolling so fast you can't see sweep
	// distance -- restrict number of scrolls
        int t = m_hScrollTimer.restart();
        if (t < 50)
            return;
    }
    
    QScrollBar* hbar = getMainHorizontalScrollBar();

    int diff = 0;

    if (hpos == 0) {
	
	// returning to zero
        hbar->setValue(0);

    } else if ((diff = int(hpos - (contentsX() + 
				   visibleWidth() * 0.9))) > 0) {

	// moving off the right hand side of the view   
	hbar->setValue(hbar->value() + diff);

    } else if ((diff = int(hpos - (contentsX() +
				   visibleWidth() * 0.1))) < 0) {

	// moving off the left
	hbar->setValue(hbar->value() + diff);

    }
}

void RosegardenCanvasView::slotScrollVertSmallSteps(int vpos)
{
    if (m_smoothScroll) {
        int t = m_vScrollTimer.restart();
        if (t < 50)
            return;
    }

    QScrollBar* vbar = verticalScrollBar();

//    RG_DEBUG << "RosegardenCanvasView::slotScrollVertSmallSteps: vpos is " << vpos << ", contentsY is " << contentsY() << ", visibleHeight is " << visibleHeight() << endl;

    // As a special case (or hack), ignore any request made before we've
    // actually been rendered and sized
    if (visibleHeight() <= 1) return;

    int diff = 0;

    if (vpos == 0) {
	
	// returning to zero
        vbar->setValue(0);

    } else if ((diff = int(vpos - (contentsY() + 
				   visibleHeight() * 0.9))) > 0) {

	// moving off up
	vbar->setValue(vbar->value() + diff);

    } else if ((diff = int(vpos - (contentsY() +
				   visibleHeight() * 0.1))) < 0) {

	// moving off down
	vbar->setValue(vbar->value() + diff);

    }
}

void RosegardenCanvasView::slotScrollVertToTop(int vpos)
{
    QScrollBar* vbar = verticalScrollBar();
    if (vpos < visibleHeight() / 3) vbar->setValue(0);
    else vbar->setValue(vpos - visibleHeight() / 5);
}

void RosegardenCanvasView::slotSetScrollPos(const QPoint &pos)
{
    getMainHorizontalScrollBar()->setValue(pos.x());
    verticalScrollBar()->setValue(pos.y());
}

void RosegardenCanvasView::resizeEvent(QResizeEvent* e)
{
    QCanvasView::resizeEvent(e);
    if (!horizontalScrollBar()->isVisible())
        updateBottomWidgetGeometry();
    
}

void RosegardenCanvasView::setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h)
{
    QCanvasView::setHBarGeometry(hbar, x, y, w, h);
    updateBottomWidgetGeometry();
}

void RosegardenCanvasView::updateBottomWidgetGeometry()
{
    if (!m_bottomWidget) return;

    int bottomWidgetHeight = m_bottomWidget->sizeHint().height();

    setMargins(0, 0, 0, bottomWidgetHeight);
    QRect r = frameRect();
    int hScrollBarHeight = 0;
    if (horizontalScrollBar()->isVisible())
        hScrollBarHeight = horizontalScrollBar()->height() + 2; // + 2 offset needed to preserve border shadow

    int vScrollBarWidth = 0;
    if (verticalScrollBar()->isVisible())
        vScrollBarWidth = verticalScrollBar()->width();

    m_bottomWidget->setGeometry(r.x(),
                                r.y() + r.height() - bottomWidgetHeight - hScrollBarHeight,
                                r.width() - vScrollBarWidth,
                                bottomWidgetHeight);

    if (bottomWidgetHeight != m_currentBottomWidgetHeight) {
        emit bottomWidgetHeightChanged(bottomWidgetHeight);
        m_currentBottomWidgetHeight = bottomWidgetHeight;
    }
    
}

//----------------------------------------------------------------------

void CanvasItemGC::mark(QCanvasItem* item)
{
    if (!item) return;

    item->hide();
//     RG_DEBUG << "CanvasItemGC::mark() : "
//                          << item << std::endl;
    m_garbage.push_back(item);
}

void CanvasItemGC::gc()
{
    for(unsigned int i = 0; i < m_garbage.size(); ++i) {
//         RG_DEBUG << "CanvasItemGC::gc() : delete "
//                              << m_garbage[i] << "\n";
        delete m_garbage[i];
    }

    m_garbage.clear();
}

void CanvasItemGC::flush()
{
    m_garbage.clear();
}

std::vector<QCanvasItem*> CanvasItemGC::m_garbage;

