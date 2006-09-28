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


#include "SetWaitCursor.h"

#include "gui/editors/segment/TrackEditor.h"
#include "misc/Debug.h"
#include "RosegardenGUIApp.h"
#include <kcursor.h>
#include <qcursor.h>
#include <qwidget.h>


namespace Rosegarden
{

SetWaitCursor::SetWaitCursor()
        : m_guiApp(dynamic_cast<RosegardenGUIApp*>(kapp->mainWidget()))
{
    if (m_guiApp) {

        // play it safe, so we can use this class at anytime even very early in the app init
        if ((m_guiApp->getView() &&
                m_guiApp->getView()->getTrackEditor() &&
                m_guiApp->getView()->getTrackEditor()->getSegmentCanvas() &&
                m_guiApp->getView()->getTrackEditor()->getSegmentCanvas()->viewport())) {

            m_saveSegmentCanvasCursor = m_guiApp->getView()->getTrackEditor()->getSegmentCanvas()->viewport()->cursor();

        }

        RG_DEBUG << "SetWaitCursor::SetWaitCursor() : setting waitCursor\n";
        m_saveCursor = m_guiApp->cursor();

        m_guiApp->setCursor(KCursor::waitCursor());
    }
}

SetWaitCursor::~SetWaitCursor()
{
    if (m_guiApp) {

        RG_DEBUG << "SetWaitCursor::SetWaitCursor() : restoring normal cursor\n";
        QWidget* viewport = 0;
        QCursor currentSegmentCanvasCursor;

        if ((m_guiApp->getView() &&
                m_guiApp->getView()->getTrackEditor() &&
                m_guiApp->getView()->getTrackEditor()->getSegmentCanvas() &&
                m_guiApp->getView()->getTrackEditor()->getSegmentCanvas()->viewport())) {
            viewport = m_guiApp->getView()->getTrackEditor()->getSegmentCanvas()->viewport();
            currentSegmentCanvasCursor = viewport->cursor();
        }

        m_guiApp->setCursor(m_saveCursor);

        if (viewport) {
            if (currentSegmentCanvasCursor.shape() == KCursor::waitCursor().shape()) {
                viewport->setCursor(m_saveSegmentCanvasCursor);
            } else {
                viewport->setCursor(currentSegmentCanvasCursor); // because m_guiApp->setCursor() has replaced it
            }
        }

        // otherwise, it's been modified elsewhere, so leave it as is

    }

}

}
