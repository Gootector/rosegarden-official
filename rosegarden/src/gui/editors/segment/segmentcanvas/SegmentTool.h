
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

#ifndef _RG_SEGMENTTOOL_H_
#define _RG_SEGMENTTOOL_H_

#include "gui/general/BaseTool.h"
#include "CompositionItem.h"
#include <qpoint.h>
#include <utility>
#include <vector>


class QMouseEvent;
class KCommand;


namespace Rosegarden
{

class SegmentToolBox;
class RosegardenGUIDoc;
class CompositionView;


//////////////////////////////////////////////////////////////////////

class SegmentToolBox;
class SegmentSelector;

// Allow the tools to share the Selector tool's selection
// through these.
//
typedef std::pair<QPoint, CompositionItem> SegmentItemPair;
typedef std::vector<SegmentItemPair> SegmentItemList;

class SegmentTool : public BaseTool
{
public:
    friend class SegmentToolBox;

    virtual ~SegmentTool();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) when
     * the tool is set as current.
     * Add any setup here
     */
    virtual void ready();

    virtual void handleRightButtonPress(QMouseEvent*);
    virtual void handleMouseButtonPress(QMouseEvent*)     = 0;
    virtual void handleMouseButtonRelease(QMouseEvent*)   = 0;
    virtual int  handleMouseMove(QMouseEvent*)            = 0;

    void addCommandToHistory(KCommand *command);

protected:
    SegmentTool(CompositionView*, RosegardenGUIDoc *doc);

    virtual void createMenu();
    virtual bool hasMenu() { return true; }
    
    void setCurrentItem(CompositionItem item) { if (item != m_currentItem) { delete m_currentItem; m_currentItem = item; } }

    SegmentToolBox* getToolBox();

    bool changeMade() { return m_changeMade; }
    void setChangeMade(bool c) { m_changeMade = c; }

    //--------------- Data members ---------------------------------

    CompositionView*  m_canvas;
    CompositionItem   m_currentItem;
    RosegardenGUIDoc* m_doc;
    QPoint            m_origPos;
    bool              m_changeMade;
};


}

#endif
