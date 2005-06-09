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


#ifndef _SEGMENTCOMMANDS_H_
#define _SEGMENTCOMMANDS_H_

#include <klocale.h>
#include <kcommand.h>

#include "Segment.h"
#include "NotationTypes.h"
#include "rosegardenguidoc.h"
#include "AudioFileManager.h"
#include "Selection.h"
#include "TriggerSegment.h"
#include "basiccommand.h"

namespace Rosegarden { class Composition; }

/**
 * Base class for commands from the SegmentParameterBox
 */
class SegmentCommand : public KNamedCommand
{
public:
    SegmentCommand(QString name, const std::vector<Rosegarden::Segment*>&);

    typedef std::vector<Rosegarden::Segment*> segmentlist;
    
protected:
    segmentlist m_segments;
};

/**
 * Repeat segment command from the SegmentParameterBox
 */
class SegmentCommandRepeat : public SegmentCommand
{
public:
    SegmentCommandRepeat(const std::vector<Rosegarden::Segment*>&,
                         bool repeat);

    virtual void execute();
    virtual void unexecute();

protected:
    bool m_repeatState;
};

// Disabled until we find a better solution
//
// As it is, command creation happens on every change of the text
// value of the combo box
//
//
// class SegmentCommandChangeTransposeValue : public SegmentCommand
// {
// public:
//     SegmentCommandChangeTransposeValue(const std::vector<Rosegarden::Segment*>&,
//                                        int transposeValue);

//     virtual void execute();
//     virtual void unexecute();

// protected:
//     int m_transposeValue;
//     std::vector<int> m_savedValues;
// };


////////////////////////////////////////////////////////////

class SegmentEraseCommand : public KNamedCommand
{
public:
    /// for removing segment normally
    SegmentEraseCommand(Rosegarden::Segment *segment);

    /// for removing audio segment when removing an audio file
    SegmentEraseCommand(Rosegarden::Segment *segment,
			Rosegarden::AudioFileManager *mgr);
    virtual ~SegmentEraseCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
    Rosegarden::AudioFileManager *m_mgr;
    std::string m_audioFileName;
    bool m_detached;
};

class SegmentRepeatToCopyCommand : public KNamedCommand
{
public:
    SegmentRepeatToCopyCommand(Rosegarden::Segment *segment);
    virtual ~SegmentRepeatToCopyCommand();

    virtual void execute();
    virtual void unexecute();
    
private:

    Rosegarden::Composition           *m_composition;
    Rosegarden::Segment               *m_segment;
    std::vector<Rosegarden::Segment*>  m_newSegments;
    bool                               m_detached;
};

class SegmentSingleRepeatToCopyCommand : public KNamedCommand
{
public:
    SegmentSingleRepeatToCopyCommand(Rosegarden::Segment *segment,
				     Rosegarden::timeT time);
    virtual ~SegmentSingleRepeatToCopyCommand();

    virtual void execute();
    virtual void unexecute();

    Rosegarden::Segment *getNewSegment() const { return m_newSegment; }

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegment;
    Rosegarden::timeT m_time;
    bool m_detached;
};

class SegmentQuickCopyCommand : public KNamedCommand
{
public:
    SegmentQuickCopyCommand(Rosegarden::Segment *segment);
    virtual ~SegmentQuickCopyCommand();

    virtual void execute();
    virtual void unexecute();

    // return pointer to new copy
    Rosegarden::Segment* getCopy() { return m_segment; }

    static QString getGlobalName() { return i18n("Quick-Copy Segment"); }

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment     *m_segmentToCopy;
    Rosegarden::Segment     *m_segment;
    bool m_detached;
};


class AudioSegmentInsertCommand : public KNamedCommand
{
public:
    AudioSegmentInsertCommand(RosegardenGUIDoc *doc,
                              Rosegarden::TrackId track,
                              Rosegarden::timeT startTime,
                              Rosegarden::AudioFileId audioFileId,
                              const Rosegarden::RealTime &audioStartTime,
                              const Rosegarden::RealTime &audioEndTime);
    virtual ~AudioSegmentInsertCommand();

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition      *m_composition;
    Rosegarden::Studio           *m_studio;
    Rosegarden::AudioFileManager *m_audioFileManager;
    Rosegarden::Segment          *m_segment;
    int                           m_track;
    Rosegarden::timeT             m_startTime;
    Rosegarden::AudioFileId       m_audioFileId;
    Rosegarden::RealTime          m_audioStartTime;
    Rosegarden::RealTime          m_audioEndTime;
    bool                          m_detached;
};

class SegmentInsertCommand : public KNamedCommand
{
public:
    SegmentInsertCommand(RosegardenGUIDoc *doc,
                         Rosegarden::TrackId track,
                         Rosegarden::timeT startTime,
                         Rosegarden::timeT endTime);
    SegmentInsertCommand(Rosegarden::Composition *composition,
			 Rosegarden::Segment *segment,
			 Rosegarden::TrackId track);
    virtual ~SegmentInsertCommand();

    Rosegarden::Segment *getSegment() const; // after invocation

    virtual void execute();
    virtual void unexecute();
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    Rosegarden::Segment     *m_segment;
    int                      m_track;
    Rosegarden::timeT        m_startTime;
    Rosegarden::timeT        m_endTime;
    bool m_detached;
};


/**
 * SegmentRecordCommand pretends to insert a Segment that is actually
 * already in the Composition (the currently-being-recorded one).  It's
 * used at the end of recording, to ensure that GUI updates happen
 * correctly, and it provides the ability to undo recording.  (The
 * unexecute does remove the segment, it doesn't just pretend to.)
 */
class SegmentRecordCommand : public KNamedCommand
{
public:
    SegmentRecordCommand(Rosegarden::Segment *segment);
    virtual ~SegmentRecordCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
    bool m_detached;
};


/**
 * SegmentReconfigureCommand is a general-purpose command for
 * moving, resizing or changing the track of one or more segments
 */
class SegmentReconfigureCommand : public KNamedCommand
{
public:
    SegmentReconfigureCommand(QString name);
    virtual ~SegmentReconfigureCommand();

    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::timeT startTime;
        Rosegarden::timeT endMarkerTime;
        Rosegarden::TrackId track;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;

    void addSegment(Rosegarden::Segment *segment,
                    Rosegarden::timeT startTime,
                    Rosegarden::timeT endMarkerTime,
                    Rosegarden::TrackId track);

    void addSegments(const SegmentRecSet &records);

    void execute();
    void unexecute();

private:
    SegmentRecSet m_records;
    void swap();
};


/**
 * SegmentResizeFromStartCommand moves the start time of a segment
 * leaving the events in it at the same absolute times as before, so
 * padding with rests or deleting events as appropriate.  (Distinct
 * from Segment::setStartTime, as used by SegmentReconfigureCommand,
 * which moves all the events in the segment.)
 */

class SegmentResizeFromStartCommand : public BasicCommand
{
public:
    SegmentResizeFromStartCommand(Rosegarden::Segment *segment,
				  Rosegarden::timeT newStartTime);
    virtual ~SegmentResizeFromStartCommand();

protected:
    virtual void modifySegment();

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::timeT m_oldStartTime;
    Rosegarden::timeT m_newStartTime;
};


class AudioSegmentSplitCommand : public KNamedCommand
{
public:
    AudioSegmentSplitCommand(Rosegarden::Segment *segment,
                             Rosegarden::timeT splitTime);
    virtual ~AudioSegmentSplitCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegment;
    Rosegarden::timeT m_splitTime;
    Rosegarden::timeT *m_previousEndMarkerTime;
    bool m_detached;
    std::string m_segmentLabel;
    Rosegarden::RealTime m_previousEndAudioTime;
};

class SegmentSplitCommand : public KNamedCommand
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
    Rosegarden::timeT *m_previousEndMarkerTime;
    bool m_detached;
    std::string m_segmentLabel;
};

class AudioSegmentAutoSplitCommand : public KNamedCommand
{
public:
    AudioSegmentAutoSplitCommand(RosegardenGUIDoc *doc,
                                 Rosegarden::Segment *segment,
                                 int threshold);
    virtual ~AudioSegmentAutoSplitCommand();

    virtual void execute();
    virtual void unexecute();
    
    static QString getGlobalName() { return i18n("&Split on Silence"); }

private:
    Rosegarden::Segment                *m_segment;
    Rosegarden::Composition            *m_composition;
    Rosegarden::AudioFileManager       *m_audioFileManager;
    std::vector<Rosegarden::Segment *>  m_newSegments;
    bool                                m_detached;
    int                                 m_threshold;
};

class SegmentAutoSplitCommand : public KNamedCommand
{
public:
    SegmentAutoSplitCommand(Rosegarden::Segment *segment);
    virtual ~SegmentAutoSplitCommand();

    virtual void execute();
    virtual void unexecute();
    
    static QString getGlobalName() { return i18n("&Split on Silence"); }

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::Composition *m_composition;
    std::vector<Rosegarden::Segment *> m_newSegments;
    bool m_detached;
};


class SegmentMergeCommand : public KNamedCommand
{
public:
    SegmentMergeCommand(Rosegarden::SegmentSelection &segments);
    virtual ~SegmentMergeCommand();

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName() { return i18n("&Join"); }
    
private:
    std::vector<Rosegarden::Segment *> m_oldSegments;
    Rosegarden::Segment *m_newSegment;
    bool m_detached;
};


class SegmentRescaleCommand : public KNamedCommand
{
public:
    SegmentRescaleCommand(Rosegarden::Segment *segment,
			  int multiplier,
			  int divisor);
    virtual ~SegmentRescaleCommand();

    virtual void execute();
    virtual void unexecute();
    
    static QString getGlobalName() { return i18n("Stretch or S&quash..."); }

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegment;
    int m_multiplier;
    int m_divisor;
    bool m_detached;

    Rosegarden::timeT rescale(Rosegarden::timeT);
};


class SegmentChangeQuantizationCommand : public KNamedCommand
{
public:
    /// Set quantization on segments.  If unit is zero, switch quantization off
    SegmentChangeQuantizationCommand(Rosegarden::timeT);
    virtual ~SegmentChangeQuantizationCommand();

    void addSegment(Rosegarden::Segment *s);

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName(Rosegarden::timeT);

private:
    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::timeT oldUnit;
        bool wasQuantized;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;

    Rosegarden::timeT m_unit;
};


class AddTimeSignatureCommand : public KNamedCommand
{
public:
    AddTimeSignatureCommand(Rosegarden::Composition *composition,
                            Rosegarden::timeT time,
                            Rosegarden::TimeSignature timeSig);
    virtual ~AddTimeSignatureCommand();

    static QString getGlobalName() { return i18n("Add Time Si&gnature Change..."); }

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


class RemoveTimeSignatureCommand : public KNamedCommand
{
public:
    RemoveTimeSignatureCommand(Rosegarden::Composition *composition,
			       int index):
	KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_timeSigIndex(index),
        m_oldTime(0),
        m_oldTimeSignature() { }

    virtual ~RemoveTimeSignatureCommand() {}

    static QString getGlobalName() { return i18n("Remove &Time Signature Change..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition  *m_composition;
    int                       m_timeSigIndex;
    Rosegarden::timeT         m_oldTime;
    Rosegarden::TimeSignature m_oldTimeSignature;
};    


class ModifyDefaultTempoCommand : public KNamedCommand
{
public:
    ModifyDefaultTempoCommand(Rosegarden::Composition *composition,
                              double tempo):
	KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_tempo(tempo) {}

    virtual ~ModifyDefaultTempoCommand() {}

    static QString getGlobalName() { return i18n("Modify &Default Tempo..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    double                   m_tempo;
    double                   m_oldTempo;
};


class RemoveTempoChangeCommand : public KNamedCommand
{
public:
    RemoveTempoChangeCommand(Rosegarden::Composition *composition,
                             int index):
	KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_tempoChangeIndex(index),
        m_oldTime(0),
        m_oldTempo(0){}

    virtual ~RemoveTempoChangeCommand() {}

    static QString getGlobalName() { return i18n("Remove &Tempo Change..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    int                      m_tempoChangeIndex;
    Rosegarden::timeT        m_oldTime;
    long                     m_oldTempo; // bph
};



class AddTempoChangeCommand : public KNamedCommand
{
public:
    AddTempoChangeCommand(Rosegarden::Composition *composition,
                          Rosegarden::timeT time,
                          double tempo):
	KNamedCommand(getGlobalName()),
        m_composition(composition),
        m_time(time),
        m_tempo(tempo),
        m_oldTempo(0),
        m_tempoChangeIndex(0) {}

    virtual ~AddTempoChangeCommand();

    static QString getGlobalName() { return i18n("Add Te&mpo Change..."); }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    double m_tempo; // bpm
    int m_oldTempo; // bph
    int m_tempoChangeIndex;
};


class AddTracksCommand : public KNamedCommand
{
public:
    AddTracksCommand(Rosegarden::Composition *composition,
                     unsigned int nbTracks,
                     Rosegarden::InstrumentId id);
    virtual ~AddTracksCommand();

    static QString getGlobalName() { return i18n("Add Tracks..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition           *m_composition;
    unsigned int                       m_nbNewTracks;
    Rosegarden::InstrumentId           m_instrumentId;

    std::vector<Rosegarden::Track*>    m_newTracks;

    bool                               m_detached;
};

class DeleteTracksCommand : public KNamedCommand
{
public:
    DeleteTracksCommand(Rosegarden::Composition *composition,
                        std::vector<Rosegarden::TrackId> tracks);
    virtual ~DeleteTracksCommand();

    static QString getGlobalName() { return i18n("Delete Tracks..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition           *m_composition;
    std::vector<Rosegarden::TrackId>   m_tracks;

    std::vector<Rosegarden::Track*>    m_oldTracks;
    std::vector<Rosegarden::Segment*>  m_oldSegments;
    bool                               m_detached;
};

class MoveTracksCommand : public KNamedCommand
{
public:
    MoveTracksCommand(Rosegarden::Composition *composition,
                      Rosegarden::TrackId srcTrack,
                      Rosegarden::TrackId destTrack);
    virtual ~MoveTracksCommand();

    static QString getGlobalName() { return i18n("Move Tracks..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition           *m_composition;

    Rosegarden::TrackId                m_srcTrack;
    Rosegarden::TrackId                m_destTrack;
};

class RenameTrackCommand : public KNamedCommand
{
public:
    RenameTrackCommand(Rosegarden::Composition *composition,
		       Rosegarden::TrackId track, 
		       std::string name);
    virtual ~RenameTrackCommand();

    static QString getGlobalName() { return i18n("Rename Track"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::TrackId      m_track;
    std::string              m_oldName;
    std::string              m_newName;
};

class ChangeCompositionLengthCommand : public KNamedCommand
{
public:
    ChangeCompositionLengthCommand(Rosegarden::Composition *composition,
                                   Rosegarden::timeT startTime,
                                   Rosegarden::timeT endTime);
    virtual ~ChangeCompositionLengthCommand();

    static QString getGlobalName()
        { return i18n("Change &Composition Start and End..."); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT        m_startTime;
    Rosegarden::timeT        m_endTime;
    Rosegarden::timeT        m_oldStartTime;
    Rosegarden::timeT        m_oldEndTime;

};

class SegmentSplitByPitchCommand : public KNamedCommand
{
public:
    enum ClefHandling {
	LeaveClefs,
	RecalculateClefs,
	UseTrebleAndBassClefs
    };
    
    SegmentSplitByPitchCommand(Rosegarden::Segment *segment,
			       int splitPitch,
			       bool ranging,
			       bool duplicateNonNoteEvents,
			       ClefHandling clefHandling);
    virtual ~SegmentSplitByPitchCommand();

    static QString getGlobalName()
        { return i18n("Split by &Pitch..."); }

    virtual void execute();
    virtual void unexecute();

private:
    int getSplitPitchAt(Rosegarden::Segment::iterator i, int lastSplitPitch);

    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegmentA;
    Rosegarden::Segment *m_newSegmentB;
    int m_splitPitch;
    bool m_ranging;
    bool m_dupNonNoteEvents;
    ClefHandling m_clefHandling;
    bool m_executed;
};

class SegmentLabelCommand : public KNamedCommand
{
public:
    SegmentLabelCommand(Rosegarden::SegmentSelection &segments,
                        const QString &label);
    virtual ~SegmentLabelCommand();

    static QString getGlobalName()
        { return i18n("Re&label..."); }

    virtual void execute();
    virtual void unexecute();
protected:

    std::vector<Rosegarden::Segment*>  m_segments;
    std::vector<QString>               m_labels;
    QString                            m_newLabel;
};


class SegmentColourCommand : public KNamedCommand
{
public:
    SegmentColourCommand(Rosegarden::SegmentSelection &segments,
                         const unsigned int index);
    virtual ~SegmentColourCommand();

    static QString getGlobalName()
        { return i18n("Change Segment Color..."); }

    virtual void execute();
    virtual void unexecute();
protected:

    std::vector<Rosegarden::Segment*>     m_segments;
    std::vector<unsigned int>             m_oldColourIndexes;
    unsigned int                          m_newColourIndex;
};

class SegmentColourMapCommand : public KNamedCommand
{
public:
    SegmentColourMapCommand(      RosegardenGUIDoc*      doc,
                            const Rosegarden::ColourMap& map);
    virtual ~SegmentColourMapCommand();

    static QString getGlobalName()
        { return i18n("Change Segment Color Map..."); }

    virtual void execute();
    virtual void unexecute();
protected:
    RosegardenGUIDoc *                m_doc;
    Rosegarden::ColourMap             m_oldMap;
    Rosegarden::ColourMap             m_newMap;
};


// Trigger Segment commands.  These are the commands that create
// and manage the triggered segments themselves.  See editcommands.h
// for SetTriggerCommand and ClearTriggersCommand which manipulate
// the events that do the triggering.

class AddTriggerSegmentCommand : public KNamedCommand
{
public:
    AddTriggerSegmentCommand(RosegardenGUIDoc *doc,
			     Rosegarden::timeT duration, // start time always 0
			     int basePitch = -1,
			     int baseVelocity = -1);
    virtual ~AddTriggerSegmentCommand();

    Rosegarden::TriggerSegmentId getId() const; // after invocation

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_duration;
    int m_basePitch;
    int m_baseVelocity;
    Rosegarden::TriggerSegmentId m_id;
    Rosegarden::Segment *m_segment;
    bool m_detached;
};


class DeleteTriggerSegmentCommand : public KNamedCommand
{
public:
    DeleteTriggerSegmentCommand(RosegardenGUIDoc *doc,
				Rosegarden::TriggerSegmentId);
    virtual ~DeleteTriggerSegmentCommand();

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::TriggerSegmentId m_id;
    Rosegarden::Segment *m_segment;
    int m_basePitch;
    int m_baseVelocity;
    bool m_detached;
};


class PasteToTriggerSegmentCommand : public KNamedCommand
{
public:
    /// If basePitch is -1, the first pitch in the selection will be used
    PasteToTriggerSegmentCommand(Rosegarden::Composition *composition,
				 Rosegarden::Clipboard *clipboard,
				 QString label,
				 int basePitch = -1,
				 int baseVelocity = -1);
    virtual ~PasteToTriggerSegmentCommand();

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Clipboard *m_clipboard;
    QString m_label;
    int m_basePitch;
    int m_baseVelocity;
    Rosegarden::Segment *m_segment;
    Rosegarden::TriggerSegmentId m_id;
    bool m_detached;
};


class SetTriggerSegmentBasePitchCommand : public KNamedCommand
{
public:
    SetTriggerSegmentBasePitchCommand(Rosegarden::Composition *composition,
				      Rosegarden::TriggerSegmentId id,
				      int newPitch);
    virtual ~SetTriggerSegmentBasePitchCommand();
    
    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::TriggerSegmentId m_id;
    int m_newPitch;
    int m_oldPitch;
};


class SetTriggerSegmentBaseVelocityCommand : public KNamedCommand
{
public:
    SetTriggerSegmentBaseVelocityCommand(Rosegarden::Composition *composition,
				      Rosegarden::TriggerSegmentId id,
				      int newVelocity);
    virtual ~SetTriggerSegmentBaseVelocityCommand();
    
    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::TriggerSegmentId m_id;
    int m_newVelocity;
    int m_oldVelocity;
};


class SetTriggerSegmentDefaultTimeAdjustCommand : public KNamedCommand
{
public:
    SetTriggerSegmentDefaultTimeAdjustCommand(Rosegarden::Composition *composition,
					      Rosegarden::TriggerSegmentId id,
					      std::string newDefaultTimeAdjust);
    virtual ~SetTriggerSegmentDefaultTimeAdjustCommand();

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::TriggerSegmentId m_id;
    std::string m_newDefaultTimeAdjust;
    std::string m_oldDefaultTimeAdjust;
};


class SetTriggerSegmentDefaultRetuneCommand : public KNamedCommand
{
public:
    SetTriggerSegmentDefaultRetuneCommand(Rosegarden::Composition *composition,
					  Rosegarden::TriggerSegmentId id,
					  bool newDefaultRetune);
    virtual ~SetTriggerSegmentDefaultRetuneCommand();

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::TriggerSegmentId m_id;
    bool m_newDefaultRetune;
    bool m_oldDefaultRetune;
    bool m_haveOldDefaultRetune;
};


/**
 * CreateTempoMapFromSegment applies timings found in a reference
 * segment to the composition as a whole via the tempo map.
 */

class CreateTempoMapFromSegmentCommand : public KNamedCommand
{
public:
    CreateTempoMapFromSegmentCommand(Rosegarden::Segment *grooveSegment);
    virtual ~CreateTempoMapFromSegmentCommand();

    virtual void execute();
    virtual void unexecute();

private:
    void initialise(Rosegarden::Segment *s);
    
    Rosegarden::Composition *m_composition;

    typedef std::map<Rosegarden::timeT, long> TempoMap;
    TempoMap m_oldTempi;
    TempoMap m_newTempi;
};
    
    


#endif  // _SEGMENTCOMMANDS_H_
