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

#ifndef _BASIC_COMMAND_H_
#define _BASIC_COMMAND_H_

/**
 * Command that handles undo on a single Segment by brute-force,
 * saving the affected area of the Segment before the execute() and
 * then restoring it on unexecute().  Subclass this with your own
 * implementation.
 *
 * This class implements execute() and unexecute(); in order to create
 * a working subclass, you should normally put the code you would
 * otherwise have put in execute() into modifySegment().
 */

#include <kcommand.h>
#include <set>

#include "Segment.h"
#include "SegmentNotationHelper.h"

class EventSelection;


// Base-classes used for distinguishing between sorts of command
// in each view's slotCommandExecuted


/// command that affects one or more entire segments
class SegmentCommand
{
public:

    typedef std::set<Rosegarden::Segment *> SegmentSet;
    
    /** 
     * Obtain the set of affected Segments.  Note that Segments
     * in this set may have been removed from the Composition or
     * even destroyed by the time this command has completed.
     */
    virtual void getSegments(SegmentSet &) = 0;

protected:
    SegmentCommand() { }
};


/// command that changes Composition-wide timing settings
class TimeAndTempoChangeCommand
{
protected:
    TimeAndTempoChangeCommand() { }
};


/// command that modifies events within a single segment
class IntraSegmentCommand
{
public:

    /**
     * Return the segment in which the changes happened.
     */
    virtual Rosegarden::Segment &getSegment() = 0;

protected:
    IntraSegmentCommand() { }
};


/**
 * BasicCommand is an abstract subclass of KCommand that manages undo,
 * redo and notification of changes within a contiguous region of a
 * single Rosegarden Segment, by brute force.  When a subclass
 * of BasicCommand executes, it stores a copy of the events that are
 * modified by the command, ready to be restored verbatim on undo.
 */

class BasicCommand : public KCommand, public IntraSegmentCommand
{
public:
    virtual ~BasicCommand();

    virtual void execute();
    virtual void unexecute();

    virtual Rosegarden::Segment &getSegment() { return m_segment; }

    Rosegarden::timeT getBeginTime() { return m_savedEvents.getStartTime(); }
    Rosegarden::timeT getEndTime() { return m_endTime; }
    virtual Rosegarden::timeT getRelayoutEndTime() { return getEndTime(); }

protected:
    /**
     * You should pass "bruteForceRedoRequired = true" if your
     * subclass's implementation of modifySegment uses discrete
     * event pointers or segment iterators to determine which
     * events to modify, in which case it won't work when
     * replayed for redo because the pointers may no longer be
     * valid.  In which case, BasicCommand will implement redo
     * much like undo, and will only call your modifySegment 
     * the very first time the command object is executed.
     *
     * It is always safe to pass bruteForceRedoRequired true,
     * it's just normally a waste of memory.
     */
    BasicCommand(const QString &name,
		 Rosegarden::Segment &segment,
		 Rosegarden::timeT begin, Rosegarden::timeT end,
		 bool bruteForceRedoRequired = false);

    virtual void modifySegment() = 0;

    virtual void beginExecute();

private:
    //--------------- Data members ---------------------------------

    void copyTo(Rosegarden::Segment *);
    void copyFrom(Rosegarden::Segment *);

    Rosegarden::Segment &m_segment;
    Rosegarden::Segment m_savedEvents;
    Rosegarden::timeT m_endTime;

    bool m_doBruteForceRedo;
    Rosegarden::Segment *m_redoEvents;
};


/**
 * Subclass of BasicCommand that manages the brute-force undo and redo
 * extends based on a given selection.
 */

class BasicSelectionCommand : public BasicCommand
{
public:
    virtual ~BasicSelectionCommand();

protected:
    BasicSelectionCommand(const QString &name, EventSelection &selection,
			  bool bruteForceRedoRequired = false);
};

#endif
