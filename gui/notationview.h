/***************************************************************************
                          notationview.h  -  description
                             -------------------
    begin                : Fri Sep 22 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NOTATIONVIEW_H
#define NOTATIONVIEW_H

#include <kmainwindow.h>

#include "notationelement.h"
#include "viewelementsmanager.h"
#include "notationhlayout.h"
#include "notationvlayout.h"

class QCanvasItem;
class RosegardenGUIDoc;

/**
  *@author Guillaume Laurent, Chris Cannam, Rich Bown
  */

class NotationCanvasView : public QCanvasView
{
public:
    NotationCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                       const char *name=0, WFlags f=0);
    
    /** Callback for a mouse button press event in the canvas */
    virtual void contentsMousePressEvent (QMouseEvent *e);
    /** Callback for a mouse button release event in the canvas */
    virtual void contentsMouseReleaseEvent (QMouseEvent *e);
    /** Callback for a mouse move event in the canvas */
    virtual void contentsMouseMoveEvent (QMouseEvent *e);

protected:
    QCanvasItem* m_movingItem;
    bool m_draggingItem;

};


class NotationView : public KMainWindow
{
    Q_OBJECT
public:

    NotationView(RosegardenGUIDoc* doc, QWidget *parent);
    ~NotationView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    /// draw all elements
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to);

    /// same, with dx,dy offset
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to,
                              double dxoffset, double dyoffset);

    /// same, relative to the specified item
    virtual bool showElements(NotationElementList::iterator from,
                              NotationElementList::iterator to,
                              QCanvasItem*);

    /// Normally calls applyHorizontalLayout() then applyVerticalLayout()
    virtual bool applyLayout();

    /// Set the 'y'-coord on all doc elements - should be called before applyVerticalLayout()
    virtual bool applyHorizontalLayout();

    /// Set the 'x'-coord on all doc elements - should be called after applyHorizontalLayout()
    virtual bool applyVerticalLayout();
    
    void setHorizontalLayoutEngine(NotationHLayout* e) { m_hlayout = e; }
    void setVerticalLayoutEngine(NotationVLayout* e)   { m_vlayout = e; }

    LayoutEngine* getHorizontalLayoutEngine() { return m_hlayout; }
    LayoutEngine* getVerticalLayoutEngine()   { return m_vlayout; }

    QCanvas* canvas() { return m_canvasView->canvas(); }

public slots:

    /** undo
     */
    void slotEditUndo();
    /** redo
     */
    void slotEditRedo();
    
    /** put the marked text/object into the clipboard and remove
     *	it from the document
     */
    void slotEditCut();

    /** put the marked text/object into the clipboard
     */
    void slotEditCopy();

    /** paste the clipboard into the document
     */
    void slotEditPaste();

    /** toggles the toolbar
     */
    void slotToggleToolBar();

    /** toggles the statusbar
     */
    void slotToggleStatusBar();

    /** changes the statusbar contents for the standard label permanently, used to indicate current actions.
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusMsg(const QString &text);

    /** changes the status message of the whole statusbar for two seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for toolbar icons and menuentries.
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(const QString &text);


    // note switch slots
    void slotWhole();
    void slotHalf();
    void slotQuarter();
    void slot8th();
    void slot16th();
    void slot32nd();
    void slot64th();
	
protected:

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    void saveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    void readOptions();

    /**
     * create menus and toolbars
     */
    void setupActions();

    void perfTest();
    void test();

    KConfig* m_config;

    RosegardenGUIDoc* m_document;

    NotationCanvasView* m_canvasView;

    Staff* m_mainStaff;
    Staff* m_currentStaff;

    NotationElementList* m_notationElements;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    Note m_currentSelectedNote;
	
};

#endif
