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

// include files for Qt
#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qabstractlayout.h> 
#include <qlayout.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <kprinter.h>

// application specific includes
#include "MappedEvent.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "notationview.h"
#include "matrixview.h"
#include "trackbuttons.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "RulerScale.h"
#include "segmentparameterbox.h"
#include "instrumentparameterbox.h"

using Rosegarden::SimpleRulerScale;


RosegardenGUIView::RosegardenGUIView(bool showTrackLabels,
                                     QWidget *parent,
                                     const char* /*name*/)
    : QVBox(parent),
      m_rulerScale(0),
      m_trackEditor(0)
{
    RosegardenGUIDoc* doc = getDocument();

    // This apparently arbitrary figure is what we think is an
    // appropriate width in pixels for a 4/4 bar.  Beware of making it
    // too narrow, as shorter bars will be proportionally smaller --
    // the visual difference between 2/4 and 4/4 is perhaps greater
    // than it sounds.

    double barWidth44 = 100.0;  // so random, so rare
    double unitsPerPixel =
        Rosegarden::TimeSignature(4, 4).getBarDuration() / barWidth44;

    Rosegarden::Composition *comp = &doc->getComposition();
    m_rulerScale = new SimpleRulerScale(comp, 0, unitsPerPixel);

    QHBox *hbox = new QHBox(this);
    QFrame *vbox = new QFrame(hbox);
    QVBoxLayout* vboxLayout = new QVBoxLayout(vbox, 5);

    // Segment and Instrument Parameter Boxes [rwb]
    //
    m_segmentParameterBox = new SegmentParameterBox(this, vbox);
    vboxLayout->addWidget(m_segmentParameterBox);
    m_instrumentParameterBox = new InstrumentParameterBox(vbox);
    vboxLayout->addWidget(m_instrumentParameterBox);
    vboxLayout->addStretch();

    // Construct the trackEditor first so we can then
    // query it for placement information
    //
    m_trackEditor  = new TrackEditor(doc, m_rulerScale, showTrackLabels, hbox);

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(slotEditSegmentNotation(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(slotEditSegmentMatrix(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentAudio(Rosegarden::Segment*)),
            SLOT(slotEditSegmentAudio(Rosegarden::Segment*)));

    connect(m_trackEditor, SIGNAL(trackSelected(int)),
            SLOT(slotSelectTrackSegments(int)));

    connect(m_trackEditor, SIGNAL(instrumentSelected(int)),
            SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(m_trackEditor,
           SIGNAL(selectedSegments(std::vector<Rosegarden::Segment*>)),
           this,
           SLOT(slotSelectedSegments(std::vector<Rosegarden::Segment*>)));

    // Re-emit the sendMidiController
    //
    connect(m_instrumentParameterBox,
            SIGNAL(sendMappedEvent(Rosegarden::MappedEvent*)),
            this,
            SLOT(slotSendMappedEvent(Rosegarden::MappedEvent*)));

    connect(m_instrumentParameterBox,
            SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this,
            SLOT(slotChangeInstrumentLabel(Rosegarden::InstrumentId, QString)));

    if (doc)
        m_trackEditor->setupSegments();
}


RosegardenGUIView::~RosegardenGUIView()
{
    delete m_rulerScale;
    kdDebug(KDEBUG_AREA) << "~RosegardenGUIView()\n";
}

RosegardenGUIDoc*
RosegardenGUIView::getDocument() const
{
    QWidget *t = parentWidget();
    
    if (!t) {
        kdDebug(KDEBUG_AREA) << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is 0\n";
        return 0;
    }

    RosegardenGUIApp *theApp = dynamic_cast<RosegardenGUIApp*>(t);
    
    if (!theApp) {
        kdDebug(KDEBUG_AREA) << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is of the wrong type\n";
        return 0;
    }
    
    return theApp->getDocument();
}

void RosegardenGUIView::print(KPrinter *pPrinter, Rosegarden::Composition* p)
{
    SetWaitCursor waitCursor;

    QPainter printpainter;
    printpainter.begin(pPrinter);

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Rosegarden::Composition::iterator i = p->begin(); i != p->end(); ++i) {
        segmentsToEdit.push_back(*i);
    }

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->setPageMode(true);
//     notationView->show();
    notationView->print(&printpainter);

    printpainter.end();
}

void RosegardenGUIView::selectTool(SegmentCanvas::ToolType tool)
{
    m_trackEditor->getSegmentCanvas()->slotSetTool(tool);
}


void RosegardenGUIView::slotEditSegmentNotation(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->show();
}

void RosegardenGUIView::slotEditSegmentMatrix(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;
    segmentsToEdit.push_back(p);

    MatrixView *matrixView = new MatrixView(getDocument(), segmentsToEdit, this);

    // For sending key presses
    //
    connect(matrixView, SIGNAL(keyPressed(Rosegarden::MappedEvent*)),
            this, SLOT(slotSendMappedEvent(Rosegarden::MappedEvent*)));

    matrixView->show();
}

void RosegardenGUIView::slotEditSegmentAudio(Rosegarden::Segment* p)
{
    std::cout << "RosegardenGUIView::slotEditSegmentAudio() - got segment" << endl;
}

void RosegardenGUIView::editAllTracks(Rosegarden::Composition* p)
{
    if (p->getNbSegments() == 0) {
        KMessageBox::sorry(0, "Please create some tracks first (until we implement menu state management)");
        return;
    }

    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Rosegarden::Composition::iterator i = p->begin(); i != p->end(); ++i) {
        segmentsToEdit.push_back(*i);
    }

    NotationView *notationView = new NotationView(getDocument(), segmentsToEdit, this);
    notationView->show();
}


// Select a track label and segments (when we load a file or
// move up a 
//
//
void RosegardenGUIView::selectTrack(int trackId)
{
    m_trackEditor->getTrackButtons()->slotLabelSelected(trackId);
    slotSelectTrackSegments(trackId);
}


void RosegardenGUIView::slotSelectTrackSegments(int trackId)
{
    std::vector<Rosegarden::Segment*> segments;

    for (Rosegarden::Composition::iterator i =
              getDocument()->getComposition().begin();
         i != getDocument()->getComposition().end(); i++)
    {
        if (((int)(*i)->getTrack()) == trackId)
            segments.push_back(*i);
    }

    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (segments.size() > 0) emit activateTool(SegmentCanvas::Selector);

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getSegmentCanvas()->slotSelectSegments(segments);

    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);

    // update the instrument parameter box
    Rosegarden::Composition &comp = getDocument()->getComposition();
    slotUpdateInstrumentParameterBox(comp.getTrackByIndex(trackId)->
                                     getInstrument());

    // Store the selected Track in the Composition
    //
    comp.setSelectedTrack(trackId);
}

void RosegardenGUIView::slotUpdateInstrumentParameterBox(int id)
{
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Composition &comp = getDocument()->getComposition();

    Rosegarden::Instrument *instrument = studio.getInstrumentById(id);

    m_instrumentParameterBox->useInstrument(instrument);
}


// Show a segment as it records
//
void RosegardenGUIView::showRecordingSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->slotUpdateRecordingSegmentItem(segment);
}

void RosegardenGUIView::deleteRecordingSegmentItem()
{
    m_trackEditor->slotDeleteRecordingSegmentItem();
}


// Send a MappedEvent to the track meters by Instrument id
//
void RosegardenGUIView::showVisuals(const Rosegarden::MappedEvent *mE)
{
    double value = ((double)mE->getVelocity()) / 127.0;

    m_trackEditor->getTrackButtons()->
        slotSetMetersByInstrument(value, mE->getInstrument());
}


void
RosegardenGUIView::setControl(const bool &value)
{
    m_trackEditor->slotSetSelectCopy(value);
}

void
RosegardenGUIView::setShift(const bool &value)
{
    m_trackEditor->slotSetSelectAdd(value);
    m_trackEditor->getTopBarButtons()->getLoopRuler()->
	slotSetLoopingMode(value);
    m_trackEditor->slotSetFineGrain(value);
} 

void
RosegardenGUIView::slotSelectedSegments(std::vector<Rosegarden::Segment*> segments)
{
    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);
}

void RosegardenGUIView::slotShowSegmentParameters(bool v)
{
    if (v)
        m_segmentParameterBox->show();
    else
        m_segmentParameterBox->hide();
}

void RosegardenGUIView::slotShowInstrumentParameters(bool v)
{
    if (v)
        m_instrumentParameterBox->show();
    else
        m_instrumentParameterBox->hide();
}

void RosegardenGUIView::slotShowRulers(bool v)
{
    if (v) {
        m_trackEditor->getTopBarButtons()->getLoopRuler()->show();
        m_trackEditor->getBottomBarButtons()->getLoopRuler()->show();
    } else {
        m_trackEditor->getTopBarButtons()->getLoopRuler()->hide();
        m_trackEditor->getBottomBarButtons()->getLoopRuler()->hide();
    }
}

void RosegardenGUIView::slotAddTracks(unsigned int nbTracks)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIView::slotAddTracks(" << nbTracks << ")\n";
    m_trackEditor->slotAddTracks(nbTracks);
}


MultiViewCommandHistory*
RosegardenGUIView::getCommandHistory()
{
    return getDocument()->getCommandHistory();
}

void
RosegardenGUIView::slotAddCommandToHistory(KCommand *command)
{           
    getCommandHistory()->addCommand(command);
}


void 
RosegardenGUIView::slotSendMappedEvent(Rosegarden::MappedEvent *mE)
{
    emit sendMappedEvent(mE);
}

void
RosegardenGUIView::slotChangeInstrumentLabel(Rosegarden::InstrumentId id,
                                             QString label)
{
    m_trackEditor->getTrackButtons()->changeInstrumentLabel(id, label);
}


