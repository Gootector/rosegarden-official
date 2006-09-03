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

#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <vector>

#include <qcanvas.h>
#include <qwmatrix.h>
#include <kmainwindow.h>

#include "Staff.h"
#include "RulerScale.h"
#include "SnapGrid.h"
#include "Quantizer.h"
#include "MappedInstrument.h"
#include "PropertyName.h"
#include "SequencerDataBlock.h" // for LevelInfo

#include "editview.h"
#include "matrixcanvasview.h"
#include "matrixelement.h"
#include "matrixhlayout.h"
#include "matrixvlayout.h"

namespace Rosegarden { 
    class Segment;
    class EventSelection;
    class MappedEvent;
}

class RosegardenGUIDoc;
class MatrixStaff;
class MatrixCanvasView;
class PitchRuler;
class InstrumentParameterBox;
class KComboBox;
template <class T> class ZoomSlider;
class ControlRuler;
class ChordNameRuler;
class PropertyViewRuler;
class PropertyBox;

class QDeferScrollView;
class QMouseEvent;
class QLabel;
class QWidget;


typedef std::vector<MatrixElement*> SelectedElements;

/**
 * Matrix ("Piano Roll") View
 *
 * Note: we currently display only one staff
 */
class MatrixView : public EditView
{
    Q_OBJECT

    friend class MatrixSelector;

public:
    MatrixView(RosegardenGUIDoc *doc,
               std::vector<Rosegarden::Segment *> segments,
               QWidget *parent, bool drumMode);

    virtual ~MatrixView();

    static const char* const ConfigGroup;

    virtual bool applyLayout(int staffNo = -1,
			     Rosegarden::timeT startTime = 0,
			     Rosegarden::timeT endTime = 0);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    QCanvas* canvas() { return getCanvasView()->canvas(); }

    void setCanvasCursor(const QCursor &cursor) {
	getCanvasView()->viewport()->setCursor(cursor);
    }

    MatrixStaff* getStaff(int i)
    {
        if (i >= 0 && unsigned(i) < m_staffs.size()) return m_staffs[i];
        else return 0;
    }

    MatrixStaff *getStaff(const Rosegarden::Segment &segment);

    virtual void updateView();

    bool isDrumMode() { return m_drumMode; }

    /**
     * Discover whether chord-mode insertions are enabled (as opposed
     * to the default melody-mode)
     */
    bool isInChordMode();

    /**
     * Set the current event selection.
     *
     * If preview is true, sound the selection as well.
     *
     * If redrawNow is true, recolour the elements on the canvas;
     * otherwise just line up a refresh for the next paint event.
     *
     * (If the selection has changed as part of a modification to a
     * segment, redrawNow should be unnecessary and undesirable, as a
     * paint event will occur in the next event loop following the
     * command invocation anyway.)
     */
    virtual void setCurrentSelection(Rosegarden::EventSelection* s,
				     bool preview = false,
				     bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(int staffNo,
                                Rosegarden::Event *event,
				bool preview = false,
				bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(Rosegarden::Segment &segment,
                                Rosegarden::Event *event,
				bool preview = false,
				bool redrawNow = false);


    /**
     * Play a Note Event using the keyPressed() signal
     */
    void playNote(Rosegarden::Event *event);

    /**
     * Play a preview (same as above but a simpler interface)
     */
    void playNote(const Rosegarden::Segment &segment, int pitch, int velocity = -1);

    /**
     * Get the SnapGrid
     */
    Rosegarden::SnapGrid getSnapGrid() { return *m_snapGrid; }

    /**
     * Add a ruler that allows control of a single property -
     * return the number of the added ruler
     * 
     */
    unsigned int addPropertyViewRuler(const Rosegarden::PropertyName &property);

    /**
     * Remove a control ruler - return true if it's a valid ruler number
     */
    bool removePropertyViewRuler(unsigned int number);

    /**
     * Adjust an X coord by world matrix
     */
    double getXbyWorldMatrix(double value)
    { return m_canvasView->worldMatrix().m11() * value; }
        
    double getXbyInverseWorldMatrix(double value)
    { return m_canvasView->inverseWorldMatrix().m11() * value; }

    QPoint inverseMapPoint(const QPoint& p) { return m_canvasView->inverseMapPoint(p); }

    /*
     * Repaint the control rulers
     *
     */
    void repaintRulers();

    /*
     * Readjust the canvas size
     *
     */
    void readjustCanvasSize();

    /*
     * Scrolls the view such that the given time is centered
     */
    void scrollToTime(Rosegarden::timeT t);

    /**
     * Get the local keyMapping (when in drum mode)
     */
    Rosegarden::MidiKeyMapping *getKeyMapping() { return m_localMapping; }

signals:
    /**
     * Emitted when the selection has been cut or copied
     *
     * @see MatrixSelector#hideSelection
     */
    void usedSelection();

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void jumpPlaybackTo(Rosegarden::timeT);
    void panic();

    void stepByStepTargetRequested(QObject *);

    void editTriggerSegment(int);
    
    void editTimeSignature(Rosegarden::timeT);

public slots:

    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut();

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste();

    /**
     * Delete the current selection
     */
    void slotEditDelete();

    virtual void slotStepBackward(); // override from EditView
    virtual void slotStepForward(); // override from EditView

    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();

    /**
     * Filter selection by event type
     */
    void slotFilterSelection(); // dummy - not actually functional yet

    /// edition tools
    void slotPaintSelected();
    void slotEraseSelected();
    void slotSelectSelected();
    void slotMoveSelected();
    void slotResizeSelected();

    void slotToggleStepByStep();

    /// status stuff
    void slotUpdateInsertModeStatus();

    /// transforms
    void slotTransformsQuantize();
    void slotTransformsRepeatQuantize();
    void slotTransformsLegato();
    void slotRescale();
    void slotDoubleSpeed();
    void slotHalfSpeed();
    void slotVelocityUp();
    void slotVelocityDown();

    /// settings
    void slotToggleChordsRuler();
    void slotToggleTempoRuler();

    /// cursor moves
    void slotJumpCursorToPlayback();
    void slotJumpPlaybackToCursor();
    void slotToggleTracking();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a matrix element
     * or somewhere on the staff
     */
    void slotMousePressed(Rosegarden::timeT time, int pitch,
                          QMouseEvent*, MatrixElement*);

    void slotMouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent*);
    void slotMouseReleased(Rosegarden::timeT time, int pitch, QMouseEvent*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChanged()
     */
    void slotHoveredOverNoteChanged(int evPitch);

    /**
     * Called when the mouse cursor moves over a different key on
     * the piano keyboard
     *
     * @see PianoKeyboard#hoveredOverKeyChanged()
     */
    void slotHoveredOverKeyChanged(unsigned int);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see MatrixCanvasView#hoveredOverNoteChange()
     */
    void slotHoveredOverAbsoluteTimeChanged(unsigned int);

    /**
     * Set the time pointer position during playback
     */
    void slotSetPointerPosition(Rosegarden::timeT time);

    /**
     * Set the time pointer position during playback
     */
    void slotSetPointerPosition(Rosegarden::timeT time,
				bool scroll);

    /**
     * Set the insertion pointer position (from the bottom LoopRuler)
     */
    void slotSetInsertCursorPosition(Rosegarden::timeT position, bool scroll);

    virtual void slotSetInsertCursorPosition(Rosegarden::timeT position) {
	slotSetInsertCursorPosition(position, true);
    }

    /**
     * Catch the keyboard being pressed
     */
    void slotKeyPressed(unsigned int y, bool repeating);

    /**
     * Catch the keyboard being released
     */
    void slotKeyReleased(unsigned int y, bool repeating);

    /**
     * Catch the keyboard being pressed with selection modifier
     */
    void slotKeySelected(unsigned int y, bool repeating);

    /**
     * Handle scrolling between view and PianoKeyboard
     */
    void slotVerticalScrollPianoKeyboard(int y);

    /**
     * Close
     */
    void closeWindow();

    /**
     * A new selection has been acquired by a tool
     */
    void slotNewSelection();

    /**
     * Set the snaptime of the grid from an item in the snap combo
     */
    void slotSetSnapFromIndex(int);

    /**
     * Set the snaptime of the grid based on the name of the invoking action
     */
    void slotSetSnapFromAction();

    /**
     * Set the snaptime of the grid
     */
    void slotSetSnap(Rosegarden::timeT);

    /**
     * Quantize a selection to a given level
     */
    void slotQuantizeSelection(int);

    /**
     * Collapse equal pitch notes
     */
    void slotTransformsCollapseNotes();

    /**
     * Pop-up the velocity modification dialog
     */
    void slotSetVelocities();

    /**
     * Pop-up the select trigger segment dialog
     */
    void slotTriggerSegment();

    /**
     * Clear triggers from selection
     */
    void slotRemoveTriggers();

    /**
     * Change horizontal zoom
     */
    void slotChangeHorizontalZoom(int);

    void slotZoomIn();
    void slotZoomOut();

    /**
     * Select all
     */
    void slotSelectAll();

    /**
     * Keyboard insert
     */
    void slotInsertNoteFromAction();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    /// The given QObject has originated a step-by-step-editing request
    void slotStepByStepTargetRequested(QObject *);

    void slotInstrumentLevelsChanged(Rosegarden::InstrumentId,
				     const Rosegarden::LevelInfo &);

protected slots:
    void slotCanvasBottomWidgetHeightChanged(int newHeight);

    /**
     * A new percussion key mapping has to be displayed
     */
    void slotPercussionSetChanged(Rosegarden::Instrument *);

    /**
     * Re-dock the parameters box to its initial position
     */
    void slotDockParametersBack();

    /**
     * The parameters box was closed
     */
    void slotParametersClosed();

    /**
     * The parameters box was docked back
     */
    void slotParametersDockedBack(KDockWidget*, KDockWidget::DockPosition);

    /**
     * The instrument for this track may have changed
     */
    void slotCheckTrackAssignments();
    
protected:
    virtual Rosegarden::RulerScale* getHLayout();

    virtual Rosegarden::Segment *getCurrentSegment();
    virtual Rosegarden::Staff *getCurrentStaff();
    virtual Rosegarden::timeT getInsertionTime();

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void slotSaveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    /**
     * update the current quantize level from selection or entire segment
     */
    virtual void updateQuantizeCombo();

    /**
     * Return the size of the MatrixCanvasView
     */
    virtual QSize getViewSize();

    /**
     * Set the size of the MatrixCanvasView
     */
    virtual void setViewSize(QSize);

    virtual MatrixCanvasView *getCanvasView();

    /**
     * Init matrix actions toolbar
     */
    void initActionsToolbar();

    /**
     * Zoom toolbar
     */
    void initZoomToolbar();

    /**
     * Test whether we've had too many preview notes recently
     */
    bool canPreviewAnotherNote();

    virtual void paintEvent(QPaintEvent* e);

    virtual void updateViewCaption();

    int computePostLayoutWidth();

    /**
     * Get min and max pitches of notes on matrix.
     * Return false if no notes.
     */
    bool getMinMaxPitches(int& minPitch, int& maxPitch);

    /**
     * If necessary, extend local keymapping to contain
     * all notes currently on staff
     */
    void extendKeyMapping();

    //--------------- Data members ---------------------------------

    std::vector<MatrixStaff*> m_staffs;

    MatrixHLayout             m_hlayout;
    MatrixVLayout             m_vlayout;
    Rosegarden::SnapGrid     *m_snapGrid;

    Rosegarden::timeT         m_lastEndMarkerTime;

    // Status bar elements
    QLabel* m_hoveredOverAbsoluteTime;
    QLabel* m_hoveredOverNoteName;
    QLabel *m_selectionCounter;
    QLabel *m_insertModeLabel;

    /**
     * used in slotHoveredOverKeyChanged to track moves over the piano
     * keyboard
     */
    int m_previousEvPitch;

    KDockWidget         *m_dockLeft;
    MatrixCanvasView    *m_canvasView;
    QDeferScrollView    *m_pianoView;
    PitchRuler          *m_pitchRuler;

    Rosegarden::MidiKeyMapping *m_localMapping;

    // The last note we sent in case we're swooshing up and
    // down the keyboard and don't want repeat notes sending
    //
    Rosegarden::MidiByte m_lastNote;

    // The first note we sent in similar case (only used for
    // doing effective sweep selections
    //
    Rosegarden::MidiByte m_firstNote;

    Rosegarden::PropertyName m_selectedProperty;

    // The parameter box
    //
    InstrumentParameterBox *m_parameterBox;

    // Toolbar flora
    //
    KComboBox          *m_quantizeCombo;
    KComboBox          *m_snapGridCombo;
    ZoomSlider<double> *m_hZoomSlider;
    ZoomSlider<double> *m_vZoomSlider;
    QLabel             *m_zoomLabel;
 
    // Hold our matrix quantization values and snap values
    //
    std::vector<Rosegarden::timeT> m_quantizations;
    std::vector<Rosegarden::timeT> m_snapValues;

    std::vector<std::pair<PropertyViewRuler*, PropertyBox*> >  m_propertyViewRulers;

    ChordNameRuler *m_chordNameRuler;
    QWidget        *m_tempoRuler;

    std::vector<std::pair<int, int> > m_pendingInsertableNotes;

    bool m_playTracking;
    bool m_dockVisible;
    bool m_drumMode;
};

// Commented this out - was a MatrixView inner class, but we get a warning
// that Q_OBJECT can't be used in an inner class - gl
//

// class NoteSender : public QObject
// {
//     Q_OBJECT
	
// public:
//     NoteSender(int i, int p) : m_insid(i), m_pitch(p) { }
//     virtual ~NoteSender();
	
// public slots:
// void sendNote();
    
// private:
//     int m_insid, m_pitch;
// };

#endif
