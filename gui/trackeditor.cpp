
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <algorithm>

#include "trackseditor.h"
#include "trackscanvas.h"
#include "rosegardenguidoc.h"

#include "rosedebug.h"

#include <qlayout.h>
#include <qcanvas.h>

#include <kmessagebox.h>

using Rosegarden::Composition;

TracksEditor::TracksEditor(RosegardenGUIDoc* doc,
                           QWidget* parent, const char* name,
                           WFlags)
    : QWidget(parent, name),
      m_document(doc),
      m_tracksCanvas(0),
      m_hHeader(0), m_vHeader(0)
{
    unsigned int docNbTracks = 0,
        docNbBars = 0;

    if (doc) {
//         kdDebug(KDEBUG_AREA) << "TracksEditor() : doc " << doc << endl;
        docNbTracks = doc->getNbTracks();
        docNbBars = doc->getNbBars();
    }

    init(64, 100);
}


TracksEditor::TracksEditor(unsigned int nbTracks,
                           unsigned int nbBars,
                           QWidget *parent,
                           const char *name,
                           WFlags)
    : QWidget(parent, name),
      m_document(0),
      m_tracksCanvas(0),
      m_hHeader(0), m_vHeader(0)
{
    init(nbTracks, nbBars);
}

void
TracksEditor::init(unsigned int nbTracks, unsigned int nbBars)
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::init(nbTracks = "
                         << nbTracks << ", nbBars = " << nbBars
                         << ")" << endl;

    QGridLayout *grid = new QGridLayout(this, 2, 2);

    grid->addWidget(m_hHeader = new QHeader(nbBars, this), 0, 1);
    grid->addWidget(m_vHeader = new QHeader(nbTracks, this), 1, 0);
    m_vHeader->setOrientation(Qt::Vertical);

    // set up horiz. header
    QString num;

    for (int i = 0; i < m_hHeader->count(); ++i) {

        m_hHeader->resizeSection(i, 50);
        m_hHeader->setLabel(i, num.setNum(i));
    }

    // set up vert. header
    for (int i = 0; i < m_vHeader->count(); ++i) {
        m_vHeader->resizeSection(i, 25);
        m_vHeader->setLabel(i, QString("Instr. %1").arg(i));
    }

    m_vHeader->setMinimumWidth(100);
    m_vHeader->setResizeEnabled(false);

    QObject::connect(m_vHeader, SIGNAL(indexChange(int,int,int)),
                     this, SLOT(trackOrderChanged(int,int,int)));

    QCanvas *canvas = new QCanvas(this);
    canvas->resize(m_hHeader->sectionSize(0) * nbBars,
                   m_vHeader->sectionSize(0) * nbTracks);

    canvas->setBackgroundColor(Qt::lightGray);

    m_tracksCanvas = new TracksCanvas(m_hHeader->sectionSize(0),
                                      m_vHeader->sectionSize(0),
                                      *canvas, this);

    grid->addWidget(m_tracksCanvas, 1,1);
    connect(this, SIGNAL(needUpdate()),
            m_tracksCanvas, SLOT(update()));

    QObject::connect(m_tracksCanvas, SIGNAL(addTrack(TrackPartItem*)),
                     this,           SLOT(addTrack(TrackPartItem*)));

    QObject::connect(m_tracksCanvas, SIGNAL(deleteTrack(Rosegarden::Track*)),
                     this,           SLOT(deleteTrack(Rosegarden::Track*)));

    QObject::connect(m_tracksCanvas, SIGNAL(resizeTrack(Rosegarden::Track*)),
                     this,           SLOT(resizeTrack(Rosegarden::Track*)));

}

void
TracksEditor::setupTracks()
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::setupTracks() begin" << endl;

    if (!m_document) return; // sanity check
    
    const Composition &comp = m_document->getComposition();

    for (Composition::const_iterator i = comp.begin();
         i != comp.end(); ++i) {

        if ((*i)) {

            kdDebug(KDEBUG_AREA) << "TracksEditor::setupTracks() add track"
                                 << " - start idx : " << (*i)->getStartIndex()
                                 << " - nb bars : " << (*i)->getNbBars()
                                 << " - instrument : " << (*i)->getInstrument()
                                 << endl;

            int x = 0, y = 0;

            y = m_vHeader->sectionPos((*i)->getInstrument());
            // TODO : compute x according to track start

            TrackPartItem *newItem = m_tracksCanvas->addPartItem(x, y,
                                                                 (*i)->getNbBars());    
            newItem->setTrack(*i);
        }
        
    }
}


void
TracksEditor::trackOrderChanged(int section, int fromIdx, int toIdx)
{
    kdDebug(KDEBUG_AREA) << QString("TracksEditor::trackOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    if (moveTrack(section, fromIdx, toIdx))
        emit needUpdate();
}

void
TracksEditor::addTrack(TrackPartItem *p)
{
    // find instrument for part
    //
    int instrument = m_vHeader->sectionAt(static_cast<int>(p->y()));
    p->setInstrument(instrument);

    kdDebug(KDEBUG_AREA) << QString("TracksEditor::addTrack() : track instr is %1 at %2")
        .arg(instrument).arg(p->y())
                         << ", p = " << p << endl;

    emit createNewTrack(p);
}

void
TracksEditor::deleteTrack(Rosegarden::Track *p)
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::moveTrack() : not implemented\n";

//     std::list<TrackPart*>::iterator iter = std::find(m_trackParts.begin(),
//                                                      m_trackParts.end(),
//                                                      p);

//     if (iter != m_trackParts.end()) {
//         kdDebug(KDEBUG_AREA) << "TracksEditor::deleteTrackPart() : part "
//                              << p << " deleted" << endl;
//         TrackPart *p = *iter;
//         m_trackParts.erase(iter);
//         delete p;

//     } else {
//         KMessageBox::error(0, QString("TracksEditor::deleteTrackPart() : part %1 not found").arg(long(p), 0, 16));
        
//         kdDebug(KDEBUG_AREA) << "TracksEditor::deleteTrackPart() : part "
//                              << p << " not found" << endl;
//     }
}

// TODO : get rid of this
void
TracksEditor::resizeTrack(Rosegarden::Track *p)
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::resizeTrack() : not implemented\n";
}


bool
TracksEditor::moveTrack(int /*section*/, int /*fromIdx*/, int /*toIdx*/)
{
    kdDebug(KDEBUG_AREA) << "TracksEditor::moveTrack() : not implemented\n";

//     // just reset every part's Y coordinate
//     for (std::list<TrackPart*>::iterator iter = m_trackParts.begin();
//          iter != m_trackParts.end(); ++iter) {
//         TrackPart* p = *iter;
//         int trackSection = p->trackNb();
//         QCanvasRectangle *r = p->canvasPartItem();
//         r->setY(m_vHeader->sectionPos(trackSection));
//     }

    return true;
}

void
TracksEditor::clear()
{
    m_tracksCanvas->clear();
}
