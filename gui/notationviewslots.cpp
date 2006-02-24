
// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#include <qlabel.h>
#include <qinputdialog.h>
#include <qvalidator.h>
#include <qtimer.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>
#include <klineeditdlg.h>
#include <kprinter.h>

#include <algorithm>

#include "notationview.h"

#include "NotationTypes.h"
#include "Selection.h"
#include "Segment.h"
#include "Clipboard.h"
#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"
#include "MidiTypes.h"

#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "dialogs.h"
#include "chordnameruler.h"
#include "temporuler.h"
#include "rawnoteruler.h"
#include "notationhlayout.h"
#include "notefont.h"
#include "eventfilter.h"
#include "qcanvassimplesprite.h"
#include "tempoview.h"
#include "notationstrings.h"

#include "ktmpstatusmsg.h"

#include <sys/time.h>

using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::EventSelection;


void
NotationView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
        if (isInTripletMode()) {
            message = i18n(" Triplet Chord");
        } else {
            message = i18n(" Chord");
        }
    } else {
        if (isInTripletMode()) {
            message = i18n(" Triplet");
        } else {
            message = "";
        }
    }
    m_insertModeLabel->setText(message);
}

void
NotationView::slotUpdateAnnotationsStatus()
{
    if (!areAnnotationsVisible()) {
        for (int i = 0; i < getStaffCount(); ++i) {
            Segment &s = getStaff(i)->getSegment();
            for (Segment::iterator j = s.begin(); j != s.end(); ++j) {
                if ((*j)->isa(Rosegarden::Text::EventType) &&
                    ((*j)->get<Rosegarden::String>
                     (Rosegarden::Text::TextTypePropertyName)
                     == Rosegarden::Text::Annotation)) {
                    m_annotationsLabel->setText(i18n("Hidden annotations"));
                    return;
                }
            }
        }
    }
    m_annotationsLabel->setText("");
    getToggleAction("show_annotations")->setChecked(areAnnotationsVisible());
}

void
NotationView::slotChangeSpacingFromStringValue(const QString& spacingT)
{
    // spacingT has a '%' at the end
    //
    int spacing = spacingT.left(spacingT.length() - 1).toInt();
    slotChangeSpacing(spacing);
}

void
NotationView::slotChangeSpacingFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(8) == "spacing_") {
        int spacing = name.right(name.length() - 8).toInt();

        if (spacing > 0) slotChangeSpacing(spacing);

    } else {
        KMessageBox::sorry
            (this, i18n("Unknown spacing action %1").arg(name));
    }
}

void
NotationView::slotChangeSpacing(int spacing)
{
    if (m_hlayout->getSpacing() == spacing) return;

    m_hlayout->setSpacing(spacing);
    
//     m_spacingSlider->setSize(spacing);

    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action(QString("spacing_%1").arg(spacing)));
    if (action) action->setChecked(true);
    else {
	std::cerr
            << "WARNING: Expected action \"spacing_" << spacing
            << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
            << std::endl;
    }

    positionStaffs();
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	m_staffs[i]->markChanged();
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}


void
NotationView::slotChangeProportionFromIndex(int n)
{
    std::vector<int> proportions = m_hlayout->getAvailableProportions();
    if (n >= (int)proportions.size()) n = proportions.size() - 1;
    slotChangeProportion(proportions[n]);
}

void
NotationView::slotChangeProportionFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(11) == "proportion_") {
        int proportion = name.right(name.length() - 11).toInt();
        slotChangeProportion(proportion);

    } else {
        KMessageBox::sorry
            (this, i18n("Unknown proportion action %1").arg(name));
    }
}

void
NotationView::slotChangeProportion(int proportion)
{
    if (m_hlayout->getProportion() == proportion) return;

    m_hlayout->setProportion(proportion);
    
//    m_proportionSlider->setSize(proportion);

    KToggleAction *action = dynamic_cast<KToggleAction *>
        (actionCollection()->action(QString("proportion_%1").arg(proportion)));
    if (action) action->setChecked(true);
    else {
	std::cerr
            << "WARNING: Expected action \"proportion_" << proportion
            << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
            << std::endl;
    }

    positionStaffs();
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	m_staffs[i]->markChanged();
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}



void
NotationView::slotChangeFontFromAction()
{
    const QObject *s = sender();
    QString name = s->name();
    if (name.left(10) == "note_font_") {
        name = name.right(name.length() - 10);
        slotChangeFont(name);
    } else {
        KMessageBox::sorry
            (this, i18n("Unknown font action %1").arg(name));
    }
}

void
NotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(15) == "note_font_size_") {
        name = name.right(name.length() - 15);
        bool ok = false;
        int size = name.toInt(&ok);
        if (ok) slotChangeFont(m_fontName, size);
        else {
            KMessageBox::sorry
                (this, i18n("Unknown font size %1").arg(name));
        }           
    } else {
        KMessageBox::sorry
            (this, i18n("Unknown font size action %1").arg(name));
    }
}


void
NotationView::slotChangeFont(const QString &newName)
{
    NOTATION_DEBUG << "changeFont: " << newName << endl;
    slotChangeFont(std::string(newName.utf8()));
}


void
NotationView::slotChangeFont(std::string newName)
{
    int newSize = m_fontSize;

    if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {

	int defaultSize = NoteFontFactory::getDefaultSize(newName);
	newSize = m_config->readUnsignedNumEntry
	    ((getStaffCount() > 1 ?
	      "multistaffnotesize" : "singlestaffnotesize"), defaultSize);

	if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {
	    newSize = defaultSize;
	}
    }

    slotChangeFont(newName, newSize);
}


void
NotationView::slotChangeFontSize(int newSize)
{
    slotChangeFont(m_fontName, newSize);
}


void
NotationView::slotChangeFontSizeFromStringValue(const QString& sizeT)
{
    int size = sizeT.toInt();
    slotChangeFont(m_fontName, size);
}


void
NotationView::slotChangeFont(std::string newName, int newSize)
{
    if (newName == m_fontName && newSize == m_fontSize) return;

    NotePixmapFactory* npf = 0;
    
    try {
        npf = new NotePixmapFactory(newName, newSize);
    } catch (...) {
        return;
    }

    bool changedFont = (newName != m_fontName || newSize != m_fontSize);

    std::string oldName = m_fontName;
    m_fontName = newName;
    m_fontSize = newSize;
    setNotePixmapFactory(npf);

    // update the various GUI elements

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (unsigned int i = 0; i < f.size(); ++i) {
        bool thisOne = (f[i] == m_fontName);
        if (thisOne) m_fontCombo->setCurrentItem(i);
        KToggleAction *action = dynamic_cast<KToggleAction *>
            (actionCollection()->action("note_font_" + strtoqstr(f[i])));
        NOTATION_DEBUG << "inspecting " << f[i] << (action ? ", have action" : ", no action") << endl;
        if (action) action->setChecked(thisOne);
        else {
	    std::cerr
                << "WARNING: Expected action \"note_font_" << f[i]
                << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
                << std::endl;
        }
    }

    NOTATION_DEBUG << "about to reinitialise sizes" << endl;

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeCombo->clear();
    QString value;
    for (std::vector<int>::iterator i = sizes.begin(); i != sizes.end(); ++i) {
        value.setNum(*i);
        m_fontSizeCombo->insertItem(value);
    }
    value.setNum(m_fontSize);
    m_fontSizeCombo->setCurrentText(value);

    setupFontSizeMenu(oldName);

    if (!changedFont) return; // might have been called to initialise menus etc

    NOTATION_DEBUG << "about to change font" << endl;

    if (m_pageMode == LinedStaff::MultiPageMode) {

	int pageWidth = getPageWidth();
	int topMargin = 0, leftMargin = 0;
	getPageMargins(leftMargin, topMargin);

	m_hlayout->setPageWidth(pageWidth - leftMargin * 2);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->changeFont(m_fontName, m_fontSize);
    }

    NOTATION_DEBUG << "about to position staffs" << endl;

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->markChanged();
        }
    }

    positionPages();
    updateControlRulers(true);
    updateView();
}


void
NotationView::slotFilePrint()
{
    KTmpStatusMsg msg(i18n("Printing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
			      (QWidget *)parent(), this);

    if (!printingView.isOK()) {
	NOTATION_DEBUG << "Print : operation cancelled\n";
	return;
    }

    printingView.print();
}

void
NotationView::slotFilePrintPreview()
{
    KTmpStatusMsg msg(i18n("Previewing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
			      (QWidget *)parent(), this);

    if (!printingView.isOK()) {
	NOTATION_DEBUG << "Print preview : operation cancelled\n";
	return;
    }

    printingView.print(true);
}


//
// Cut, Copy, Paste
//
void NotationView::slotEditCut()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
                                       getDocument()->getClipboard()));
}

void NotationView::slotEditDelete()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));
}

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
                                        getDocument()->getClipboard()));
}

void NotationView::slotEditCutAndClose()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutAndCloseCommand(*m_currentEventSelection,
                                               getDocument()->getClipboard()));
}

static const QString RESTRICTED_PASTE_FAILED_DESCRIPTION = i18n(
                      "The Restricted paste type requires enough empty\n" \
                      "space (containing only rests) at the paste position\n" \
                      "to hold all of the events to be pasted.\n" \
                      "Not enough space was found.\n" \
                      "If you want to paste anyway, consider using one of\n" \
                      "the other paste types from the \"Paste...\" option\n" \
                      "on the Edit menu.  You can also change the default\n" \
                      "paste type to something other than Restricted if\n" \
                      "you wish."
    );

void NotationView::slotEditPaste()
{
    Rosegarden::Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }
    if (!clipboard->isSingleSegment()) {
        slotStatusHelpMsg(i18n("Can't paste multiple Segments into one"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
        (clipboard->getSingleSegment()->getEndTime() - 
         clipboard->getSingleSegment()->getStartTime());

    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        config->readUnsignedNumEntry("pastetype",
                                     PasteEventsCommand::Restricted);

    PasteEventsCommand *command = new PasteEventsCommand
        (segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        KMessageBox::detailedError
            (this,
             i18n("Couldn't paste at this point."), RESTRICTED_PASTE_FAILED_DESCRIPTION);
    } else {
        addCommandToHistory(command);
        //!!! well, we really just want to select the events
        // we just pasted
        setCurrentSelection(new EventSelection
                            (segment, insertionTime, endTime));
	slotSetInsertCursorPosition(endTime, true, false);
    }
}

void NotationView::slotEditGeneralPaste()
{
    Rosegarden::Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        config->readUnsignedNumEntry("pastetype",
                                     PasteEventsCommand::Restricted);
    
    PasteNotationDialog dialog(this, defaultType);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();
        if (dialog.setAsDefault()) {
	    config->setGroup(NotationView::ConfigGroup);
            config->writeEntry("pastetype", type);
        }

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() - 
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            KMessageBox::detailedError
                (this,
                 i18n("Couldn't paste at this point."),
                 i18n(RESTRICTED_PASTE_FAILED_DESCRIPTION));
        } else {
            addCommandToHistory(command);
            setCurrentSelection(new EventSelection
                                (segment, insertionTime, endTime));
	    slotSetInsertCursorPosition(endTime, true, false);
        }
    }
}

void NotationView::slotPreviewSelection()
{
    if (!m_currentEventSelection) return;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
                            m_currentEventSelection->getEndTime());
}


void NotationView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}


void NotationView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    NotationSelector *selector = dynamic_cast<NotationSelector *>(m_tool);
    
    if (!selector) {
        slotSelectSelected();
    } else {
        setCurrentSelection(0);
    }
}

//!!! these should be in matrix too

void NotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           segment.getStartTime(),
                                           t));
}

void NotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           t,
                                           segment.getEndMarkerTime()));
}

void NotationView::slotEditSelectWholeStaff()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           segment.getStartTime(),
                                           segment.getEndMarkerTime()));
}

void NotationView::slotFilterSelection()
{
    NOTATION_DEBUG << "NotationView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection) return;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotFilterSelection- accepted" << endl;

	bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
		haveEvent = true;
		newSelection->addEvent(*i);
	    }
        }

	if (haveEvent) setCurrentSelection(newSelection);
	else setCurrentSelection(0);
    }
}

void NotationView::slotFinePositionLeft()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pushing selection left..."), this);

    // half a note body width
    addCommandToHistory(new IncrementDisplacementsCommand
			(*m_currentEventSelection, -500, 0));
}

void NotationView::slotFinePositionRight()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pushing selection right..."), this);

    // half a note body width
    addCommandToHistory(new IncrementDisplacementsCommand
			(*m_currentEventSelection, 500, 0));
}

void NotationView::slotFinePositionUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pushing selection up..."), this);

    // half line height
    addCommandToHistory(new IncrementDisplacementsCommand
			(*m_currentEventSelection, 0, -500));
}

void NotationView::slotFinePositionDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pushing selection down..."), this);

    // half line height
    addCommandToHistory(new IncrementDisplacementsCommand
			(*m_currentEventSelection, 0, 500));
}

void NotationView::slotFinePositionRestore()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring computed positions..."), this);

    addCommandToHistory(new ResetDisplacementsCommand(*m_currentEventSelection));
}

void NotationView::slotMakeVisible()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making visible..."), this);

    addCommandToHistory(new SetVisibilityCommand(*m_currentEventSelection, true));
}

void NotationView::slotMakeInvisible()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making invisible..."), this);

    addCommandToHistory(new SetVisibilityCommand(*m_currentEventSelection, false));
}

void NotationView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("Notes Toolbar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("Rests Toolbar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("Accidentals Toolbar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("Clefs Toolbar");
}

void NotationView::slotToggleMetaToolBar()
{
    toggleNamedToolBar("Meta Toolbar");
}

void NotationView::slotToggleMarksToolBar()
{
    toggleNamedToolBar("Marks Toolbar");
}

void NotationView::slotToggleGroupToolBar()
{
    toggleNamedToolBar("Group Toolbar");
}

void NotationView::slotToggleLayoutToolBar()
{
    toggleNamedToolBar("Layout Toolbar");
}

void NotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
}

void NotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        NOTATION_DEBUG << "NotationView::toggleNamedToolBar() : toolBar "
                             << toolBarName << " not found" << endl;
        return;
    }

    if (!force) {
    
        if (namedToolBar->isVisible())
            namedToolBar->hide();
        else
            namedToolBar->show();
    } else {

        if (*force)
            namedToolBar->show();
        else
            namedToolBar->hide();
    }

    setSettingsDirty();

}

//
// Group stuff
//

void NotationView::slotGroupBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Beaming group..."), this);

    addCommandToHistory(new NotesMenuBeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupAutoBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Auto-beaming selection..."), this);

    addCommandToHistory(new NotesMenuAutoBeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Breaking groups..."), this);

    addCommandToHistory(new NotesMenuBreakCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupSimpleTuplet()
{
    slotGroupTuplet(true);
}

void NotationView::slotGroupGeneralTuplet()
{
    slotGroupTuplet(false);
}

void NotationView::slotGroupTuplet(bool simple)
{
    timeT t = 0;
    timeT unit = 0;
    int tupled = 2;
    int untupled = 3;
    Segment *segment = 0;
    bool hasTimingAlready = false;

    if (m_currentEventSelection) {

        t = m_currentEventSelection->getStartTime();

        timeT duration = m_currentEventSelection->getTotalDuration();
        Rosegarden::Note::Type unitType =
            Rosegarden::Note::getNearestNote(duration / 3, 0).getNoteType();
        unit = Rosegarden::Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType, duration);
            if (dialog.exec() != QDialog::Accepted) return;
            unit = Rosegarden::Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
	    hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &m_currentEventSelection->getSegment();

    } else {

        t = getInsertionTime();

        NoteInserter *currentInserter = dynamic_cast<NoteInserter *>
            (m_toolBox->getTool(NoteInserter::ToolName));

        Rosegarden::Note::Type unitType;

        if (currentInserter) {
            unitType = currentInserter->getCurrentNote().getNoteType();
        } else {
            unitType = Rosegarden::Note::Quaver;
        }

        unit = Rosegarden::Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType);
            if (dialog.exec() != QDialog::Accepted) return;
            unit = Rosegarden::Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
	    hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &m_staffs[m_currentStaff]->getSegment();
    }

    addCommandToHistory(new AdjustMenuTupletCommand
                        (*segment, t, unit, untupled, tupled, hasTimingAlready));

    if (!hasTimingAlready) {
	slotSetInsertCursorPosition(t + (unit * tupled), true, false);
    }
}

void NotationView::slotGroupUnTuplet()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untupleting..."), this);

    addCommandToHistory(new AdjustMenuUnTupletCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupGrace()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making grace notes..."), this);

    addCommandToHistory(new AdjustMenuGraceCommand(*m_currentEventSelection));
}

void NotationView::slotGroupUnGrace()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making non-grace notes..."), this);

    addCommandToHistory(new AdjustMenuUnGraceCommand(*m_currentEventSelection));
}


//
// indications stuff
//

void NotationView::slotGroupSlur()
{
    KTmpStatusMsg msg(i18n("Adding slur..."), this);
    slotAddIndication(Rosegarden::Indication::Slur, i18n("slur"));
} 

void NotationView::slotGroupPhrasingSlur()
{
    KTmpStatusMsg msg(i18n("Adding phrasing slur..."), this);
    slotAddIndication(Rosegarden::Indication::PhrasingSlur, i18n("phrasing slur"));
} 

void NotationView::slotGroupGlissando()
{
    KTmpStatusMsg msg(i18n("Adding glissando..."), this);
    slotAddIndication(Rosegarden::Indication::Glissando, i18n("glissando"));
} 
  
void NotationView::slotGroupCrescendo()
{
    KTmpStatusMsg msg(i18n("Adding crescendo..."), this);
    slotAddIndication(Rosegarden::Indication::Crescendo, i18n("dynamic"));
} 
  
void NotationView::slotGroupDecrescendo()
{
    KTmpStatusMsg msg(i18n("Adding decrescendo..."), this);
    slotAddIndication(Rosegarden::Indication::Decrescendo, i18n("dynamic"));
} 

void NotationView::slotGroupOctave2Up()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Rosegarden::Indication::QuindicesimaUp, i18n("ottava"));
} 

void NotationView::slotGroupOctaveUp()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Rosegarden::Indication::OttavaUp, i18n("ottava"));
} 

void NotationView::slotGroupOctaveDown()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Rosegarden::Indication::OttavaDown, i18n("ottava"));
} 

void NotationView::slotGroupOctave2Down()
{
    KTmpStatusMsg msg(i18n("Adding octave..."), this);
    slotAddIndication(Rosegarden::Indication::QuindicesimaDown, i18n("ottava"));
} 

void NotationView::slotAddIndication(std::string type, QString desc)
{
    if (!m_currentEventSelection) return;

    NotesMenuAddIndicationCommand *command =
        new NotesMenuAddIndicationCommand(type, *m_currentEventSelection);

    if (command->canExecute()) {
        addCommandToHistory(command);
	setSingleSelectedEvent(m_currentEventSelection->getSegment(),
			       command->getLastInsertedEvent());
    } else {
	KMessageBox::sorry(this, i18n("Can't add overlapping %1 indications").arg(desc)); // TODO PLURAL - how many 'indications' ?
	delete command;
    }
} 

void NotationView::slotGroupMakeChord()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making chord..."), this);

    AdjustMenuMakeChordCommand *command =
	new AdjustMenuMakeChordCommand(*m_currentEventSelection);

    addCommandToHistory(command);
}
    
 
// 
// transforms stuff
//
 
void NotationView::slotTransformsNormalizeRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Normalizing rests..."), this);

    addCommandToHistory(new AdjustMenuNormalizeRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), this);

    addCommandToHistory(new AdjustMenuCollapseRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), this);

    addCommandToHistory(new AdjustMenuCollapseNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsTieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Tying notes..."), this);

    addCommandToHistory(new NotesMenuTieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsUntieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untying notes..."), this);

    addCommandToHistory(new AdjustMenuUntieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsMakeNotesViable()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making notes viable..."), this);

    addCommandToHistory(new AdjustMenuMakeNotesViableCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsDeCounterpoint()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Removing counterpoint..."), this);

    addCommandToHistory(new AdjustMenuDeCounterpointCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsStemsUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems up..."), this);

    addCommandToHistory(new AdjustMenuChangeStemsCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsStemsDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems down..."), this);

    addCommandToHistory(new AdjustMenuChangeStemsCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreStems()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring computed stem directions..."), this);

    addCommandToHistory(new AdjustMenuRestoreStemsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsSlursAbove()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Positioning slurs..."), this);

    addCommandToHistory(new AdjustMenuChangeSlurPositionCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsSlursBelow()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Positioning slurs..."), this);

    addCommandToHistory(new AdjustMenuChangeSlurPositionCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreSlurs()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring slur positions..."), this);

    addCommandToHistory(new AdjustMenuRestoreSlursCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsFixQuantization()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Fixing notation quantization..."), this);

    addCommandToHistory(new AdjustMenuFixNotationQuantizeCommand
                        (*m_currentEventSelection));
}    

void NotationView::slotTransformsRemoveQuantization()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Removing notation quantization..."), this);

    addCommandToHistory(new AdjustMenuRemoveNotationQuantizeCommand
                        (*m_currentEventSelection));
}    

void NotationView::slotSetStyleFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (!m_currentEventSelection) return;

    if (name.left(6) == "style_") {
        name = name.right(name.length() - 6);

        KTmpStatusMsg msg(i18n("Changing to %1 style...").arg(name),
                          this);

        addCommandToHistory(new AdjustMenuChangeStyleCommand
                            (NoteStyleName(qstrtostr(name)),
                             *m_currentEventSelection));
    } else {
        KMessageBox::sorry
            (this, i18n("Unknown style action %1").arg(name));
    }
}

void NotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        KMessageBox::sorry(this, i18n("No note duration selected"));
        return;
    }

    int pitch = 0;
    Rosegarden::Accidental accidental =
        Rosegarden::Accidentals::NoAccidental;

    Rosegarden::timeT time(getInsertionTime());
    Rosegarden::Key key = segment.getKeyAtTime(time);
    Rosegarden::Clef clef = segment.getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {
        
        KMessageBox::sorry
            (this, i18n("Unknown note insert action %1").arg(name));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);
        
    NOTATION_DEBUG << "Inserting note at pitch " << pitch << endl;
    
    noteInserter->insertNote(segment, time, pitch, accidental);
}

void NotationView::slotInsertRest()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    timeT time = getInsertionTime();

    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);

    if (!restInserter) {

	NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
	if (!noteInserter) {
	    KMessageBox::sorry(this, i18n("No note duration selected"));
	    return;
	}

	Rosegarden::Note note(noteInserter->getCurrentNote());
    
	restInserter = dynamic_cast<RestInserter*>
	    (m_toolBox->getTool(RestInserter::ToolName));

	restInserter->slotSetNote(note.getNoteType());
	restInserter->slotSetDots(note.getDots());
    }

    restInserter->insertNote(segment, time,
                             0, Rosegarden::Accidentals::NoAccidental, true);
}

void NotationView::slotSwitchFromRestToNote()
{
    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
    if (!restInserter) {
        KMessageBox::sorry(this, i18n("No rest duration selected"));
        return;
    }

    Rosegarden::Note note(restInserter->getCurrentNote());

    QString actionName = NotationStrings::getReferenceName(note, false);
    actionName = actionName.replace("-", "_");
    
    KRadioAction *action = dynamic_cast<KRadioAction *>
	(actionCollection()->action(actionName));

    if (!action) {
	std::cerr << "WARNING: Failed to find note action \""
		  << actionName << "\"" << std::endl;
    } else {
	action->activate();
    }

    NoteInserter *noteInserter = dynamic_cast<NoteInserter*>
	(m_toolBox->getTool(NoteInserter::ToolName));

    if (noteInserter) {
	noteInserter->slotSetNote(note.getNoteType());
	noteInserter->slotSetDots(note.getDots());
	setTool(noteInserter);
    }

    setMenuStates();
}

void NotationView::slotSwitchFromNoteToRest()
{
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
        KMessageBox::sorry(this, i18n("No note duration selected"));
        return;
    }

    Rosegarden::Note note(noteInserter->getCurrentNote());
    
    QString actionName = NotationStrings::getReferenceName(note, true);
    actionName = actionName.replace("-", "_");
    
    KRadioAction *action = dynamic_cast<KRadioAction *>
	(actionCollection()->action(actionName));

    if (!action) {
	std::cerr << "WARNING: Failed to find rest action \""
		  << actionName << "\"" << std::endl;
    } else {
	action->activate();
    }

    RestInserter *restInserter = dynamic_cast<RestInserter*>
	(m_toolBox->getTool(RestInserter::ToolName));

    if (restInserter) {
	restInserter->slotSetNote(note.getNoteType());
	restInserter->slotSetDots(note.getDots());
	setTool(restInserter);
    }

    setMenuStates();
}

void NotationView::slotToggleDot()
{
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (noteInserter) {
	Rosegarden::Note note(noteInserter->getCurrentNote());
	if (note.getNoteType() == Rosegarden::Note::Shortest ||
	    note.getNoteType() == Rosegarden::Note::Longest) return;
	noteInserter->slotSetDots(note.getDots() ? 0 : 1);
	setTool(noteInserter);
    } else {
	RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
	if (restInserter) {
	    Rosegarden::Note note(restInserter->getCurrentNote());
	    if (note.getNoteType() == Rosegarden::Note::Shortest ||
		note.getNoteType() == Rosegarden::Note::Longest) return;
	    restInserter->slotSetDots(note.getDots() ? 0 : 1);
	    setTool(restInserter);
	} else {
	    KMessageBox::sorry(this, i18n("No note or rest duration selected"));
	}
    }

    setMenuStates();
}

void NotationView::slotRespellDoubleFlat()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::DoubleFlat,
					   *m_currentEventSelection));
}

void NotationView::slotRespellFlat()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::Flat,
					   *m_currentEventSelection));
}

void NotationView::slotRespellNatural()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::Natural,
					   *m_currentEventSelection));
}

void NotationView::slotRespellSharp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::Sharp,
					   *m_currentEventSelection));
}

void NotationView::slotRespellDoubleSharp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::DoubleSharp,
					   *m_currentEventSelection));
}

void NotationView::slotRespellUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Up,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotRespellDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Down,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotRespellRestore()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Restore,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotShowCautionary()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Showing cautionary accidentals..."), this);

    addCommandToHistory(new MakeAccidentalsCautionaryCommand
			(true, *m_currentEventSelection));
}

void NotationView::slotCancelCautionary()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cancelling cautionary accidentals..."), this);

    addCommandToHistory(new MakeAccidentalsCautionaryCommand
			(false, *m_currentEventSelection));
}
    

void NotationView::slotTransformsQuantize()
{
    if (!m_currentEventSelection) return;

    QuantizeDialog dialog(this, true);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
        addCommandToHistory(new EventQuantizeCommand
                            (*m_currentEventSelection,
                             dialog.getQuantizer()));
    }
}

void NotationView::slotTransformsInterpret()
{
    if (!m_currentEventSelection) return;

    InterpretDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Interpreting selection..."), this);
	addCommandToHistory(new AdjustMenuInterpretCommand
			    (*m_currentEventSelection,
			     getDocument()->getComposition().getNotationQuantizer(),
			     dialog.getInterpretations()));
    }
}
    
void NotationView::slotSetNoteDurations(Rosegarden::Note::Type type, bool notationOnly)
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Setting note durations..."), this);
    addCommandToHistory(new SetNoteTypeCommand(*m_currentEventSelection, type, notationOnly));
}

void NotationView::slotAddDot()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding dot..."), this);
    addCommandToHistory(new AddDotCommand(*m_currentEventSelection, false));
}

void NotationView::slotAddDotNotationOnly()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding dot..."), this);
    addCommandToHistory(new AddDotCommand(*m_currentEventSelection, true));
}

void NotationView::slotAddSlashes()
{
    const QObject *s = sender();
    if (!m_currentEventSelection) return;

    QString name = s->name();
    int slashes = name.right(1).toInt();

    addCommandToHistory(new NotesMenuAddSlashesCommand
                        (slashes, *m_currentEventSelection));
}


void NotationView::slotMarksAddTextMark()
{
    if (m_currentEventSelection) {
        bool pressedOK = false;
        
        QString txt = KLineEditDlg::getText(i18n("Text: "), "", &pressedOK, this);
        
        if (pressedOK) {
            addCommandToHistory(new NotesMenuAddTextMarkCommand
                                (qstrtostr(txt), *m_currentEventSelection));
        }
    }
}

void NotationView::slotMarksAddFingeringMark()
{
    if (m_currentEventSelection) {
        bool pressedOK = false;
        
        QString txt = KLineEditDlg::getText(i18n("Fingering: "), "", &pressedOK, this);
        
        if (pressedOK) {
            addCommandToHistory(new NotesMenuAddFingeringMarkCommand
                                (qstrtostr(txt), *m_currentEventSelection));
        }
    }
}

void NotationView::slotMarksAddFingeringMarkFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(14) == "add_fingering_") {

	QString fingering = name.right(name.length() - 14);

	if (fingering == "plus") fingering = "+";

	if (m_currentEventSelection) {
	    addCommandToHistory(new NotesMenuAddFingeringMarkCommand
				(qstrtostr(fingering), *m_currentEventSelection));
	}
    }
}

void NotationView::slotMarksRemoveMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new NotesMenuRemoveMarksCommand
                            (*m_currentEventSelection));
}

void NotationView::slotMarksRemoveFingeringMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new NotesMenuRemoveFingeringMarksCommand
                            (*m_currentEventSelection));
}


void
NotationView::slotMakeOrnament()
{
    if (!m_currentEventSelection) return;

    EventSelection::eventcontainer &ec =
	m_currentEventSelection->getSegmentEvents();

    int basePitch = -1;
    int baseVelocity = -1;
    NoteStyle *style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);

    for (EventSelection::eventcontainer::iterator i =
	     ec.begin(); i != ec.end(); ++i) {
	if ((*i)->isa(Rosegarden::Note::EventType)) {
	    if ((*i)->has(Rosegarden::BaseProperties::PITCH)) {
		basePitch = (*i)->get<Rosegarden::Int>
		    (Rosegarden::BaseProperties::PITCH);
		style = NoteStyleFactory::getStyleForEvent(*i);
		if (baseVelocity != -1) break;
	    }
	    if ((*i)->has(Rosegarden::BaseProperties::VELOCITY)) {
		baseVelocity = (*i)->get<Rosegarden::Int>
		    (Rosegarden::BaseProperties::VELOCITY);
		if (basePitch != -1) break;
	    }
	}
    }

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    Rosegarden::timeT absTime = m_currentEventSelection->getStartTime();
    Rosegarden::timeT duration = m_currentEventSelection->getTotalDuration();
    Rosegarden::Note note(Rosegarden::Note::getNearestNote(duration));

    Rosegarden::Track *track =
	segment.getComposition()->getTrackById(segment.getTrack());
    QString name;
    int barNo = segment.getComposition()->getBarNumber(absTime);
    if (track) {
	name = QString("Ornament track %1 bar %2").arg(track->getPosition() + 1).arg(barNo + 1);
    } else {
	name = QString("Ornament bar %1").arg(barNo + 1);
    }

    MakeOrnamentDialog dialog(this, name, basePitch);
    if (dialog.exec() != QDialog::Accepted) return;
    
    name = dialog.getName();
    basePitch = dialog.getBasePitch();

    KMacroCommand *command = new KMacroCommand(i18n("Make Ornament"));

    command->addCommand(new CutCommand
			(*m_currentEventSelection,
			 getDocument()->getClipboard()));

    command->addCommand(new PasteToTriggerSegmentCommand
			(&getDocument()->getComposition(), 
			 getDocument()->getClipboard(),
			 name, basePitch));

    command->addCommand(new InsertTriggerNoteCommand
			(segment, absTime, note, basePitch, baseVelocity,
			 style->getName(),
			 getDocument()->getComposition().getNextTriggerSegmentId(),
			 true,
			 Rosegarden::BaseProperties::TRIGGER_SEGMENT_ADJUST_SQUISH,
			 Rosegarden::Marks::NoMark)); //!!!

    addCommandToHistory(command);
}

void
NotationView::slotUseOrnament()
{
    // Take an existing note and match an ornament to it.
    
    if (!m_currentEventSelection) return;
    
    UseOrnamentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted) return;

    addCommandToHistory(new SetTriggerCommand(*m_currentEventSelection,
					      dialog.getId(),
					      true,
					      dialog.getRetune(),
					      dialog.getTimeAdjust(),
					      dialog.getMark(),
					      i18n("Use Ornament")));
}

void
NotationView::slotRemoveOrnament()
{
    if (!m_currentEventSelection) return;

    addCommandToHistory(new ClearTriggersCommand(*m_currentEventSelection,
						 i18n("Remove Ornaments")));
}


void NotationView::slotEditAddClef()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    static Rosegarden::Clef lastClef;
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);

    ClefDialog dialog(this, m_notePixmapFactory, lastClef);
    
    if (dialog.exec() == QDialog::Accepted) {
        
        ClefDialog::ConversionType conversion = dialog.getConversionType();
        
        bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
        bool shouldTranspose = (conversion == ClefDialog::Transpose);
        
        addCommandToHistory
            (new ClefInsertionCommand
             (segment, insertionTime, dialog.getClef(),
              shouldChangeOctave, shouldTranspose));

	lastClef = dialog.getClef();
    }
}                       

void NotationView::slotEditAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);

    //!!! experimental:
    Rosegarden::CompositionTimeSliceAdapter adapter
        (&getDocument()->getComposition(), insertionTime,
         getDocument()->getComposition().getDuration());
    Rosegarden::AnalysisHelper helper;
    key = helper.guessKey(adapter);

    KeySignatureDialog dialog
        (this, m_notePixmapFactory, clef, key, true, true,
         i18n("Estimated key signature shown"));
    
    if (dialog.exec() == QDialog::Accepted &&
        dialog.isValid()) {
        
        KeySignatureDialog::ConversionType conversion =
            dialog.getConversionType();
        
        bool applyToAll = dialog.shouldApplyToAll();
        
        if (applyToAll) {
            addCommandToHistory
                (new MultiKeyInsertionCommand
                 (getDocument()->getComposition(),
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose));
        } else {
            addCommandToHistory
                (new KeyInsertionCommand
                 (segment,
                  insertionTime, dialog.getKey(),
                  conversion == KeySignatureDialog::Convert,
                  conversion == KeySignatureDialog::Transpose));
        }
    }
}                      
 
void NotationView::slotEditAddSustain(bool down)
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT insertionTime = getInsertionTime();
        
    Rosegarden::Studio *studio = &getDocument()->getStudio();
    Rosegarden::Track *track = segment.getComposition()->getTrackById(segment.getTrack());

    if (track) {

	Rosegarden::Instrument *instrument = studio->getInstrumentById
	    (track->getInstrument());
	if (instrument) {
	    Rosegarden::MidiDevice *device = dynamic_cast<Rosegarden::MidiDevice *>
		(instrument->getDevice());
	    if (device) {
		for (Rosegarden::ControlList::const_iterator i =
			 device->getControlParameters().begin();
		     i != device->getControlParameters().end(); ++i) {

		    if (i->getType() == Rosegarden::Controller::EventType &&
			(i->getName() == "Sustain" ||
			 strtoqstr(i->getName()) == i18n("Sustain"))) {

			addCommandToHistory
			    (new SustainInsertionCommand(segment, insertionTime, down,
							 i->getControllerValue()));
			return;
		    }
		}
	    } else if (instrument->getDevice() &&
		       instrument->getDevice()->getType() == Rosegarden::Device::SoftSynth) {
		addCommandToHistory
		    (new SustainInsertionCommand(segment, insertionTime, down, 64));
	    }
	}
    }

    KMessageBox::sorry(this, i18n("There is no sustain controller defined for this device.\nPlease ensure the device is configured correctly in the Manage MIDI Devices dialog in the main window."));
}                       
 
void NotationView::slotEditAddSustainDown()
{
    slotEditAddSustain(true);
}
 
void NotationView::slotEditAddSustainUp()
{
    slotEditAddSustain(false);
}

void NotationView::slotEditElement(NotationStaff *staff,
				   NotationElement *element, bool advanced)
{
    if (advanced) {
	
	EventEditDialog dialog(this, *element->event(), true);
	
	if (dialog.exec() == QDialog::Accepted &&
	    dialog.isModified()) {
	    
	    EventEditCommand *command = new EventEditCommand
		(staff->getSegment(),
		 element->event(),
		 dialog.getEvent());
	    
	    addCommandToHistory(command);
	}

    } else if (element->event()->isa(Rosegarden::Clef::EventType)) {

	try {
	    ClefDialog dialog(this, m_notePixmapFactory,
			      Rosegarden::Clef(*element->event()));
	    
	    if (dialog.exec() == QDialog::Accepted) {
		
		ClefDialog::ConversionType conversion = dialog.getConversionType();
		bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
		bool shouldTranspose = (conversion == ClefDialog::Transpose);
		addCommandToHistory
		    (new ClefInsertionCommand
		     (staff->getSegment(), element->event()->getAbsoluteTime(),
		      dialog.getClef(), shouldChangeOctave, shouldTranspose));
	    }
	} catch (Rosegarden::Exception e) {
	    std::cerr << e.getMessage() << std::endl;
	}
	
	return;
	
    } else if (element->event()->isa(Rosegarden::Key::EventType)) {
	
	try {
	    Rosegarden::Clef clef(staff->getSegment().getClefAtTime
				  (element->event()->getAbsoluteTime()));
		KeySignatureDialog dialog
		    (this, m_notePixmapFactory, clef, Rosegarden::Key(*element->event()),
		     false, true);
		
		if (dialog.exec() == QDialog::Accepted &&
		    dialog.isValid()) {
		
		    KeySignatureDialog::ConversionType conversion =
			dialog.getConversionType();
		
		    addCommandToHistory
			(new KeyInsertionCommand
			 (staff->getSegment(),
			  element->event()->getAbsoluteTime(), dialog.getKey(),
			  conversion == KeySignatureDialog::Convert,
			  conversion == KeySignatureDialog::Transpose));
		}

	} catch (Rosegarden::Exception e) {
	    std::cerr << e.getMessage() << std::endl;
	}

	return;

    } else if (element->event()->isa(Rosegarden::Text::EventType)) {
	
	try {
	    TextEventDialog dialog
		(this, m_notePixmapFactory, Rosegarden::Text(*element->event()));
	    if (dialog.exec() == QDialog::Accepted) {
		TextInsertionCommand *command = new TextInsertionCommand
		    (staff->getSegment(),
		     element->event()->getAbsoluteTime(),
		     dialog.getText());
		KMacroCommand *macroCommand = new KMacroCommand(command->name());
		macroCommand->addCommand(new EraseEventCommand(staff->getSegment(),
							       element->event(), false));
		macroCommand->addCommand(command);
		addCommandToHistory(macroCommand);
	    }
	} catch (Rosegarden::Exception e) {
	    std::cerr << e.getMessage() << std::endl;
	}

	return;

    } else if (element->isNote() &&
	       element->event()->has(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID)) {
	    
	int id = element->event()->get<Rosegarden::Int>
	    (Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID);
	emit editTriggerSegment(id);
	return;

    } else {

	SimpleEventEditDialog dialog(this, getDocument(), *element->event(), false);
	
	if (dialog.exec() == QDialog::Accepted &&
	    dialog.isModified()) {
	    
	    EventEditCommand *command = new EventEditCommand
		(staff->getSegment(),
		 element->event(),
		 dialog.getEvent());
	    
	    addCommandToHistory(command);
	}
    }
}                       

void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
        EventSelection::eventcontainer &ec =
            m_currentEventSelection->getSegmentEvents();
        int n = 0;
        for (EventSelection::eventcontainer::iterator i =
                 ec.begin();
             i != ec.end(); ++i) {
	    std::cerr << "\n" << n++ << " [" << (*i) << "]" << std::endl;
            (*i)->dump(std::cerr);
        }
    }
}


void
NotationView::slotSetPointerPosition(timeT time)
{
    slotSetPointerPosition(time, m_playTracking);
}


void
NotationView::slotSetPointerPosition(timeT time, bool scroll)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
    int barNo = comp.getBarNumber(time);

    int minCy = 0;
    double cx = 0;
    bool haveMinCy = false;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	double layoutX = m_hlayout->getXForTimeByEvent(time);
	Segment &seg = m_staffs[i]->getSegment();

	bool good = true;

	if (barNo >= m_hlayout->getLastVisibleBarOnStaff(*m_staffs[i])) {
	    if (seg.isRepeating() && time < seg.getRepeatEndTime()) {
		timeT mappedTime =
		    seg.getStartTime() +
		    ((time - seg.getStartTime()) %
		     (seg.getEndMarkerTime() - seg.getStartTime()));
		layoutX = m_hlayout->getXForTimeByEvent(mappedTime);
	    } else {
		good = false;
	    }
	} else if (barNo < m_hlayout->getFirstVisibleBarOnStaff(*m_staffs[i])) {
	    good = false;
	}

	if (!good) {

            m_staffs[i]->hidePointer();

        } else {

            m_staffs[i]->setPointerPosition(layoutX);

	    int cy;
	    m_staffs[i]->getPointerPosition(cx, cy);

	    if (!haveMinCy || cy < minCy) {
		minCy = cy;
		haveMinCy = true;
	    }
        }
    }

    if (m_pageMode == LinedStaff::LinearMode) {
	// be careful not to prevent user from scrolling up and down
	haveMinCy = false;
    }

    if (scroll) {
	getCanvasView()->slotScrollHoriz(int(cx));
	if (haveMinCy) {
	    getCanvasView()->slotScrollVertToTop(minCy);
	}
    }

    updateView();
}


void
NotationView::slotUpdateRecordingSegment(Rosegarden::Segment *segment,
					 timeT updateFrom)
{
    NOTATION_DEBUG << "NotationView::slotUpdateRecordingSegment: segment " << segment << ", updateFrom " << updateFrom << ", end time " << segment->getEndMarkerTime() << endl;
    if (updateFrom >= segment->getEndMarkerTime()) return;
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	if (&m_staffs[i]->getSegment() == segment) {
//	    refreshSegment(segment, updateFrom, segment->getEndMarkerTime());
	    refreshSegment(segment, 0, 0);
	}
    }
    NOTATION_DEBUG << "NotationView::slotUpdateRecordingSegment: don't have segment " << segment << endl;
}


void
NotationView::slotSetCurrentStaff(double x, int y)
{
    unsigned int staffNo;
    for (staffNo = 0; staffNo < m_staffs.size(); ++staffNo) {
        if (m_staffs[staffNo]->containsCanvasCoords(x, y)) break;
    }
    
    if (staffNo < m_staffs.size()) {
	slotSetCurrentStaff(staffNo);
    }
}

void
NotationView::slotSetCurrentStaff(int staffNo)
{
    NOTATION_DEBUG << "NotationView::slotSetCurrentStaff(" << staffNo << ")" << endl;

    if (m_currentStaff != staffNo) {

	m_staffs[m_currentStaff]->setCurrent(false);

	m_currentStaff = staffNo;

	m_staffs[m_currentStaff]->setCurrent(true);

	Rosegarden::Segment *segment = &m_staffs[m_currentStaff]->getSegment();

	m_chordNameRuler->setCurrentSegment(segment);
	m_rawNoteRuler->setCurrentSegment(segment);
	m_rawNoteRuler->repaint();
	setControlRulersCurrentSegment();

	updateView();
	
	slotSetInsertCursorPosition(getInsertionTime(), false, false);
    }
}    

void
NotationView::slotCurrentStaffUp()
{
    if (m_staffs.size() < 2) return;

    Rosegarden::Composition *composition =
	m_staffs[m_currentStaff]->getSegment().getComposition();
	
    Rosegarden::Track *track = composition->
	getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track) return;

    int position = track->getPosition();
    Rosegarden::Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(--position))) {
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
		slotSetCurrentStaff(i);
		return;
	    }
	}
    }
}

void
NotationView::slotCurrentStaffDown()
{
    if (m_staffs.size() < 2) return;

    Rosegarden::Composition *composition =
	m_staffs[m_currentStaff]->getSegment().getComposition();
	
    Rosegarden::Track *track = composition->
	getTrackById(m_staffs[m_currentStaff]->getSegment().getTrack());
    if (!track) return;

    int position = track->getPosition();
    Rosegarden::Track *newTrack = 0;

    while ((newTrack = composition->getTrackByPosition(++position))) {
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    if (m_staffs[i]->getSegment().getTrack() == newTrack->getId()) {
		slotSetCurrentStaff(i);
		return;
	    }
	}
    }
}

void
NotationView::slotSetInsertCursorPosition(double x, int y, bool scroll,
                                          bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorPosition: x " << x << ", y " << y << ", scroll " << scroll << ", now " << updateNow << endl;

    slotSetCurrentStaff(x, y);

    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt, *keyEvt;
    NotationElementList::iterator i =
        staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (i == staff->getViewElementList()->end()) {
        slotSetInsertCursorPosition(staff->getSegment().getEndTime(), scroll,
                                    updateNow);
    } else {
        slotSetInsertCursorPosition((*i)->getViewAbsoluteTime(), scroll,
                                    updateNow);
    }
}    

void
NotationView::slotSetInsertCursorPosition(timeT t, bool scroll, bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorPosition: time " << t << ", scroll " << scroll << ", now " << updateNow << endl;

    m_insertionTime = t;
    if (scroll) {
        m_deferredCursorMove = CursorMoveAndMakeVisible;
    } else {
        m_deferredCursorMove = CursorMoveOnly;
    }
    if (updateNow) doDeferredCursorMove();
}

void
NotationView::slotSetInsertCursorAndRecentre(timeT t, double cx, int,
                                             bool updateNow)
{
    NOTATION_DEBUG << "NotationView::slotSetInsertCursorAndRecentre: time " << t << ", cx " << cx << ", now " << updateNow << ", contentsx" << getCanvasView()->contentsX() << ", w " << getCanvasView()->visibleWidth() << endl;

    m_insertionTime = t;

    // We only do the scroll bit if cx is in the right two-thirds of
    // the window
    
    if (cx < (getCanvasView()->contentsX() +
              getCanvasView()->visibleWidth() / 3)) {

        m_deferredCursorMove = CursorMoveOnly;
    } else {
        m_deferredCursorMove = CursorMoveAndScrollToPosition;
        m_deferredCursorScrollToX = cx;
    }

    if (updateNow) doDeferredCursorMove();
}

void
NotationView::doDeferredCursorMove()
{
    NOTATION_DEBUG << "NotationView::doDeferredCursorMove: m_deferredCursorMove == " << m_deferredCursorMove << endl;

    if (m_deferredCursorMove == NoCursorMoveNeeded) {
        return;
    }
    
    DeferredCursorMoveType type = m_deferredCursorMove;
    m_deferredCursorMove = NoCursorMoveNeeded;

    timeT t = m_insertionTime;

    if (m_staffs.size() == 0) return;
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    if (t < segment.getStartTime()) {
        t = segment.getStartTime();
    }
    if (t > segment.getEndTime()) {
        t = segment.getEndTime();
    }

    NotationElementList::iterator i = 
        staff->getViewElementList()->findNearestTime(t);

    while (i != staff->getViewElementList()->end() &&
           !static_cast<NotationElement*>(*i)->getCanvasItem()) ++i;

    if (i == staff->getViewElementList()->end()) {
        //!!! ???
        if (m_insertionTime >= staff->getSegment().getStartTime()) {
            i = staff->getViewElementList()->begin();
        }
        m_insertionTime = staff->getSegment().getStartTime();
    } else {
        m_insertionTime = static_cast<NotationElement*>(*i)->getViewAbsoluteTime();
    }

    if (i == staff->getViewElementList()->end() ||
        t == segment.getEndTime() ||
        t == segment.getBarStartForTime(t)) {

        staff->setInsertCursorPosition(*m_hlayout, t);

        if (type == CursorMoveAndMakeVisible) {
	    double cx;
	    int cy;
	    staff->getInsertCursorPosition(cx, cy);
	    getCanvasView()->slotScrollHoriz(int(cx));
	    getCanvasView()->slotScrollVertSmallSteps(cy);
        }

    } else {

        // prefer a note or rest, if there is one, to a non-spacing event
        if (!static_cast<NotationElement*>(*i)->isNote() &&
	    !static_cast<NotationElement*>(*i)->isRest()) {
            NotationElementList::iterator j = i;
            while (j != staff->getViewElementList()->end()) {
		if (static_cast<NotationElement*>(*j)->getViewAbsoluteTime() !=
		    static_cast<NotationElement*>(*i)->getViewAbsoluteTime()) break;
		if (static_cast<NotationElement*>(*j)->getCanvasItem()) {
		    if (static_cast<NotationElement*>(*j)->isNote() ||
			static_cast<NotationElement*>(*j)->isRest()) {
			i = j;
			break;
		    }
		}
                ++j;
            }
        }

	if (static_cast<NotationElement*>(*i)->getCanvasItem()) {

	    staff->setInsertCursorPosition
		(static_cast<NotationElement*>(*i)->getCanvasX() - 2,
		 int(static_cast<NotationElement*>(*i)->getCanvasY()));
	    
	    if (type == CursorMoveAndMakeVisible) {
		getCanvasView()->slotScrollHoriz
		    (int(static_cast<NotationElement*>(*i)->getCanvasX()) - 4);
	    }
	} else {
	    std::cerr << "WARNING: No canvas item for this notation element:";
	    (*i)->event()->dump(std::cerr);
	}
    }

    if (type == CursorMoveAndScrollToPosition) {

        // get current canvas x of insert cursor, which might not be
        // what we just set
    
        double ccx = 0.0;

        NotationElementList::iterator i = 
            staff->getViewElementList()->findTime(t);

	if (i == staff->getViewElementList()->end()) {
	    if (i == staff->getViewElementList()->begin()) return;
	    double lx, lwidth;
	    --i;
	    if (static_cast<NotationElement*>(*i)->getCanvasItem()) {
		ccx = static_cast<NotationElement*>(*i)->getCanvasX();
		static_cast<NotationElement*>(*i)->getLayoutAirspace(lx, lwidth);
	    } else {
		std::cerr << "WARNING: No canvas item for this notation element*:";
		(*i)->event()->dump(std::cerr);
	    }		
	    ccx += lwidth;
	} else {
	    if (static_cast<NotationElement*>(*i)->getCanvasItem()) {
		ccx = static_cast<NotationElement*>(*i)->getCanvasX();
	    } else {
		std::cerr << "WARNING: No canvas item for this notation element*:";
		(*i)->event()->dump(std::cerr);
	    }
	}
        
	QScrollBar* hbar = getCanvasView()->horizontalScrollBar();
	hbar->setValue(int(hbar->value() - (m_deferredCursorScrollToX - ccx)));
    }

    updateView();
}

void
NotationView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
NotationView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

void
NotationView::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

//----------------------------------------
// Accidentals
//----------------------------------------
void NotationView::slotNoAccidental()
{
    emit changeAccidental(Rosegarden::Accidentals::NoAccidental, false);
}

void NotationView::slotFollowAccidental()
{
    emit changeAccidental(Rosegarden::Accidentals::NoAccidental, true);
}

void NotationView::slotSharp()
{
    emit changeAccidental(Rosegarden::Accidentals::Sharp, false);
}

void NotationView::slotFlat()
{
    emit changeAccidental(Rosegarden::Accidentals::Flat, false);
}

void NotationView::slotNatural()
{
    emit changeAccidental(Rosegarden::Accidentals::Natural, false);
}

void NotationView::slotDoubleSharp()
{
    emit changeAccidental(Rosegarden::Accidentals::DoubleSharp, false);
}

void NotationView::slotDoubleFlat()
{
    emit changeAccidental(Rosegarden::Accidentals::DoubleFlat, false);
}


//----------------------------------------
// Clefs
//----------------------------------------
void NotationView::slotTrebleClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-treble")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Treble);
    setMenuStates();
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-tenor")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Tenor);
    setMenuStates();
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-alto")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Alto);
    setMenuStates();
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("clef-bass")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Bass);
    setMenuStates();
}



void NotationView::slotText()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    setTool(m_toolBox->getTool(TextInserter::ToolName));
    setMenuStates();
}


void NotationView::slotFretboard()
{
    m_currentNotePixmap->setPixmap
    (NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("text")));
    setTool(m_toolBox->getTool(FretboardInserter::ToolName));
    setMenuStates();	
}


//----------------------------------------
// Editing Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    NOTATION_DEBUG << "NotationView::slotEraseSelected()" << endl;
    setTool(m_toolBox->getTool(NotationEraser::ToolName));
    setMenuStates();
}

void NotationView::slotSelectSelected()
{
    NOTATION_DEBUG << "NotationView::slotSelectSelected()" << endl;
    setTool(m_toolBox->getTool(NotationSelector::ToolName));
    setMenuStates();
}




void NotationView::slotLinearMode()
{
    setPageMode(LinedStaff::LinearMode);
}

void NotationView::slotContinuousPageMode()
{
    setPageMode(LinedStaff::ContinuousPageMode);
}

void NotationView::slotMultiPageMode()
{
    setPageMode(LinedStaff::MultiPageMode);
}

void NotationView::slotToggleChordsRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void NotationView::slotToggleRawNoteRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_rawNoteRuler, "show_raw_note_ruler");
}

void NotationView::slotToggleTempoRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void NotationView::slotToggleAnnotations()
{
    m_annotationsVisible = !m_annotationsVisible;
    slotUpdateAnnotationsStatus();
//!!! use refresh mechanism
    refreshSegment(0, 0, 0);
}

void NotationView::slotEditLyrics()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    LyricEditDialog dialog(this, &segment);

    if (dialog.exec() == QDialog::Accepted) {
        
        SetLyricsCommand *command = new SetLyricsCommand
            (&segment, dialog.getLyricData());

        addCommandToHistory(command);
    }
}

//----------------------------------------------------------------------

void NotationView::slotItemPressed(int height, int staffNo,
                                   QMouseEvent* e,
                                   NotationElement* el)
{
    NOTATION_DEBUG << "NotationView::slotItemPressed(height = "
                         << height << ", staffNo = " << staffNo
                         << ")" << endl;

    if (staffNo < 0 && el != 0) {
	// We have an element but no staff -- that's because the
	// element extended outside the staff region.  But we need
	// to handle it properly, so we rather laboriously need to
	// find out which staff it was.
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    if (m_staffs[i]->getViewElementList()->findSingle(el) !=
		m_staffs[i]->getViewElementList()->end()) {
		staffNo = m_staffs[i]->getId();
		break;
	    }
	}
    }

    ButtonState btnState = e->state();

    if (btnState & ControlButton) { // on ctrl-click, set cursor position

        slotSetInsertCursorPosition(e->x(), (int)e->y());

    } else {

        setActiveItem(0);

        timeT unknownTime = 0;

        if (e->type() == QEvent::MouseButtonDblClick) {
            m_tool->handleMouseDoubleClick(unknownTime, height,
					   staffNo, e, el);
	} else {
            m_tool->handleMousePress(unknownTime, height,
                                     staffNo, e, el);
	}
    }
}

void NotationView::slotNonNotationItemPressed(QMouseEvent *e, QCanvasItem *it)
{
    if (e->type() != QEvent::MouseButtonDblClick) return;

    NotationStaff *staff = dynamic_cast<NotationStaff *>
	(getStaffForCanvasCoords(e->x(), e->y()));
    if (!staff) return;

    NOTATION_DEBUG << "NotationView::slotNonNotationItemPressed(doubly)" << endl;

    if (dynamic_cast<QCanvasStaffNameSprite *>(it)) {

	std::string name =
	    staff->getSegment().getComposition()->
	    getTrackById(staff->getSegment().getTrack())->getLabel();

	bool ok = false;
	QRegExpValidator validator(QRegExp(".*"), this); // empty is OK

	QString newText = KLineEditDlg::getText(QString("Change staff name"),
						QString("Enter new staff name"),
						strtoqstr(name),
						&ok,
						this,
						&validator);

	if (ok) {
	    addCommandToHistory(new RenameTrackCommand
				(staff->getSegment().getComposition(),
				 staff->getSegment().getTrack(),
				 qstrtostr(newText)));

	    emit staffLabelChanged(staff->getSegment().getTrack(), newText);
	}

    } else if (dynamic_cast<QCanvasTimeSigSprite *>(it)) {

	double layoutX = (dynamic_cast<QCanvasTimeSigSprite *>(it))->getLayoutX();
	emit editTimeSignature(m_hlayout->getTimeForX(layoutX));
    }
}

void NotationView::slotTextItemPressed(QMouseEvent *e, QCanvasItem *it)
{
    if (e->type() != QEvent::MouseButtonDblClick) return;

    if (it == m_title) {
	emit editMetadata(strtoqstr(Rosegarden::CompositionMetadataKeys::Title.getName()));
    } else if (it == m_subtitle) {
	emit editMetadata(strtoqstr(Rosegarden::CompositionMetadataKeys::Subtitle.getName()));
    } else if (it == m_composer) {
	emit editMetadata(strtoqstr(Rosegarden::CompositionMetadataKeys::Composer.getName()));
    } else if (it == m_copyright) {
	emit editMetadata(strtoqstr(Rosegarden::CompositionMetadataKeys::Copyright.getName()));
    } else {
	return;
    }
   
    positionStaffs();
}

void NotationView::slotMouseMoved(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        updateView();
    } else {
        int follow = m_tool->handleMouseMove(0, 0, // unknown time and height
                                             e);

        if(getCanvasView()->isTimeForSmoothScroll()) {
            
            if (follow & RosegardenCanvasView::FollowHorizontal) {
                getCanvasView()->slotScrollHorizSmallSteps(e->x());
            }

            if (follow & RosegardenCanvasView::FollowVertical) {
                getCanvasView()->slotScrollVertSmallSteps(e->y());
            }

        }
    }
}

void NotationView::slotMouseReleased(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }
    else
        m_tool->handleMouseRelease(0, 0, // unknown time and height
                                   e);
}

void
NotationView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NotationView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    Rosegarden::RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    QString message;
    QString format("%ld (%ld.%03lds)");
    format = i18n("Time: %1").arg(format);
    message.sprintf(format, t, rt.sec, ms);

    m_hoveredOverAbsoluteTime->setText(message);
}

// Ignore velocity for the moment -- we need the option to use or ignore it
void
NotationView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    //!!! Problematic.  Ideally we wouldn't insert events into windows
    //that weren't actually visible, otherwise all hell could break
    //loose (metaphorically speaking, I should probably add).  I did
    //think of checking isActiveWindow() and returning if the current
    //window wasn't active, but that will prevent anyone from
    //step-recording from e.g. vkeybd, which cannot be used without
    //losing focus (and thus active-ness) from the Rosegarden window.

    //!!! I know -- we'll keep track of which edit view (or main view,
    //or mixer, etc) is active, and we'll only allow insertion into
    //the most recently activated.  How about that?

    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (!action->isChecked()) return;

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
	static bool showingError = false;
	if (showingError) return;
	showingError = true;
	KMessageBox::sorry(this, i18n("Can't insert note: No note duration selected"));
	showingError = false;
        return;
    }

    if (m_inPaintEvent) {
	NOTATION_DEBUG << "NotationView::slotInsertableNoteEventReceived: in paint event already" << endl;
	if (noteOn) {
	    m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
	}
	return;
    }

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment.getTranspose();

//    KTmpStatusMsg msg(i18n("Inserting note"), this);

    // We need to ensure that multiple notes hit at once come out as
    // chords, without imposing the interpretation that overlapping
    // notes are always chords and without getting too involved with
    // the actual absolute times of the notes (this is still step
    // editing, not proper recording).

    // First, if we're in chord mode, there's no problem.

    if (isInChordMode()) {
	if (!noteOn) return;
	NOTATION_DEBUG << "Inserting note in chord at pitch " << pitch << endl;
	noteInserter->insertNote(segment, getInsertionTime(), pitch,
				 Rosegarden::Accidentals::NoAccidental,
				 true);

    } else {

	if (noteOn) {

	    // Rules:
	    //
	    // * If no other note event has turned up within half a
	    //   second, insert this note and advance.
	    // 
	    // * Relatedly, if this note is within half a second of
	    //   the previous one, they're chords.  Insert the previous
	    //   one, don't advance, and use the same rules for this.
	    // 
	    // * If a note event turns up before that time has elapsed,
	    //   we need to wait for the note-off events: if the second
	    //   note happened less than half way through the first,
	    //   it's a chord.

	    // We haven't implemented these yet... For now:
	    noteInserter->insertNote(segment, getInsertionTime(), pitch,
				     Rosegarden::Accidentals::NoAccidental,
				     true);
	}
    }
}

void
NotationView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
NotationView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
NotationView::slotInsertableTimerElapsed()
{
}

void
NotationView::slotToggleStepByStep()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (action->isChecked()) { // after toggling, that is
	emit stepByStepTargetRequested(this);
    } else {
	emit stepByStepTargetRequested(0);
    }
}

void
NotationView::slotStepByStepTargetRequested(QObject *obj)
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    action->setChecked(obj == this);
}

void
NotationView::slotCheckRendered(double cx0, double cx1)
{
//    NOTATION_DEBUG << "slotCheckRendered(" << cx0 << "," << cx1 << ")" << endl;

    bool something = false;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

	NotationStaff *staff = m_staffs[i];

	LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
	    (cx0, 0);

	LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
	    (cx1, staff->getTotalHeight() + staff->getY());

	timeT t0 = m_hlayout->getTimeForX(cc0.first);
	timeT t1 = m_hlayout->getTimeForX(cc1.first);

	if (staff->checkRendered(t0, t1)) something = true;
    }

    if (something) {
	emit renderComplete();
	if (m_renderTimer) delete m_renderTimer;
	m_renderTimer = new QTimer(this);
	connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	m_renderTimer->start(0, true);
    }

    if (m_deferredCursorMove != NoCursorMoveNeeded) doDeferredCursorMove();
}

void
NotationView::slotRenderSomething()
{
    delete m_renderTimer;
    m_renderTimer = 0;
    static clock_t lastWork = 0;
    
    clock_t now = clock();
    long elapsed = ((now - lastWork) * 1000 / CLOCKS_PER_SEC);
    if (elapsed < 70) {
	m_renderTimer = new QTimer(this);
	connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	m_renderTimer->start(0, true);
	return;
    }
    lastWork = now;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

	if (m_staffs[i]->doRenderWork(m_staffs[i]->getSegment().getStartTime(),
				      m_staffs[i]->getSegment().getEndTime())) {
	    m_renderTimer = new QTimer(this);
	    connect(m_renderTimer, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	    m_renderTimer->start(0, true);
	    return;
	}
    }

    PixmapArrayGC::deleteAll();
    NOTATION_DEBUG << "NotationView::slotRenderSomething: updating thumbnails" << endl;
    updateThumbnails(true);
}

void
NotationView::slotEditTempos(Rosegarden::timeT t)
{
    TempoView *tempoView = new TempoView(getDocument(), this, t);

    connect(tempoView,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               Rosegarden::tempoT,
			       TempoDialog::TempoDialogAction)),
	    RosegardenGUIApp::self(),
            SLOT(slotChangeTempo(Rosegarden::timeT,
                                 Rosegarden::tempoT,
				 TempoDialog::TempoDialogAction)));

    connect(tempoView, SIGNAL(saveFile()),
	    RosegardenGUIApp::self(), SLOT(slotFileSave()));

    tempoView->show();
}
