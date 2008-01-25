/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "LinedStaff.h"

#include "misc/Debug.h"
#include "base/Event.h"
#include "base/LayoutEngine.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Segment.h"
#include "base/SnapGrid.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "GUIPalette.h"
#include "BarLine.h"
#include <qcanvas.h>
#include <qcolor.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpen.h>
#include <qrect.h>
#include <qstring.h>
#include <algorithm>


namespace Rosegarden
{

// width of pointer
//
const int pointerWidth = 3;


LinedStaff::LinedStaff(QCanvas *canvas, Segment *segment,
                       SnapGrid *snapGrid, int id,
                       int resolution, int lineThickness) :
    Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(LinearMode),
    m_pageWidth(2000.0),  // fairly arbitrary, but we need something non-zero
    m_rowsPerPage(0),
    m_rowSpacing(0),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas)),
    m_insertCursorTime(segment->getStartTime()),
    m_insertCursorTimeValid(false)
{
    initCursors();
}

LinedStaff::LinedStaff(QCanvas *canvas, Segment *segment,
                       SnapGrid *snapGrid,
                       int id, int resolution, int lineThickness,
                       double pageWidth, int rowsPerPage, int rowSpacing) :
    Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(rowsPerPage ? MultiPageMode : ContinuousPageMode),
    m_pageWidth(pageWidth),
    m_rowsPerPage(rowsPerPage),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas)),
    m_insertCursorTime(segment->getStartTime()),
    m_insertCursorTimeValid(false)
{
    initCursors();
}

LinedStaff::LinedStaff(QCanvas *canvas, Segment *segment,
                       SnapGrid *snapGrid,
                       int id, int resolution, int lineThickness,
                       PageMode pageMode, double pageWidth, int rowsPerPage,
                       int rowSpacing) :
    Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(pageMode),
    m_pageWidth(pageWidth),
    m_rowsPerPage(rowsPerPage),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas)),
    m_insertCursorTime(segment->getStartTime()),
    m_insertCursorTimeValid(false)
{
    initCursors();
}

LinedStaff::~LinedStaff()
{
    /*!!! No, the canvas items are all deleted by the canvas on destruction.
     
        deleteBars();
        for (int i = 0; i < (int)m_staffLines.size(); ++i) clearStaffLineRow(i);
    */
}

void
LinedStaff::initCursors()
{
    QPen pen(GUIPalette::getColour(GUIPalette::Pointer));
    pen.setWidth(pointerWidth);

    m_pointer->setPen(pen);
    m_pointer->setBrush(GUIPalette::getColour(GUIPalette::Pointer));

    pen.setColor(GUIPalette::getColour(GUIPalette::InsertCursor));

    m_insertCursor->setPen(pen);
    m_insertCursor->setBrush(GUIPalette::getColour(GUIPalette::InsertCursor));
}

void
LinedStaff::setResolution(int resolution)
{
    m_resolution = resolution;
}

void
LinedStaff::setLineThickness(int lineThickness)
{
    m_lineThickness = lineThickness;
}

void
LinedStaff::setPageMode(PageMode pageMode)
{
    m_pageMode = pageMode;
}

void
LinedStaff::setPageWidth(double pageWidth)
{
    m_pageWidth = pageWidth;
}

void
LinedStaff::setRowsPerPage(int rowsPerPage)
{
    m_rowsPerPage = rowsPerPage;
}

void
LinedStaff::setRowSpacing(int rowSpacing)
{
    m_rowSpacing = rowSpacing;
}

void
LinedStaff::setConnectingLineLength(int connectingLineLength)
{
    m_connectingLineLength = connectingLineLength;
}

int
LinedStaff::getId() const
{
    return m_id;
}

void
LinedStaff::setX(double x)
{
    m_x = x;
}

double
LinedStaff::getX() const
{
    return m_x;
}

void
LinedStaff::setY(int y)
{
    m_y = y;
}

int
LinedStaff::getY() const
{
    return m_y;
}

void
LinedStaff::setMargin(double margin)
{
    m_margin = margin;
}

double
LinedStaff::getMargin() const
{
    if (m_pageMode != MultiPageMode)
        return 0;
    return m_margin;
}

void
LinedStaff::setTitleHeight(int titleHeight)
{
    m_titleHeight = titleHeight;
}

int
LinedStaff::getTitleHeight() const
{
    return m_titleHeight;
}

double
LinedStaff::getTotalWidth() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return getCanvasXForRightOfRow(getRowForLayoutX(m_endLayoutX)) - m_x;

    case MultiPageMode:
        return getCanvasXForRightOfRow(getRowForLayoutX(m_endLayoutX)) + m_margin - m_x;

    case LinearMode:
    default:
        return getCanvasXForLayoutX(m_endLayoutX) - m_x;
    }
}

int
LinedStaff::getTotalHeight() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return getCanvasYForTopOfStaff(getRowForLayoutX(m_endLayoutX)) +
               getHeightOfRow() - m_y;

    case MultiPageMode:
        return getCanvasYForTopOfStaff(m_rowsPerPage - 1) +
               getHeightOfRow() - m_y;

    case LinearMode:
    default:
        return getCanvasYForTopOfStaff(0) + getHeightOfRow() - m_y;
    }
}

int
LinedStaff::getHeightOfRow() const
{
    return getTopLineOffset() + getLegerLineCount() * getLineSpacing()
           + getBarLineHeight() + m_lineThickness;
}

bool
LinedStaff::containsCanvasCoords(double x, int y) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:

        for (int row = getRowForLayoutX(m_startLayoutX);
                row <= getRowForLayoutX(m_endLayoutX); ++row) {
            if (y >= getCanvasYForTopOfStaff(row) &&
                    y < getCanvasYForTopOfStaff(row) + getHeightOfRow()) {
                return true;
            }
        }

        return false;

    case MultiPageMode:

        for (int row = getRowForLayoutX(m_startLayoutX);
                row <= getRowForLayoutX(m_endLayoutX); ++row) {
            if (y >= getCanvasYForTopOfStaff(row) &&
                    y < getCanvasYForTopOfStaff(row) + getHeightOfRow() &&
                    x >= getCanvasXForLeftOfRow(row) &&
                    x <= getCanvasXForRightOfRow(row)) {
                return true;
            }
        }

        return false;

    case LinearMode:
    default:

        return (y >= getCanvasYForTopOfStaff() &&
                y < getCanvasYForTopOfStaff() + getHeightOfRow());
    }
}

int
LinedStaff::getCanvasYForHeight(int h, double baseX, int baseY) const
{
    int y;

    //    NOTATION_DEBUG << "LinedStaff::getCanvasYForHeight(" << h << "," << baseY
    //		   << ")" << endl;

    if (baseX < 0)
        baseX = getX() + getMargin();

    if (baseY >= 0) {
        y = getCanvasYForTopLine(getRowForCanvasCoords(baseX, baseY));
    } else {
        y = getCanvasYForTopLine();
    }

    y += getLayoutYForHeight(h);

    return y;
}

int
LinedStaff::getLayoutYForHeight(int h) const
{
    int y = ((getTopLineHeight() - h) * getLineSpacing()) / getHeightPerLine();
    if (h < getTopLineHeight() && (h % getHeightPerLine() != 0))
        ++y;

    return y;
}

int
LinedStaff::getHeightAtCanvasCoords(double x, int y) const
{
    //!!! the lazy route: approximate, then get the right value
    // by calling getCanvasYForHeight a few times... ugh

    //    RG_DEBUG << "\nNotationStaff::heightOfYCoord: y = " << y
    //                         << ", getTopLineOffset() = " << getTopLineOffset()
    //                         << ", getLineSpacing() = " << m_npf->getLineSpacing()
    //                         << endl;

    if (x < 0)
        x = getX() + getMargin();

    int row = getRowForCanvasCoords(x, y);
    int ph = (y - getCanvasYForTopLine(row)) * getHeightPerLine() /
             getLineSpacing();
    ph = getTopLineHeight() - ph;

    int i;
    int mi = -2;
    int md = getLineSpacing() * 2;

    int testi = -2;
    int testMd = 1000;

    for (i = -1; i <= 1; ++i) {
        int d = y - getCanvasYForHeight(ph + i, x, y);
        if (d < 0)
            d = -d;
        if (d < md) {
            md = d;
            mi = i;
        }
        if (d < testMd) {
            testMd = d;
            testi = i;
        }
    }

    if (mi > -2) {
        //         RG_DEBUG << "LinedStaff::getHeightAtCanvasCoords: " << y
        //                              << " -> " << (ph + mi) << " (mi is " << mi << ", distance "
        //                              << md << ")" << endl;
        //         if (mi == 0) {
        //             RG_DEBUG << "GOOD APPROXIMATION" << endl;
        //         } else {
        //             RG_DEBUG << "BAD APPROXIMATION" << endl;
        //         }
        return ph + mi;
    } else {
        RG_DEBUG << "LinedStaff::getHeightAtCanvasCoords: heuristic got " << ph << ", nothing within range (closest was " << (ph + testi) << " which is " << testMd << " away)" << endl;
        return 0;
    }
}

QRect
LinedStaff::getBarExtents(double x, int y) const
{
    int row = getRowForCanvasCoords(x, y);

    for (int i = 1; i < m_barLines.size(); ++i) {

        double layoutX = m_barLines[i]->getLayoutX();
        int barRow = getRowForLayoutX(layoutX);

        if (m_pageMode != LinearMode && (barRow < row))
            continue;

        BarLine *line = m_barLines[i];

        if (line) {
            if (line->x() <= x)
                continue;

            return QRect(int(m_barLines[i -1]->x()),
                         getCanvasYForTopOfStaff(barRow),
                         int(line->x() - m_barLines[i - 1]->x()),
                         getHeightOfRow());
        }
    }

    // failure
    return QRect(int(getX() + getMargin()), getCanvasYForTopOfStaff(), 4, getHeightOfRow());
}

double
LinedStaff::getCanvasXForLayoutX(double x) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return m_x + x - (m_pageWidth * getRowForLayoutX(x));

    case MultiPageMode: {
            int pageNo = getRowForLayoutX(x) / getRowsPerPage();
            double cx = m_x + x - (m_pageWidth * getRowForLayoutX(x));
            cx += m_margin + (m_margin * 2 + m_pageWidth) * pageNo;
            return cx;
        }

    case LinearMode:
    default:
        return m_x + x;
    }
}

LinedStaff::LinedStaffCoords

LinedStaff::getLayoutCoordsForCanvasCoords(double x, int y) const
{
    int row = getRowForCanvasCoords(x, y);
    return LinedStaffCoords
           ((row * m_pageWidth) + x - getCanvasXForLeftOfRow(row),
            y - getCanvasYForTopOfStaff(row));
}

LinedStaff::LinedStaffCoords

LinedStaff::getCanvasCoordsForLayoutCoords(double x, int y) const
{
    int row = getRowForLayoutX(x);
    return LinedStaffCoords
           (getCanvasXForLayoutX(x), getCanvasYForTopLine(row) + y);
}

int
LinedStaff::getRowForCanvasCoords(double x, int y) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return ((y - m_y) / m_rowSpacing);

    case MultiPageMode: {
            int px = int(x - m_x - m_margin);
            int pw = int(m_margin * 2 + m_pageWidth);
            if (px < pw)
                y -= m_titleHeight;
            return (getRowsPerPage() * (px / pw)) + ((y - m_y) / m_rowSpacing);
        }

    case LinearMode:
    default:
        return (int)((x - m_x) / m_pageWidth);
    }
}

int
LinedStaff::getCanvasYForTopOfStaff(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        if (row <= 0)
            return m_y;
        else
            return m_y + (row * m_rowSpacing);

    case MultiPageMode:
        if (row <= 0)
            return m_y + m_titleHeight;
        else if (row < getRowsPerPage())
            return m_y + ((row % getRowsPerPage()) * m_rowSpacing) + m_titleHeight;
        else
            return m_y + ((row % getRowsPerPage()) * m_rowSpacing);

    case LinearMode:
    default:
        return m_y;
    }
}

double
LinedStaff::getCanvasXForLeftOfRow(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return m_x;

    case MultiPageMode:
        return m_x + m_margin +
               (m_margin*2 + m_pageWidth) * (row / getRowsPerPage());

    case LinearMode:
    default:
        return m_x + (row * m_pageWidth);
    }
}

void
LinedStaff::sizeStaff(HorizontalLayoutEngine &layout)
{
    Profiler profiler("LinedStaff::sizeStaff", true);

    deleteBars();
    deleteRepeatedClefsAndKeys();
    deleteTimeSignatures();

    //    RG_DEBUG << "LinedStaff::sizeStaff" << endl;

    int lastBar = layout.getLastVisibleBarOnStaff(*this);

    double xleft = 0, xright = 0;
    bool haveXLeft = false;

    xright = layout.getBarPosition(lastBar) - 1;

    TimeSignature currentTimeSignature;

    for (int barNo = layout.getFirstVisibleBarOnStaff(*this);
            barNo <= lastBar; ++barNo) {

        double x = layout.getBarPosition(barNo);

        if (!haveXLeft) {
            xleft = x;
            haveXLeft = true;
        }

        double timeSigX = 0;
        TimeSignature timeSig;
        bool isNew = layout.getTimeSignaturePosition(*this, barNo, timeSig, timeSigX);

        if (isNew && barNo < lastBar) {
            currentTimeSignature = timeSig;
            insertTimeSignature(timeSigX, currentTimeSignature);
        }

        RG_DEBUG << "LinedStaff::sizeStaff: inserting bar at " << x << " on staff " << this << " (isNew " << isNew << ", timeSigX " << timeSigX << ")" << endl;

        bool showBarNo =
            (showBarNumbersEvery() > 0 &&
             ((barNo + 1) % showBarNumbersEvery()) == 0);

        insertBar(x,
                  ((barNo == lastBar) ? 0 :
                   (layout.getBarPosition(barNo + 1) - x)),
                  layout.isBarCorrectOnStaff(*this, barNo - 1),
                  currentTimeSignature,
                  barNo,
                  showBarNo);
    }

    m_startLayoutX = xleft;
    m_endLayoutX = xright;

    drawStaffName();
    resizeStaffLines();
}

void
LinedStaff::deleteBars()
{
    for (BarLineList::iterator i = m_barLines.begin();
            i != m_barLines.end(); ++i) {
        (*i)->hide();
        delete *i;
    }

    for (LineRecList::iterator i = m_beatLines.begin();
            i != m_beatLines.end(); ++i) {
        i->second->hide();
        delete i->second;
    }

    for (LineRecList::iterator i = m_barConnectingLines.begin();
            i != m_barConnectingLines.end(); ++i) {
        i->second->hide();
        delete i->second;
    }

    for (ItemList::iterator i = m_barNumbers.begin();
            i != m_barNumbers.end(); ++i) {
        (*i)->hide();
        delete *i;
    }

    m_barLines.clear();
    m_beatLines.clear();
    m_barConnectingLines.clear();
    m_barNumbers.clear();
}

void
LinedStaff::insertBar(double layoutX, double width, bool isCorrect,
                      const TimeSignature &timeSig,
                      int barNo, bool showBarNo)
{
    //    RG_DEBUG << "insertBar: " << layoutX << ", " << width
    //			 << ", " << isCorrect << endl;

    int barThickness = m_lineThickness * 5 / 4;

    // hack to ensure the bar line appears on the correct row in
    // notation page layouts, with a conditional to prevent us from
    // moving the bar and beat lines in the matrix
    if (!showBeatLines()) {
        if (width > 0.01) { // not final bar in staff
            layoutX += 1;
        } else {
            layoutX -= 1;
        }
    }

    int row = getRowForLayoutX(layoutX);
    double x = getCanvasXForLayoutX(layoutX);
    int y = getCanvasYForTopLine(row);

    bool firstBarInRow = false, lastBarInRow = false;

    if (m_pageMode != LinearMode &&
            (getRowForLayoutX(layoutX) >
             getRowForLayoutX(layoutX - getMargin() - 2)))
        firstBarInRow = true;

    if (m_pageMode != LinearMode &&
            width > 0.01 &&  // width == 0 for final bar in staff
            (getRowForLayoutX(layoutX) <
             getRowForLayoutX(layoutX + width + getMargin() + 2)))
        lastBarInRow = true;

    BarStyle style = getBarStyle(barNo);

    if (style == RepeatBothBar && firstBarInRow)
        style = RepeatStartBar;

    if (firstBarInRow)
        insertRepeatedClefAndKey(layoutX, barNo);

    // If we're supposed to be hiding bar lines, we do just that --
    // create them as normal, then hide them.  We can't simply not
    // create them because we rely on this to find bar extents for
    // things like double-click selection in notation.
    bool hidden = false;
    if (style == PlainBar && timeSig.hasHiddenBars())
        hidden = true;

    double inset = 0.0;
    if (style == RepeatStartBar || style == RepeatBothBar) {
        inset = getBarInset(barNo, firstBarInRow);
    }

    BarLine *line = new BarLine(m_canvas, layoutX,
                                getBarLineHeight(), barThickness, getLineSpacing(),
                                (int)inset, style);

    line->moveBy(x, y);

    if (isCorrect) {
        line->setPen(GUIPalette::getColour(GUIPalette::BarLine));
        line->setBrush(GUIPalette::getColour(GUIPalette::BarLine));
    } else {
        line->setPen(GUIPalette::getColour(GUIPalette::BarLineIncorrect));
        line->setBrush(GUIPalette::getColour(GUIPalette::BarLineIncorrect));
    }

    line->setZ( -1);
    if (hidden)
        line->hide();
    else
        line->show();

    // The bar lines have to be in order of layout-x (there's no
    // such interesting stipulation for beat or connecting lines)
    BarLineList::iterator insertPoint = lower_bound
                                        (m_barLines.begin(), m_barLines.end(), line, compareBars);
    m_barLines.insert(insertPoint, line);

    if (lastBarInRow) {

        double xe = x + width - barThickness;
        style = getBarStyle(barNo + 1);
        if (style == RepeatBothBar)
            style = RepeatEndBar;

        BarLine *eline = new BarLine(m_canvas, layoutX,
                                     getBarLineHeight(), barThickness, getLineSpacing(),
                                     0, style);
        eline->moveBy(xe, y);

        eline->setPen(GUIPalette::getColour(GUIPalette::BarLine));
        eline->setBrush(GUIPalette::getColour(GUIPalette::BarLine));

        eline->setZ( -1);
        if (hidden)
            eline->hide();
        else
            eline->show();

        BarLineList::iterator insertPoint = lower_bound
                                            (m_barLines.begin(), m_barLines.end(), eline, compareBars);
        m_barLines.insert(insertPoint, eline);
    }

    if (showBarNo) {

        QFont font;
        font.setPixelSize(m_resolution * 3 / 2);
        QFontMetrics metrics(font);
        QString text = QString("%1").arg(barNo + 1);

        QCanvasItem *barNoText = new QCanvasText(text, font, m_canvas);
        barNoText->setX(x);
        barNoText->setY(y - metrics.height() - m_resolution * 2);
        barNoText->setZ( -1);
        if (hidden)
            barNoText->hide();
        else
            barNoText->show();

        m_barNumbers.push_back(barNoText);
    }

    QCanvasRectangle *rect = 0;

    if (showBeatLines()) {

        double gridLines; // number of grid lines per bar may be fractional

        // If the snap time is zero we default to beat markers
        //
        if (m_snapGrid && m_snapGrid->getSnapTime(x))
            gridLines = double(timeSig.getBarDuration()) /
                        double(m_snapGrid->getSnapTime(x));
        else
            gridLines = timeSig.getBeatsPerBar();

        double dx = width / gridLines;

        for (int gridLine = hidden ? 0 : 1; gridLine < gridLines; ++gridLine) {

            rect = new QCanvasRectangle
                   (0, 0, barThickness, getBarLineHeight(), m_canvas);

            rect->moveBy(x + gridLine * dx, y);

            double currentGrid = gridLines / double(timeSig.getBeatsPerBar());

            rect->setPen(GUIPalette::getColour(GUIPalette::BeatLine));
            rect->setBrush(GUIPalette::getColour(GUIPalette::BeatLine));

            // Reset to SubBeatLine colour if we're not a beat line - avoid div by zero!
            //
            if (currentGrid > 1.0 && double(gridLine) / currentGrid != gridLine / int(currentGrid)) {
                rect->setPen(GUIPalette::getColour(GUIPalette::SubBeatLine));
                rect->setBrush(GUIPalette::getColour(GUIPalette::SubBeatLine));
            }

            rect->setZ( -1);
            rect->show(); // show beat lines even if the bar lines are hidden

            LineRec beatLine(layoutX + gridLine * dx, rect);
            m_beatLines.push_back(beatLine);
        }
    }

    if (m_connectingLineLength > 0) {

        rect = new QCanvasRectangle
               (0, 0, barThickness, m_connectingLineLength, m_canvas);

        rect->moveBy(x, y);

        rect->setPen(GUIPalette::getColour(GUIPalette::StaffConnectingLine));
        rect->setBrush(GUIPalette::getColour(GUIPalette::StaffConnectingLine));
        rect->setZ( -3);
        if (hidden)
            rect->hide();
        else
            rect->show();

        LineRec connectingLine(layoutX, rect);
        m_barConnectingLines.push_back(connectingLine);
    }
}

bool
LinedStaff::compareBars(const BarLine *barLine1, const BarLine *barLine2)
{
    return (barLine1->getLayoutX() < barLine2->getLayoutX());
}

bool
LinedStaff::compareBarToLayoutX(const BarLine *barLine1, int x)
{
    return (barLine1->getLayoutX() < x);
}

void
LinedStaff::deleteTimeSignatures()
{
    // default implementation is empty
}

void
LinedStaff::insertTimeSignature(double, const TimeSignature &)
{
    // default implementation is empty
}

void
LinedStaff::deleteRepeatedClefsAndKeys()
{
    // default implementation is empty
}

void
LinedStaff::insertRepeatedClefAndKey(double, int)
{
    // default implementation is empty
}

void
LinedStaff::drawStaffName()
{
    // default implementation is empty
}

void
LinedStaff::resizeStaffLines()
{
    int firstRow = getRowForLayoutX(m_startLayoutX);
    int lastRow = getRowForLayoutX(m_endLayoutX);

    RG_DEBUG << "LinedStaff::resizeStaffLines: firstRow "
    << firstRow << ", lastRow " << lastRow
    << " (startLayoutX " << m_startLayoutX
    << ", endLayoutX " << m_endLayoutX << ")" << endl;

    assert(lastRow >= firstRow);

    int i;
    while ((int)m_staffLines.size() <= lastRow) {
        m_staffLines.push_back(ItemList());
        m_staffConnectingLines.push_back(0);
    }

    // Remove all the staff lines that precede the start of the staff

    for (i = 0; i < firstRow; ++i)
        clearStaffLineRow(i);

    // now i == firstRow

    while (i <= lastRow) {

        double x0;
        double x1;

        if (i == firstRow) {
            x0 = getCanvasXForLayoutX(m_startLayoutX);
        } else {
            x0 = getCanvasXForLeftOfRow(i);
        }

        if (i == lastRow) {
            x1 = getCanvasXForLayoutX(m_endLayoutX);
        } else {
            x1 = getCanvasXForRightOfRow(i);
        }

        resizeStaffLineRow(i, x0, x1 - x0);

        ++i;
    }

    // now i == lastRow + 1

    while (i < (int)m_staffLines.size())
        clearStaffLineRow(i++);
}

void
LinedStaff::clearStaffLineRow(int row)
{
    for (int h = 0; h < (int)m_staffLines[row].size(); ++h) {
        delete m_staffLines[row][h];
    }
    m_staffLines[row].clear();

    delete m_staffConnectingLines[row];
    m_staffConnectingLines[row] = 0;
}

void
LinedStaff::resizeStaffLineRow(int row, double x, double length)
{
    //    RG_DEBUG << "LinedStaff::resizeStaffLineRow: row "
    //	     << row << ", x " << x << ", length "
    //	     << length << endl;


    // If the resolution is 8 or less, we want to reduce the blackness
    // of the staff lines somewhat to make them less intrusive

    int level = 0;
    int z = 2;
    if (m_resolution < 6) {
        z = -1;
        level = (9 - m_resolution) * 32;
        if (level > 200)
            level = 200;
    }

    QColor lineColour(level, level, level);

    int h;

    /*!!! No longer really good enough. But we could potentially use the
      bar positions to sort this out
     
        if (m_pageMode && row > 0 && offset == 0.0) {
            offset = (double)m_npf->getBarMargin() / 2;
            length -= offset;
        }
    */

    int y;

    delete m_staffConnectingLines[row];

    if (m_pageMode != LinearMode && m_connectingLineLength > 0.1) {

        // rather arbitrary (dup in insertBar)
        int barThickness = m_resolution / 12 + 1;
        y = getCanvasYForTopLine(row);
        QCanvasRectangle *line = new QCanvasRectangle
                                 (int(x + length), y, barThickness, m_connectingLineLength, m_canvas);
        line->setPen(GUIPalette::getColour(GUIPalette::StaffConnectingTerminatingLine));
        line->setBrush(GUIPalette::getColour(GUIPalette::StaffConnectingTerminatingLine));
        line->setZ( -2);
        line->show();
        m_staffConnectingLines[row] = line;

    } else {
        m_staffConnectingLines[row] = 0;
    }

    while ((int)m_staffLines[row].size() <= getLineCount() * m_lineThickness) {
        m_staffLines[row].push_back(0);
    }

    int lineIndex = 0;

    for (h = 0; h < getLineCount(); ++h) {

        y = getCanvasYForHeight
            (getBottomLineHeight() + getHeightPerLine() * h,
             x, getCanvasYForTopLine(row));

        if (elementsInSpaces()) {
            y -= getLineSpacing() / 2 + 1;
        }

        //      RG_DEBUG << "LinedStaff: drawing line from ("
        //                           << x << "," << y << ") to (" << (x+length-1)
        //                           << "," << y << ")" << endl;

        QCanvasItem *line;
        delete m_staffLines[row][lineIndex];
        m_staffLines[row][lineIndex] = 0;

        if (m_lineThickness > 1) {
            QCanvasRectangle *rline = new QCanvasRectangle
                                      (int(x), y, int(length), m_lineThickness, m_canvas);
            rline->setPen(lineColour);
            rline->setBrush(lineColour);
            line = rline;
        } else {
            QCanvasLine *lline = new QCanvasLine(m_canvas);
            lline->setPoints(int(x), y, int(x + length), y);
            lline->setPen(lineColour);
            line = lline;
        }

        //      if (j > 0) line->setSignificant(false);

        line->setZ(z);
        m_staffLines[row][lineIndex] = line;
        line->show();

        ++lineIndex;
    }

    while (lineIndex < (int)m_staffLines[row].size()) {
        delete m_staffLines[row][lineIndex];
        m_staffLines[row][lineIndex] = 0;
        ++lineIndex;
    }
}

void
LinedStaff::setCurrent(bool current)
{
    m_current = current;
    if (m_current) {
        m_insertCursor->show();
    } else {
        m_insertCursor->hide();
    }
}

double
LinedStaff::getLayoutXOfPointer() const
{
    double x = m_pointer->x();
    int row = getRowForCanvasCoords(x, int(m_pointer->y()));
    return getLayoutCoordsForCanvasCoords(x, getCanvasYForTopLine(row)).first;
}

void
LinedStaff::getPointerPosition(double &cx, int &cy) const
{
    cx = m_pointer->x();
    cy = getCanvasYForTopOfStaff(getRowForCanvasCoords(cx, int(m_pointer->y())));
}

double
LinedStaff::getLayoutXOfInsertCursor() const
{
    if (!m_current) return -1;
    double x = m_insertCursor->x();
    int row = getRowForCanvasCoords(x, int(m_insertCursor->y()));
    return getLayoutCoordsForCanvasCoords(x, getCanvasYForTopLine(row)).first;
}

timeT
LinedStaff::getInsertCursorTime(HorizontalLayoutEngine &layout) const
{
    if (m_insertCursorTimeValid) return m_insertCursorTime;
    return layout.getTimeForX(getLayoutXOfInsertCursor());
}

void
LinedStaff::getInsertCursorPosition(double &cx, int &cy) const
{
    if (!m_current) {
        cx = -1;
        cy = -1;
        return ;
    }
    cx = m_insertCursor->x();
    cy = getCanvasYForTopOfStaff(getRowForCanvasCoords(cx, int(m_insertCursor->y())));
}

void
LinedStaff::setPointerPosition(double canvasX, int canvasY)
{
    int row = getRowForCanvasCoords(canvasX, canvasY);
    canvasY = getCanvasYForTopOfStaff(row);
    m_pointer->setX(int(canvasX));
    m_pointer->setY(int(canvasY));
    m_pointer->setZ( -30); // behind everything else
    m_pointer->setPoints(0, 0, 0, getHeightOfRow() /* - 1 */);
    m_pointer->show();
}

void
LinedStaff::setPointerPosition(HorizontalLayoutEngine &layout,
                               timeT time)
{
    setPointerPosition(layout.getXForTime(time));
}

void
LinedStaff::setPointerPosition(double layoutX)
{
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(layoutX, 0);
    setPointerPosition(coords.first, coords.second);
}

void
LinedStaff::hidePointer()
{
    m_pointer->hide();
}

void
LinedStaff::setInsertCursorPosition(double canvasX, int canvasY)
{
    if (!m_current) return;

    int row = getRowForCanvasCoords(canvasX, canvasY);
    canvasY = getCanvasYForTopOfStaff(row);
    m_insertCursor->setX(canvasX);
    m_insertCursor->setY(canvasY);
    m_insertCursor->setZ( -28); // behind everything else except playback pointer
    m_insertCursor->setPoints(0, 0, 0, getHeightOfRow() - 1);
    m_insertCursor->show();
    m_insertCursorTimeValid = false;
}

void
LinedStaff::setInsertCursorPosition(HorizontalLayoutEngine &layout,
                                    timeT time)
{
    double x = layout.getXForTime(time);
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(x, 0);
    setInsertCursorPosition(coords.first, coords.second);
    m_insertCursorTime = time;
    m_insertCursorTimeValid = true;
}

void
LinedStaff::hideInsertCursor()
{
    m_insertCursor->hide();
}

void
LinedStaff::renderElements(ViewElementList::iterator,
                           ViewElementList::iterator)
{
    // nothing -- we assume rendering will be done by the implementation
    // of positionElements
}

void
LinedStaff::renderAllElements()
{
    renderElements(getViewElementList()->begin(),
                   getViewElementList()->end());
}

void
LinedStaff::positionAllElements()
{
    positionElements(getSegment().getStartTime(),
                     getSegment().getEndTime());
}

}
