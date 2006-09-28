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


#include "BarButtonsWidget.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/RulerScale.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/HZoomable.h"
#include <qbrush.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

BarButtonsWidget::BarButtonsWidget(RosegardenGUIDoc *doc,
                                   RulerScale *rulerScale,
                                   int barHeight,
                                   double xorigin,
                                   QWidget* parent,
                                   const char* name,
                                   WFlags f)
        : QWidget(parent, name, f),
        m_barHeight(barHeight),
        m_xorigin(xorigin),
        m_currentXOffset(0),
        m_width( -1),
        m_doc(doc),
        m_rulerScale(rulerScale)
{
    //    m_barFont = new QFont("helvetica", 12);
    //    m_barFont->setPixelSize(12);
    m_barFont = new QFont();
    m_barFont->setPointSize(10);
}

BarButtonsWidget::~BarButtonsWidget()
{
    delete m_barFont;
}

void BarButtonsWidget::scrollHoriz(int x)
{
    m_currentXOffset = static_cast<int>( -x / getHScaleFactor());
    repaint();
}

QSize BarButtonsWidget::sizeHint() const
{
    int lastBar =
        m_rulerScale->getLastVisibleBar();
    double width =
        m_rulerScale->getBarPosition(lastBar) +
        m_rulerScale->getBarWidth(lastBar) + m_xorigin;

    return QSize(std::max(int(width), m_width), m_barHeight);
}

QSize BarButtonsWidget::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;

    return QSize(static_cast<int>(firstBarWidth), m_barHeight);
}

void BarButtonsWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(*m_barFont);

    if (getHScaleFactor() != 1.0)
        painter.scale(getHScaleFactor(), 1.0);

    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset -
                                            m_xorigin);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    painter.drawLine(m_currentXOffset, 0, static_cast<int>(visibleRect().width() / getHScaleFactor()), 0);

    float minimumWidth = 25.0;
    float testSize = ((float)(m_rulerScale->getBarPosition(firstBar + 1) -
                              m_rulerScale->getBarPosition(firstBar)))
                     / minimumWidth;

    int every = 0;
    int count = 0;

    if (testSize < 1.0) {
        every = (int(1.0 / testSize));

        if (every % 2 == 0)
            every++;
    }

    for (int i = firstBar; i <= lastBar; ++i) {

        double x = m_rulerScale->getBarPosition(i) + m_xorigin + m_currentXOffset;

        if ((x * getHScaleFactor()) > clipRect.x() + clipRect.width())
            break;

        // always the first bar number
        if (every && i != firstBar) {
            if (count < every) {
                count++;
                continue;
            }

            // reset count if we passed
            count = 0;
        }

        // adjust count for first bar line
        if (every == firstBar)
            count++;

        if (i != lastBar) {
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), m_barHeight);

            // disable worldXForm for text
            QPoint textDrawPoint = painter.xForm(QPoint(static_cast<int>(x + 4), 12));

            bool enableXForm = painter.hasWorldXForm();
            painter.setWorldXForm(false);

            if (i >= 0)
                painter.drawText(textDrawPoint, QString("%1").arg(i + 1));

            painter.setWorldXForm(enableXForm);
        } else {
            const QPen normalPen = painter.pen();
            ;
            QPen endPen(black, 2);
            painter.setPen(endPen);
            painter.drawLine(static_cast<int>(x), 0, static_cast<int>(x), m_barHeight);
            painter.setPen(normalPen);
        }
    }

    if (m_doc) {
        Composition &comp = m_doc->getComposition();
        Composition::markercontainer markers = comp.getMarkers();
        Composition::markerconstiterator it;

        timeT start = comp.getBarStart(firstBar);
        timeT end = comp.getBarEnd(lastBar);

        QFontMetrics metrics = painter.fontMetrics();

        for (it = markers.begin(); it != markers.end(); ++it) {
            if ((*it)->getTime() >= start && (*it)->getTime() < end) {
                QString name(strtoqstr((*it)->getName()));

                double x = m_rulerScale->getXForTime((*it)->getTime())
                           + m_xorigin + m_currentXOffset;

                painter.fillRect(static_cast<int>(x), 1,
                                 static_cast<int>(metrics.width(name) + 5),
                                 m_barHeight - 2,
                                 QBrush(GUIPalette::getColour(GUIPalette::MarkerBackground)));

                painter.drawLine(int(x), 1, int(x), m_barHeight - 2);
                painter.drawLine(int(x) + 1, 1, int(x) + 1, m_barHeight - 2);

                QPoint textDrawPoint = painter.xForm
                                       (QPoint(static_cast<int>(x + 3), m_barHeight - 4));

                // disable worldXForm for text
                bool enableXForm = painter.hasWorldXForm();
                painter.setWorldXForm(false);

                painter.drawText(textDrawPoint, name);

                painter.setWorldXForm(enableXForm);
            }
        }
    }
}

void
BarButtonsWidget::mousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "BarButtonsWidget::mousePressEvent: x = " << e->x() << endl;

    if (!m_doc || !e)
        return ;
    bool shiftPressed = ((e->state() & Qt::ShiftButton) != 0);

    Composition &comp = m_doc->getComposition();
    Composition::markercontainer markers = comp.getMarkers();

    if (shiftPressed) {

        timeT t = m_rulerScale->getTimeForX
                  (e->x() - m_xorigin - m_currentXOffset);

        timeT prev = 0;

        for (Composition::markerconstiterator i = markers.begin();
                i != markers.end(); ++i) {

            timeT cur = (*i)->getTime();

            if (cur >= t) {
                emit setLoop(prev, cur);
                return ;
            }

            prev = cur;
        }

        if (prev > 0)
            emit setLoop(prev, comp.getEndMarker());

        return ;
    }

    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
                                            m_currentXOffset -
                                            m_xorigin);
    int lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
        firstBar = m_rulerScale->getFirstVisibleBar();
    }

    timeT start = comp.getBarStart(firstBar);
    timeT end = comp.getBarEnd(lastBar);

    // need these to calculate the visible extents of a marker tag
    QPainter painter(this);
    painter.setFont(*m_barFont);
    QFontMetrics metrics = painter.fontMetrics();

    for (Composition::markerconstiterator i = markers.begin();
            i != markers.end(); ++i) {

        if ((*i)->getTime() >= start && (*i)->getTime() < end) {

            QString name(strtoqstr((*i)->getName()));

            int x = m_rulerScale->getXForTime((*i)->getTime())
                    + m_xorigin + m_currentXOffset;

            int width = metrics.width(name) + 5;

            int nextX = -1;
            Composition::markerconstiterator j = i;
            ++j;
            if (j != markers.end()) {
                nextX = m_rulerScale->getXForTime((*j)->getTime())
                        + m_xorigin + m_currentXOffset;
            }

            if (e->x() >= x && e->x() <= x + width) {

                if (nextX < x || e->x() <= nextX) {

                    RG_DEBUG << "BarButtonsWidget::mousePressEvent: setting pointer to " << (*i)->getTime() << endl;

                    emit setPointerPosition((*i)->getTime());
                    return ;
                }
            }
        }
    }
}

void
BarButtonsWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    RG_DEBUG << "BarButtonsWidget::mouseDoubleClickEvent" << endl;

    emit editMarkers();
}

}
