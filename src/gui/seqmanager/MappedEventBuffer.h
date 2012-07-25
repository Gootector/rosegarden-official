/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAPPEDEVENTBUFFER_H
#define RG_MAPPEDEVENTBUFFER_H

#include "base/RealTime.h"
#include <QReadWriteLock>
#include <QAtomicInt>

namespace Rosegarden
{

class MappedEvent;
class MappedInserterBase;
class RosegardenDocument;

/// Abstract Base Class container for MappedEvent objects.
/**
 * MappedEventBuffer is an Abstract Base Class.  Consequently,
 * MappedEventBuffer objects are never created.  See the three
 * derived classes: MetronomeMapper, SegmentMapper, and
 * SpecialSegmentMapper.
 *
 * MappedEventBuffer is the container class for sound-making events
 * that have been mapped linearly into memory for ease of reading by
 * the sequencer code (after things like tempo mappings, repeats etc
 * have been mapped and unfolded).
 *
 * The mapping logic is handled by mappers derived from this class; this
 * class provides the basic container and the reading logic.  Reading
 * and writing may take place simultaneously without locks (we are
 * prepared to accept the lossage from individual mangled MappedEvent
 * reads) but a read/write lock on the buffer space guards against bad
 * access caused by resizes.
 *
 * MappedEventBuffer only concerns itself with the state of the
 * composition, as opposed to the state of performance.  No matter how
 * the user jumps around in time, it shouldn't change.  For the
 * current state of performance, see MappedEventBuffer::iterator.
 *
 * A MappedEventBuffer-derived object is jointly owned by one or more
 * metaiterators (MappedBufMetaIterator?) and by ChannelManager and deletes
 * itself when the last owner is removed.  See addOwner() and removeOwner().
 */
class MappedEventBuffer
{
public:
    MappedEventBuffer(RosegardenDocument *);
    // Some derived classes need to release channels, so dtor is
    // virtual.
    virtual ~MappedEventBuffer();

    /// Is this object a MetronomeMapper?
    /**
     * Used by MappedBufMetaIterator::fillNoncompeting() for special
     * handling of the metronome.
     *
     * This might be implemented as a virtual function that just returns
     * false, but is overridden in MetronomeMapper to return true.
     * setMetronome() and m_isMetronome could then be removed.
     * Switching on type like this is usually considered a Bad Thing.
     * Examination of MappedBufMetaIterator might lead to a more useful
     * function that could be overridden by MetronomeMapper.
     *
     * @see setMetronome()
     */
    bool isMetronome() const { return m_isMetronome; }

    /// Enables special handling related to MetronomeMapper
    /**
     * Used by MetronomeMapper to communicate with
     * MappedBufMetaIterator::fillNoncompeting().
     *
     * @see isMetronome()
     */
    void setMetronome(bool isMetronome) { m_isMetronome = isMetronome; }

    /// Access to the internal buffer of events.  NOT LOCKED
    /**
     * un-locked, use only from write/resize thread
     */
    MappedEvent *getBuffer() { return m_buffer; }

    /// Capacity of the buffer in MappedEvent's.  (STL's capacity().)
    /**
     * The use of "size" and "fill" in this class is in conflict with STL
     * terminology.  Recommend changing "size" to "capacity" and "fill"
     * to "size" in keeping with the STL.  So this routine would be
     * renamed to "capacity()".
     *
     * rename: capacity() (per STL)
     */
    int getBufferSize() const;
    /// Number of MappedEvent's in the buffer.  (STL's size().)
    /**
     * rename: size() (per STL)
     */
    int getBufferFill() const;

    /// Sets the buffer capacity.
    /**
     * Ignored if smaller than old capacity.
     *
     * @see getBufferSize()
     *
     * rename: reserve() (per STL)
     */
    void resizeBuffer(int newSize);

    /// Sets the number of events in the buffer.
    /**
     * Must be no bigger than buffer capacity.
     *
     * @see getBufferFill()
     *
     * rename: resize() (per STL)
     */
    void setBufferFill(int newFill);

    /// Refresh the object after the segment has been modified.
    /**
     * Returns true if size changed (and thus the sequencer
     * needs to be told about it).
     */
    bool refresh();

    /* Virtual functions */

    virtual int getSegmentRepeatCount()=0;

    /// Calculates the required capacity based on the current document.
    /**
     * Overridden by each deriver to compute the number of MappedEvent's
     * needed to hold the contents of the current document.
     *
     * Used by init() and refresh() to adjust the buffer capacity to be
     * big enough to hold all the events in the current document.
     *
     * @see m_doc
     * @see resizeBuffer()
     */
    virtual int calculateSize() = 0;

    /// Two-phase initialization.
    /**
     * Actual setup, must be called after ctor, calls virtual methods.
     * Dynamic Binding During Initialization idiom.
     */
    void init();

    /// Initialization particular to various classes.  (UNUSED)
    /**
     * The only overrider is SegmentMapper and it just debug-prints.
     */
    virtual void initSpecial(void)  { }
    
    /// dump all segment data into m_buffer
    virtual void dump()=0;

    /// Insert a MappedEvent with appropriate setup for channel.
    /**
     * refTime is not neccessarily the same as MappedEvent's
     * getEventTime() because we might jump into the middle of a long
     * note.
     */
    virtual void
        doInsert(MappedInserterBase &inserter, MappedEvent &evt,
                 RealTime refTime, bool firstOutput);

    /// Record one more owner of this mapper.
    /**
     * This increases the reference count to prevent deletion of a mapper
     * that is still owned.
     *
     * Called by:
     *   - MappedEventBuffer::iterator's ctor
     *   - CompositionMapper::mapSegment()
     *   - SequenceManager::resetMetronomeMapper()
     *   - SequenceManager::resetTempoSegmentMapper()
     *   - SequenceManager::resetTimeSigSegmentMapper()
     *
     * @see removeOwner()
     */
    void addOwner(void);

    /// Record one fewer owner of this mapper.
    /**
     * @see addOwner()
     */
    void removeOwner(void);

    // Get the earliest and latest sounding times.
    void getStartEnd(RealTime &start, RealTime &end) {
        start = m_start;
        end   = m_end;
    }

 protected:
    void mapAnEvent(MappedEvent *e);

    // Set the sounding times.
    void setStartEnd(RealTime &start, RealTime &end) {
        m_start = start;
        m_end   = end;
    }
 public:
    class iterator 
    {
    public:
        iterator(MappedEventBuffer* s);
        ~iterator(void) { m_s->removeOwner(); };

        // Never used.  Default bitwise version is fine anyway.
        //iterator& operator=(const iterator&);

        bool operator==(const iterator&);
        bool operator!=(const iterator& it) { return !operator==(it); }

        bool atEnd() const;

        /// go back to beginning of stream
        void reset();

        iterator& operator++();
        iterator  operator++(int);
        iterator& operator+=(int);
        iterator& operator-=(int);

        MappedEvent operator*();  // returns MappedEvent() if atEnd()
        MappedEvent *peek() const; // returns 0 if atEnd()

        MappedEventBuffer *getSegment() { return m_s; }
        const MappedEventBuffer *getSegment() const { return m_s; }

        void setActive(bool value, RealTime currentTime) {
            m_active = value;
            m_currentTime = currentTime;
        }
        void setInactive(void) { m_active = false; }
        bool getActive(void) { return m_active; }
        void setReady (bool value) { m_ready  = value; };
        bool getReady(void)  { return m_ready; }

        // Do appropriate preparation for inserting event, including
        // possibly setting up the channel.
        void doInsert(MappedInserterBase &inserter, MappedEvent &evt);

    private:
        // Hide the default ctor.
        // (Why?  The other ctor will hide this anyway.)
        //iterator();

    protected:
        MappedEventBuffer *m_s;
        int m_index;

        // Whether we are ready with regard to performance time.  We
        // always are except when starting or jumping in time.  
        // Making us ready is derived classes' job via "doInsert",
        bool m_ready;
        // Whether this iterator has more events to give within the
        // current time slice.
        bool m_active;

        // RealTime when the current event starts sounding.  Either
        // the current event's time or the time the loop starts,
        // whichever is greater.  Used for calculating the correct controllers
        RealTime  m_currentTime;
    };

protected:
    friend class iterator;

    /// Capacity of the buffer.
    /**
     * To be consistent with the STL, this would be better named m_capacity.
     */
    mutable QAtomicInt m_size;

    /// Number of events in the buffer.
    /**
     * To be consistent with the STL, this would be better named m_size.
     */
    mutable QAtomicInt m_fill;

    /// The Mapped Event Buffer
    MappedEvent *m_buffer;

    bool m_isMetronome;

    QReadWriteLock m_lock;

    /// Not used here.  Convenience for derivers.
    /**
     * Derivers should probably use RosegardenMainWindow::self() to get to
     * RosegardenDocument.
     */
    RosegardenDocument *m_doc;

    /// Earliest sounding time.
    /**
     * It is the responsibility of "dump()" to keep this field up to date.
     *
     * @see m_end
     */
    RealTime m_start;

    /// Latest sounding time.
    /**
     * It is the responsibility of "dump()" to keep this field up to date.
     *
     * @see m_start
     */
    RealTime m_end;

    /// How many metaiterators share this mapper.
    /**
     * We won't delete while it has any owner.  This is changed just in
     * owner's ctors and dtors.
     *
     * @see addOwner()
     * @see removeOwner()
     */
    int m_refCount;

private:
    // Hide copy ctor and op=
    MappedEventBuffer(const MappedEventBuffer &);
    MappedEventBuffer &operator=(const MappedEventBuffer &);
};

}

#endif /* ifndef _MAPPEDSEGMENT_H_ */
