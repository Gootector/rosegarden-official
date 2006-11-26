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


#include "Rotary.h"

#include "misc/Debug.h"
#include "gui/dialogs/FloatEdit.h"
#include "gui/general/GUIPalette.h"
#include "TextFloat.h"
#include <kapplication.h>
#include <klocale.h>
#include <qbrush.h>
#include <qcolor.h>
#include <qdialog.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qstring.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qwidget.h>
#include <cmath>


namespace Rosegarden
{

#define ROTARY_MIN (0.25 * M_PI)
#define ROTARY_MAX (1.75 * M_PI)
#define ROTARY_RANGE (ROTARY_MAX - ROTARY_MIN)

static TextFloat* _float = 0;
static QTimer *_floatTimer = 0;

Rotary::PixmapCache Rotary::m_pixmaps;


Rotary::Rotary(QWidget *parent,
               float minValue,
               float maxValue,
               float step,
               float pageStep,
               float initialPosition,
               int size,
               TickMode ticks,
               bool snapToTicks,
               bool centred) :
        QWidget(parent),
        m_minValue(minValue),
        m_maxValue(maxValue),
        m_step(step),
        m_pageStep(pageStep),
        m_size(size),
        m_tickMode(ticks),
        m_snapToTicks(snapToTicks),
        m_centred(centred),
        m_position(initialPosition),
        m_snapPosition(m_position),
        m_initialPosition(initialPosition),
        m_buttonPressed(false),
        m_lastY(0),
        m_lastX(0),
        m_knobColour(0, 0, 0)
{
    setBackgroundMode(Qt::NoBackground);

    if (!_float)
        _float = new TextFloat(this);

    if (!_floatTimer) {
        _floatTimer = new QTimer();
    }

    // connect timer
    connect(_floatTimer, SIGNAL(timeout()), this,
            SLOT(slotFloatTimeout()));
    _float->hide();

    QToolTip::add
        (this,
                i18n("Click and drag up and down or left and right to modify.\nDouble click to edit value directly."));
    setFixedSize(size, size);

    emit valueChanged(m_snapPosition);
}

Rotary::~Rotary()
{
    // Remove this connection
    //
    disconnect(_floatTimer, SIGNAL(timeout()), this,
               SLOT(slotFloatTimeout()));

    delete _float;
    _float = 0;
}

void
Rotary::slotFloatTimeout()
{
    if (_float)
        _float->hide();
}

void
Rotary::setKnobColour(const QColor &colour)
{
    m_knobColour = colour;
    repaint();
}

void
Rotary::paintEvent(QPaintEvent *)
{
    QPainter paint;

    double angle = ROTARY_MIN // offset
                   + (ROTARY_RANGE *
                      (double(m_snapPosition - m_minValue) /
                       (double(m_maxValue) - double(m_minValue))));
    int degrees = int(angle * 180.0 / M_PI);

    //    RG_DEBUG << "degrees: " << degrees << ", size " << m_size << ", pixel " << m_knobColour.pixel() << endl;

    int numTicks = 0;
    switch (m_tickMode) {
    case LimitTicks:
        numTicks = 2;
        break;
    case IntervalTicks:
        numTicks = 5;
        break;
    case PageStepTicks:
        numTicks = 1 + (m_maxValue + 0.0001 - m_minValue) / m_pageStep;
        break;
    case StepTicks:
        numTicks = 1 + (m_maxValue + 0.0001 - m_minValue) / m_step;
        break;
    default:
        break;
    }

    CacheIndex index(m_size, m_knobColour.pixel(), degrees, numTicks, m_centred);

    if (m_pixmaps.find(index) != m_pixmaps.end()) {
        paint.begin(this);
        paint.drawPixmap(0, 0, m_pixmaps[index]);
        paint.end();
        return ;
    }

    int scale = 4;
    int width = m_size * scale;
    QPixmap map(width, width);
    map.fill(paletteBackgroundColor());
    paint.begin(&map);

    QPen pen;
    pen.setColor(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    pen.setWidth(scale);
    paint.setPen(pen);

    if (m_knobColour != Qt::black) {
        paint.setBrush(m_knobColour);
    } else {
        paint.setBrush(
            kapp->palette().color(QPalette::Active, QColorGroup::Base));
    }

    QColor c(m_knobColour);
    pen.setColor(c);
    paint.setPen(pen);

    int indent = width * 0.15 + 1;

    paint.drawEllipse(indent, indent, width - 2*indent, width - 2*indent);

    pen.setWidth(2 * scale);
    int pos = indent + (width - 2 * indent) / 8;
    int darkWidth = (width - 2 * indent) * 2 / 3;
    int darkQuote = (130 * 2 / (darkWidth ? darkWidth : 1)) + 100;
    while (darkWidth) {
        c = c.light(101);
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        if (!--darkWidth)
            break;
        paint.drawEllipse(pos, pos, darkWidth, darkWidth);
        ++pos;
        --darkWidth;
    }

    paint.setBrush(QBrush::NoBrush);

    pen.setColor(colorGroup().dark());
    pen.setWidth(scale);
    paint.setPen(pen);

    for (int i = 0; i < numTicks; ++i) {
        int div = numTicks;
        if (div > 1)
            --div;
        drawTick(paint, ROTARY_MIN + (ROTARY_MAX - ROTARY_MIN) * i / div,
                 width, i != 0 && i != numTicks - 1);
    }

    // now the bright metering bit

    pen.setColor(GUIPalette::getColour(GUIPalette::RotaryMeter));
    pen.setWidth(indent);
    paint.setPen(pen);

    if (m_centred) {
        paint.drawArc(indent / 2, indent / 2, width - indent, width - indent,
                      90 * 16, -(degrees - 180) * 16);
    } else {
        paint.drawArc(indent / 2, indent / 2, width - indent, width - indent,
                      (180 + 45) * 16, -(degrees - 45) * 16);
    }

    pen.setWidth(scale);
    paint.setPen(pen);

    int shadowAngle = -720;
    c = colorGroup().dark();
    for (int arc = 120; arc < 2880; arc += 240) {
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawArc(indent, indent, width - 2*indent, width - 2*indent, shadowAngle + arc, 240);
        paint.drawArc(indent, indent, width - 2*indent, width - 2*indent, shadowAngle - arc, 240);
        c = c.light( 110 );
    }

    shadowAngle = 2160;
    c = colorGroup().dark();
    for (int arc = 120; arc < 2880; arc += 240) {
        pen.setColor(c);
        paint.setPen(pen);
        paint.drawArc(scale / 2, scale / 2, width - scale, width - scale, shadowAngle + arc, 240);
        paint.drawArc(scale / 2, scale / 2, width - scale, width - scale, shadowAngle - arc, 240);
        c = c.light( 109 );
    }

    // and un-draw the bottom part
    pen.setColor(paletteBackgroundColor());
    paint.setPen(pen);
    paint.drawArc(scale / 2, scale / 2, width - scale, width - scale,
                  -45 * 16, -90 * 16);

    double hyp = double(width) / 2.0;
    double len = hyp - indent;
    --len;

    double x0 = hyp;
    double y0 = hyp;

    double x = hyp - len * sin(angle);
    double y = hyp + len * cos(angle);

    pen.setWidth(scale * 2);
    pen.setColor(colorGroup().dark());
    paint.setPen(pen);

    paint.drawLine(int(x0), int(y0), int(x), int(y));

    paint.end();

    QImage i = map.convertToImage().smoothScale(m_size, m_size);
    m_pixmaps[index] = QPixmap(i);
    paint.begin(this);
    paint.drawPixmap(0, 0, m_pixmaps[index]);
    paint.end();
}

void
Rotary::drawTick(QPainter &paint, double angle, int size, bool internal)
{
    double hyp = double(size) / 2.0;
    double x0 = hyp - (hyp - 1) * sin(angle);
    double y0 = hyp + (hyp - 1) * cos(angle);

    if (internal) {

        double len = hyp / 4;
        double x1 = hyp - (hyp - len) * sin(angle);
        double y1 = hyp + (hyp - len) * cos(angle);

        paint.drawLine(int(x0), int(y0), int(x1), int(y1));

    } else {

        double len = hyp / 4;
        double x1 = hyp - (hyp + len) * sin(angle);
        double y1 = hyp + (hyp + len) * cos(angle);

        paint.drawLine(int(x0), int(y0), int(x1), int(y1));
    }
}

void
Rotary::snapPosition()
{
    m_snapPosition = m_position;

    if (m_snapToTicks) {

        switch (m_tickMode) {

        case NoTicks:
            break; // meaningless

        case LimitTicks:
            if (m_position < (m_minValue + m_maxValue) / 2.0) {
                m_snapPosition = m_minValue;
            } else {
                m_snapPosition = m_maxValue;
            }
            break;

        case IntervalTicks:
            m_snapPosition = m_minValue +
                             (m_maxValue - m_minValue) / 4.0 *
                             int((m_snapPosition - m_minValue) /
                                 ((m_maxValue - m_minValue) / 4.0));
            break;

        case PageStepTicks:
            m_snapPosition = m_minValue +
                             m_pageStep *
                             int((m_snapPosition - m_minValue) / m_pageStep);
            break;

        case StepTicks:
            m_snapPosition = m_minValue +
                             m_step *
                             int((m_snapPosition - m_minValue) / m_step);
            break;
        }
    }
}

void
Rotary::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) {
        m_buttonPressed = true;
        m_lastY = e->y();
        m_lastX = e->x();
        //_float->setText(QString("%1").arg(m_snapPosition));
    } else if (e->button() == MidButton) // reset to default
    {
        m_position = m_initialPosition;
        snapPosition();
        update();
        emit valueChanged(m_snapPosition);
    } else if (e->button() == RightButton) // reset to centre position
    {
        m_position = (m_maxValue + m_minValue) / 2.0;
        snapPosition();
        update();
        emit valueChanged(m_snapPosition);
    }

    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));

    if (!_float)
        _float = new TextFloat(this);
    _float->reparent(this);
    _float->move(totalPos + QPoint(width() + 2, -height() / 2));
    _float->setText(QString("%1").arg(m_position));
    _float->show();

    if (e->button() == RightButton || e->button() == MidButton) {
        // one shot, 500ms
        _floatTimer->start(500, true);
    }
}

void
Rotary::mouseDoubleClickEvent(QMouseEvent * /*e*/)
{
    FloatEdit dialog(this,
                     i18n("Select a new value"),
                     i18n("Enter a new value"),
                     m_minValue,
                     m_maxValue,
                     m_position,
                     m_step);

    if (dialog.exec() == QDialog::Accepted) {
        m_position = dialog.getValue();
        snapPosition();
        update();

        emit valueChanged(m_snapPosition);
    }
}

void
Rotary::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == LeftButton) {
        m_buttonPressed = false;
        m_lastY = 0;
        m_lastX = 0;

        // Hide the float text
        //
        if (_float)
            _float->hide();
    }
}

void
Rotary::mouseMoveEvent(QMouseEvent *e)
{
    if (m_buttonPressed) {
        // Dragging by x or y axis when clicked modifies value
        //
        float newValue = m_position +
                         (m_lastY - float(e->y()) + float(e->x()) - m_lastX) * m_step;

        if (newValue > m_maxValue)
            m_position = m_maxValue;
        else
            if (newValue < m_minValue)
                m_position = m_minValue;
            else
                m_position = newValue;

        m_lastY = e->y();
        m_lastX = e->x();

        snapPosition();

        // don't update if there's nothing to update
        //        if (m_lastPosition == m_snapPosition) return;

        update();

        emit valueChanged(m_snapPosition);

        // draw on the float text
        _float->setText(QString("%1").arg(m_snapPosition));
    }
}

void
Rotary::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0)
        m_position -= m_pageStep;
    else
        m_position += m_pageStep;

    if (m_position > m_maxValue)
        m_position = m_maxValue;

    if (m_position < m_minValue)
        m_position = m_minValue;

    snapPosition();
    update();

    if (!_float)
        _float = new TextFloat(this);

    // draw on the float text
    _float->setText(QString("%1").arg(m_snapPosition));

    // Reposition - we need to sum the relative positions up to the
    // topLevel or dialog to please move(). Move just top/right of the rotary
    //
    QPoint totalPos = mapTo(topLevelWidget(), QPoint(0, 0));
    _float->reparent(this);
    _float->move(totalPos + QPoint(width() + 2, -height() / 2));
    _float->show();

    // one shot, 500ms
    _floatTimer->start(500, true);

    // set it to show for a timeout value
    emit valueChanged(m_snapPosition);
}

void
Rotary::setPosition(float position)
{
    m_position = position;

    snapPosition();
    update();
}

}
#include "Rotary.moc"
