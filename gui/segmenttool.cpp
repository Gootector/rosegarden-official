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

#include <qpopupmenu.h>
#include <qcursor.h>

#include <kmessagebox.h>

#include "segmenttool.h"

#include "Colour.h"
#include "SnapGrid.h"
#include "NotationTypes.h"

#include "segmentcommands.h" // for SegmentRecSet
#include "compositionview.h"
#include "compositionitemhelper.h"
#include "colours.h"
#include "rosegardencanvasview.h"

#include "rosegardengui.h"
#include "rosedebug.h"

using Rosegarden::TrackId;
using Rosegarden::timeT;
using Rosegarden::SnapGrid;
using Rosegarden::Note;
using Rosegarden::SegmentSelection;
using Rosegarden::GUIPalette;

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentToolBox::SegmentToolBox(CompositionView* parent, RosegardenGUIDoc* doc)
    : BaseToolBox(parent),
      m_canvas(parent),
      m_doc(doc)
{
}

SegmentTool* SegmentToolBox::createTool(const QString& toolName)
{
    SegmentTool* tool = 0;

    QString toolNamelc = toolName.lower();
    
    if (toolNamelc == SegmentPencil::ToolName)

        tool = new SegmentPencil(m_canvas, m_doc);

    else if (toolNamelc == SegmentEraser::ToolName)

        tool = new SegmentEraser(m_canvas, m_doc);

    else if (toolNamelc == SegmentMover::ToolName)

        tool = new SegmentMover(m_canvas, m_doc);

    else if (toolNamelc == SegmentResizer::ToolName)

        tool = new SegmentResizer(m_canvas, m_doc);

    else if (toolNamelc == SegmentSelector::ToolName)

        tool = new SegmentSelector(m_canvas, m_doc);

    else if (toolNamelc == SegmentSplitter::ToolName)

        tool = new SegmentSplitter(m_canvas, m_doc);

    else if (toolNamelc == SegmentJoiner::ToolName)

        tool = new SegmentJoiner(m_canvas, m_doc);

    else {
        KMessageBox::error(0, QString("SegmentToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
}

SegmentTool* SegmentToolBox::getTool(const QString& toolName)
{
    return dynamic_cast<SegmentTool*>(BaseToolBox::getTool(toolName));
}


// TODO : relying on doc->parent being a KMainWindow sucks
SegmentTool::SegmentTool(CompositionView* canvas, RosegardenGUIDoc *doc)
    : BaseTool("segment_tool_menu", dynamic_cast<KMainWindow*>(doc->parent())->factory(), canvas),
      m_canvas(canvas),
      m_doc(doc),
      m_changeMade(false)
{
}

SegmentTool::~SegmentTool()
{}

void SegmentTool::ready()
{
    m_canvas->viewport()->setCursor(Qt::arrowCursor);
}

void 
SegmentTool::handleRightButtonPress(QMouseEvent *e)
{
    if (m_currentItem) // mouse button is pressed for some tool
	return;
    
    RG_DEBUG << "SegmentTool::handleRightButtonPress()\n";

    setCurrentItem(m_canvas->getFirstItemAt(e->pos()));

    if (m_currentItem) {
        if (!m_canvas->getModel()->isSelected(m_currentItem)) {

            m_canvas->getModel()->clearSelected();
            m_canvas->getModel()->setSelected(m_currentItem);
            m_canvas->getModel()->signalSelection();
        }
    }
    
    showMenu();
    setCurrentItem(CompositionItem());
}

void
SegmentTool::createMenu()
{
    RG_DEBUG << "SegmentTool::createMenu()\n";

    RosegardenGUIApp *app =
        dynamic_cast<RosegardenGUIApp*>(m_doc->parent());

    if (app) {
        m_menu = static_cast<QPopupMenu*>
            //(app->factory()->container("segment_tool_menu", app));
            (m_parentFactory->container("segment_tool_menu", app));

        if (!m_menu) {
            RG_DEBUG << "SegmentTool::createMenu() failed\n";
        }
    } else {
        RG_DEBUG << "SegmentTool::createMenu() failed: !app\n";
    }
}

void
SegmentTool::addCommandToHistory(KCommand *command)
{
    m_doc->getCommandHistory()->addCommand(command);
}

SegmentToolBox* SegmentTool::getToolBox()
{
    return m_canvas->getToolBox();
}

//////////////////////////////
// SegmentPencil
//////////////////////////////

SegmentPencil::SegmentPencil(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_newRect(false),
      m_track(0),
      m_startTime(0),
      m_endTime(0)
{
    RG_DEBUG << "SegmentPencil()\n";
}

void SegmentPencil::ready()
{
    m_canvas->viewport()->setCursor(Qt::ibeamCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

}

void SegmentPencil::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentPencil::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}


void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    if (e->button() == RightButton) return;
    
    m_newRect = false;

    // Check if mouse click was on a rect
    //
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        delete item;
        return; // mouse click was on a rect, nothing to do
    }
    
    // make new item
    //
    m_canvas->setSnapGrain(false);

    int trackPosition = m_canvas->grid().getYBin(e->pos().y());
    Rosegarden::Track *t = m_doc->getComposition().getTrackByPosition(trackPosition);

    if (!t) return;
    
    TrackId track = t->getId();

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (track >= TrackId(m_doc->getComposition().getNbTracks())) return;

    timeT time = m_canvas->grid().snapX(e->pos().x(), SnapGrid::SnapLeft);
    timeT duration = m_canvas->grid().getSnapTime(double(e->pos().x()));
    if (duration == 0) duration = Note(Note::Shortest).getDuration();

    QRect tmpRect;
    tmpRect.setX(int(m_canvas->grid().getRulerScale()->getXForTime(time)));
    tmpRect.setY(m_canvas->grid().getYBinCoordinate(trackPosition));
    tmpRect.setHeight(m_canvas->grid().getYSnap());
    tmpRect.setWidth(int(m_canvas->grid().getRulerScale()->getWidthForDuration(time, duration)));

    m_canvas->setTmpRect(tmpRect);
    m_newRect = true;
    m_origPos = e->pos();

    updateOnTmpRect(tmpRect);
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent* e)
{
    if (e->button() == RightButton) return;

    if (m_newRect) {

        QRect tmpRect = m_canvas->getTmpRect();
        
        int trackPosition = m_canvas->grid().getYBin(tmpRect.y());
        Rosegarden::Track *track = m_doc->getComposition().getTrackByPosition(trackPosition);
        timeT startTime = m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x()),
            endTime = m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x() + tmpRect.width());
        

        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc, track->getId(),
                                     startTime, endTime);

	m_newRect = false;

        addCommandToHistory(command);

	// add the SegmentItem by hand, instead of allowing the usual
	// update mechanism to spot it.  This way we can select the
	// segment as we add it; otherwise we'd have no way to know
	// that the segment was created by this tool rather than by
	// e.g. a simple file load

	Rosegarden::Segment *segment = command->getSegment();

        CompositionItem item = CompositionItemHelper::makeCompositionItem(segment);
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->setSelected(item);
        m_canvas->getModel()->setSelected(item);
        m_canvas->getModel()->signalSelection();
        m_canvas->setTmpRect(QRect());
        updateOnTmpRect(tmpRect);

    } else {

	m_newRect = false;
    }
}

int SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_newRect) return RosegardenCanvasView::NoFollow;

    QRect tmpRect =  m_canvas->getTmpRect();
    QRect oldTmpRect = tmpRect;

    m_canvas->setSnapGrain(false);

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (e->pos().x() <= m_origPos.x()) direction = SnapGrid::SnapLeft;

    timeT snap = m_canvas->grid().getSnapTime(double(e->pos().x()));
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    timeT time = m_canvas->grid().snapX(e->pos().x(), direction);

    timeT startTime = m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.x()),
        endTime = m_canvas->grid().getRulerScale()->getTimeForX(tmpRect.right());

    if (direction == SnapGrid::SnapRight) {
        
        if (time >= startTime) {
            if ((time - startTime) < snap) {
                time = startTime + snap;
            }
        } else {
            if ((startTime - time) < snap) {
                time = startTime - snap;
            }
        }

        int w = int(m_canvas->grid().getRulerScale()->getWidthForDuration(startTime, time - startTime));
        tmpRect.setWidth(w);

    } else { // SnapGrid::SnapLeft

//             time += std::max(endTime - startTime, timeT(0));
        tmpRect.setX(int(m_canvas->grid().getRulerScale()->getXForTime(time)));

    }

    m_canvas->setTmpRect(tmpRect);
    updateOnTmpRect(oldTmpRect | tmpRect);
    return RosegardenCanvasView::FollowHorizontal;
}

void SegmentPencil::updateOnTmpRect(QRect tmpRect)
{
    if (tmpRect.x() > 2)
        tmpRect.setX(tmpRect.x() - 2);
    tmpRect.setWidth(tmpRect.width() + 2);
    m_canvas->updateContents(tmpRect);
}


//////////////////////////////
// SegmentEraser
//////////////////////////////

SegmentEraser::SegmentEraser(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentEraser()\n";
}

void SegmentEraser::ready()
{
    m_canvas->viewport()->setCursor(Qt::pointingHandCursor);
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    setCurrentItem(m_canvas->getFirstItemAt(e->pos()));
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        // no need to test the result, we know it's good (see handleMouseButtonPress)
        CompositionItemImpl* item = dynamic_cast<CompositionItemImpl*>((_CompositionItem*)m_currentItem);
        
        addCommandToHistory(new SegmentEraseCommand(item->getSegment()));
        m_canvas->updateContents();
    }

    setCurrentItem(CompositionItem());
}

int SegmentEraser::handleMouseMove(QMouseEvent*)
{
    return RosegardenCanvasView::NoFollow;
}

//////////////////////////////
// SegmentMover
//////////////////////////////

SegmentMover::SegmentMover(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentMover()\n";
}

void SegmentMover::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

}

void SegmentMover::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentMover::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}


void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());
    SegmentSelector* selector = dynamic_cast<SegmentSelector*>
            (getToolBox()->getTool("segmentselector"));

    // #1027303: Segment move issue
    // Clear selection if we're clicking on an item that's not in it
    // and we're not in add mode

    // TODO
//     if (selector && item &&
// 	!item->isSelected() && !selector->isSegmentAdding()) {
// 	selector->clearSelected();
//     }

    if (item) {

        setCurrentItem(item);
	m_clickPoint = e->pos();
        Rosegarden::Segment* s = CompositionItemHelper::getSegment(m_currentItem);

        int x = int(m_canvas->grid().getRulerScale()->getXForTime(s->getStartTime()));
        int y = int(m_canvas->grid().getYBinCoordinate(s->getTrack()));

        m_canvas->setGuidesPos(x, y);
        m_canvas->setDrawGuides(true);

        if (m_canvas->getModel()->haveSelection()) {
            RG_DEBUG << "SegmentMover::handleMouseButtonPress() : haveSelection\n";
            // startMove on all selected segments
            m_canvas->getModel()->startMoveSelection();


            CompositionModel::itemcontainer& movingItems = m_canvas->getModel()->getMovingItems();
            // set m_currentItem to its "sibling" among selected (now moving) items
            setCurrentItem(CompositionItemHelper::findSiblingCompositionItem(movingItems, m_currentItem));

        } else {
            RG_DEBUG << "SegmentMover::handleMouseButtonPress() : no selection\n";
            m_canvas->getModel()->startMove(item);
        }
        
        m_canvas->updateContents();

	m_passedInertiaEdge = false;

    } else {

        // check for addmode - clear the selection if not
        RG_DEBUG << "SegmentMover::handleMouseButtonPress() : clear selection\n";
        m_canvas->getModel()->clearSelected();
        m_canvas->getModel()->signalSelection();
        m_canvas->updateContents();
    }

}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem) {

        if (changeMade()) {

            CompositionModel::itemcontainer& movingItems = m_canvas->getModel()->getMovingItems();
        


            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (movingItems.size() == 1 ? i18n("Move Segment") : i18n("Move Segments"));


            CompositionModel::itemcontainer::iterator it;
        
            for (it = movingItems.begin();
                 it != movingItems.end();
                 it++) {

                CompositionItem item = *it;

                Rosegarden::Segment* segment = CompositionItemHelper::getSegment(item);
                Rosegarden::TrackId itemTrackId = m_canvas->grid().getYBin(item->rect().y());
                timeT itemStartTime = CompositionItemHelper::getStartTime(item, m_canvas->grid());
                timeT itemEndTime   = CompositionItemHelper::getEndTime(item, m_canvas->grid());

                command->addSegment(segment,
                                    itemStartTime,
                                    itemEndTime,
                                    itemTrackId);
            }

            addCommandToHistory(command);
        }
        
        m_canvas->hideTextFloat();
        m_canvas->setDrawGuides(false);
        m_canvas->getModel()->endMove();
        m_canvas->updateContents();

    }

    setChangeMade(false);
    m_currentItem = CompositionItem();
}

int SegmentMover::handleMouseMove(QMouseEvent *e)
{
    m_canvas->setSnapGrain(true);
    if (m_currentItem) {
        CompositionModel::itemcontainer& movingItems = m_canvas->getModel()->getMovingItems();

        RG_DEBUG << "SegmentMover::handleMouseMove : nb movingItems = "
                 << movingItems.size() << endl;

	CompositionModel::itemcontainer::iterator it;
        int guideX = 0;
        int guideY = 0;

	for (it = movingItems.begin();
	     it != movingItems.end();
	     it++) {
//             it->second->showRepeatRect(false);

            int dx = e->pos().x() - m_clickPoint.x(),
                dy = e->pos().y() - m_clickPoint.y();

            const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
            if (!m_passedInertiaEdge &&
                (dx < inertiaDistance && dx > -inertiaDistance) &&
                (dy < inertiaDistance && dy > -inertiaDistance)) {
                return RosegardenCanvasView::NoFollow;
            } else {
                m_passedInertiaEdge = true;
            }

            timeT newStartTime = m_canvas->grid().snapX((*it)->savedRect().x() + dx);

            int newX = int(m_canvas->grid().getRulerScale()->getXForTime(newStartTime));
            int newY = m_canvas->grid().snapY((*it)->savedRect().y() + dy);
            // Make sure we don't set a non-existing track
            if (newY < 0) { newY = 0; }
            TrackId track = m_canvas->grid().getYBin(newY);

//             RG_DEBUG << "SegmentMover::handleMouseMove: orig y " 
//                      << (*it)->savedRect().y()
//                      << ", dy " << dy << ", newY " << newY 
//                      << ", track " << track << endl;

            // Make sure we don't set a non-existing track (c'td)
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= TrackId(m_doc->getComposition().getNbTracks()))
                track  = TrackId(m_doc->getComposition().getNbTracks() - 1);

            newY = m_canvas->grid().getYBinCoordinate(track);

//             RG_DEBUG << "SegmentMover::handleMouseMove: moving to "
//                      << newX << "," << newY << endl;

            (*it)->moveTo(newX, newY);
            setChangeMade(true);
        }

        guideX = m_currentItem->rect().x();
        guideY = m_currentItem->rect().y();

        m_canvas->setGuidesPos(guideX, guideY);

        timeT currentItemStartTime = m_canvas->grid().snapX(m_currentItem->rect().x());

        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::RealTime time = 
            comp.getElapsedRealTime(currentItemStartTime);
        QString ms;
        ms.sprintf("%03d", time.msec());

        int bar, beat, fraction, remainder;
        comp.getMusicalTimeForAbsoluteTime(currentItemStartTime, bar, beat, fraction, remainder);

        QString posString = QString("%1.%2s (%3, %4, %5)")
            .arg(time.sec).arg(ms)
            .arg(bar+1).arg(beat).arg(fraction);

        m_canvas->setTextFloat(guideX + 10, guideY - 30, posString);
	m_canvas->updateContents();

	return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
    }

    return RosegardenCanvasView::NoFollow;
}

//////////////////////////////
// SegmentResizer
//////////////////////////////

SegmentResizer::SegmentResizer(CompositionView *c, RosegardenGUIDoc *d,
			       int edgeThreshold)
    : SegmentTool(c, d),
      m_edgeThreshold(edgeThreshold)
{
    RG_DEBUG << "SegmentResizer()\n";
}

void SegmentResizer::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeHorCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

}

void SegmentResizer::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentResizer::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}


void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    RG_DEBUG << "SegmentResizer::handleMouseButtonPress" << endl;
    m_canvas->getModel()->clearSelected();

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        RG_DEBUG << "SegmentResizer::handleMouseButtonPress - got item" << endl;
        setCurrentItem(item);
        m_canvas->getModel()->startMove(item);

	// Are we resizing from start or end?
	if (item->rect().x() + item->rect().width()/2 > e->pos().x()) {
	    m_resizeStart = true;
	} else {
	    m_resizeStart = false;
	}
    }

    m_previewSuspended = false;
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem || !changeMade()) return;

    timeT newStartTime = CompositionItemHelper::getStartTime(m_currentItem, m_canvas->grid());
    timeT newEndTime = CompositionItemHelper::getEndTime(m_currentItem, m_canvas->grid());
    Rosegarden::Segment* segment = CompositionItemHelper::getSegment(m_currentItem);

    if (m_resizeStart && (newStartTime < newEndTime)) {

	addCommandToHistory(new SegmentResizeFromStartCommand(segment, newStartTime));

    } else {

	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand("Resize Segment");

        int trackPos = CompositionItemHelper::getTrackPos(m_currentItem, m_canvas->grid());
	
	Rosegarden::Composition &comp = m_doc->getComposition();
	Rosegarden::Track *track = comp.getTrackByPosition(trackPos);

	command->addSegment(segment,
			    newStartTime,
			    newEndTime,
			    track->getId());
	addCommandToHistory(command);
    }

    m_canvas->updateContents();
    m_canvas->getModel()->endMove();
    setChangeMade(false);
    m_currentItem = CompositionItem();
}

int SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return RosegardenCanvasView::NoFollow;

    Rosegarden::Segment* segment = CompositionItemHelper::getSegment(m_currentItem);

    // Don't allow Audio segments to resize yet
    //
    if (segment->getType() == Rosegarden::Segment::Audio)
    {
        m_currentItem = CompositionItem();
        KMessageBox::information(m_canvas,
                i18n("You can't yet resize an audio segment!"));
        return RosegardenCanvasView::NoFollow;
    }

    m_canvas->setSnapGrain(true);

    timeT time = m_canvas->grid().snapX(e->pos().x());
    timeT snap = m_canvas->grid().getSnapTime(double(e->pos().x()));
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    timeT itemStartTime = CompositionItemHelper::getStartTime(m_currentItem, m_canvas->grid());
    timeT itemEndTime = CompositionItemHelper::getEndTime(m_currentItem, m_canvas->grid());


    if (m_resizeStart) {

	timeT duration = itemEndTime - time;
        RG_DEBUG << "SegmentResizer::handleMouseMove() : duration = "
                 << duration << " - snap = " << snap
                 << " - itemEndTime : " << itemEndTime
                 << " - time : " << time
                 << endl;

	if ((duration > 0 && duration <  snap) ||
	    (duration < 0 && duration > -snap)) {
	    CompositionItemHelper::setStartTime(m_currentItem,
                                                itemEndTime - (duration < 0 ? -snap : snap),
                                                m_canvas->grid());
	} else {
            CompositionItemHelper::setStartTime(m_currentItem,
                                                itemEndTime - duration,
                                                m_canvas->grid());
	}
        if (duration != 0)
            setChangeMade(true);

	// avoid updating preview, as it will update incorrectly
	// (moving the events rather than leaving them alone and
	// truncating if appropriate)
// 	if (m_currentItem->getShowPreview()) {
// 	    m_previewSuspended = true;
// 	    m_currentItem->setShowPreview(false);
// 	}

    } else {

	timeT duration = time - itemStartTime;

	if ((duration > 0 && duration <  snap) ||
	    (duration < 0 && duration > -snap)) {
            CompositionItemHelper::setEndTime(m_currentItem,
                                              (duration < 0 ? -snap : snap) + itemStartTime,
                                              m_canvas->grid());
	} else {

            CompositionItemHelper::setEndTime(m_currentItem,
                                              duration + itemStartTime,
                                              m_canvas->grid());
	}
        if (duration != 0)
            setChangeMade(true);

	// update preview
// 	if (m_currentItem->getPreview())
// 	    m_currentItem->getPreview()->setPreviewCurrent(false);
    }

    m_canvas->updateContents();

    return RosegardenCanvasView::FollowHorizontal;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(const CompositionItem& p, const QPoint &coord,
					       int edgeThreshold, bool &start)
{
    if (abs(p->rect().x() + p->rect().width() - coord.x()) < edgeThreshold) {
	start = false;
	return true;
    } else if (abs(p->rect().x() - coord.x()) < edgeThreshold) {
	start = true;
	return true;
    } else {
	return false;
    }
}

//////////////////////////////
// SegmentSelector (bo!)
//////////////////////////////

SegmentSelector::SegmentSelector(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_segmentAddMode(false),
      m_segmentCopyMode(false),
      m_segmentQuickCopyDone(false),
      m_buttonPressed(false),
      m_dispatchTool(0)
{
    RG_DEBUG << "SegmentSelector()\n";
}

SegmentSelector::~SegmentSelector()
{
}

void SegmentSelector::ready()
{
    m_canvas->viewport()->setCursor(Qt::arrowCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

}

void SegmentSelector::stow()
{
}

void SegmentSelector::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}

void
SegmentSelector::handleMouseButtonPress(QMouseEvent *e)
{
    RG_DEBUG << "SegmentSelector::handleMouseButtonPress\n";
    m_buttonPressed = true;

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    // If we're in segmentAddMode or not clicking on an item then we don't 
    // clear the selection vector.  If we're clicking on an item and it's 
    // not in the selection - then also clear the selection.
    //
    if ((!m_segmentAddMode && !item) || 
        (!m_segmentAddMode && !(m_canvas->getModel()->isSelected(item)))) {
        m_canvas->getModel()->clearSelected();
    }

    if (item) {
	
        // Ten percent of the width of the SegmentItem
        //
        int threshold = int(float(item->rect().width()) * 0.15);
        if (threshold  == 0) threshold = 1;
        if (threshold > 10) threshold = 10;

	bool start = false;

	if (!m_segmentAddMode &&
	    SegmentResizer::cursorIsCloseEnoughToEdge(item, e->pos(), threshold, start)) {

            SegmentResizer* resizer = 
                dynamic_cast<SegmentResizer*>(getToolBox()->getTool(SegmentResizer::ToolName));

            resizer->setEdgeThreshold(threshold);

            // For the moment we only allow resizing of a single segment
            // at a time.
            //
            m_canvas->getModel()->clearSelected();

	    m_dispatchTool = resizer;
            
	    m_dispatchTool->ready(); // set mouse cursor
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}


        m_canvas->getModel()->startMove(item);
        m_canvas->getModel()->setSelected(item);

        // Moving
        //
//         RG_DEBUG << "SegmentSelector::handleMouseButtonPress - m_currentItem = " << item << endl;
        m_currentItem = item;
        m_clickPoint = e->pos();

        m_canvas->setGuidesPos(item->rect().topLeft());

        m_canvas->setDrawGuides(true);
        
    } else {


        // Add on middle button - bounding box on rest
        //
	if (e->button() == MidButton) {
	    m_dispatchTool =  getToolBox()->getTool(SegmentPencil::ToolName);

            if (m_dispatchTool) {
		m_dispatchTool->ready(); // set mouse cursor
                m_dispatchTool->handleMouseButtonPress(e);
	    }

	    return;

	} else {

            m_canvas->setSelectionRectPos(e->pos());
            m_canvas->setSelectionRectSize(0,0);
            m_canvas->setDrawSelectionRect(true);
            if (!m_segmentAddMode)
                m_canvas->getModel()->clearSelected();

        }
    }
 
    // Tell the RosegardenGUIView that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    m_canvas->getModel()->signalSelection();

    m_passedInertiaEdge = false;
}

void
SegmentSelector::handleMouseButtonRelease(QMouseEvent *e)
{
    m_buttonPressed = false;

    // Hide guides and stuff
    //
    m_canvas->setDrawGuides(false);
    m_canvas->hideTextFloat();

    if (m_dispatchTool) {
	m_dispatchTool->handleMouseButtonRelease(e);
	m_dispatchTool = 0;
	m_canvas->viewport()->setCursor(Qt::arrowCursor);
	return;
    }

    if (!m_currentItem) {
        m_canvas->setDrawSelectionRect(false);
        m_canvas->getModel()->finalizeSelectionRect();
        m_canvas->updateContents();
        m_canvas->getModel()->signalSelection();
        return;
    }

    m_canvas->viewport()->setCursor(Qt::arrowCursor);

    if (m_canvas->getModel()->isSelected(m_currentItem)) {

        CompositionModel::itemcontainer& movingItems = m_canvas->getModel()->getMovingItems();
	CompositionModel::itemcontainer::iterator it;

        if (changeMade()) {

            SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand
                (m_selectedItems.size() == 1 ? i18n("Move Segment") :
                 i18n("Move Segments"));

            SegmentSelection newSelection;

            for (it = movingItems.begin();
                 it != movingItems.end();
                 it++) {

                CompositionItem item = *it;

                Rosegarden::Segment* segment = CompositionItemHelper::getSegment(item);
                Rosegarden::TrackId itemTrackId = m_canvas->grid().getYBin(item->rect().y());
                timeT itemStartTime = CompositionItemHelper::getStartTime(item, m_canvas->grid());
                timeT itemEndTime   = CompositionItemHelper::getEndTime(item, m_canvas->grid());

                command->addSegment(segment,
                                    itemStartTime,
                                    itemEndTime,
                                    itemTrackId);

            }

            addCommandToHistory(command);
        }
        
        m_canvas->getModel()->endMove();
	m_canvas->updateContents();

    }
    
    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone)
    {
        m_segmentQuickCopyDone = false;
//        m_currentItem->setZ(2); // see SegmentItem::setSelected  --??
    }

    setChangeMade(false);
    
    m_currentItem = CompositionItem();
}

// In Select mode we implement movement on the Segment
// as movement _of_ the Segment - as with SegmentMover
//
int
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
//     RG_DEBUG << "SegmentSelector::handleMouseMove\n";

    if (!m_buttonPressed)
        return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;

    if (m_dispatchTool) {
	return m_dispatchTool->handleMouseMove(e);
    }

    if (!m_currentItem)  {

	RG_DEBUG << "SegmentSelector::handleMouseMove: no current item\n";

        // do a bounding box
        QRect selectionRect  = m_canvas->getSelectionRect();

        m_canvas->setDrawSelectionRect(true);

        // same as for notation view
        int w = int(e->pos().x() - selectionRect.x());
        int h = int(e->pos().y() - selectionRect.y());
        if (w > 0) ++w; else --w;
        if (h > 0) ++h; else --h;

        // Translate these points
        //
        m_canvas->setSelectionRectSize(w, h);

        m_canvas->updateContents(selectionRect.normalize() | m_canvas->getSelectionRect().normalize());

        m_canvas->getModel()->signalSelection();
        return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
    }

    m_canvas->viewport()->setCursor(Qt::sizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone) {
	KMacroCommand *mcommand = new KMacroCommand
	    (SegmentQuickCopyCommand::getGlobalName());

        Rosegarden::SegmentSelection selectedItems = m_canvas->getSelectedSegments();
	Rosegarden::SegmentSelection::iterator it;
	for (it = selectedItems.begin();
	     it != selectedItems.end();
	     it++) {
            SegmentQuickCopyCommand *command =
                new SegmentQuickCopyCommand(*it);

            mcommand->addCommand(command);
        }

        addCommandToHistory(mcommand);

        // generate SegmentItem
        //
	m_canvas->updateContents();
        m_segmentQuickCopyDone = true;
    }

    m_canvas->setSnapGrain(true);

    if (m_canvas->getModel()->isSelected(m_currentItem)) {
// 	RG_DEBUG << "SegmentSelector::handleMouseMove: current item is selected\n";

        m_canvas->getModel()->startMoveSelection();
        CompositionModel::itemcontainer& movingItems = m_canvas->getModel()->getMovingItems();
        setCurrentItem(CompositionItemHelper::findSiblingCompositionItem(movingItems, m_currentItem));

	CompositionModel::itemcontainer::iterator it;
        int guideX = 0;
        int guideY = 0;

	for (it = movingItems.begin();
	     it != movingItems.end();
	     ++it) {

//             RG_DEBUG << "SegmentSelector::handleMouseMove() : movingItem at "
//                      << (*it)->rect().x() << "," << (*it)->rect().y() << endl;

	    int dx = e->pos().x() - m_clickPoint.x(),
		dy = e->pos().y() - m_clickPoint.y();

	    const int inertiaDistance = m_canvas->grid().getYSnap() / 3;
	    if (!m_passedInertiaEdge &&
		(dx < inertiaDistance && dx > -inertiaDistance) &&
		(dy < inertiaDistance && dy > -inertiaDistance)) {
		return RosegardenCanvasView::NoFollow;
	    } else {
		m_passedInertiaEdge = true;
	    }

	    timeT newStartTime = m_canvas->grid().snapX((*it)->savedRect().x() + dx);

            int newX = int(m_canvas->grid().getRulerScale()->getXForTime(newStartTime));
            int newY = m_canvas->grid().snapY((*it)->savedRect().y() + dy);
            // Make sure we don't set a non-existing track
            if (newY < 0) { newY = 0; }
            TrackId track = m_canvas->grid().getYBin(newY);

// 	    RG_DEBUG << "SegmentSelector::handleMouseMove: orig y " << (*it)->rect().y()
// 		     << ", dy " << dy << ", newY " << newY << ", track " << track << endl;

            // Make sure we don't set a non-existing track (c'td)
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= TrackId(m_doc->getComposition().getNbTracks()))
                track  = TrackId(m_doc->getComposition().getNbTracks() - 1);

            newY = m_canvas->grid().getYBinCoordinate(track);

            (*it)->moveTo(newX, newY);
            setChangeMade(true);
	}

        guideX = m_currentItem->rect().x();
        guideY = m_currentItem->rect().y();

        m_canvas->setGuidesPos(guideX, guideY);

        timeT currentItemStartTime = m_canvas->grid().snapX(m_currentItem->rect().x());

        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::RealTime time = 
            comp.getElapsedRealTime(currentItemStartTime);
        QString ms;
        ms.sprintf("%03d", time.msec());

        int bar, beat, fraction, remainder;
        comp.getMusicalTimeForAbsoluteTime(currentItemStartTime, bar, beat, fraction, remainder);

        QString posString = QString("%1.%2s (%3, %4, %5)")
            .arg(time.sec).arg(ms)
            .arg(bar+1).arg(beat).arg(fraction);

        m_canvas->setTextFloat(guideX + 10, guideY - 30, posString);
	m_canvas->updateContents();

    } else {
// 	RG_DEBUG << "SegmentSelector::handleMouseMove: current item not selected\n";
    }

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

//////////////////////////////
//
// SegmentSplitter
//
//////////////////////////////


SegmentSplitter::SegmentSplitter(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_prevX(0),
      m_prevY(0)
{
    RG_DEBUG << "SegmentSplitter()\n";
}

SegmentSplitter::~SegmentSplitter()
{
}

void SegmentSplitter::ready()
{
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        m_canvas->viewport()->setCursor(Qt::blankCursor);
        m_prevX = item->rect().x();
        m_prevX = item->rect().y();
        drawSplitLine(e);
        delete item;
    }

}

// Actually perform a split if we're on a Segment.
// Return the Segment pointer and the desired split
// time to the Document level.
//
void
SegmentSplitter::handleMouseButtonRelease(QMouseEvent *e)
{
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
	m_canvas->setSnapGrain(true);
        Rosegarden::Segment* segment = CompositionItemHelper::getSegment(item);
        
        if (segment->getType() == Rosegarden::Segment::Audio)
        {
            AudioSegmentSplitCommand *command =
                new AudioSegmentSplitCommand(segment, m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        }
        else
        {
            SegmentSplitCommand *command =
                new SegmentSplitCommand(segment, m_canvas->grid().snapX(e->pos().x()));
            addCommandToHistory(command);
        }

        m_canvas->updateContents(item->rect());
        delete item;
    }

    // Reinstate the cursor
    m_canvas->viewport()->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}


int
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item)
    {
        m_canvas->viewport()->setCursor(Qt::blankCursor);
        drawSplitLine(e);
        delete item;
	return RosegardenCanvasView::FollowHorizontal;
    }
    else
    {
        m_canvas->viewport()->setCursor(Qt::splitHCursor);
        m_canvas->slotHideSplitLine();
	return RosegardenCanvasView::NoFollow;
    }
}

// Draw the splitting line
//
void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{ 
    m_canvas->setSnapGrain(true);

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

    m_canvas->slotShowSplitLine(x, y);

    QRect updateRect(std::max(0, std::min(x, m_prevX) - 5), y,
                     std::max(m_prevX, x) + 5, m_prevY + m_canvas->grid().getYSnap());
    m_canvas->updateContents(updateRect);
    m_prevX = x;
    m_prevY = y;
}


void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent*)
{
    // DO NOTHING
}


//////////////////////////////
//
// SegmentJoiner
//
//////////////////////////////
SegmentJoiner::SegmentJoiner(CompositionView *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    RG_DEBUG << "SegmentJoiner() - not implemented\n";
}

SegmentJoiner::~SegmentJoiner()
{
}

void
SegmentJoiner::handleMouseButtonPress(QMouseEvent*)
{
}

void
SegmentJoiner::handleMouseButtonRelease(QMouseEvent*)
{
}


int
SegmentJoiner::handleMouseMove(QMouseEvent*)
{
    return RosegardenCanvasView::NoFollow;
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}

//------------------------------

const QString SegmentPencil::ToolName   = "segmentpencil";
const QString SegmentEraser::ToolName   = "segmenteraser";
const QString SegmentMover::ToolName    = "segmentmover";
const QString SegmentResizer::ToolName  = "segmentresizer";
const QString SegmentSelector::ToolName = "segmentselector";
const QString SegmentSplitter::ToolName = "segmentsplitter";
const QString SegmentJoiner::ToolName   = "segmentjoiner";
