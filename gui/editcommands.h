// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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

#ifndef EDIT_COMMANDS_H
#define EDIT_COMMANDS_H


#include "basiccommand.h"

#include "Quantizer.h"

namespace Rosegarden {
    class Clipboard;
    class EventSelection;
    class SegmentSelection;

    // Patterns of properties
    //
    typedef enum 
    {
        FlatPattern,          // set selection to velocity 1.
        AlternatingPattern,   // alternate between velocity 1 and 2 on
                              // subsequent events.
        CrescendoPattern,     // increasing from velocity 1 to velocity 2.
        DecrescendoPattern,   // decreasing from velocity 1 to velocity 2.
        RingingPattern        // between velocity 1 and 2, dying away.
    } PropertyPattern;

}


/// Cut a selection

class CutCommand : public KMacroCommand
{
public:
    /// Make a CutCommand that cuts events from within a Segment
    CutCommand(Rosegarden::EventSelection &selection,
	       Rosegarden::Clipboard *clipboard);

    /// Make a CutCommand that cuts whole Segments
    CutCommand(Rosegarden::SegmentSelection &selection,
	       Rosegarden::Clipboard *clipboard);

    static QString getGlobalName() { return "Cu&t"; }
};


/// Cut a selection and close the gap

class CutAndCloseCommand : public KMacroCommand
{
public:
    CutAndCloseCommand(Rosegarden::EventSelection &selection,
		       Rosegarden::Clipboard *clipboard);

    static QString getGlobalName() { return "C&ut and Close"; }

protected:
    class CloseCommand : public XKCommand
    {
    public:
	CloseCommand(Rosegarden::Segment *segment,
		     Rosegarden::timeT fromTime,
		     Rosegarden::timeT toTime) :
	    XKCommand("Close"),
	    m_segment(segment),
	    m_fromTime(fromTime),
	    m_toTime(toTime) { }

	virtual void execute();
	virtual void unexecute();

    private:
	Rosegarden::Segment *m_segment;
	Rosegarden::timeT m_fromTime;
	Rosegarden::timeT m_toTime;
    };
};    


/// Copy a selection

class CopyCommand : public XKCommand
{
public:
    /// Make a CopyCommand that copies events from within a Segment
    CopyCommand(Rosegarden::EventSelection &selection,
		Rosegarden::Clipboard *clipboard);

    /// Make a CopyCommand that copies whole Segments
    CopyCommand(Rosegarden::SegmentSelection &selection,
		Rosegarden::Clipboard *clipboard);

    virtual ~CopyCommand();

    static QString getGlobalName() { return "&Copy"; }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Clipboard *m_sourceClipboard;
    Rosegarden::Clipboard *m_targetClipboard;
};


/// Paste one or more segments from the clipboard into the composition

class PasteSegmentsCommand : public XKCommand
{
public:
    PasteSegmentsCommand(Rosegarden::Composition *composition,
			 Rosegarden::Clipboard *clipboard,
			 Rosegarden::timeT pasteTime);

    virtual ~PasteSegmentsCommand();

    static QString getGlobalName() { return "&Paste"; }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Clipboard *m_clipboard;
    Rosegarden::timeT m_pasteTime;
    std::vector<Rosegarden::Segment *> m_addedSegments;
};


/// Paste from a single-segment clipboard to a segment

class PasteEventsCommand : public BasicCommand
{
public:
    enum PasteType {
	Restricted,		// paste into existing gap
	Simple,			// erase existing events to make room
	OpenAndPaste,		// bump up existing events to make room
	NoteOverlay,		// overlay and tie notation-style
	MatrixOverlay		// overlay raw matrix-style
    };

    typedef std::map<PasteType, std::string> PasteTypeMap;
    static PasteTypeMap getPasteTypes(); // type, descrip

    PasteEventsCommand(Rosegarden::Segment &segment,
		       Rosegarden::Clipboard *clipboard,
		       Rosegarden::timeT pasteTime,
		       PasteType pasteType);

    static QString getGlobalName() { return "&Paste"; }

    /// Determine whether this paste will succeed (without executing it yet)
    bool isPossible();

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();
    Rosegarden::timeT getEffectiveEndTime(Rosegarden::Segment &,
					  Rosegarden::Clipboard *,
					  Rosegarden::timeT);
    Rosegarden::timeT m_relayoutEndTime;
    Rosegarden::Clipboard *m_clipboard;
    PasteType m_pasteType;
};


/// Erase a selection from within a segment

class EraseCommand : public BasicSelectionCommand
{
public:
    EraseCommand(Rosegarden::EventSelection &selection);

    static QString getGlobalName() { return "&Erase"; }

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};


/**
 * Replace an event with another one (likely to be used in
 * conjunction with EventEditDialog)
 */

class EventEditCommand : public BasicCommand
{
public:
    EventEditCommand(Rosegarden::Segment &segment,
		     Rosegarden::Event *eventToModify,
		     const Rosegarden::Event &newEvent);

    static QString getGlobalName() { return "Edit E&vent"; }

protected:
    virtual void modifySegment();

private:
    Rosegarden::Event *m_oldEvent; // only used on 1st execute
    Rosegarden::Event m_newEvent; // only used on 1st execute
};


class EventQuantizeCommand : public BasicCommand
{
public:
    EventQuantizeCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT startTime,
			 Rosegarden::timeT endTime,
			 Rosegarden::Quantizer);
    
    EventQuantizeCommand(Rosegarden::EventSelection &selection,
			 Rosegarden::Quantizer);
    
    static QString getGlobalName(Rosegarden::Quantizer *quantizer = 0);
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::Quantizer m_quantizer;
    Rosegarden::EventSelection *m_selection;
};

// Set the (numerical) property of a selection according given pattern.
//
class SelectionPropertyCommand : public BasicSelectionCommand
{
public:

    SelectionPropertyCommand(Rosegarden::EventSelection *selection,
                             const Rosegarden::PropertyName &property,
                             Rosegarden::PropertyPattern pattern,
                             int value1,
                             int value2);

    static QString getGlobalName() { return "Set &Property"; }

    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;
    Rosegarden::PropertyName    m_property;
    Rosegarden::PropertyPattern m_pattern;
    int                         m_value1;
    int                         m_value2;

};

class EventUnquantizeCommand : public BasicCommand
{
public:
    EventUnquantizeCommand(Rosegarden::Segment &segment,
			   Rosegarden::timeT startTime,
			   Rosegarden::timeT endTime,
			   Rosegarden::Quantizer);
    
    EventUnquantizeCommand(Rosegarden::EventSelection &selection,
			   Rosegarden::Quantizer);
    
    static QString getGlobalName(Rosegarden::Quantizer *quantizer = 0);
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::Quantizer m_quantizer;
    Rosegarden::EventSelection *m_selection;
};


class SetLyricsCommand : public XKCommand
{
public:
    SetLyricsCommand(Rosegarden::Segment *segment, QString newLyricData);
    ~SetLyricsCommand();
    
    static QString getGlobalName() { return "Edit L&yrics"; }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Segment *m_segment;
    std::vector<Rosegarden::Event *> m_oldLyricEvents;
    QString m_newLyricData;
};


class TransposeCommand : public BasicSelectionCommand
{
public:
    TransposeCommand(int semitones, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(semitones), selection, true),
	m_selection(&selection), m_semitones(semitones) { }

    static QString getGlobalName(int semitones = 0) {
	switch (semitones) {
	case   1: return "&Up a Semitone";
	case  -1: return "&Down a Semitone";
	case  12: return "Up an &Octave";
	case -12: return "Down an Octa&ve";
	default:  return "&Transpose...";
	}
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};


/** Add or subtract a constant from all event velocities.
    Use SelectionPropertyCommand if you want to do something more
    creative. */
class ChangeVelocityCommand : public BasicSelectionCommand
{
public:
    ChangeVelocityCommand(int delta, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(delta), selection, true),
	m_selection(&selection), m_delta(delta) { }

    static QString getGlobalName(int delta = 0) {
	if (delta > 0) return "&Increase Velocity";
	else return "&Reduce Velocity";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_delta;
};


#endif
