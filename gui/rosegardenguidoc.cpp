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

#include <iostream>

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>

#include <string>

#include <zlib.h>

// application specific includes
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "Event.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"

QList<RosegardenGUIView> *RosegardenGUIDoc::pViewList = 0L;

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::String;

using namespace Rosegarden::BaseProperties;


RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent, const char *name)
    : QObject(parent, name), m_recordSegment(0), m_endOfLastRecordedNote(0)
{
    if(!pViewList) {
        pViewList = new QList<RosegardenGUIView>();
    }

    pViewList->setAutoDelete(true);

    connect(&m_commandHistory, SIGNAL(commandExecuted()),
	    this, SLOT(documentModified()));

    connect(&m_commandHistory, SIGNAL(documentRestored()),
	    this, SLOT(documentRestored()));
}

RosegardenGUIDoc::~RosegardenGUIDoc()
{
}

void RosegardenGUIDoc::addView(RosegardenGUIView *view)
{
    pViewList->append(view);
}

void RosegardenGUIDoc::removeView(RosegardenGUIView *view)
{
    pViewList->remove(view);
}

void RosegardenGUIDoc::setAbsFilePath(const QString &filename)
{
    m_absFilePath=filename;
}

const QString &RosegardenGUIDoc::getAbsFilePath() const
{
    return m_absFilePath;
}

void RosegardenGUIDoc::setTitle(const QString &_t)
{
    m_title=_t;
}

const QString& RosegardenGUIDoc::getTitle() const
{
    return m_title;
}

void RosegardenGUIDoc::slotUpdateAllViews(RosegardenGUIView *sender)
{
    RosegardenGUIView *w;
    if(pViewList)
        {
            for(w=pViewList->first(); w!=0; w=pViewList->next())
                {
                    if(w!=sender)
                        w->repaint();
                }
        }

}

void RosegardenGUIDoc::documentModified()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::documentModified()" << endl;
    setModified(true);
}

void RosegardenGUIDoc::documentRestored()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::documentRestored()" << endl;
    setModified(false);
}

bool RosegardenGUIDoc::saveIfModified()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::saveIfModified()" << endl;
    bool completed=true;

    if(m_modified) {
        RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
        int want_save = KMessageBox::warningYesNoCancel(win,
                                                        i18n("The current file has been modified.\n"
                                                             "Do you want to save it?"),
                                                        i18n("Warning"));
        kdDebug(KDEBUG_AREA) << "want_save = " << want_save << endl;

        switch(want_save)
            {
            case KMessageBox::Yes:
                if (m_title == i18n("Untitled")) {
                    win->fileSaveAs();
                } else {
                    saveDocument(getAbsFilePath());
                };

                deleteContents();
                completed=true;
                break;

            case KMessageBox::No:
                setModified(false);
                deleteContents();
                completed=true;
                break;	

            case KMessageBox::Cancel:
                completed=false;
                break;

            default:
                completed=false;
                break;
            }
    }

    return completed;
}

void RosegardenGUIDoc::closeDocument()
{
    deleteContents();
}

bool RosegardenGUIDoc::newDocument()
{
    m_modified=false;
    m_absFilePath=QString::null;
    m_title=i18n("Untitled");

    return true;
}

bool RosegardenGUIDoc::openDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocument("
                         << filename << ")" << endl;
    
    if (!filename || filename.isEmpty())
        return false;

    QFileInfo fileInfo(filename);
    m_title=fileInfo.fileName();

    // Check if file readable with fileInfo ?
    if (!fileInfo.isReadable() || fileInfo.isDir()) {
        QString msg(i18n("Can't open file '"));
        msg += filename;
        msg += "'";
        
        KMessageBox::sorry(0, msg);

        return false;
    }
    
    m_absFilePath=fileInfo.absFilePath();	

    QString errMsg;

    QString fileContents;
    bool okay = readFromFile(filename, fileContents);
    if (!okay) errMsg = "Couldn't read from file";
    else okay = xmlParse(fileContents, errMsg);

/*
    QFile file(filename);
    bool okay = xmlParse(file, errMsg);
    file.close();   
*/

    if (!okay) {
        QString msg(i18n("Error when parsing file '%1' : %2")
                    .arg(filename)
                    .arg(errMsg));
        
        KMessageBox::sorry(0, msg);

        return false;
    }

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocument() end - "
                         << "m_composition : " << &m_composition
                         << " - m_composition->getNbSegments() : "
                         << m_composition.getNbSegments()
                         << " - m_composition->getDuration() : "
                         << m_composition.getDuration() << endl;

    return true;
}

bool RosegardenGUIDoc::saveDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::saveDocument("
                         << filename << ")" << endl;

    QString outText;
    QTextStream outStream(&outText, IO_WriteOnly);

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE rosegarden-data>\n"
               << "<rosegarden-data>\n";

    // Send out Composition (this includes Tracks, Instruments, Tempo
    // and Time Signature changes and any other sub-objects)
    //
    outStream << QString(m_composition.toXmlString().c_str()) << endl << endl;

    // output all elements
    //
    // Iterate on segments
    for (Composition::iterator segitr = m_composition.begin();
         segitr != m_composition.end(); ++segitr) {

	Segment *segment = *segitr;

        //--------------------------
        outStream << QString("<segment track=\"%1\" start=\"%2\">")
            .arg(segment->getTrack())
            .arg(segment->getStartIndex()) << endl;

        long currentGroup = -1;
	bool inChord = false;
	timeT expectedTime = 0;

        for (Segment::iterator i = segment->begin();
             i != segment->end(); ++i) {

            long group;
            if ((*i)->get<Int>(BEAMED_GROUP_ID, group)) {
                if (group != currentGroup) {
                    if (currentGroup != -1) outStream << "</group>" << endl;
                    std::string type = (*i)->get<String>(BEAMED_GROUP_TYPE);
                    outStream << "<group type=\"" << type.c_str() << "\"";
		    if (type == GROUP_TYPE_TUPLED) {
			outStream
			    << " length=\""
			    << (*i)->get<Int>(BEAMED_GROUP_TUPLED_LENGTH)
			    << "\" untupled=\""
			    << (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_LENGTH)
			    << "\" count=\""
			    << (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT)
			    << "\"";
		    }
			    
		    outStream << ">" << endl;
                    currentGroup = group;
                }
            } else if (currentGroup != -1) {
                outStream << "</group>" << endl;
                currentGroup = -1;
            }

	    timeT absTime = (*i)->getAbsoluteTime();

            Segment::iterator nextEl = i;
            ++nextEl;

            if (nextEl != segment->end() &&
                (*nextEl)->getAbsoluteTime() == absTime &&
		(*i)->getDuration() != 0 &&
		!inChord) {
		outStream << "<chord>" << endl;
		inChord = true;
	    }

	    outStream << '\t'
		<< XmlStorableEvent(**i).toXmlString(expectedTime) << endl;

	    if (nextEl != segment->end() &&
		(*nextEl)->getAbsoluteTime() != absTime &&
		inChord) {
		outStream << "</chord>" << endl;
		inChord = false;
	    }

	    if (inChord) {
		expectedTime = absTime;
	    } else {
		expectedTime = absTime + (*i)->getDuration();
	    }
        }

	if (inChord) {
	    outStream << "</chord>" << endl;
	}

        if (currentGroup != -1) {
            outStream << "</group>" << endl;
        }

        outStream << "</segment>" << endl; //-------------------------

    }
    
    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    bool okay = writeToFile(filename, outText);
    if (!okay) return false;

    kdDebug(KDEBUG_AREA) << endl << "RosegardenGUIDoc::saveDocument() finished"
                         << endl;

    m_modified = false;
    m_commandHistory.documentSaved();
    return true;
}

void RosegardenGUIDoc::deleteContents()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::deleteContents()" << endl;

    deleteViews();

    m_composition.clear();
}

void RosegardenGUIDoc::deleteViews()
{
    // auto-deletion is enabled : GUIViews will be deleted
    pViewList->clear();
}


bool
RosegardenGUIDoc::xmlParse(QString &fileContents, QString &errMsg)
{
    // parse xml file
    RoseXmlHandler handler(m_composition);

    QXmlInputSource source;
    source.setData(fileContents);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);

    if (!ok) errMsg = handler.errorString();

    return ok;
}


bool
RosegardenGUIDoc::writeToFile(const QString &file, const QString &text)
{
    gzFile fd = gzopen(file.latin1(), "wb");
    if (!fd) return false;
    
    const char *ctext = text.latin1();
    size_t csize = strlen(ctext);
    int actual = gzwrite(fd, (void *)ctext, csize);
    gzclose(fd);

    return ((size_t)actual == csize);
}

bool
RosegardenGUIDoc::readFromFile(const QString &file, QString &text)
{
    text = "";
    gzFile fd = gzopen(file.latin1(), "rb");
    if (!fd) return false;

    static char buffer[1000];

    while (gzgets(fd, buffer, 1000)) {
	text.append(buffer);
	if (gzeof(fd)) {
	    gzclose(fd);
	    return true;
	}
    }
	
    // gzgets only returns false on error
    return false;
}    
    

void
RosegardenGUIDoc::createNewSegment(SegmentItem *p, int track)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment(item : "
                         << p << ")\n";

    Segment *newSegment = new Segment();
    newSegment->setTrack(track);

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() new segment = "
                         << newSegment << endl;
    
    int startBar = p->getStartBar();
    int barCount = p->getItemNbBars();

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() startBar = " 
			 << startBar << ", barCount = " << barCount << endl;

    timeT startIndex = m_composition.getBarRange(startBar, false).first;
    timeT duration = m_composition.getBarRange(startBar + barCount, false).first -
	startIndex;

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() startIndex = " 
			 << startIndex << ", duration = " << duration << endl;

    newSegment->setStartIndex(startIndex);

    // Now we can add the segment to the composition, because its start index
    // has been set
    //
    m_composition.addSegment(newSegment);

    // And we can set its duration, because it's been added to the
    // Composition
    //
    newSegment->setDuration(duration);

    // store ptr to new segment in segment part item
    p->setSegment(newSegment);

    setModified();

}

// Take a MappedComposition from the Sequencer and turn it
// into an Event-rich Segment ripe for insertion into the
// Composition (once a "stop" is called).
//
//
void
RosegardenGUIDoc::insertRecordedMidi(const Rosegarden::MappedComposition &mC,
                                     const Rosegarden::RealTime &playLatency)
{
    bool firstEvent = false;

    // Just create a new record Segment if we don't have one
    // currently open and running
    //
    if (m_recordSegment == 0)
    {
        m_recordSegment = new Segment();
        m_recordSegment->setTrack(m_composition.getRecordTrack());
        m_recordSegment->setStartIndex(0);
        m_endOfLastRecordedNote = 0;
        m_composition.addSegment(m_recordSegment);

        firstEvent = true; // use this flag to move the start index 
    }

    
    Rosegarden::MappedComposition::iterator i;
    Rosegarden::Event *rEvent;
    timeT duration, absTime;

    // process all the incoming MappedEvents
    //
    for (i = mC.begin(); i != mC.end(); ++i)
    {
        // Create and populate a new Event (for the moment
        // all we get from the Sequencer is Notes)
        //
        //
        rEvent = new Event(Rosegarden::Note::EventType);

        absTime = m_composition.
                          getElapsedTimeForRealTime((*i)->getAbsoluteTime());
        duration = m_composition.getElapsedTimeForRealTime((*i)->getDuration());

        rEvent->setAbsoluteTime(absTime);
        rEvent->setDuration(duration);
        rEvent->set<Int>(PITCH, (*i)->getPitch());
        rEvent->set<Int>(VELOCITY, (*i)->getVelocity());

        // Set the start index
        //
        if (firstEvent)
        {
            m_recordSegment->setStartIndex(absTime);
            firstEvent = false;
        }

        // If there was a gap between the last note and this one
        // then fill it with rests
        //
        if (rEvent->getAbsoluteTime() + rEvent->getDuration() >
            m_endOfLastRecordedNote)
        {
            m_recordSegment->fillWithRests(absTime + duration);
        }

        // Now insert the new event
        //
        Segment::iterator loc = m_recordSegment->insert(rEvent);

        // And now fiddle with it
        //
        SegmentNotationHelper helper(*m_recordSegment);
        if (!helper.isViable(rEvent))
            helper.makeNoteViable(loc);

        // Update our counter
        //
        m_endOfLastRecordedNote = absTime + duration;

        cout << "insertRecordedMidi() - RECORD TIME = " 
             << (*i)->getAbsoluteTime() - playLatency
             << endl;
    }

}

// Tidy up a recorded Segment when we've finished recording
//
void
RosegardenGUIDoc::stopRecordingMidi()
{
    RosegardenGUIView *w;
    if(pViewList)
    {
        for(w=pViewList->first(); w!=0; w=pViewList->next())
        {
            w->createSegmentItem(m_recordSegment);
        }
    }

    m_recordSegment = 0;
    m_endOfLastRecordedNote = 0;

    slotUpdateAllViews(0);
}



