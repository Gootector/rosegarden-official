
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NEW_NOTATION_VIEW_H_
#define _RG_NEW_NOTATION_VIEW_H_

#include <vector>
#include <QMainWindow>

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"

class QWidget;

namespace Rosegarden
{

class RosegardenDocument;
class NotationWidget;
class Segment;
class CommandRegistry;
	
class NewNotationView : public EditViewBase,
                        public SelectionManager
{
    Q_OBJECT

public:
    NewNotationView(RosegardenDocument *doc,
                    std::vector<Segment *> segments,
                    QWidget *parent = 0);

    virtual ~NewNotationView();

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview = false);

    virtual void initStatusBar() { }//!!!
    virtual void updateViewCaption() { }//!!!

    virtual timeT getInsertionTime() const;

signals:
    void play();
    void stop();
    void rewindPlayback();
    void fastForwardPlayback();
    void rewindPlaybackToBeginning();
    void fastForwardPlaybackToEnd();
    void panic();


protected slots:
    void slotPrintLilyPond();
    void slotPreviewLilyPond();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();
    void slotEditCutAndClose();
    void slotEditGeneralPaste();
    void slotEditAddClef();
    void slotEditAddKeySignature();
    void slotEditAddSustainDown();
    void slotEditAddSustainUp();
    void slotEditAddSustain(bool down);
    void slotEditTranspose();
    void slotEditSwitchPreset();


    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotEditSelectFromStart();
    void slotEditSelectToEnd();
    void slotEditSelectWholeStaff();
    void slotFilterSelection();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();

    void slotSetSelectTool();
    void slotSetEraseTool();

    void slotSwitchToNotes();
    void slotSwitchToRests();
    void slotNoteAction();
    void slotNoAccidental();
    void slotFollowAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();
    void slotClefAction();
    void slotText();
    void slotGuitarChord();

    void slotLinearMode();
    void slotContinuousPageMode();
    void slotMultiPageMode();

    void slotShowHeadersGroup();

    void slotChangeFontFromAction();
    void slotChangeFontSizeFromAction();

    void slotUpdateMenuStates();

    void slotTransformsInterpret();

    void slotMakeOrnament();
    void slotUseOrnament();
    void slotRemoveOrnament();

    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotUpdateInsertModeStatus();

    /// Show or hide rulers
    void slotToggleChordsRuler();
    void slotToggleRawNoteRuler();
    void slotToggleTempoRuler();

    void slotToggleGeneralToolBar();
    void slotToggleToolsToolBar();
    void slotToggleNotesToolBar();
    void slotToggleRestsToolBar();
    void slotToggleAccidentalsToolBar();
    void slotToggleClefsToolBar();
    void slotToggleMetaToolBar();
    void slotToggleMarksToolBar();
    void slotToggleGroupToolBar();
    void slotToggleSymbolsToolBar();
    void slotToggleLayoutToolBar();
    void slotToggleTransportToolBar();

    void slotToggleTracking();

    /**
     * Insert a segno Symbol
     */
    void slotAddSegno();

    /** 
     * Insert a coda Symbol
     */
    void slotAddCoda();

    /**
     * Insert a breath mark Symbol
     */
    void slotAddBreath();

private:
    /**
     * export a LilyPond file (used by slotPrintLilyPond and
     * slotPreviewLilyPond)
     */
    bool exportLilyPondFile(QString url, bool forPreview = false);

    /** 
     * Use QTemporaryFile to obtain a tmp filename that is guaranteed to be unique.
     */
    QString getLilyPondTmpFilename();

    /**
     * Get the average velocity of the selected notes
     */
    int getVelocityFromSelection();

    /**
     * Helper function to toggle a toolbar given its name
     * If \a force point to a bool, then the bool's value
     * is used to show/hide the toolbar.
     */
    void toggleNamedToolBar(const QString& toolBarName, bool* force = 0);


    RosegardenDocument *m_document;
    NotationWidget *m_notationWidget;
    CommandRegistry *m_commandRegistry;

    /** Curiously enough, the window geometry code never fired in the dtor.  I
     * can only conclude the dtor is never being called for some reason, and
     * it's probably a memory leak for the command registry object it wants to
     * delete, but I won't try to work that one out at the moment.  I'll just
     * implement closeEvent() and whistle right on past that other thing.
     */
    void closeEvent(QCloseEvent *event);
    void setupActions();
    void updateWindowTitle();
    bool isInChordMode();
    bool isInTripletMode();
    bool isInGraceMode();
};

}

#endif
