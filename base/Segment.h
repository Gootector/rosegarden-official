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

#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include <set>
#include <string>

#include "Track.h"
#include "Event.h"
#include "NotationTypes.h"
#include "RefreshStatus.h"

namespace Rosegarden 
{

class SegmentRefreshStatus : public RefreshStatus
{
public:
    SegmentRefreshStatus() : m_from(0), m_to(0) {}

    void push(Rosegarden::timeT from, Rosegarden::timeT to);

    Rosegarden::timeT from() { return m_from; }
    Rosegarden::timeT to()   { return m_to; }

protected:
    Rosegarden::timeT m_from;
    Rosegarden::timeT m_to;
};


/**
 * Segment is the container for a set of Events that are all played on
 * the same track.  Each event has an absolute starting time,
 * which is used as the index within the segment.  Multiple events may
 * have the same absolute time.
 * 
 * (For example, chords are represented simply as a sequence of notes
 * that share a starting time.  The Segment can contain counterpoint --
 * notes that overlap, rather than starting and ending together -- but
 * in practice it's probably too hard to display so we should make
 * more than one Segment if we want to represent true counterpoint.)
 *
 * If you want to carry out notation-related editing operations on
 * a Segment, take a look at SegmentNotationHelper.  If you want to play a
 * Segment, try SegmentPerformanceHelper for duration calculations.
 *
 * The Segment owns the Events its items are pointing at.
 */

class SegmentObserver;
class Composition;
class Quantizer;
struct StandardQuantization;

class Segment : public std::multiset<Event*, Event::EventCmp>
{
public:
    /// A Segment contains either Internal representation or Audio
    typedef enum {
        Internal,
        Audio
    } SegmentType;

    /**
     * Construct a Segment of a given type with a given formal starting time.
     */
    Segment(SegmentType segmentType = Internal,
            timeT startTime = 0);
    /*
     * Copy constructor
     */
    Segment(const Segment &);

    virtual ~Segment();


    //////
    //
    // BASIC SEGMENT ATTRIBUTES

    /**
     * Get the Segment type (Internal or Audio)
     */
    SegmentType getType() const { return m_type; }

    /**
     * Note that a Segment does not have to be in a Composition;
     * if it isn't, this will return zero
     */
    Composition *getComposition() const {
	return m_composition;
    }

    /**
     * Get the track number this Segment is associated with.
     */
    TrackId getTrack() const { return m_track; }

    /**
     * Set the track number this Segment is associated with.  It is
     * usually VERY DANGEROUS to call this on a Segment that has been
     * stored in a Composition, because the Composition uses the track
     * number as part of the ordering for Segments and if the number
     * changes the ordering may break.  If your Segment is already in a
     * Composition, use Composition::setSegmentTrack instead.
     */
    void setTrack(TrackId i) { m_track = i; }

    // label
    //
    void setLabel(const std::string &label) { m_label = label; }
    std::string getLabel() const { return m_label; }

    /**
     * Returns a numeric id of some sort
     * The id is guaranteed to be unique within the segment, but not to
     * have any other interesting properties
     */
    int getNextId() const;


    //////
    //
    // TIME & DURATION VALUES

    /**
     * Get the formal starting time of the Segment.  This is not
     * necessarily the same as the time of the first event in it.
     */
    timeT getStartTime() const { return m_startTime; }

    /**
     * Set the formal starting time of the Segment.  It is usually
     * VERY DANGEROUS to call this on a Segment that has been stored
     * in a Composition, because the Composition uses the start time
     * as a part of the ordering for Segments and if the time changes
     * the ordering may break.  If your Segment is already in a
     * Composition, use Composition::setSegmentStartTime instead.
     * 
     * Changing the start time of a Segment will change the absolute
     * times of all Events within that Segment.
     */
    void setStartTime(timeT i);

    /**
     * Get the time of the first actual event on the Segment.
     * Returns the end time (which should be the same as the start
     * time) if there are no events.
     */
    timeT getFirstEventTime() const;

    /**
     * Adjust the stored start time for this Segment to be equal to
     * the time of the first Event in it.  Do not alter the absolute
     * times of any of the Events.  This effectively tidies up the
     * start time for a Segment that was built up by inserting Events
     * into an empty Segment without knowing in advance where the
     * first Event would appear.  It is usually VERY DANGEROUS to call
     * this on a Segment that has been stored in a Composition,
     * because the Composition uses the start time as a part of the
     * ordering for Segments and if the time changes the ordering may
     * break.  There is no easy way to do this if your Segment is
     * already in a Composition.
     */
    void recalculateStartTime();

    /**
     * Return the effective duration of the segment.  This is the
     * time at which the final event ends relative to the start time
     * of the segment.
     */
    timeT getDuration() const;

    /**
     * Ensure that the duration of the segment reaches the given
     * time, by filling it with suitable rests if it needs
     * lengthening.
     * 
     * It is not strictly necessary to call setDuration to change the
     * duration of a segment -- the duration is always taken from the
     * time and duration of the final event, and events may be
     * inserted anywhere.  But for segments that may be rendered in
     * a score it's vital that the filling rests are present, so
     * in practice setDuration should always be used.
     */
    void setDuration(timeT);

    /**
     * Return the end time of the Segment.  This is the end time of
     * the final event.
     */
    timeT getEndTime() const { return m_startTime + getDuration(); }

    /**
     * Return the end-marker time.  This is earlier than or equal
     * to the value returned by getEndTime() (segments always expand
     * themselves to meet their end markers).
     */
    timeT getEndMarker() const;


    //////
    //
    // QUANTIZATION

    /**
     * Switch quantization on or off.
     */
    void setQuantization(bool quantize);

    /**
     * Find out whether quantization is on or off.
     */
    bool hasQuantization() const;

    /**
     * Set the quantization level to one of a set of standard levels.
     * (This does not switch quantization on, if it's currently off,
     * it only changes the level that will be used when it's next
     * switched on.)
     */
    void setQuantizeLevel(const StandardQuantization &);

    /**
     * Set the quantization level by copying from another Quantizer.
     * (This does not switch quantization on, if it's currently off,
     * it only changes the level that will be used when it's next
     * switched on.)
     */
    void setQuantizeLevel(const Quantizer &);

    /**
     * Get the quantizer currently in (or not in) use.
     */
    const Quantizer &getQuantizer() const;



    //////
    //
    // EVENT MANIPULATION

    /**
     * Inserts a single Event
     */
    iterator insert(Event *e);

    /**
     * Erases a single Event
     */
    void erase(iterator pos);

    /**
     * Erases a set of Events
     */
    void erase(iterator from, iterator to);

    /**
     * Looks up an Event and if it finds it, erases it.
     * @return true if the event was found and erased, false otherwise.
     */
    bool eraseSingle(Event*);

    /**
     * Returns an iterator pointing to that specific element,
     * end() otherwise
     */
    iterator findSingle(Event*) const;

    /**
     * Returns an iterator pointing to the first element starting at
     * or beyond the given absolute time
     */
    iterator findTime(timeT time) const;

    /**
     * Returns an iterator pointing to the first element starting at
     * or before the given absolute time
     */
    iterator findNearestTime(timeT time) const;


    //////
    //
    // ADVANCED, ESOTERIC, or PLAIN STUPID MANIPULATION

    /**
     * Returns the range [start, end[ of events which are at absoluteTime
     */
    void getTimeSlice(timeT absoluteTime, iterator &start, iterator &end) const;
    
    /**
     * Return the starting time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the start time of a bar that is at least partially
     * within this Segment.
     * 
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarStartForTime(timeT t) const;

    /**
     * Return the ending time of the bar that contains time t.  This
     * differs from Composition's bar methods in that it will truncate
     * to the start and end times of this Segment, and is guaranteed
     * to return the end time of a bar that is at least partially
     * within this Segment.
     * 
     * (See Composition for most of the generally useful bar methods.)
     */
    timeT getBarEndForTime(timeT t) const;

    /**
     * Fill up the segment with rests, from the end of the last event
     * currently on the segment to the endTime given.  Actually, this
     * does much the same as setDuration does when it extends a segment,
     * although the endTime is absolute whereas the argument to
     * setDuration is relative to the start of the segment.
     *
     * If permitQuantize is true, the rest duration may be rounded
     * before filling -- this could significantly simplify the
     * resulting score when (for example) interpreting a MIDI file.
     * permitQuantize should not be used if the precise duration of
     * the track will subsequently be of interest.
     */
    void fillWithRests(timeT endTime, bool permitQuantize = false);

    /**
     * Fill up a section within a segment with rests, from the
     * startTime given to the endTime given.  This may be useful if
     * you have a pathological segment that contains notes already but
     * not rests, but it is is likely to be dangerous unless you're
     * quite careful about making sure the given range doesn't overlap
     * any notes.
     *
     * If permitQuantize is true, the rest duration may be rounded
     * before filling -- this could significantly simplify the
     * resulting score when (for example) interpreting a MIDI file.
     * permitQuantize should not be used if the precise duration of
     * the track will subsequently be of interest.
     */
    void fillWithRests(timeT startTime, timeT endTime,
		       bool permitQuantize = false);

    /**
     * For each series of contiguous rests found between the start and
     * end time, replace the series of rests with another series of
     * the same duration but composed of the theoretically "correct"
     * rest durations to fill the gap, in the current time signature.
     */
    void normalizeRests(timeT startTime, timeT endTime,
			bool permitQuantize = false);


    //////
    //
    // REPEAT, DELAY, TRANSPOSE

    // Is this Segment repeating?
    //
    bool isRepeating() const { return m_repeating; }
    void setRepeating(bool value) { m_repeating = value; }

    /**
     * If this Segment is repeating, calculate and return the time at
     * which the repeating stops.  This is the start time of the
     * following Segment on the same Track, if any, or else the end
     * time of the Composition.  (If this Segment does not repeat,
     * return the end time of the Segment.)
     */
    timeT getRepeatEndTime() const;

    Rosegarden::timeT getDelay() const { return m_delay; }
    void setDelay(const Rosegarden::timeT &delay) { m_delay = delay; }

    int getTranspose() const { return m_transpose; }
    void setTranspose(const int &transpose) { m_transpose = transpose; }



    //////
    //
    // AUDIO

    // Get and set Audio file ID (see the AudioFileManager)
    //
    unsigned int getAudioFileID() const { return m_audioFileID; }
    void setAudioFileID(const unsigned int &id) { m_audioFileID = id; }

    // The audio start and end indices tell us how far into
    // audio file "m_audioFileID" this Segment starts and
    // how far into the sample the Segment finishes.
    //
    // The absolute time this Segment finishes is:
    //
    //    audioEnd - audioStart + start of Segment
    //
    void setAudioStartTime(const timeT& audioStart)
        { m_audioStartIdx = audioStart; }

    void setAudioEndTime(const timeT & audioEnd)
        { m_audioEndIdx = audioEnd; }

    timeT getAudioStartTime() const { return m_audioStartIdx; }
    timeT getAudioEndTime() const { return m_audioEndIdx; }


    //////
    //
    // MISCELLANEOUS

    /// Should only be called by Composition
    void setComposition(Composition *composition) {
	m_composition = composition;
    }

    /**
     * The compare class used by Composition
     */
    struct SegmentCmp
    {
        bool operator()(const Segment* a, const Segment* b) const 
        {
            if (a->getTrack() == b->getTrack())
                return a->getStartTime() < b->getStartTime();

            return a->getTrack() < b->getTrack();
        }
    };

    /**
     * An alternative compare class that orders by start time first
     */
    struct SegmentTimeCmp
    {
	bool operator()(const Segment *a, const Segment *b) const {
	    return a->getStartTime() < b->getStartTime();
	}
    };

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void    addObserver(SegmentObserver *obs) { m_observers.insert(obs); }

    /// For use by SegmentObserver objects like Composition & ViewElementsManager
    void removeObserver(SegmentObserver *obs) { m_observers.erase (obs); }


    //////
    //
    // REFRESH STATUS

    // delegate part of the RefreshStatusArray API

    unsigned int getNewRefreshStatusId() {
	return m_refreshStatusArray.getNewRefreshStatusId();
    }

    SegmentRefreshStatus &getRefreshStatus(unsigned int id) {
	return m_refreshStatusArray.getRefreshStatus(id);
    }

    void updateRefreshStatuses(timeT startTime, timeT endTime);

private:
    Composition *m_composition; // owns me, if it exists

    timeT m_startTime;
    TrackId m_track;
    SegmentType m_type;         // identifies Segment type
    std::string m_label;        // segment label
    timeT *m_endMarkerTime;     // points to end time, or zero pointer if none

    mutable int m_id; // not id of Segment, but a value for return by getNextId

    unsigned int m_audioFileID; // audio file ID (see AudioFileManager)
    timeT m_audioStartIdx;      // how far into m_audioFileID our Segment starts
    timeT m_audioEndIdx;        // how far into m_audioFileID our Segment ends

    bool m_repeating;           // is this segment repeating?

    Quantizer *m_quantizer;
    bool m_quantize;

    int m_transpose;            // all Events tranpose
    Rosegarden::timeT m_delay;  // all Events delay

    RefreshStatusArray<SegmentRefreshStatus> m_refreshStatusArray;

private: // stuff to support SegmentObservers

    typedef std::set<SegmentObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifyAdd(Event *) const;
    void notifyRemove(Event *) const;

private: // assignment operator not provided

    Segment &operator=(const Segment &);

};


class SegmentObserver
{
public:
    // called after the event has been added to the segment:
    virtual void eventAdded(const Segment *, Event *) = 0;

    // called after the event has been removed from the segment,
    // and just before it is deleted:
    virtual void eventRemoved(const Segment *, Event *) = 0;
};



// an abstract base

class SegmentHelper
{
protected:
    SegmentHelper(Segment &t) : m_segment(t) { }
    virtual ~SegmentHelper();

    typedef Segment::iterator iterator;

    Segment &segment() { return m_segment; }

    Segment::iterator begin() { return segment().begin(); }
    Segment::iterator end()   { return segment().end();   }

    Segment::iterator insert(Event *e) { return segment().insert(e); }
    void erase(Segment::iterator i)    { segment().erase(i); }

private:
    Segment &m_segment;
};

}


#endif
