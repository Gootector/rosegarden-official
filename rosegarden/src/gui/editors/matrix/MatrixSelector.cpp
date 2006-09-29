/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixSelector.h"

#include "base/BaseProperties.h"
#include <klocale.h>
#include <kstddirs.h>
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/ViewElement.h"
#include "commands/edit/EventEditCommand.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/general/EditTool.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/RosegardenCanvasView.h"
#include "MatrixElement.h"
#include "MatrixMover.h"
#include "MatrixPainter.h"
#include "MatrixResizer.h"
#include "MatrixStaff.h"
#include "MatrixTool.h"
#include "MatrixView.h"
#include <kaction.h>
#include <kglobal.h>
#include <qdialog.h>
#include <qiconset.h>
#include <qpoint.h>
#include <qstring.h>
#include "misc/Debug.h"


namespace Rosegarden
{

MatrixSelector::MatrixSelector(MatrixView* view)
        : MatrixTool("MatrixSelector", view),
        m_selectionRect(0),
        m_updateRect(false),
        m_currentStaff(0),
        m_clickedElement(0),
        m_dispatchTool(0),
        m_justSelectedBar(false),
        m_matrixView(view),
        m_selectionToMerge(0)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this, SLOT(slotHideSelection()));

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/resize.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Resize Tool"), icon, 0, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    createMenu("matrixselector.rc");
}

void MatrixSelector::handleEventRemoved(Event *event)
{
    if (m_dispatchTool)
        m_dispatchTool->handleEventRemoved(event);
    if (m_clickedElement && m_clickedElement->event() == event) {
        m_clickedElement = 0;
    }
}

void MatrixSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void MatrixSelector::handleLeftButtonPress(timeT time,
        int height,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    MATRIX_DEBUG << "MatrixSelector::handleMousePress" << endl;

    if (m_justSelectedBar) {
        handleMouseTripleClick(time, height, staffNo, e, element);
        m_justSelectedBar = false;
        return ;
    }

    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    m_currentStaff = m_mParentView->getStaff(staffNo);

    // Do the merge selection thing
    //
    delete m_selectionToMerge; // you can safely delete 0, you know?
    const EventSelection *selectionToMerge = 0;
    if (e->state() & Qt::ShiftButton)
        selectionToMerge = m_mParentView->getCurrentSelection();

    m_selectionToMerge =
        (selectionToMerge ? new EventSelection(*selectionToMerge) : 0);

    // Now the rest of the element stuff
    //
    m_clickedElement = dynamic_cast<MatrixElement*>(element);

    if (m_clickedElement) {
        int x = int(m_clickedElement->getLayoutX());
        int width = m_clickedElement->getWidth();
        int resizeStart = int(double(width) * 0.85) + x;

        // max size of 10
        if ((x + width ) - resizeStart > 10)
            resizeStart = x + width - 10;

        if (p.x() > resizeStart) {
            m_dispatchTool = m_parentView->
                             getToolBox()->getTool(MatrixResizer::ToolName);
        } else {
            m_dispatchTool = m_parentView->
                             getToolBox()->getTool(MatrixMover::ToolName);
        }

        m_dispatchTool->handleLeftButtonPress(time,
                                              height,
                                              staffNo,
                                              e,
                                              element);
        return ;
    } else {
        // Workaround for #930420 Positional error in sweep-selection box
        // boundary
        int zoomValue = (int)m_matrixView->m_hZoomSlider->getCurrentSize();
        MatrixStaff *staff = m_mParentView->getStaff(staffNo);
        int pitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());
        int pitchCentreHeight = staff->getTotalHeight() -
                                pitch * staff->getLineSpacing() - 2; // 2 or ?
        int pitchLineHeight = pitchCentreHeight + staff->getLineSpacing() / 2;
        int drawHeight = p.y();
        if (drawHeight <= pitchLineHeight + 1 &&
                drawHeight >= pitchLineHeight - 1) {
            if (drawHeight == pitchLineHeight)
                drawHeight += 2;
            else
                drawHeight += 2 * (drawHeight - pitchLineHeight);
        }
        MATRIX_DEBUG << "#### MatrixSelector::handleLeftButtonPress() : zoom "
        << zoomValue
        << " pitch " << pitch
        << " pitchCentreHeight " << pitchCentreHeight
        << " pitchLineHeight " << pitchLineHeight
        << " lineSpacing " << staff->getLineSpacing()
        << " drawHeight " << drawHeight << endl;
        m_selectionRect->setX(int(p.x() / 4)*4); // more workaround for #930420
        m_selectionRect->setY(drawHeight);
        m_selectionRect->setSize(0, 0);

        m_selectionRect->show();
        m_updateRect = true;

        // Clear existing selection if we're not merging
        //
        if (!m_selectionToMerge) {
            m_mParentView->setCurrentSelection(0, false, true);
            m_mParentView->canvas()->update();
        }
    }

    //m_parentView->setCursorPosition(p.x());
}

void MatrixSelector::handleMidButtonPress(timeT time,
        int height,
        int staffNo,
        QMouseEvent* e,
        ViewElement *element)
{
    m_clickedElement = 0; // should be used for left-button clicks only

    // Don't allow overlapping elements on the same channel
    if (dynamic_cast<MatrixElement*>(element))
        return ;

    m_dispatchTool = m_parentView->
                     getToolBox()->getTool(MatrixPainter::ToolName);

    m_dispatchTool->handleLeftButtonPress(time, height, staffNo, e, element);
}

void MatrixSelector::handleMouseDoubleClick(timeT ,
        int ,
        int staffNo,
        QMouseEvent *ev,
        ViewElement *element)
{
    /*
        if (m_dispatchTool)
        {
            m_dispatchTool->handleMouseDoubleClick(time, height, staffNo, e, element);
        }
    */

    m_clickedElement = dynamic_cast<MatrixElement*>(element);

    MatrixStaff *staff = m_mParentView->getStaff(staffNo);
    if (!staff)
        return ;

    if (m_clickedElement) {

        if (m_clickedElement->event()->isa(Note::EventType) &&
                m_clickedElement->event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {

            int id = m_clickedElement->event()->get
                     <Int>
                     (BaseProperties::TRIGGER_SEGMENT_ID);
            emit editTriggerSegment(id);
            return ;
        }

        if (ev->state() & ShiftButton) { // advanced edit

            EventEditDialog dialog(m_mParentView, *m_clickedElement->event(), true);

            if (dialog.exec() == QDialog::Accepted &&
                    dialog.isModified()) {

                EventEditCommand *command = new EventEditCommand
                                            (staff->getSegment(),
                                             m_clickedElement->event(),
                                             dialog.getEvent());

                m_mParentView->addCommandToHistory(command);
            }
        } else {

            SimpleEventEditDialog dialog(m_mParentView, m_mParentView->getDocument(),
                                         *m_clickedElement->event(), false);

            if (dialog.exec() == QDialog::Accepted &&
                    dialog.isModified()) {

                EventEditCommand *command = new EventEditCommand
                                            (staff->getSegment(),
                                             m_clickedElement->event(),
                                             dialog.getEvent());

                m_mParentView->addCommandToHistory(command);
            }
        }

    } /*
    	  
          #988167: Matrix:Multiclick select methods don't work in matrix editor
          Postponing this, as it falls foul of world-matrix transformation
          etiquette and other such niceties
     
    	  else {
     
    	QRect rect = staff->getBarExtents(ev->x(), ev->y());
     
    	m_selectionRect->setX(rect.x() + 2);
    	m_selectionRect->setY(rect.y());
    	m_selectionRect->setSize(rect.width() - 4, rect.height());
     
    	m_selectionRect->show();
    	m_updateRect = false;
    	
    	m_justSelectedBar = true;
    	QTimer::singleShot(QApplication::doubleClickInterval(), this,
    			   SLOT(slotClickTimeout()));
        } */
}

void MatrixSelector::handleMouseTripleClick(timeT t,
        int height,
        int staffNo,
        QMouseEvent *ev,
        ViewElement *element)
{
    if (!m_justSelectedBar)
        return ;
    m_justSelectedBar = false;

    MatrixStaff *staff = m_mParentView->getStaff(staffNo);
    if (!staff)
        return ;

    if (m_clickedElement) {

        // should be safe, as we've already set m_justSelectedBar false
        handleLeftButtonPress(t, height, staffNo, ev, element);
        return ;

    } else {

        m_selectionRect->setX(staff->getX());
        m_selectionRect->setY(staff->getY());
        m_selectionRect->setSize(int(staff->getTotalWidth()) - 1,
                                 staff->getTotalHeight() - 1);

        m_selectionRect->show();
        m_updateRect = false;
    }
}

int MatrixSelector::handleMouseMove(timeT time, int height,
                                    QMouseEvent *e)
{
    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    if (m_dispatchTool) {
        return m_dispatchTool->handleMouseMove(time, height, e);
    }

    if (!m_updateRect)
        return RosegardenCanvasView::NoFollow;

    int w = int(p.x() - m_selectionRect->x());
    int h = int(p.y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0)
        ++w;
    else
        --w;
    if (h > 0)
        ++h;
    else
        --h;

    // Workaround for #930420 Positional error in sweep-selection box boundary
    int wFix = (w > 0) ? 3 : 0;
    int hFix = (h > 0) ? 3 : 0;
    int xFix = (w < 0) ? 3 : 0;
    m_selectionRect->setSize(w - wFix, h - hFix);
    m_selectionRect->setX(m_selectionRect->x() + xFix);
    setViewCurrentSelection();
    m_selectionRect->setSize(w, h);
    m_selectionRect->setX(m_selectionRect->x() - xFix);
    m_mParentView->canvas()->update();

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void MatrixSelector::handleMouseRelease(timeT time, int height, QMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixSelector::handleMouseRelease" << endl;

    if (m_dispatchTool) {
        m_dispatchTool->handleMouseRelease(time, height, e);

        // don't delete the tool as it's still part of the toolbox
        m_dispatchTool = 0;

        return ;
    }

    m_updateRect = false;

    if (m_clickedElement) {
        m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                              m_clickedElement->event(),
                                              false, true);
        m_mParentView->canvas()->update();
        m_clickedElement = 0;

    } else if (m_selectionRect) {
        setViewCurrentSelection();
        m_selectionRect->hide();
        m_mParentView->canvas()->update();
    }

    // Tell anyone who's interested that the selection has changed
    emit gotSelection();
}

void MatrixSelector::ready()
{
    if (m_mParentView) {
        m_selectionRect = new QCanvasRectangle(m_mParentView->canvas());
        m_selectionRect->hide();
        m_selectionRect->setPen(GUIPalette::getColour(GUIPalette::SelectionRectangle));

        m_mParentView->setCanvasCursor(Qt::arrowCursor);
        //m_mParentView->setPositionTracking(false);
    }

    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));

}

void MatrixSelector::stow()
{
    if (m_selectionRect) {
        delete m_selectionRect;
        m_selectionRect = 0;
        m_mParentView->canvas()->update();
    }

    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));

}

void MatrixSelector::slotHideSelection()
{
    if (!m_selectionRect)
        return ;
    m_selectionRect->hide();
    m_selectionRect->setSize(0, 0);
    m_mParentView->canvas()->update();
}

void MatrixSelector::slotMatrixScrolled(int newX, int newY)
{
    if (m_updateRect) {
        int offsetX = newX - m_parentView->getCanvasView()->contentsX();
        int offsetY = newY - m_parentView->getCanvasView()->contentsY();

        int w = int(m_selectionRect->width() + offsetX);
        int h = int(m_selectionRect->height() + offsetY);

        // Qt rectangle dimensions appear to be 1-based
        if (w > 0)
            ++w;
        else
            --w;
        if (h > 0)
            ++h;
        else
            --h;

        m_selectionRect->setSize(w, h);
        setViewCurrentSelection();
        m_mParentView->canvas()->update();
    }
}

void MatrixSelector::setViewCurrentSelection()
{
    EventSelection* selection = getSelection();

    if (m_selectionToMerge && selection &&
            m_selectionToMerge->getSegment() == selection->getSegment()) {

        selection->addFromSelection(m_selectionToMerge);
        m_mParentView->setCurrentSelection(selection, true, true);

    } else if (!m_selectionToMerge) {

        m_mParentView->setCurrentSelection(selection, true, true);

    }

}

}
#include "MatrixSelector.moc"
