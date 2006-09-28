
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_EDITVIEW_H_
#define _RG_EDITVIEW_H_

#include "base/PropertyName.h"
#include "EditViewBase.h"
#include "gui/dialogs/TempoDialog.h"
#include <qsize.h>
#include <qstring.h>
#include <qwmatrix.h>
#include <vector>
#include "base/Event.h"


class QWidget;
class QVBoxLayout;
class QVBox;
class QPaintEvent;
class QMouseEvent;
class QCanvasItem;
class KTabWidget;
class Accidental;


namespace Rosegarden
{

class Staff;
class Segment;
class RulerScale;
class RosegardenGUIDoc;
class RosegardenCanvasView;
class PropertyControlRuler;
class Key;
class EventSelection;
class Device;
class ControlRuler;
class ControlParameter;
class ControllerEventsRuler;
class Clef;
class BarButtons;
class ActiveItem;


class EditView : public EditViewBase
{
    Q_OBJECT

public:

    /**
     * Create an EditView for the segments \a segments from document \a doc.
     *
     * \arg cols : number of columns, main column is always rightmost
     *
     */
    EditView(RosegardenGUIDoc *doc,
             std::vector<Segment *> segments,
             unsigned int cols,
             QWidget *parent,
             const char *name = 0);

    virtual ~EditView();

    /**
     * "Clever" readjustment of the view size
     * If the new size is larger, enlarge to that size plus a margin
     * If it is smaller, only shrink if the reduction is significant
     * (e.g. new size is less than 75% of the old one)
     *
     * @arg exact if true, then set to newSize exactly
     */
    virtual void readjustViewSize(QSize newSize, bool exact = false);

    /**
     * Return the active item
     */
    ActiveItem* activeItem() { return m_activeItem; }

    /**
     * Set the active item
     */
    void setActiveItem(ActiveItem* i) { m_activeItem = i; }

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
    virtual void setCurrentSelection(EventSelection* s,
				     bool preview = false,
				     bool redrawNow = false) = 0;

    EventSelection* getCurrentSelection()
        { return m_currentEventSelection; }

    RosegardenCanvasView* getRawCanvasView() { return m_canvasView; }
    virtual RosegardenCanvasView* getCanvasView();

signals:
    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // target value
                     TempoDialog::TempoDialogAction); // tempo action

public slots:
    /**
     * Called when a mouse press occurred on an active canvas item
     *
     * @see ActiveItem
     * @see QCanvasItem#setActive
     */
    virtual void slotActiveItemPressed(QMouseEvent*, QCanvasItem*);

    virtual void slotSetInsertCursorPosition(timeT position) = 0;
    
    void slotExtendSelectionBackward();
    void slotExtendSelectionForward();
    void slotExtendSelectionBackwardBar();
    void slotExtendSelectionForwardBar();
    void slotExtendSelectionBackward(bool bar);
    void slotExtendSelectionForward(bool bar);

    virtual void slotStepBackward(); // default is event-by-event
    virtual void slotStepForward(); // default is event-by-event
    void slotJumpBackward();
    void slotJumpForward();
    void slotJumpToStart();
    void slotJumpToEnd();

    void slotAddTempo();
    void slotAddTimeSignature();

    virtual void slotShowVelocityControlRuler();
    virtual void slotShowControllerEventsRuler();
    virtual void slotShowPropertyControlRuler();

    // rescale
    void slotHalfSpeed();
    void slotDoubleSpeed();
    void slotRescale();

    // transpose
    void slotTranspose();
    void slotTransposeUp();
    void slotTransposeUpOctave();
    void slotTransposeDown();
    void slotTransposeDownOctave();

    // invert
    void slotInvert();
    void slotRetrograde();
    void slotRetrogradeInvert();

    // jog events
    void slotJogLeft();
    void slotJogRight();

    // Control ruler actions
    //
    void slotInsertControlRulerItem();
    void slotEraseControlRulerItem();
    void slotClearControlRulerItem();
    void slotStartControlLineItem();
    void slotFlipForwards();
    void slotFlipBackwards();

    // Property ruler actions
    //
    void slotDrawPropertyLine();
    void slotSelectAllProperties();

    // add control ruler
    void slotAddControlRuler(int);
    void slotRemoveControlRuler(QWidget*);

protected:
    virtual RulerScale* getHLayout() = 0;

    QVBox* getBottomWidget() { return m_bottomBox; }

    virtual void updateBottomWidgetGeometry();
    
    virtual void paintEvent(QPaintEvent* e);

    /**
     * Locate the given widget in the top bar-buttons position and
     * connect up its scrolling signals.
     */
    void setTopBarButtons(BarButtons*);

    /**
     * Locate the given widget in the bottom bar-buttons position and
     * connect up its scrolling signals.
     */
    void setBottomBarButtons(BarButtons*);

    /**
     * Set the 'Rewind' and 'Fast Forward' buttons in the transport
     * toolbar to AutoRepeat
     */
    void setRewFFwdToAutoRepeat();

    /**
     * Locate the given widget right above the top bar-buttons and
     * connect up its scrolling signals.
     * The widget has to have a slotScrollHoriz(int) slot
     */
    void addRuler(QWidget*);

    /**
     * Add a ruler control box
     */
    void addPropertyBox(QWidget*);

    /**
     * Make a control ruler for the given property,
     */
    PropertyControlRuler* makePropertyControlRuler(PropertyName propertyName);

    /**
     * Make a ruler for controller events
     */
    ControllerEventsRuler* makeControllerEventRuler(const ControlParameter *controller = 0);

    /**
     * Add control ruler
     */
    void addControlRuler(ControlRuler* ruler);

    /**
     * Update all control rulers
     */
    void updateControlRulers(bool updateHPos=false);

    /**
     * Set zoom factor of control rulers
     */
    void setControlRulersZoom(QWMatrix);

    /**
     * Set current segment for control rulers
     */
    void setControlRulersCurrentSegment();

    /**
     * Find the control ruler for the given property name
     * if it's already been created, return 0 otherwise
     */
    ControlRuler* findRuler(PropertyName propertyName, int &index);

    /**
     * Find the control ruler for the given controller
     * if it's already been created, return 0 otherwise
     */
    ControlRuler* findRuler(const ControlParameter& controller, int &index);

    /**
     * Show a control ruler for the given property
     * If the ruler already exists, activate the tab it's in,
     * otherwise create the ruler and add it to the control rulers tab
     * widget
     */ 
    void showPropertyControlRuler(PropertyName propertyName);
    
    /**
     * Return the control ruler currently displayed, or 0 if none exist
     */
    ControlRuler* getCurrentControlRuler();
    
    /**
     * Set up those actions common to any EditView (e.g. note insertion,
     * time signatures etc)
     */
    void setupActions();

    /**
     * Set up the 'Add control ruler' sub-menu
     */
    void setupAddControlRulerMenu();

    /**
     * Do this after any other segment setup in a subordinate view.
     * Returns true if there were any tabs to set up.
     */
    bool setupControllerTabs();

    /**
     * Create an action menu for inserting notes from the PC keyboard,
     * and add it to the action collection.  This is one of the methods
     * called by setupActions().
     */
    void createInsertPitchActionMenu();

    /**
     * Get a note pitch from an action name (where the action is one of
     * those created by createInsertPitchActionMenu).  Can throw an
     * Exception to mean that the action is not an insert one.  Also
     * returns any specified accidental through the reference arg.
     */
    int getPitchFromNoteInsertAction(QString actionName,
				     Accidental &acc,
				     const Clef &clef,
				     const Key &key);

    /**
     * Abstract method to get the view size
     * Typically implemented as canvas()->size().
     */
    virtual QSize getViewSize() = 0;

    /**
     * Abstract method to set the view size
     * Typically implemented as canvas()->resize().
     */
    virtual void setViewSize(QSize) = 0;

    /**
     * Abstract method to get current insert-pointer time
     */
    virtual timeT getInsertionTime() = 0;

    /**
     * Return the time at which the insert cursor may be found,
     * and the time signature, clef and key at that time.  Default
     * implementation is okay but slow.
     */ 
    virtual timeT getInsertionTime(Clef &clef,
					       Key &key);

    /**
     * Abstract method to get current staff (the returned staff will be
     * that representing the segment of getCurrentSegment())
     */
    virtual Staff *getCurrentStaff() = 0;

    /**
     * Return the device of the current segment, if any
     */
    Device *getCurrentDevice();

    virtual void setCanvasView(RosegardenCanvasView *cv);

    //--------------- Data members ---------------------------------

    /// The current selection of Events (for cut/copy/paste)
    EventSelection* m_currentEventSelection;

    ActiveItem* m_activeItem;

    RosegardenCanvasView *m_canvasView;

    QVBoxLayout  *m_rulerBox;
    QVBoxLayout  *m_controlBox;
    QVBox        *m_bottomBox;
    BarButtons   *m_topBarButtons;
    BarButtons   *m_bottomBarButtons;
    ControlRuler *m_controlRuler;
    KTabWidget   *m_controlRulers;
    QWMatrix      m_currentRulerZoomMatrix;

    static const unsigned int RULERS_ROW;
    static const unsigned int CONTROLS_ROW;
    static const unsigned int TOPBARBUTTONS_ROW;
    static const unsigned int CANVASVIEW_ROW;
    static const unsigned int CONTROLRULER_ROW;
};


}

#endif
