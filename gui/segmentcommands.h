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


#ifndef _SEGMENTCOMMANDS_H_
#define _SEGMENTCOMMANDS_H_

#include <klocale.h>
#include <kcommand.h>

#include "Segment.h"
#include "Composition.h"
#include "NotationTypes.h"
#include "rosegardenguidoc.h"


class SegmentEraseCommand : public KCommand
{
public:
    SegmentEraseCommand(Rosegarden::Segment *segment);
    virtual ~SegmentEraseCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};

class SegmentCopyCommand : public KCommand
{
public:
    SegmentCopyCommand(Rosegarden::Segment *segment);
    virtual ~SegmentCopyCommand();

    virtual void execute();
    virtual void unexecute();

    // return pointer to new copy
    Rosegarden::Segment* getCopy() { return m_segment; }

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment     *m_segmentToCopy;
    Rosegarden::Segment     *m_segment;

};


class SegmentInsertCommand : public KCommand
{
public:
    SegmentInsertCommand(Rosegarden::Composition *composition,
                         Rosegarden::TrackId track,
                         Rosegarden::timeT startTime,
                         Rosegarden::timeT duration);
    virtual ~SegmentInsertCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment     *m_segment;
    int                      m_track;
    Rosegarden::timeT        m_startTime;
    Rosegarden::timeT        m_duration;
};


/**
 * SegmentRecordCommand pretends to insert a Segment that is actually
 * already in the Composition (the currently-being-recorded one).  It's
 * used at the end of recording, to ensure that GUI updates happen
 * correctly, and it provides the ability to undo recording.  (The
 * unexecute does remove the segment, it doesn't just pretend to.)
 */
class SegmentRecordCommand : public KCommand
{
public:
    SegmentRecordCommand(Rosegarden::Segment *segment);
    virtual ~SegmentRecordCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};


/**
 * SegmentReconfigureCommand is a general-purpose command for
 * moving, resizing or changing the track of one or more segments
 */
class SegmentReconfigureCommand : public KCommand
{
public:
    SegmentReconfigureCommand(QString name);
    virtual ~SegmentReconfigureCommand();

    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::timeT startTime;
        Rosegarden::timeT duration;
        Rosegarden::TrackId track;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;

    void addSegment(Rosegarden::Segment *segment,
                    Rosegarden::timeT startTime,
                    Rosegarden::timeT duration,
                    Rosegarden::TrackId track);

    void addSegments(const SegmentRecSet &records);

    void execute();
    void unexecute();

private:
    SegmentRecSet m_records;
    void swap();
};


class SegmentSplitCommand : public KCommand
{
public:
    SegmentSplitCommand(Rosegarden::Segment *segment,
                        Rosegarden::timeT splitTime);
    virtual ~SegmentSplitCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegment;
    Rosegarden::timeT m_splitTime;
};


class SegmentChangeQuantizationCommand : public KCommand
{
public:
    /// Set quantization on segments.  If sq is null, switch quantization off.
    SegmentChangeQuantizationCommand(Rosegarden::StandardQuantization *sq);
    virtual ~SegmentChangeQuantizationCommand();

    void addSegment(Rosegarden::Segment *s);

    virtual void execute();
    virtual void unexecute();

    static QString name(Rosegarden::StandardQuantization *sq);

private:
    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::Quantizer *oldQuantizer;
        bool wasQuantized;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;

    Rosegarden::StandardQuantization *m_quantization;
};


class AddTimeSignatureCommand : public KCommand
{
public:
    AddTimeSignatureCommand(Rosegarden::Composition *composition,
                            Rosegarden::timeT time,
                            Rosegarden::TimeSignature timeSig);
    virtual ~AddTimeSignatureCommand();

    static QString name() {
        return i18n("Add Time Si&gnature Change...");
    }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    Rosegarden::TimeSignature m_timeSignature;

    Rosegarden::TimeSignature *m_oldTimeSignature; // for undo
    int m_timeSigIndex; // for undo
};    



class AddTimeSignatureAndNormalizeCommand : public KMacroCommand
{
public:
    AddTimeSignatureAndNormalizeCommand(Rosegarden::Composition *composition,
					Rosegarden::timeT time,
					Rosegarden::TimeSignature timeSig);
    virtual ~AddTimeSignatureAndNormalizeCommand();
};



class AddTempoChangeCommand : public KCommand
{
public:
    AddTempoChangeCommand(Rosegarden::Composition *composition,
                          Rosegarden::timeT time,
                          double tempo):
	KCommand(name()),
        m_composition(composition),
        m_time(time),
        m_tempo(tempo),
        m_oldTempo(0),
        m_tempoChangeIndex(0) {}

    virtual ~AddTempoChangeCommand();

    static QString name()
    {
        return i18n("Add &Tempo Change...");
    }
    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    double m_tempo; // bpm
    int m_oldTempo; // bph
    int m_tempoChangeIndex;
};


class AddTracksCommand : public KCommand
{
public:
    AddTracksCommand(Rosegarden::Composition *composition,
                     unsigned int nbTracks): 
        KCommand(name()),
        m_composition(composition),
        m_nbNewTracks(nbTracks) {}

    static QString name()
    {
        return i18n("Add Tracks...");
    }
    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    unsigned int m_nbNewTracks;
};



#endif  // _SEGMENTCOMMANDS_H_
