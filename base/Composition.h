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

#ifndef _COMPOSITION_H_
#define _COMPOSITION_H_

#include <set>
#include <map>

#include "FastVector.h"

#include "RealTime.h"
#include "Segment.h"
#include "Track.h"
#include "Configuration.h"
#include "XmlExportable.h"
#include "ColourMap.h"
#include "TriggerSegment.h"

#include "Marker.h"

namespace Rosegarden 
{

class Quantizer;
class BasicQuantizer;
class NotationQuantizer;

/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Segments, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Segments it holds, and deletes them on
 * destruction.  When Segments are removed, it will also delete them.
 */

class CompositionObserver;

class Composition : public XmlExportable
{
    friend class Track; // to call notifyTrackChanged()
    friend class Segment; // to call notifySegmentRepeatChanged()
    
public:
    typedef std::multiset<Segment*, Segment::SegmentCmp> segmentcontainer;
    typedef segmentcontainer::iterator iterator;
    typedef segmentcontainer::const_iterator const_iterator;

    typedef std::map<TrackId, Track*> trackcontainer;
    typedef trackcontainer::iterator trackiterator;
    typedef trackcontainer::const_iterator trackconstiterator;

    typedef std::vector<Marker*> markercontainer;
    typedef markercontainer::iterator markeriterator;
    typedef markercontainer::const_iterator markerconstiterator;

    typedef std::set<TriggerSegmentRec *, TriggerSegmentCmp> triggersegmentcontainer;
    typedef triggersegmentcontainer::iterator triggersegmentcontaineriterator;
    typedef triggersegmentcontainer::const_iterator triggersegmentcontainerconstiterator;

    typedef std::set<TrackId> recordtrackcontainer;
    typedef recordtrackcontainer::iterator recordtrackiterator;
    typedef recordtrackcontainer::const_iterator recordtrackconstiterator;

    Composition();
    virtual ~Composition();

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);
public:

    /**
     * Remove all Segments from the Composition and destroy them
     */
    void clear();

    /**
     * Return the absolute end time of the segment that ends last
     */
    timeT getDuration() const;


    //////
    //
    // START AND END MARKERS

    timeT getStartMarker() const { return m_startMarker; }
    timeT getEndMarker() const { return m_endMarker; }

    void setStartMarker(const timeT &sM);
    void setEndMarker(const timeT &eM);


    //////
    //
    //  INSTRUMENT & TRACK

    Track* getTrackById(TrackId track) const;

    Track* getTrackByPosition(int position) const;
 
    trackcontainer& getTracks() { return m_tracks; }
 
    const trackcontainer& getTracks() const { return m_tracks; }

    // Reset id and position
    void resetTrackIdAndPosition(TrackId oldId, TrackId newId, int position);

    TrackId getMinTrackId() const;

    TrackId getMaxTrackId() const;

    const recordtrackcontainer &getRecordTracks() const { return m_recordTracks; }
    void setTrackRecording(TrackId track, bool recording);
    bool isTrackRecording(TrackId track) const;

    // Get and set Solo Track
    //
    TrackId getSelectedTrack() const { return m_selectedTrack; }

    void setSelectedTrack(TrackId track);

    // Are we soloing a Track?
    //
    bool isSolo() const { return m_solo; }
    void setSolo(bool value);

    unsigned int getNbTracks() const { return m_tracks.size(); }

    /**
     * Clear out the Track container
     */
    void clearTracks();

    /**
     * Insert a new Track.  The Composition takes over ownership of
     * the track object.
     */
    void addTrack(Track *track);
 
    /**
     * Delete a Track by index
     */
    void deleteTrack(Rosegarden::TrackId track);

    /**
     * Detach a Track (revert ownership of the Track object to the
     * caller).
     */
    bool detachTrack(Rosegarden::Track *track);

    /**
     * Get the highest running track id (generated and kept
     * through addTrack)
     */
    Rosegarden::TrackId getNewTrackId() const;


    //////
    //
    // MARKERS

    markercontainer& getMarkers() { return m_markers; }
    const markercontainer& getMarkers() const { return m_markers; }

    /**
     * Add a new Marker.  The Composition takes ownership of the
     * marker object.
     */
    void addMarker(Rosegarden::Marker *marker);

    /**
     * Detach a Marker (revert ownership of the Marker object to the
     * caller).
     */
    bool detachMarker(Rosegarden::Marker *marker);

    bool isMarkerAtPosition(Rosegarden::timeT time) const;

    void clearMarkers();


    //////
    //
    //  SEGMENT

    segmentcontainer& getSegments() { return m_segments; }
    const segmentcontainer& getSegments() const { return m_segments; }

    unsigned int getNbSegments() const { return m_segments.size(); }

    /**
     * Add a new Segment and return an iterator pointing to it
     * The inserted Segment is owned by the Composition object
     */
    iterator addSegment(Segment*);

    /**
     * Delete the Segment pointed to by the specified iterator
     *
     * NOTE: The Segment is deleted from the Composition and
     * destroyed
     */
    void deleteSegment(iterator);

    /**
     * Delete the Segment if it is part of the Composition
     * \return true if the Segment was found and deleted
     *
     * NOTE: The Segment is deleted from the composition and
     * destroyed
     */
    bool deleteSegment(Segment*);

    /**
     * DO NOT USE THIS METHOD
     *
     * Set a Segment's start time while keeping the integrity of the
     * Composition multiset.
     *
     * The segment is removed and re-inserted from the composition
     * so the ordering is preserved.
     */
    void setSegmentStartTime(Segment*, timeT);

    /**
     * Test whether a Segment exists in this Composition.
     */
    bool contains(const Segment *);

    /**
     * Return an iterator pointing at the given Segment, or end()
     * if it does not exist in this Composition.
     */
    iterator findSegment(const Segment *);

    /**
     * Remove the Segment if it is part of the Composition,
     * but do not destroy it (passing it to addSegment again
     * would restore it correctly).
     * \return true if the Segment was found and removed
     *
     * NOTE: Many of the Segment methods will fail if the
     * Segment is not in a Composition.  You should not
     * expect to do anything meaningful with a Segment that
     * has been detached from the Composition in this way.
     */
    bool detachSegment(Segment*);

    /**
     * Add a new Segment which has been "weakly detached"
     *
     * Like addSegment(), but doesn't send the segmentAdded signal
     * nor updating refresh statuses
     */
    iterator weakAddSegment(Segment*);

    /**
     * Detach a segment which you're going to re-add (with weakAddSegment)
     * later.
     * Like detachSegment(), but without sending the segmentDeleted signal
     * nor updating refresh statuses.
     */
    bool weakDetachSegment(Segment*);


    //////
    //
    //  TRIGGER SEGMENTS

    triggersegmentcontainer &getTriggerSegments() { return m_triggerSegments; }
    const triggersegmentcontainer &getTriggerSegments() const { return m_triggerSegments; }

    /**
     * Add a new trigger Segment with a given base pitch and base
     * velocity, and return its record.  If pitch or velocity is -1,
     * it will be taken from the first note event in the segment
     */
    TriggerSegmentRec *addTriggerSegment(Segment *, int pitch = -1, int velocity = -1);

    /**
     * Delete a trigger Segment.
     */
    void deleteTriggerSegment(TriggerSegmentId);

    /**
     * Detach a trigger Segment from the Composition.
     */
    void detachTriggerSegment(TriggerSegmentId);

    /**
     * Delete all trigger Segments.
     */
    void clearTriggerSegments();
    
    /**
     * Return the TriggerSegmentId for the given Segment, or -1 if it is
     * not a trigger Segment.
     */
    int getTriggerSegmentId(Segment *);
    
    /**
     * Return the Segment for a given TriggerSegmentId
     */
    Segment *getTriggerSegment(TriggerSegmentId);

    /**
     * Return the TriggerSegmentRec (with Segment, base pitch, base velocity,
     * references etc) for a given TriggerSegmentId
     */
    TriggerSegmentRec *getTriggerSegmentRec(TriggerSegmentId);

    /**
     * Add a new trigger Segment with a given ID and base pitch and
     * velocity.  Fails and returns 0 if the ID is already in use.
     * This is intended for use from file load or from undo/redo.
     */
    TriggerSegmentRec *addTriggerSegment(Segment *, TriggerSegmentId,
					 int basePitch = -1, int baseVelocity = -1);

    /**
     * Get the ID of the next trigger segment that will be inserted.
     */
    TriggerSegmentId getNextTriggerSegmentId() const;

    /**
     * Specify the next trigger ID.  This is intended for use from file
     * load only.  Do not use this function unless you know what you're
     * doing.
     */
    void setNextTriggerSegmentId(TriggerSegmentId);

    /**
     * Update the trigger segment references for all trigger segments.
     * To be called after file load.
     */
    void updateTriggerSegmentReferences();


    //////
    //
    //  BAR

    /**
     * Return the total number of bars in the composition
     */
    int getNbBars() const;

    /**
     * Return the number of the bar that starts at or contains time t.
     *
     * Will happily return computed bar numbers for times before
     * the start or beyond the real end of the composition.
     */
    int getBarNumber(timeT t) const;

    /**
     * Return the starting time of bar n
     */
    timeT getBarStart(int n) const {
	return getBarRange(n).first;
    } 

    /**
     * Return the ending time of bar n
     */
    timeT getBarEnd(int n) const {
	return getBarRange(n).second;
    }

    /**
     * Return the time range of bar n.
     * 
     * Will happily return theoretical timings for bars before the
     * start or beyond the end of composition (i.e. there is no
     * requirement that 0 <= n < getNbBars()).
     */
    std::pair<timeT, timeT> getBarRange(int n) const;

    /**
     * Return the starting time of the bar that contains time t
     */
    timeT getBarStartForTime(timeT t) const {
	return getBarRangeForTime(t).first;
    }

    /**
     * Return the ending time of the bar that contains time t
     */
    timeT getBarEndForTime(timeT t) const {
	return getBarRangeForTime(t).second;
    }

    /**
     * Return the starting and ending times of the bar that contains
     * time t.
     * 
     * Will happily return theoretical timings for bars before the
     * start or beyond the end of composition.
     */
    std::pair<timeT, timeT> getBarRangeForTime(timeT t) const;

    /**
     * Get the default number of bars in a new empty composition
     */
    static int getDefaultNbBars() { return m_defaultNbBars; }
    
    /**
     * Set the default number of bars in a new empty composition
     */
    static void setDefaultNbBars(int b) { m_defaultNbBars = b; }


    //////
    //
    //  TIME SIGNATURE

    /**
     * Add the given time signature at the given time.  Returns the
     * resulting index of the time signature (suitable for passing
     * to removeTimeSignature, for example)
     */
    int addTimeSignature(timeT t, TimeSignature timeSig);

    /**
     * Return the time signature in effect at time t
     */
    TimeSignature getTimeSignatureAt(timeT t) const;

    /**
     * Return the time signature in effect at time t, and the time at
     * which it came into effect
     */
    timeT getTimeSignatureAt(timeT, TimeSignature &) const;

    /**
     * Return the time signature in effect in bar n.  Also sets
     * isNew to true if the time signature is a new one that did
     * not appear in the previous bar.
     */
    TimeSignature getTimeSignatureInBar(int n, bool &isNew) const;

    /**
     * Return the total number of time signature changes in the
     * composition.
     */
    int getTimeSignatureCount() const;

    /**
     * Return the index of the last time signature change before
     * the given time, in a range suitable for passing to 
     * getTimeSignatureChange.  Return -1 if there has been no
     * time signature by this time.
     */
    int getTimeSignatureNumberAt(timeT time) const;

    /**
     * Return the absolute time of and time signature introduced
     * by time-signature change n.
     */
    std::pair<timeT, TimeSignature> getTimeSignatureChange(int n) const;

    /**
     * Remove time signature change event n from the composition.
     */
    void removeTimeSignature(int n);



    //////
    //
    //  TEMPO

    /**
     * Return the tempo in effect at time t, in quarters per minute.
     */
    double getTempoAt(timeT t) const;

    /**
     * Return the tempo in effect at the current playback position.
     */
    double getTempo() const { return getTempoAt(getPosition()); }

    /**
     * Set a default tempo for the composition.  This will be
     * overridden by any tempo events encountered during playback.
     */
    void setDefaultTempo(double tempo) { m_defaultTempo = tempo; }
    double getDefaultTempo() const { return m_defaultTempo; }

    /**
     * Add a tempo-change event at the given time, to the given
     * tempo (in quarters per minute).  Removes any existing tempo
     * event at that time.  Returns the index of the new tempo
     * event in a form suitable for passing to removeTempoChange.
     */
    int addTempo(timeT time, double tempo);

    /**
     * Add a tempo-change event at the given time, to the given
     * tempo (in quarters per hour).  Removes any existing tempo
     * event at that time.  Returns the index of the new tempo
     * event in a form suitable for passing to removeTempoChange.
     */
    int addRawTempo(timeT time, int tempo);

    /**
     * Return the number of tempo changes in the composition.
     */
    int getTempoChangeCount() const;

    /**
     * Return the index of the last tempo change before the given
     * time, in a range suitable for passing to getRawTempoChange.
     * Return -1 if the default tempo is in effect at this time.
     */
    int getTempoChangeNumberAt(timeT time) const;

    /**
     * Return the absolute time of and tempo introduced by tempo
     * change number n, in quarters per hour (this is the value that's
     * actually stored)
     */
    std::pair<timeT, long> getRawTempoChange(int n) const;

    /**
     * Remove tempo change event n from the composition.
     */
    void removeTempoChange(int n);



    //////
    //
    //  REAL TIME

    /**
     * Return the number of microseconds elapsed between
     * the beginning of the composition and the given timeT time.
     * (timeT units are independent of tempo; this takes into
     * account any tempo changes in the first t units of time.)
     *
     * This is a fairly efficient operation, not dependent on the
     * magnitude of t or the number of tempo changes in the piece.
     */
    RealTime getElapsedRealTime(timeT t) const;

    /**
     * Return the nearest time in timeT units to the point at the
     * given number of microseconds after the beginning of the
     * composition.  (timeT units are independent of tempo; this takes
     * into account any tempo changes in the first t microseconds.)
     * The result will be approximate, as timeT units are obviously
     * less precise than microseconds.
     *
     * This is a fairly efficient operation, not dependent on the
     * magnitude of t or the number of tempo changes in the piece.
     */
    timeT getElapsedTimeForRealTime(RealTime t) const;

    /**
     * Return the number of microseconds elapsed between
     * the two given timeT indices into the composition, taking
     * into account any tempo changes between the two times.
     */
    RealTime getRealTimeDifference(timeT t0, timeT t1) const {
	if (t1 > t0) return getElapsedRealTime(t1) - getElapsedRealTime(t0);
	else	     return getElapsedRealTime(t0) - getElapsedRealTime(t1);
    }


    //////
    //
    //  OTHER TIME CONVERSIONS

    /**
     * Return (by reference) the bar number and beat/division values
     * corresponding to a given absolute time.
     */
    void getMusicalTimeForAbsoluteTime(timeT absoluteTime,
				       int &bar, int &beat,
				       int &fraction, int &remainder);

    /**
     * Return (by reference) the number of bars and beats/divisions
     * corresponding to a given duration.  The absolute time at which
     * the duration starts is also required, so as to know the correct
     * time signature.
     */
    void getMusicalTimeForDuration(timeT absoluteTime, timeT duration,
				   int &bars, int &beats,
				   int &fractions, int &remainder);

    /**
     * Return the absolute time corresponding to a given bar number
     * and beat/division values.
     */
    timeT getAbsoluteTimeForMusicalTime(int bar, int beat,
					int fraction, int remainder);

    /**
     * Return the duration corresponding to a given number of bars and
     * beats/divisions.  The absolute time at which the duration
     * starts is also required, so as to know the correct time
     * signature.
     */
    timeT getDurationForMusicalTime(timeT absoluteTime,
				    int bars, int beats,
				    int fractions, int remainder);


    /**
     * Get the current playback position.
     */
    timeT getPosition() const { return m_position; }

    /**
     * Set the current playback position.
     */
    void setPosition(timeT position);



    //////
    //
    // LOOP 

    timeT getLoopStart() const { return m_loopStart; }
    timeT getLoopEnd() const { return m_loopEnd;}

    void setLoopStart(const timeT &lS) { m_loopStart = lS; }
    void setLoopEnd(const timeT &lE) { m_loopEnd = lE; }

    // Determine if we're currently looping
    //
    bool isLooping() const { return (m_loopStart != m_loopEnd); }


    
    //////
    //
    // OTHER STUFF


    // Some set<> API delegation
    iterator       begin()       { return m_segments.begin(); }
    const_iterator begin() const { return m_segments.begin(); }
    iterator       end()         { return m_segments.end(); }
    const_iterator end() const   { return m_segments.end(); }


    // XML exportable method
    //
    virtual std::string toXmlString();

    static RealTime getTempoTimestamp(const Event *e);
    static void setTempoTimestamp(Event *e, RealTime r);

    // Who's making this racket?
    //
    Configuration &getMetadata() {
	return m_metadata;
    }
    const Configuration &getMetadata() const {
	return m_metadata;
    }
    
    std::string getCopyrightNote() const { 
	return m_metadata.get<String>(CompositionMetadataKeys::Copyright,
				      "");
    }
    void setCopyrightNote(const std::string &cr) {
	m_metadata.set<String>(CompositionMetadataKeys::Copyright, cr);
    }


    // We can have the metronome on or off while playing or
    // recording - get and set values from here
    //
    bool usePlayMetronome() const { return m_playMetronome; }
    bool useRecordMetronome() const { return m_recordMetronome; }

    void setPlayMetronome(bool value);
    void setRecordMetronome(bool value);


    // Colour stuff
    ColourMap& getSegmentColourMap() { return m_segmentColourMap; }
    void setSegmentColourMap(Rosegarden::ColourMap &newmap);

    // General colourmap for non-segments
    //
    ColourMap& getGeneralColourMap() { return m_generalColourMap; }
    void setGeneralColourMap(Rosegarden::ColourMap &newmap);


    //////
    //
    // QUANTIZERS

    /**
     * Return a quantizer that quantizes to the our most basic
     * units (i.e. a unit quantizer whose unit is our shortest
     * note duration).
     */
    const BasicQuantizer *getBasicQuantizer() const {
	return m_basicQuantizer;
    }

    /**
     * Return a quantizer that does quantization for notation
     * only.
     */
    const NotationQuantizer *getNotationQuantizer() const {
	return m_notationQuantizer;
    }


    //////
    //
    // REFRESH STATUS

    // delegate RefreshStatusArray API
    unsigned int getNewRefreshStatusId() {
	return m_refreshStatusArray.getNewRefreshStatusId();
    }
    
    RefreshStatus& getRefreshStatus(unsigned int id) {
	return m_refreshStatusArray.getRefreshStatus(id);
    }

    /// Set all refresh statuses to true
    void updateRefreshStatuses() {
	m_refreshStatusArray.updateRefreshStatuses();
    }


    void    addObserver(CompositionObserver *obs) { m_observers.push_back(obs); }
    void removeObserver(CompositionObserver *obs) { m_observers.remove(obs); }

    //////
    // DEBUG FACILITIES
    void dump(std::ostream&, bool full=false) const;
    
protected:

    static const std::string TempoEventType; 
    static const PropertyName TempoProperty; // stored in quarters per hour

    static const PropertyName NoAbsoluteTimeProperty;
    static const PropertyName BarNumberProperty;
    static const PropertyName TempoTimestampProperty;


    struct ReferenceSegmentEventCmp
    {
	bool operator()(const Event &e1, const Event &e2) const;
	bool operator()(const Event *e1, const Event *e2) const {
	    return operator()(*e1, *e2);
	}
    };
    
    struct BarNumberComparator
    {
	bool operator()(const Event &e1, const Event &e2) const {
	    return (e1.get<Int>(BarNumberProperty) <
		    e2.get<Int>(BarNumberProperty));
	}
	bool operator()(const Event *e1, const Event *e2) const {
	    return operator()(*e1, *e2);
	}
    };
 
    /**
     * Ensure the selected and record trackids still point to something valid
     * Must be called after deletion of detach of a track
     */
    void checkSelectedAndRecordTracks();
    TrackId getClosestValidTrackId(TrackId id) const;
    

    //--------------- Data members ---------------------------------
    //
    trackcontainer                    m_tracks;
    segmentcontainer                  m_segments;

    // The tracks we are armed for record on
    //
    recordtrackcontainer              m_recordTracks;

    // Are we soloing and if so which Track?
    //
    bool                              m_solo;
    Rosegarden::TrackId               m_selectedTrack;

    /**
     * This is a bit like a segment, but can only contain one sort of
     * event, and can only have one event at each absolute time
     */
    class ReferenceSegment :
	public FastVector<Event *> // not a set: want random access for bars
    {
	typedef FastVector<Event *> Impl;

    public:
	ReferenceSegment(std::string eventType);
	virtual ~ReferenceSegment();
    private:
        ReferenceSegment(const ReferenceSegment &);
        ReferenceSegment& operator=(const ReferenceSegment &);
    public:
	typedef Impl::iterator iterator;
	typedef Impl::size_type size_type;
	typedef Impl::difference_type difference_type;

	void clear();

	timeT getDuration() const;
	
	/// Inserts a single event, removing any existing one at that time
	iterator insert(Event *e); // may throw Event::BadType

	void erase(Event *e);

	iterator findTime(timeT time);
	iterator findNearestTime(timeT time);

	iterator findRealTime(RealTime time);
	iterator findNearestRealTime(RealTime time);

        std::string getEventType() const { return m_eventType; }

    private:
  	iterator find(Event *e);
	std::string m_eventType;
    };

    /// Contains time signature events
    mutable ReferenceSegment          m_timeSigSegment;

    /// Contains tempo events
    mutable ReferenceSegment          m_tempoSegment;

    /// affects m_timeSigSegment
    void calculateBarPositions() const;
    mutable bool                      m_barPositionsNeedCalculating;
    ReferenceSegment::iterator getTimeSignatureAtAux(timeT t) const;

    /// affects m_tempoSegment
    void calculateTempoTimestamps() const;
    mutable bool m_tempoTimestampsNeedCalculating;
    RealTime time2RealTime(timeT time, double tempo) const;
    timeT realTime2Time(RealTime rtime, double tempo) const;

    typedef std::list<CompositionObserver *> ObserverSet;
    ObserverSet m_observers;

    void notifySegmentAdded(Segment *) const;
    void notifySegmentRemoved(Segment *) const;
    void notifySegmentRepeatChanged(Segment *, bool) const;
    void notifySegmentEventsTimingChanged(Segment *s, timeT delay, RealTime rtDelay) const;
    void notifySegmentTransposeChanged(Segment *s, int transpose) const;
    void notifySegmentTrackChanged(Segment *s, TrackId id) const;
    void notifyEndMarkerChange(bool shorten) const;
    void notifyTrackChanged(Track*) const;
    void notifyTrackDeleted(TrackId) const;
    void notifyMetronomeChanged() const;
    void notifyTimeSignatureChanged() const;
    void notifySoloChanged() const;
    void notifyTempoChanged() const;
    void notifySourceDeletion() const;

    BasicQuantizer                   *m_basicQuantizer;
    NotationQuantizer                *m_notationQuantizer;

    timeT                             m_position;
    double                            m_defaultTempo;

    // Notional Composition markers - these define buffers for the
    // start and end of the piece, Segments can still exist outside
    // of these markers - these are for visual and playback cueing.
    //
    timeT                             m_startMarker;
    timeT                             m_endMarker;

    static int                        m_defaultNbBars;

    // Loop start and end positions.  If they're both the same
    // value (usually 0) then there's no loop set.
    //
    timeT                             m_loopStart;
    timeT                             m_loopEnd;

    Configuration                     m_metadata;

    bool                              m_playMetronome;
    bool                              m_recordMetronome;

    RefreshStatusArray<RefreshStatus> m_refreshStatusArray;

    // User defined markers in the composition
    //
    markercontainer                   m_markers;

    // Trigger segments (unsorted segments fired by events elsewhere)
    //
    triggersegmentcontainer           m_triggerSegments;
    TriggerSegmentId                  m_nextTriggerSegmentId;
 
    ColourMap                         m_segmentColourMap;
    ColourMap                         m_generalColourMap;
};


/**
 * If you subclass from CompositionObserver, you can then attach to a
 * Composition to receive notification when something changes.
 *
 * Normally all the methods in this class would be pure virtual.  But
 * because there are so many, that imposes far too much work on the
 * subclass implementation in a case where it only really wants to
 * know about one thing, such as segments being deleted.  So we have
 * empty default implementations, and you'll just have to take a bit
 * more care to make sure you really are making the correct
 * declarations in the subclass.
 */

class CompositionObserver
{
public:
    /**
     * Called after the segment has been added to the composition
     */
    virtual void segmentAdded(const Composition *, Segment *) { }

    /**
     * Called after the segment has been removed from the segment,
     * and just before it is deleted
     */
    virtual void segmentRemoved(const Composition *, Segment *) { }

    /**
     * Called when the segment's repeat status has changed
     */
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool) { }

    /**
     * Called when the segment's delay timing has changed
     */
    virtual void segmentEventsTimingChanged(const Composition *, Segment *,
                                            timeT /* delay */,
					    RealTime /* rtDelay */) { }

    /**
     * Called when the segment's transpose value has changed
     */
    virtual void segmentTransposeChanged(const Composition *, Segment *,
                                         int /* transpose */) { }

    /**
     * Called when the segment's track has changed
     */
    virtual void segmentTrackChanged(const Composition *, Segment *,
				     TrackId /* id */) { }

    /**
     * Called after the composition's end marker time has been
     * changed
     */
    virtual void endMarkerTimeChanged(const Composition *, bool /* shorten */) { }

    /**
     * Called when a track is changed (instrument id, muted status...)
     */
    virtual void trackChanged(const Composition *, Track*) { }

    /**
     * Called when a track has been deleted
     */
    virtual void trackDeleted(const Composition *, TrackId) { }

    /**
     * Called when some time signature has changed
     */
    virtual void timeSignatureChanged(const Composition *) { }
    
    /**
     * Called when metronome status has changed (on/off)
     */
    virtual void metronomeChanged(const Composition *) { }

    /**
     * Called when solo status changes (solo on/off, and selected track)
     */
    virtual void soloChanged(const Composition *, bool /* solo */,
			     TrackId /* selectedTrack */) { }

    /**
     * Called when solo status changes (solo on/off, and selected track)
     */
    virtual void tempoChanged(const Composition *) { }
    
    /**
     * Called from the composition dtor
     */
    virtual void compositionDeleted(const Composition *) { }
};

}


#endif

