
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

#ifndef LINED_STAFF_H
#define LINED_STAFF_H

#include "Staff.h"
#include "LayoutEngine.h"
#include "FastVector.h"

#include <string>
#include <vector>

#include "qcolor.h"
#include "qcanvas.h"

/**
 * LinedStaffManager is a trivial abstract base for classes that own
 * and position sets of LinedStaffs, as a convenient API to permit
 * clients (such as canvas implementations) to discover which staff
 * lies where.
 * 
 * LinedStaffManager is not used by LinedStaff.
 */

template <class T> class LinedStaff;

template <class T>
class LinedStaffManager
{
public:
    virtual LinedStaff<T> *getStaffForCanvasY(int y) const = 0;
};


/**
 * LinedStaff is a base class for implementations of Staff that
 * display the contents of a Segment on a set of horizontal lines
 * with optional vertical bar lines plus a StaffRuler at the top.  
 * Likely subclasses include the notation and piano-roll staffs.
 *
 * In general, this class handles x coordinates in floating-point,
 * but y-coordinates as integers because of the requirement that
 * staff lines be a precise integral distance apart.
 */

template <class T>
class LinedStaff : public Rosegarden::Staff<T>
{
public:
    typedef std::pair<double, int> LinedStaffCoords;

protected:
    /**
     * Create a new LinedStaff for the given Segment, with a
     * linear layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the LinedStaff implementation but
     *    queryable via getId
     *
     * \a resolution is the number of blank pixels between
     *    staff lines
     *
     * \a lineThickness is the number of pixels thick a
     *    staff line should be
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, int id,
	       int resolution, int lineThickness);

    /**
     * Create a new LinedStaff for the given Segment, with a
     * page layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the LinedStaff implementation but
     *    queryable via getId
     *
     * \a resolution is the number of blank pixels between
     *    staff lines
     *
     * \a lineThickness is the number of pixels thick a
     *    staff line should be
     *
     * \a pageWidth is the width of a window (to determine
     *    when to break lines for page layout)
     *
     * \a rowSpacing is the distance in pixels between
     *    the tops of consecutive rows on this staff
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, int id,
	       int resolution, int lineThickness,
	       double pageWidth, int rowSpacing);

    /**
     * Create a new LinedStaff for the given Segment, with
     * either page or linear layout.
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, int id,
	       int resolution, int lineThickness,
	       bool pageMode, double pageWidth, int rowSpacing);

public:
    virtual ~LinedStaff();

protected:
    // Methods required to define the type of staff this is

    /**
     * Returns the number of visible staff lines
     */
    virtual int getLineCount() const = 0;

    /**
     * Returns the number of invisible staff lines
     * to leave space for above (and below) the visible staff
     */
    virtual int getLegerLineCount() const = 0;

    /**
     * Returns the height-on-staff value for
     * the bottom visible staff line (a shorthand means for
     * referring to staff lines)
     */
    virtual int getBottomLineHeight() const = 0;

    /**
     * Returns the difference between the height-on-
     * staff value of one visible staff line and the next one
     * above it
     */
    virtual int getHeightPerLine() const = 0;

    /**
     * Returns the height-on-staff value for the top visible
     * staff line.  This is deliberately not virtual.
     */
    int getTopLineHeight() const {
	return getBottomLineHeight() +
	    (getLineCount() - 1) * getHeightPerLine();
    }

    /**
     * Returns true if elements fill the spaces between lines,
     * false if elements can fall on lines.  If true, the lines
     * will be displaced vertically by half a line spacing.
     */
    virtual bool elementsInSpaces() const {
	return false;
    }

protected:
    /// Subclass may wish to expose this
    virtual void setResolution(int resolution);

    /// Subclass may wish to expose this
    virtual void setLineThickness(int lineThickness);

    /// Subclass may wish to expose this
    virtual void setPageMode(bool pageMode);

    /// Subclass may wish to expose this
    virtual void setPageWidth(double pageWidth);

    /// Subclass may wish to expose this
    virtual void setRowSpacing(int rowSpacing);

    /// Subclass may wish to expose this.  Default is zero
    virtual void setConnectingLineLength(int length);

public:
    /**
     * Return the id of the staff.  This is only useful to external
     * agents, it isn't used by the LinedStaff itself.
     */
    virtual int getId() const;

    /**
     * Set the canvas x-coordinate of the left-hand end of the staff.
     * This does not move any canvas items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setX(double x);

    /**
     * Get the canvas x-coordinate of the left-hand end of the staff.
     */
    virtual double getX() const;

    /**
     * Set the canvas y-coordinate of the top of the first staff row.
     * This does not move any canvas items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setY(int y);

    /**
     * Get the canvas y-coordinate of the top of the first staff row.
     */
    virtual int getY() const;

    /**
     * Returns the width of the entire staff after layout.  Call
     * this only after you've done the full sizeStaff/positionElements
     * procedure.
     */
    virtual double getTotalWidth() const;

    /**
     * Returns the height of the entire staff after layout.  Call
     * this only after you've done the full sizeStaff/positionElements
     * procedure.  If there are multiple rows, this will be the
     * height of all rows, including any space between rows that
     * is used to display other staffs.
     */
    virtual int getTotalHeight() const;

    /**
     * Returns the difference between the y coordinates of
     * neighbouring visible staff lines.  Deliberately non-virtual
     */
    int getLineSpacing() const {
	return m_resolution + m_lineThickness;
    }

    /**
     * Returns the total height of a single staff row
     */
    virtual int getHeightOfRow() const;
    
    /**
     * Returns true if the given canvas y-coordinate falls within
     * (any of the rows of) this staff.  False if it falls in the
     * gap between two rows.
     */
    virtual bool containsCanvasY(int canvasY) const; 

    /**
     * Returns the canvas y coordinate of the specified line on the
     * staff.  baseY is a canvas y coordinate somewhere on the
     * correct row, or -1 for the default row.
     */
    //!!! should be protected?
    virtual int getCanvasYForHeight(int height, int baseY = -1) const;

    /**
     * Returns the y coordinate of the specified line on the
     * staff, relative to the top of the row.
     */
    virtual int getLayoutYForHeight(int height) const;

    /**
     * Returns the height-on-staff value nearest to the given
     * canvas y coordinate.
     */
    virtual int getHeightAtCanvasY(int y) const;

    /**
     * Return the full width, height and origin of the bar containing
     * the given canvas cooordinates.
     */
    virtual QRect getBarExtents(double x, int y) const;

    /**
     * Query the given horizontal layout object (which is assumed to
     * have just completed its layout procedure) to determine the
     * required extents of the staff and the positions of the bars,
     * and create the bars and staff lines accordingly.  It may be
     * called either before or after renderElements and/or
     * positionElements.
     * 
     * No bars or staff lines will appear unless this method has
     * been called.
     */
    virtual void sizeStaff(Rosegarden::HorizontalLayoutEngine<T> &layout);

    /**
     * Generate or re-generate sprites for all the elements between
     * from and to.  See subclasses for specific detailed comments.
     *
     * A very simplistic staff subclass may choose not to
     * implement this (the default implementation is empty) and to
     * do all the rendering work in positionElements.  If rendering
     * elements is slow, however, it makes sense to do it here
     * because this method may be called less often.
     */
    virtual void renderElements(Rosegarden::ViewElementList<T>::iterator from,
				Rosegarden::ViewElementList<T>::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    virtual void renderElements();

    /**
     * Assign suitable coordinates to the elements on the staff
     * between the start and end times, based entirely on the layout
     * X and Y coordinates they were given by the horizontal and
     * vertical layout processes.
     *
     * The implementation is free to render any elements it
     * chooses in this method as well.
     *
     * The default of -1 for "from" should be taken to mean "the
     * start of the staff" and for "to" should be taken to mean
     * "the end of the staff".  Thus the default arguments should
     * reposition the entire staff.
     */
    virtual void positionElements(Rosegarden::timeT from = -1,
				  Rosegarden::timeT to   = -1) = 0;
    
protected:
    // Methods that the subclass may (indeed, should) use to convert
    // between the layout coordinates of elements and their canvas
    // coordinates.  These are deliberately not virtual.

    // Note that even linear-layout staffs have multiple rows; their
    // rows all have the same y coordinate but increasing x
    // coordinates, instead of the other way around.  (The only reason
    // for this is that it seems to be more efficient from the QCanvas
    // perspective to create and manipulate many relatively short
    // canvas lines rather than a smaller number of very long ones.)

    int getTopLineOffset() const {
	return getLineSpacing() * getLegerLineCount();
    }

    int getBarLineHeight() const {
	return getLineSpacing() * (getLineCount() - 1) + m_lineThickness;
    }

    int getRowForLayoutX(double x) const {
	return (int)(x / m_pageWidth);
    }

    int getRowForCanvasCoords(double x, int y) const {
	if (!m_pageMode) return (int)((x - m_x) / m_pageWidth);
	else return ((y - m_y) / m_rowSpacing);
    }

    double getCanvasXForLayoutX(double x) const {
	if (m_pageMode) {
	    return m_x + x - (m_pageWidth * getRowForLayoutX(x));
	} else {
	    return m_x + x;
	}
    }

    int getCanvasYForTopOfStaff(int row = -1) const {
	if (!m_pageMode || row <= 0) return m_y;
	else return m_y + (row * m_rowSpacing);
    }

    int getCanvasYForTopLine(int row = -1) const {
	return getCanvasYForTopOfStaff(row) + getTopLineOffset();
    }

    double getCanvasXForLeftOfRow(int row) const {
	if (!m_pageMode) return m_x + (row * m_pageWidth);
	else return m_x;
    }

    double getCanvasXForRightOfRow(int row) const {
	return getCanvasXForLeftOfRow(row) + m_pageWidth;
    }

    LinedStaffCoords
    getCanvasCoordsForLayoutCoords(double x, int y) const {
	int row = getRowForLayoutX(x);
	return LinedStaffCoords
	    (getCanvasXForLayoutX(x), getCanvasYForTopLine(row) + y);
    }

    LinedStaffCoords
    getCanvasOffsetsForLayoutCoords(double x, int y) const {
	LinedStaffCoords cc = getCanvasCoordsForLayoutCoords(x, y);
	return LinedStaffCoords(cc.first - x, cc.second - y);
    }

protected:
    // Actual implementation methods.  The default implementation
    // shows staff lines, connecting lines (where appropriate) and bar
    // lines, but does not show time signatures.  To see time
    // signatures, override the deleteTimeSignatures and
    // insertTimeSignature methods.

    virtual void resizeStaffLines();
    virtual void clearStaffLineRow(int row);
    virtual void resizeStaffLineRow(int row, double offset, double length);

    virtual void deleteBars();
    virtual void insertBar(int layoutX, bool isCorrect);

    // The default implementations of the following two are empty.
    virtual void deleteTimeSignatures();
    virtual void insertTimeSignature(int layoutX,
				     const Rosegarden::TimeSignature &);

protected:

    //--------------- Data members ---------------------------------

    QCanvas *m_canvas;

    int	     m_id;

    double   m_x;
    int	     m_y;
    int	     m_resolution;
    int	     m_lineThickness;
    
    bool     m_pageMode;
    double   m_pageWidth;
    int	     m_rowSpacing;
    int	     m_connectingLineLength;

    double   m_startLayoutX;
    double   m_endLayoutX;

    typedef std::vector<QCanvasLine *> LineList;
    typedef std::vector<LineList> LineMatrix;
    LineMatrix m_staffLines;
    LineList m_staffConnectingLines;

    typedef std::pair<int, QCanvasLine *> BarLine; // layout-x, line
    typedef FastVector<BarLine> BarLineList;
    static bool compareBars(const BarLine &, const BarLine &);
    static bool compareBarToLayoutX(const BarLine &, int);
    BarLineList m_barLines;
    BarLineList m_barConnectingLines;
};

#include "linedstaff.cpp"

#endif
