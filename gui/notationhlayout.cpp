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

#include "notationhlayout.h"
#include "notationstaff.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"
#include "Quantizer.h"
#include "Composition.h"
#include "SegmentNotationHelper.h"

#include <cmath> // for fabs()

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Text;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using namespace Rosegarden::BaseProperties;
using namespace NotationProperties;

std::vector<double> NotationHLayout::m_availableSpacings;


NotationHLayout::NotationHLayout(Composition *c, NotePixmapFactory *npf) :
    Rosegarden::HorizontalLayoutEngine<NotationElement>(c),
    m_totalWidth(0.),
    m_pageMode(false),
    m_pageWidth(0.),
    m_spacing(1.0),
    m_npf(npf)
{
//    kdDebug(KDEBUG_AREA) << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

std::vector<double>
NotationHLayout::getAvailableSpacings()
{
    if (m_availableSpacings.size() == 0) {
        m_availableSpacings.push_back(0.3);
        m_availableSpacings.push_back(0.6);
        m_availableSpacings.push_back(0.85);
        m_availableSpacings.push_back(1.0);
        m_availableSpacings.push_back(1.3);
        m_availableSpacings.push_back(1.7);
        m_availableSpacings.push_back(2.2);
    }
    return m_availableSpacings;
}

const Quantizer *
NotationHLayout::getQuantizer() const
{
    return getComposition()->getLegatoQuantizer();
}

NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
	m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}


const NotationHLayout::BarDataList &
NotationHLayout::getBarData(StaffType &staff) const
{
    return ((NotationHLayout *)this)->getBarData(staff);
}


// To find the "ideal" width of a bar, we need the sum of the minimum
// widths of the elements in the bar, plus as much extra space as
// would be needed if the bar was made up entirely of repetitions of
// its shortest note.  This space is the product of the number of the
// bar's shortest notes that will fit in the duration of the bar and
// the comfortable gap for each.

// Some ground rules for "ideal" layout:
// 
// -- The shortest notes in the bar need to have a certain amount of
//    space, but if they're _very_ short compared to the total bar
//    duration then we can probably afford to squash them somewhat
// 
// -- If there are lots of the shortest note duration, then we
//    should try not to squash them quite so much
// 
// -- If there are not very many notes in the bar altogether, we can
//    squash things up a bit more perhaps
// 
// -- Similarly if they're dotted, we need space for the dots; we
//    can't risk making the dots invisible
// 
// -- In theory we don't necessarily want the whole bar width to be
//    the product of the shortest-note width and the number of shortest
//    notes in the bar.  But it's difficult to plan the spacing
//    otherwise.  One possibility is to augment the fixedWidth with a
//    certain proportion of the width of each note, and make that a
//    higher proportion for very short notes than for long notes.

//!!! The algorithm below does not implement most of these rules; it
// can probably be improved dramatically without too much work

double NotationHLayout::getIdealBarWidth(StaffType &staff,
					 int fixedWidth,
					 int baseWidth,
					 NotationElementList::iterator shortest,
					 int shortCount,
					 int /*totalCount*/,
					 const TimeSignature &timeSignature)
    const
{
    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: shortCount is "
                         << shortCount << ", fixedWidth is "
                         << fixedWidth << ", barDuration is "
                         << timeSignature.getBarDuration() << endl;

    if (shortest == staff.getViewElementList()->end()) {
        kdDebug(KDEBUG_AREA) << "First trivial return" << endl;
        return fixedWidth;
    }

    int d = 
	SegmentNotationHelper(staff.getSegment()).getNotationDuration
	((*shortest)->event());

    if (d == 0) {
        kdDebug(KDEBUG_AREA) << "Second trivial return" << endl;
        return fixedWidth;
    }

    int smin = getMinWidth(**shortest);
    if (!(*shortest)->event()->get<Int>(NOTE_DOTS)) {
        smin += m_npf->getDotWidth()/2;
    }

    /* if there aren't many of the shortest notes, we don't want to
       allow so much space to accommodate them */
    if (shortCount < 3) smin -= 3 - shortCount;

    int gapPer = 
        getComfortableGap((*shortest)->event()->get<Int>(NOTE_TYPE)) +
        smin;

    kdDebug(KDEBUG_AREA) << "d is " << d << ", gapPer is " << gapPer << endl;

    double w = fixedWidth + ((timeSignature.getBarDuration() * gapPer) / d);

    kdDebug(KDEBUG_AREA) << "NotationHLayout::getIdealBarWidth: returning "
                         << w << endl;

    w *= m_spacing;
    if (w < (fixedWidth + baseWidth)) w = (double)(fixedWidth + baseWidth);
    return w;
} 

NotationElementList::iterator
NotationHLayout::getStartOfQuantizedSlice(const NotationElementList *notes,
					  timeT t)
    const
{
    NotationElementList::iterator i = notes->findTime(t);
    NotationElementList::iterator j(i);

    while (true) {
	if (i == notes->begin()) return i;
	--j;

	timeT absTime;
	if ((*j)->event()->has(BEAMED_GROUP_TUPLET_BASE)) {
	    absTime = (*j)->getAbsoluteTime();
	} else {
	    absTime = getQuantizer()->getQuantizedAbsoluteTime((*j)->event());
	}

	if (absTime < t) return i;
	i = j;
    }
}


void
NotationHLayout::scanStaff(StaffType &staff, timeT startTime, timeT endTime)
{
    START_TIMING;

    Segment &segment(staff.getSegment());
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    bool isFullScan = (startTime == endTime);
    bool allDone = false; // used in partial scans

    if (isFullScan) {
	clearBarList(staff);
	startTime = segment.getStartTime();
	endTime = segment.getEndTime();
    }

    int barNo = getComposition()->getBarNumber(segment.getStartTime());
    int endBarNo = getComposition()->getBarNumber(segment.getEndTime());
    if (endBarNo > barNo &&
	getComposition()->getBarStart(endBarNo) == segment.getEndTime()) {
	--endBarNo;
    }

    kdDebug(KDEBUG_AREA) << "NotationHLayout::scanStaff: full scan " << isFullScan << ", times " << startTime << "->" << endTime << ", bars " << barNo << "->" << endBarNo << endl;

    SegmentNotationHelper nh(segment);
    nh.quantize();

    PRINT_ELAPSED("NotationHLayout::scanStaff: after quantize");

    // need to invalidate any stray iterators in bar data records
    // that are no longer in the range of this staff

    if (!isFullScan) {
	for (int barScooter = 0;
	     barScooter < barNo && barScooter < barList.size();
	     ++barScooter) {
	    barList[barScooter].basicData.start = notes->end();
	    barList[barScooter].basicData.fake = true;
	}
    }

    m_fakeBarCountMap[&staff] = barNo;

    while (barNo <= endBarNo) {

	std::pair<timeT, timeT> barTimes =
	    getComposition()->getBarRange(barNo);

        NotationElementList::iterator from = 
	    getStartOfQuantizedSlice(notes, barTimes.first);

        NotationElementList::iterator to =
	    getStartOfQuantizedSlice(notes, barTimes.second);

        // fixedWidth includes clefs, keys &c, but also accidentals
	int fixedWidth = getBarMargin();

        // baseWidth is absolute minimum width of non-fixedWidth elements
        int baseWidth = 0;

	Event *timeSigEvent = 0;
	bool newTimeSig = false;
	timeSignature = getComposition()->getTimeSignatureInBar
	    (barNo, newTimeSig);

	if (newTimeSig && !timeSignature.isHidden()) {
	    timeSigEvent = timeSignature.getAsEvent(barTimes.first);
	    fixedWidth += getFixedItemSpacing() * 2 +
		m_npf->getTimeSigWidth(timeSignature);
	}

	setBarBasicData(staff, barNo, from, true, false, timeSigEvent);

	if (barTimes.second >= startTime) {
	    barList[barNo].layoutData.needsLayout = true;
	}

	if (!isFullScan && allDone) {
	    ++barNo;
	    continue;
	}

	kdDebug(KDEBUG_AREA) << "NotationHLayout::scanStaff: bar " << barNo << ", from " << barTimes.first << ", to " << barTimes.second << " (end " << segment.getEndTime() << "); from is at " << (from == notes->end() ? -1 : (*from)->getAbsoluteTime()) << ", to is at " << (to == notes->end() ? -1 : (*to)->getAbsoluteTime()) << endl;

        NotationElementList::iterator shortest = notes->end();
        int shortCount = 0;
        int totalCount = 0;

	// for partial scans where this bar turns out to precede the scan start
	bool leaveSizesAlone = false;

        timeT apparentBarDuration = 0;
	timeT actualBarEnd = barTimes.first;

	AccidentalTable accTable(key, clef);

        for (NotationElementList::iterator itr = from; itr != to; ++itr) {
        
            NotationElement *el = (*itr);
            int mw = getMinWidth(*el);

            if (el->event()->isa(Clef::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found clef" << endl;

                fixedWidth += mw;
                clef = Clef(*el->event());

                // Probably not strictly the right thing to do
                // here, but I hope it'll do well enough in practice
                accTable = AccidentalTable(key, clef);

            } else if (el->event()->isa(Key::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found key" << endl;

                fixedWidth += mw;
                key = Key(*el->event());

                accTable = AccidentalTable(key, clef);
		
            } else if (!isFullScan &&
		       (barTimes.second < startTime ||
			barTimes.first > endTime)) {

		// partial scans don't need to look at all notes, just clefs
		// and key changes

                // !!! shouldn't have to do all this to find clef and key
                // (and I don't bother to do this in layout() -- eek)
		leaveSizesAlone = true;
                if (barTimes.first > endTime) allDone = true;
                continue;

	    } else if (el->isNote()) {

		++totalCount;
		
		apparentBarDuration +=
		    scanChord(notes, itr, clef, key, accTable,
			      fixedWidth, baseWidth, shortest, shortCount, to);

	    } else if (el->isRest()) {

		++totalCount;

		apparentBarDuration +=
		    scanRest(notes, itr,
			     fixedWidth, baseWidth, shortest, shortCount);

	    } else if (el->event()->isa(Indication::EventType)) {

//		kdDebug(KDEBUG_AREA) << "Found indication" << endl;

		mw = 0;

	    } else {
		
		kdDebug(KDEBUG_AREA) << "Found something I don't know about" << endl;
		fixedWidth += mw;
	    }

	    actualBarEnd = el->getAbsoluteTime() + el->getDuration();
            el->event()->setMaybe<Int>(MIN_WIDTH, mw);
	}

	if (actualBarEnd == barTimes.first) actualBarEnd = barTimes.second;

	if (!leaveSizesAlone) {
	    double idealWidth = 
		getIdealBarWidth(staff, fixedWidth, baseWidth, shortest,
				 shortCount, totalCount, timeSignature);
	    kdDebug(KDEBUG_AREA) << "Ideal bar width: " << idealWidth << endl;
	
	    setBarSizeData(staff, barNo, idealWidth, fixedWidth, baseWidth,
			   actualBarEnd - barTimes.first);
	}

	++barNo;
    }

    while (barList.size() > endBarNo + 1) {
	BarDataList::iterator bdli(barList.end());
	barList.erase(--bdli);
    }

    PRINT_ELAPSED("NotationHLayout::scanStaff");
}

void
NotationHLayout::clearBarList(StaffType &staff)
{
    BarDataList &bdl = m_barData[&staff];
    bdl.clear();
}
    

void
NotationHLayout::setBarBasicData(StaffType &staff,
				 int barNo,
				 NotationElementList::iterator start,
				 bool correct,
				 bool fake,
				 Rosegarden::Event *timeSig)
{
    kdDebug(KDEBUG_AREA) << "setBarBasicData for " << barNo << endl;

    BarDataList &bdl(m_barData[&staff]);

    while (barNo >= bdl.size()) {
	bdl.push_back(BarData(staff.getViewElementList()->end(), true, true, 0));
    }

    if (bdl[barNo].basicData.timeSignature) {
	delete bdl[barNo].basicData.timeSignature;
    }

    bdl[barNo].basicData.start = start;
    bdl[barNo].basicData.correct = correct;
    bdl[barNo].basicData.fake = fake;
    bdl[barNo].basicData.timeSignature = timeSig;
}

void
NotationHLayout::setBarSizeData(StaffType &staff,
				int barNo,
				double width,
				int fixedWidth,
				int baseWidth,
				Rosegarden::timeT actualDuration)
{
    kdDebug(KDEBUG_AREA) << "setBarSizeData for " << barNo << endl;

    BarDataList &bdl(m_barData[&staff]);

    while (barNo >= bdl.size()) {
	bdl.push_back(BarData(staff.getViewElementList()->end(), true, true, 0));
    }

    bdl[barNo].sizeData.idealWidth = width;
    bdl[barNo].sizeData.fixedWidth = fixedWidth;
    bdl[barNo].sizeData.baseWidth = baseWidth;
    bdl[barNo].sizeData.actualDuration = actualDuration;
}

 
timeT
NotationHLayout::scanChord(NotationElementList *notes,
			   NotationElementList::iterator &itr,
			   const Rosegarden::Clef &clef,
			   const Rosegarden::Key &key,
			   AccidentalTable &accTable,
			   int &fixedWidth, int &baseWidth,
			   NotationElementList::iterator &shortest,
			   int &shortCount,
			   NotationElementList::iterator &to)
{
    Chord chord(*notes, itr, getQuantizer());
    AccidentalTable newAccTable(accTable);
    Accidental someAccidental = NoAccidental;
    bool barEndsInChord = false;

    for (unsigned int i = 0; i < chord.size(); ++i) {
	
	NotationElement *el = *chord[i];
	
	long pitch = 64;
	if (!el->event()->get<Int>(PITCH, pitch)) {
	    kdDebug(KDEBUG_AREA) <<
		"WARNING: NotationHLayout::scanChord: couldn't get pitch for element, using default pitch of " << pitch << endl;
	}

	Accidental explicitAccidental = NoAccidental;
	(void)el->event()->get<String>(ACCIDENTAL, explicitAccidental);
	
	Rosegarden::NotationDisplayPitch p
	    (pitch, clef, key, explicitAccidental);
	int h = p.getHeightOnStaff();
	Accidental acc = p.getAccidental();

	el->event()->setMaybe<Int>(HEIGHT_ON_STAFF, h);
	el->event()->setMaybe<String>(CALCULATED_ACCIDENTAL, acc);

	// update display acc for note according to the accTable
	// (accidentals in force when the last chord ended) and update
	// newAccTable with accidentals from this note.  (We don't
	// update accTable yet because there may be other notes in
	// this chord that need accTable to be the same as it is for
	// this one)
                    
	Accidental dacc = accTable.getDisplayAccidental(acc, h);
	el->event()->setMaybe<String>(DISPLAY_ACCIDENTAL, dacc);
	if (someAccidental == NoAccidental) someAccidental = dacc;

	newAccTable.update(acc, h);
	if (chord[i] == to) barEndsInChord = true;
    }

    accTable.copyFrom(newAccTable);

    if (someAccidental != NoAccidental) {
	fixedWidth += m_npf->getAccidentalWidth(someAccidental);
    }

    if (chord.hasNoteHeadShifted()) {
	fixedWidth += m_npf->getNoteBodyWidth();
    }

    NotationElementList::iterator myShortest = chord.getShortestElement();

    timeT d = getQuantizer()->getQuantizedDuration((*myShortest)->event());
    baseWidth += getMinWidth(**myShortest);

    timeT sd = 0;
    if (shortest != notes->end()) {
	sd = getQuantizer()->getQuantizedDuration((*shortest)->event());
    }

    if (d > 0 && (sd == 0 || d <= sd)) {
	if (d == sd) {
	    ++shortCount;
	} else {
	    shortest = myShortest;
	    shortCount = 1;
	}
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
    return d;
}

timeT
NotationHLayout::scanRest
(NotationElementList *notes, NotationElementList::iterator &itr,
 int &, int &baseWidth,
 NotationElementList::iterator &shortest, int &shortCount)
{
    timeT d = getQuantizer()->getQuantizedDuration((*itr)->event());
    baseWidth += getMinWidth(**itr);

    timeT sd = 0;
    if (shortest != notes->end()) {
	sd = getQuantizer()->getQuantizedDuration((*shortest)->event());
    }

    if (d > 0 && (sd == 0 || d <= sd)) {

	// assumption: rests are wider than notes
	if (sd == 0 || d < sd || (*shortest)->isNote()) {
	    shortest = itr;
	    if (d != sd) shortCount = 1;
	    else ++shortCount;
	} else {
	    ++shortCount;
	}
    }

    return d;
}

NotationHLayout::StaffType *
NotationHLayout::getStaffWithWidestBar(int barNo)
{
    double maxWidth = -1;
    StaffType *widest = 0;
    BarDataMap::iterator i;

    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	BarDataList &list = i->second;

	if ((int)list.size() > barNo && list[barNo].basicData.fake == false) {
	    if (list[barNo].sizeData.idealWidth > maxWidth) {
		maxWidth = list[barNo].sizeData.idealWidth;
		widest = i->first;
	    }
	}
    }

    return widest;
}


void
NotationHLayout::reconcileBarsLinear()
{
    START_TIMING;

    BarDataMap::iterator i;

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff.

    int barNo = 0;
    bool aWidthChanged = false;

    for (;;) {

	StaffType *widest = getStaffWithWidestBar(barNo);

	if (!widest) break; // reached end of piece
	double maxWidth = m_barData[widest][barNo].sizeData.idealWidth;
	if (m_pageWidth > 0.1 && maxWidth > m_pageWidth) {
	    maxWidth = m_pageWidth;
	}

	// Now apply width to this bar on all staffs

	for (i = m_barData.begin(); i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;

	    if (list.size() > barNo) {

		BarData::SizeData &bd(list[barNo].sizeData);

		if (bd.idealWidth != maxWidth) {
		    if (bd.idealWidth > 0.0) {
			double ratio = maxWidth / bd.idealWidth;
			bd.fixedWidth +=
			    bd.fixedWidth * (int)((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
                    aWidthChanged = true;
		}

                if (aWidthChanged) list[barNo].layoutData.needsLayout = true;
	    }
	}

	++barNo;
    }

    PRINT_ELAPSED("NotationHLayout::reconcileBarsLinear");
}	


void
NotationHLayout::reconcileBarsPage()
{
    START_TIMING;

    unsigned int barNo = 0;
    unsigned int barNoThisRow = 0;
    
    // pair of the recommended number of bars with those bars'
    // original total width, for each row
    std::vector<std::pair<int, double> > rowData;

    double pageWidthSoFar = 0.0;
    double stretchFactor = 10.0;

    BarDataMap::iterator i;

    for (;;) {
	
	StaffType *widest = getStaffWithWidestBar(barNo);
	if (!widest) break; // reached end of piece
	double maxWidth = m_barData[widest][barNo].sizeData.idealWidth;

	// Work on the assumption that this bar is the last in the
	// row.  How would that make things look?

	double nextPageWidth = pageWidthSoFar + maxWidth;
	double nextStretchFactor = m_pageWidth / nextPageWidth;

	// We have to have at least one bar per row
	
	bool tooFar = false;

	if (barNoThisRow >= 1) {

	    // If this stretch factor is "worse" than the previous
	    // one, we've come too far and have too many bars

	    if (fabs(1.0 - nextStretchFactor) > fabs(1.0 - stretchFactor)) {
		tooFar = true;
	    }

	    // If the next stretch factor is less than 1 and would
	    // make this bar on any of the staffs narrower than it can
	    // afford to be, then we've got too many bars

	    if (!tooFar && (nextStretchFactor < 1.0)) {

		for (i = m_barData.begin(); i != m_barData.end(); ++i) {

		    BarDataList &list = i->second;

		    if (list.size() > signed(barNo)) {
			BarData::SizeData &bd(list[barNo].sizeData);
			if ((nextStretchFactor * bd.idealWidth) <
			    (double)(bd.fixedWidth + bd.baseWidth)) {
			    tooFar = true;
			    break;
			}
		    }
		}
	    }
	}

	if (tooFar) {
	    rowData.push_back(std::pair<int, double>(barNoThisRow,
						     pageWidthSoFar));
	    barNoThisRow = 1;
	    pageWidthSoFar = maxWidth;
	    stretchFactor = m_pageWidth / maxWidth;
	} else {
	    ++barNoThisRow;
	    pageWidthSoFar = nextPageWidth;
	    stretchFactor = nextStretchFactor;
	}

	++barNo;
    }

    if (barNoThisRow > 0) {
	rowData.push_back(std::pair<int, double>(barNoThisRow,
						 pageWidthSoFar));
    }

    // Now we need to actually apply the widths

    barNo = 0;

    for (unsigned int row = 0; row < rowData.size(); ++row) {

	barNoThisRow = barNo;
	unsigned int finalBarThisRow = barNo + rowData[row].first - 1;

	pageWidthSoFar = 0;
	stretchFactor = m_pageWidth / rowData[row].second;

	for (; barNoThisRow <= finalBarThisRow; ++barNoThisRow, ++barNo) {

	    bool finalRow = (row == rowData.size()-1);

	    StaffType *widest = getStaffWithWidestBar(barNo);
	    if (!widest) break; // reached end of piece (shouldn't happen)
	    if (finalRow && (stretchFactor > 1.0)) stretchFactor = 1.0;
	    double maxWidth = 
		(stretchFactor * m_barData[widest][barNo].sizeData.idealWidth);

	    if (barNoThisRow == finalBarThisRow) {
		if (!finalRow ||
		    (maxWidth > (m_pageWidth - pageWidthSoFar))) {
		    maxWidth = m_pageWidth - pageWidthSoFar;
		}
	    }
	    
	    for (i = m_barData.begin(); i != m_barData.end(); ++i) {

		BarDataList &list = i->second;

		if (list.size() > signed(barNo)) {

		    BarData::SizeData &bd(list[barNo].sizeData);
		    list[barNo].layoutData.needsLayout = true;

		    if (bd.idealWidth > 0) {
			double ratio = maxWidth / bd.idealWidth;
			bd.fixedWidth +=
			    bd.fixedWidth * (int)((ratio - 1.0)/2.0);
		    }
		    bd.idealWidth = maxWidth;
		}
	    }

	    pageWidthSoFar += maxWidth;
	}
    }

    PRINT_ELAPSED("NotationHLayout::reconcileBarsPage");
}


// and for once I swear things will still be good tomorrow

NotationHLayout::AccidentalTable::AccidentalTable(Key key, Clef clef) :
    m_key(key), m_clef(clef)
{
    std::vector<int> heights(key.getAccidentalHeights(clef));
    unsigned int i;

    for (i = 0; i < 7; ++i) m_accidentals[i] = NoAccidental;
    for (i = 0; i < heights.size(); ++i) {
        m_accidentals[Key::canonicalHeight(heights[i])] =
            (key.isSharp() ? Sharp : Flat);
    }
}

NotationHLayout::AccidentalTable::AccidentalTable(const AccidentalTable &t) :
    m_key(t.m_key), m_clef(t.m_clef)
{
    copyFrom(t);
}

NotationHLayout::AccidentalTable &
NotationHLayout::AccidentalTable::operator=(const AccidentalTable &t)
{
    if (&t != this) {
	m_key = t.m_key;
	m_clef = t.m_clef;
	copyFrom(t);
    }
    return *this;
}

Accidental
NotationHLayout::AccidentalTable::getDisplayAccidental(Accidental accidental,
                                                       int height) const
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "accidental = " << accidental << ", stored accidental at height " << height << " is " << (*this)[height] << endl;

    if (m_accidentals[height] != NoAccidental) {

        if (accidental == m_accidentals[height]) {
            return NoAccidental;
        } else if (accidental == NoAccidental || accidental == Natural) {
            return Natural;
        } else {
            //!!! aargh.  What we really want to do now is have two
            //accidentals shown: first a natural, then the one
            //required for the note.  But there's no scope for that in
            //our accidental structure (RG2.1 is superior here)
            return accidental;
        }
    } else {
        return accidental;
    }
}

void
NotationHLayout::AccidentalTable::update(Accidental accidental, int height)
{
    height = Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    kdDebug(KDEBUG_AREA) << "updating height" << height << " from " << (*this)[height] << " to " << accidental << endl;


    //!!! again, we can't properly deal with the difficult case where
    //we already have an accidental at height but it's not the same
    //accidental

    m_accidentals[height] = accidental;
}

void
NotationHLayout::AccidentalTable::copyFrom(const AccidentalTable &t)
{
    for (int i = 0; i < 7; ++i) {
	m_accidentals[i] = t.m_accidentals[i];
    }
}

void
NotationHLayout::finishLayout(timeT startTime, timeT endTime)
{
    if (m_pageMode && (m_pageWidth > 0.1)) reconcileBarsPage();
    else reconcileBarsLinear();
      
    for (BarDataMap::iterator i(m_barData.begin()); i != m_barData.end(); ++i) {
	layout(i, startTime, endTime);
    }
}

void
NotationHLayout::layout(BarDataMap::iterator i, timeT startTime, timeT endTime)
{
    START_TIMING;

    StaffType &staff = *(i->first);
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    Key key;
    Clef clef;
    TimeSignature timeSignature;

    bool isFullLayout = (startTime == endTime);

    // these two are for partial layouts:
    bool haveSimpleOffset = false;
    double simpleOffset = 0;

    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout: full layout " << isFullLayout << ", times " << startTime << "->" << endTime << endl;

    double x = 0, barX = 0;
    TieMap tieMap;

    int barNo = 0;

    //!!! use m_fakeBarCountMap to decide where to start?

    for (BarDataList::iterator bdi = barList.begin();
         bdi != barList.end(); ++bdi) {

        NotationElementList::iterator from = bdi->basicData.start;
        NotationElementList::iterator to;

        kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): starting barNo " << barNo++ << ", x = " << barX << ", width = " << bdi->sizeData.idealWidth << ", time = " << (from == notes->end() ? -1 : (*from)->getAbsoluteTime()) << endl;

        BarDataList::iterator nbdi(bdi);
        if (++nbdi == barList.end()) {
            to = notes->end();
        } else {
            to = nbdi->basicData.start;
        }

        if (from == notes->end()) {
            kdDebug(KDEBUG_AREA) << "Start is end" << endl;
        }
        if (from == to) {
            kdDebug(KDEBUG_AREA) << "Start is to" << endl;
        }

        if ((!isFullLayout) && !bdi->basicData.fake &&
	    (from == notes->end() ||
	     (*from)->event()->getAbsoluteTime() > endTime)) {

            // Find how far to move everything if necessary
            if (!haveSimpleOffset) {
                simpleOffset = barX + getPreBarMargin() - bdi->layoutData.x;
                haveSimpleOffset = true;
            }

            // Move all elements

            bdi->layoutData.x += simpleOffset;
	    
            if (bdi->basicData.timeSignature)
                bdi->layoutData.timeSigX += (int) simpleOffset;

            for (NotationElementList::iterator it = from;
		 it != notes->end() && it != to; ++it) {
                (*it)->setLayoutX((*it)->getLayoutX() + simpleOffset);
		double airX, airWidth;
		(*it)->getLayoutAirspace(airX, airWidth);
		(*it)->setLayoutAirspace(airX + simpleOffset, airWidth);
            }

            // And skip the real layout work
            bdi->layoutData.needsLayout = false;
            continue;
        }

	x = barX;
	x += getPreBarMargin();
        bdi->layoutData.x = x;
        x += getPostBarMargin();
	barX += bdi->sizeData.idealWidth;

	bool timeSigToPlace = false;
	if (bdi->basicData.timeSignature) {
	    timeSignature = TimeSignature(*(bdi->basicData.timeSignature));
	    timeSigToPlace = true;
	}

	//!!! Need to make sure we cope if every staff has lots of
	// fake bars (i.e. we're not displaying any segments that
	// are anywhere near the start of the composition)

        if (bdi->basicData.fake) {
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): fake bar " << endl;
            continue;
        }
        if (!bdi->layoutData.needsLayout) {
            //!!! clef and key may not be right
            // need a better way to find them than keeping track through the
            // whole staff
            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): bar " << " has needsLayout false" << endl;
            continue;
        }

        if (timeSigToPlace) {
	    kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): there's a time sig in this bar" << endl;
	}


        for (NotationElementList::iterator it = from; it != to; ++it) {
            
            NotationElement *el = (*it);
            el->setLayoutX(x);
//            kdDebug(KDEBUG_AREA) << "NotationHLayout::layout(): setting element's x to " << x << endl;
	    long delta;

	    if (timeSigToPlace && !el->event()->isa(Clef::EventType)) {
//		kdDebug(KDEBUG_AREA) << "Placing timesig at " << x << endl;
		bdi->layoutData.timeSigX = (int)x;
		x += getFixedItemSpacing()*2 +
		    m_npf->getTimeSigWidth(timeSignature);
//		kdDebug(KDEBUG_AREA) << "and moving next elt to " << x << endl;
		el->setLayoutX(x);
		timeSigToPlace = false;
	    }

	    if (el->isNote()) {

		// This modifies "it" and "tieMap"
		delta = positionChord
		    (staff, it, bdi, timeSignature, clef, key, tieMap, to);

	    } else if (el->isRest()) {

		delta = positionRest(staff, it, bdi, timeSignature);

	    } else if (el->event()->isa(Clef::EventType)) {
		
		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
//		kdDebug(KDEBUG_AREA) << "Found clef" << endl;
		clef = Clef(*el->event());

	    } else if (el->event()->isa(Key::EventType)) {

		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
//		kdDebug(KDEBUG_AREA) << "Found key" << endl;
		key = Key(*el->event());

	    } else {
		delta = el->event()->get<Int>(MIN_WIDTH);
		el->setLayoutAirspace(x, delta);
	    }

	    if (it != to && (*it)->event()->has(BEAMED_GROUP_ID)) {

		NotationElementList::iterator scooter(it);
		++scooter;

		long groupId = (*it)->event()->get<Int>(BEAMED_GROUP_ID);
		long nextGroupId = groupId;

		if (scooter == to ||
		    !(*scooter)->event()->get<Int>(BEAMED_GROUP_ID, nextGroupId) ||
		    (groupId != nextGroupId)) {

		    kdDebug(KDEBUG_AREA) << "Moving from group " << groupId << " to group " << nextGroupId << " after event at " << (*it)->getAbsoluteTime() << endl;
		    
		    NotationStaff &notationStaff =
			dynamic_cast<NotationStaff &>(staff);
		    NotationGroup group(*staff.getViewElementList(), it,
					getQuantizer(), clef, key);
		    group.applyBeam(notationStaff);
		    group.applyTuplingLine(notationStaff);
		}
	    }

            x += delta;
        }

        bdi->layoutData.needsLayout = false;
    }

    if (!isFullLayout && haveSimpleOffset) m_totalWidth += simpleOffset;
    else if (x > m_totalWidth) m_totalWidth = x;

    PRINT_ELAPSED("NotationHLayout::layout");
}


long
NotationHLayout::positionRest(StaffType &staff,
                              const NotationElementList::iterator &itr,
                              const BarDataList::iterator &bdi,
                              const TimeSignature &timeSignature)
{
    NotationElement *rest = *itr;

    // To work out how much space to allot a rest, as for a note,
    // start with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // (We use the actual duration of the bar, not the nominal time-
    // signature duration.)

    timeT barDuration = bdi->sizeData.actualDuration;
    if (barDuration == 0) barDuration = timeSignature.getBarDuration();

    long delta = (((int)bdi->sizeData.idealWidth - bdi->sizeData.fixedWidth) *
		  getSpacingDuration(staff, itr)) /
	barDuration;
    rest->setLayoutAirspace(rest->getLayoutX(), delta);

    // Situate the rest somewhat further into its allotted space.  Not
    // convinced this is the right thing to do

    int baseWidth = m_npf->getRestWidth
	(Note(rest->event()->get<Int>(NOTE_TYPE),
	      rest->event()->get<Int>(NOTE_DOTS)));

    if (delta > 2 * baseWidth) {
        int shift = (delta - 2 * baseWidth) / 4;
        shift = std::min(shift, (baseWidth * 3));
        rest->setLayoutX(rest->getLayoutX() + shift);
    }
 
    return delta;
}


timeT
NotationHLayout::getSpacingDuration(StaffType &staff,
				    const NotationElementList::iterator &i)
{
    SegmentNotationHelper helper(staff.getSegment());
    timeT t(helper.getNotationAbsoluteTime((*i)->event()));
    timeT d(helper.getNotationDuration((*i)->event()));

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e &&
	   helper.getNotationAbsoluteTime((*j)->event()) == t) {
	++j;
    }
    if (j == e) return d;
    else return (*j)->getAbsoluteTime() - (*i)->getAbsoluteTime();
}


long
NotationHLayout::positionChord(StaffType &staff,
                               NotationElementList::iterator &itr,
			       const BarDataList::iterator &bdi,
			       const TimeSignature &timeSignature,
			       const Clef &clef, const Key &key,
			       TieMap &tieMap,
			       NotationElementList::iterator &to)
{
    Chord chord(*staff.getViewElementList(), itr, getQuantizer(), clef, key);
    double baseX = (*itr)->getLayoutX();

    // To work out how much space to allot a note (or chord), start
    // with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // (We use the actual duration of the bar, not the nominal time-
    // signature duration.)

    // In case this chord has various durations in it, we choose an
    // effective duration based on the absolute time of the first
    // following event not in the chord (see getSpacingDuration)

    timeT barDuration = bdi->sizeData.actualDuration;
    if (barDuration == 0) barDuration = timeSignature.getBarDuration();

    long delta = (((int)bdi->sizeData.idealWidth - bdi->sizeData.fixedWidth) *
		  getSpacingDuration(staff, itr)) /
	barDuration;

    int noteWidth = m_npf->getNoteBodyWidth();

    // If the chord's allowed a lot of space, situate it somewhat
    // further into its allotted space.  Not convinced this is always
    // the right thing to do.

    double unmodifiedBaseX = baseX;
    if (delta > 2 * noteWidth) {
        int shift = (delta - 2 * noteWidth) / 5;
	baseX += std::min(shift, (m_npf->getNoteBodyWidth() * 3 / 2));
    }

    // Find out whether the chord contains any accidentals, and if so,
    // make space, and also shift the notes' positions right somewhat.
    // (notepixmapfactory quite reasonably places the hot spot at the
    // start of the note head, not at the start of the whole pixmap.)
    // Also use this loop to check for beamed-group information.

    unsigned int i;
    int accWidth = 0;
    long groupId = -1;

    for (i = 0; i < chord.size(); ++i) {

	(*chord[i])->setLayoutAirspace(unmodifiedBaseX, delta);

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	Accidental acc = NoAccidental;
	if (note->event()->get<String>(DISPLAY_ACCIDENTAL, acc) &&
	    acc != NoAccidental) {
            accWidth = std::max(accWidth, m_npf->getAccidentalWidth(acc));
	}

	if (groupId != -2) {
	    long myGroupId = -1;
	    if (note->event()->get<Int>(BEAMED_GROUP_ID, myGroupId) &&
		(groupId == -1 || myGroupId == groupId)) {
		groupId = myGroupId;
	    } else {
		groupId = -2; // not all note-heads think they're in the group
	    }
	}
    }

    baseX += accWidth;
    delta += accWidth;

    // Cope with the presence of shifted note-heads

    bool shifted = chord.hasNoteHeadShifted();

    if (shifted) {
	if (delta < noteWidth * 2) delta = noteWidth * 2;
	if (!chord.hasStemUp()) baseX += noteWidth;
    }

    // Check for any ties going back, and if so work out how long it
    // must have been and assign accordingly.

    for (i = 0; i < chord.size(); ++i) {

	NotationElement *note = *(chord[i]);
	if (!note->isNote()) continue;

	bool tiedForwards = false;
	bool tiedBack = false;

	note->event()->get<Bool>(TIED_FORWARD,  tiedForwards);
	note->event()->get<Bool>(TIED_BACKWARD, tiedBack);

	int pitch = note->event()->get<Int>(PITCH);
	if (tiedForwards) {
	    note->event()->setMaybe<Int>(TIE_LENGTH, 0);
	    tieMap[pitch] = chord[i];
	} else {
	    note->event()->unset(TIE_LENGTH);
	}

	if (tiedBack) {
	    TieMap::iterator ti(tieMap.find(pitch));

	    if (ti != tieMap.end()) {
		NotationElementList::iterator otherItr(ti->second);
		
		if ((*otherItr)->getAbsoluteTime() +
		    (*otherItr)->getDuration() ==
		    note->getAbsoluteTime()) {
		    
		    (*otherItr)->event()->setMaybe<Int>
			(TIE_LENGTH,
			 (int)(baseX - (*otherItr)->getLayoutX()));
		    
		} else {
		    tieMap.erase(pitch);
		}
	    }
	}
    }

    // Now set the positions of all the notes in the chord.  We don't
    // need to shift the positions of shifted notes, because that will
    // be taken into account when making their pixmaps later (in
    // NotationStaff::makeNoteSprite / NotePixmapFactory::makeNotePixmap).

    bool barEndsInChord = false;
    for (i = 0; i < chord.size(); ++i) {
	NotationElementList::iterator subItr = chord[i];
	if (subItr == to) barEndsInChord = true;
	(*subItr)->setLayoutX(baseX);
	if (groupId < 0) (*chord[i])->event()->unset(BEAMED);
	else (*chord[i])->event()->set<Int>(BEAMED_GROUP_ID, groupId);
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
    if (groupId < 0) return delta;

    return delta;
}


int NotationHLayout::getMinWidth(NotationElement &e) const
{
    int w = 0;

    if (e.isNote()) {

        long noteType = e.event()->get<Int>(NOTE_TYPE, noteType);

        w += m_npf->getNoteBodyWidth(noteType);

        long dots;
        if (e.event()->get<Int>(NOTE_DOTS, dots)) {
            w += m_npf->getDotWidth() * dots;
        }

        return w;

    } else if (e.isRest()) {

        w += m_npf->getRestWidth(Note(e.event()->get<Int>(NOTE_TYPE),
                                     e.event()->get<Int>(NOTE_DOTS)));

        return w;
    }

    w = getFixedItemSpacing();

    if (e.event()->isa(Clef::EventType)) {

        w += m_npf->getClefWidth(Clef(*e.event()));

    } else if (e.event()->isa(Key::EventType)) {

        w += m_npf->getKeyWidth(Key(*e.event()));

    } else if (e.event()->isa(Indication::EventType) ||
	       e.event()->isa(Text::EventType)) {

	w = 0;

    } else {
        kdDebug(KDEBUG_AREA) << "NotationHLayout::getMinWidth(): no case for event type " << e.event()->getType() << endl;
        w += 24;
    }

    return w;
}

int NotationHLayout::getComfortableGap(Note::Type type) const
{
    int bw = m_npf->getNoteBodyWidth();
    if (type < Note::Quaver) return 1;
    else if (type == Note::Quaver) return (bw / 2);
    else if (type == Note::Crotchet) return (bw * 3) / 2;
    else if (type == Note::Minim) return (bw * 3);
    else if (type == Note::Semibreve) return (bw * 9) / 2;
    else if (type == Note::Breve) return (bw * 7);
    return 1;
}        

int NotationHLayout::getBarMargin() const
{
    return (int)(m_npf->getBarMargin() * m_spacing);
}

int NotationHLayout::getPreBarMargin() const
{
    return (int)(m_npf->getBarMargin()) / 2;
}

int NotationHLayout::getPostBarMargin() const
{
    return getBarMargin() - getPreBarMargin();
}

void
NotationHLayout::reset()
{
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	clearBarList(*i->first);
    }

    m_barData.clear();
    m_totalWidth = 0;
}

void
NotationHLayout::resetStaff(StaffType &staff, timeT startTime, timeT endTime)
{
    if (startTime == endTime) {
        getBarData(staff).clear();
        m_totalWidth = 0;
    }
}

int
NotationHLayout::getFirstVisibleBar()
{
    //!!! use m_fakeBarCountMap

    int bar = -1;
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = getFirstVisibleBarOnStaff(*i->first);
	if (barHere >= 0) {
	    if (barHere < bar || bar < 0) bar = barHere;
	}
    }
    return bar;
}

int
NotationHLayout::getFirstVisibleBarOnStaff(StaffType &staff)
{
    //!!! use m_fakeBarCountMap

    BarDataList &bdl(getBarData(staff));
    for (int i = 0; i < bdl.size(); ++i) {
	if (!bdl[i].basicData.fake) return i;
    }
    return -1;
}

int
NotationHLayout::getLastVisibleBar()
{
    int bar = -1;
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = i->second.size()/* - 1*/; // last visible bar_line_
	if (barHere > bar) bar = barHere;
    }
    return bar;
}

int
NotationHLayout::getLastVisibleBarOnStaff(StaffType &staff)
{
    BarDataList &bdl(getBarData(staff));
    return bdl.size() /*- 1*/; // last visible bar_line_
}

double
NotationHLayout::getBarPosition(int bar)
{
    if (bar < 0) bar = 0;

    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	int barHere = i->second.size() - 1;
	if (barHere >= bar) return i->second[bar].layoutData.x;
	else if (barHere == bar-1 && barHere >= 0) {
	    return i->second[bar-1].layoutData.x +
		   i->second[bar-1].sizeData.idealWidth;
	}
    }

    int lvb = getLastVisibleBar();
    
    if (lvb < 0) return 0;
    return getBarPosition(lvb);
}

bool
NotationHLayout::isBarCorrectOnStaff(StaffType &staff, int i)
{
    BarDataList &bdl(getBarData(staff));
    ++i;
    if (i <= 0 || i >= bdl.size()) return true;
    else return getBarData(staff)[i].basicData.correct;
}

Event *NotationHLayout::getTimeSignaturePosition(StaffType &staff,
						 int i, double &timeSigX)
{
    BarDataList &bdl(getBarData(staff));
    if (i < 0) i = 0;
    if (i >= bdl.size()) i = bdl.size() - 1;
    timeSigX = (double)(bdl[i]).layoutData.timeSigX;
    return (bdl[i]).basicData.timeSignature;
}

