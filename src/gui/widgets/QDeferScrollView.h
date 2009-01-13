
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

#ifndef _RG_QDEFERSCROLLVIEW_H_
#define _RG_QDEFERSCROLLVIEW_H_

//#include <qscrollview.h>
//#include <QScrollArea>
#include <Q3ScrollView>


class QWidget;
class QWheelEvent;


namespace Rosegarden
{



/**
 * A QScrollView which defers vertical scrolling (through mouse wheel)
 * elsewhere, typically another QScrollView, so that both can be kept
 * in sync. The master scrollview will connect its vertical scrollbar
 * to the slave view so the scrollbar will act on both views.
 *
 * The slave scrollview will defer its scrolling to the master by
 * having the gotWheelEvent() signal connected to a slot in the master
 * scrollview, which will simply process the wheel event as if it had
 * received it itself.
 *
 * @see TrackEditor
 * @see SegmentCanvas
 * @see TrackEditor::m_trackButtonScroll
 */
class QDeferScrollView : public Q3ScrollView
{
    Q_OBJECT
public:
    QDeferScrollView(QWidget* parent=0, const char *name=0 ); // WFlags f=0);

    void setBottomMargin(int);

signals:
    void gotWheelEvent(QWheelEvent*);

protected:
    virtual void contentsWheelEvent(QWheelEvent*);
    
};


}

#endif
