// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2003
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


#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>

#include <iostream>
#include <unistd.h>
#include <csignal>

#include <sys/time.h>

#include "Profiler.h"
#include "MappedComposition.h"
#include "rosegardendcop.h"
#include "rosegardensequencer.h"
#include "rosedebug.h"

using std::cout;
using std::cerr;
using std::endl;

static const char *description = I18N_NOOP("RosegardenSequencer");
static RosegardenSequencerApp *roseSeq = 0;

static KCmdLineOptions options[] =
    {
//        { "+[File]", I18N_NOOP("file to open"), 0 },
        // INSERT YOUR COMMANDLINE OPTIONS HERE
        { "+[playback_1 playback_2 capture_1 capture_2]",
            I18N_NOOP("JACK playback and capture ports"), 0 },
        { 0, 0, 0 }
    };

static void
signalHandler(int /*sig*/)
{
    if (roseSeq)
        delete roseSeq;

    exit(0);
}

int main(int argc, char *argv[])
{

    KAboutData aboutData( "rosegardensequencer",
                          I18N_NOOP("RosegardenSequencer"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000-2003, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    // Parse cmd line args
    //
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    KApplication app;
    std::vector<std::string> jackArgs;

    // Construct JACK args
    //
    for (int i = 0; i < args->count(); i++)
        jackArgs.push_back(std::string(args->arg(i)));

    if (app.isRestored())
    {
	app.quit(); // don't do session restore -- GUI will start a sequencer
        
//         int n = 1;
//         while (KMainWindow::canBeRestored(n)) {
// 	    // memory leak if more than one can be restored?
//             (roseSeq = new RosegardenSequencerApp(jackArgs))->restore(n);
//             n++;
//         }
    }
    else
    {
        roseSeq = new RosegardenSequencerApp(jackArgs);

        // we don't show() the sequencer application as we're just taking
        // advantage of DCOP/KApplication and there's nothing to show().

//!!!        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if (args->count())
	{
            //rosegardensequencer->openDocumentFile(args->arg(0));
        }
        else
        {
            // rosegardensequencer->openDocumentFile();
        }

        args->clear();
    }

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    //app.dcopClient()->setDefaultObject("RosegardenGUIIface");

    app.disableSessionManagement(); // we don't want to be
                                    // saved/restored by session
                                    // management, only run by the GUI

    // Started OK
    //
    SEQUENCER_DEBUG << "RosegardenSequencer - started OK" << endl;

    // Now we can enter our specialised event loop.
    // For each pass through we wait for some pending
    // events.  We check status on the way through and
    // act accordingly.  DCOP events fire back and
    // forth processed in the event loop changing 
    // state and hopefully controlling and providing
    // feedback.  We also put in some sleep time to
    // make sure the loop doesn't eat up all the
    // processor - we're not in that much of a rush!
    //
    // Soon perhaps we'll put some throttling into
    // the loop sleep time(s) to allow for dropped
    // notes etc. .. ?
    //
    //
    TransportStatus lastSeqStatus = roseSeq->getStatus();

    // Register signal handler
    //
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    signal(SIGQUIT, signalHandler);

    Rosegarden::RealTime sleepTime = Rosegarden::RealTime(0, 10000);

    while (roseSeq && roseSeq->getStatus() != QUIT)
    {
        // Update internal clock and send pointer position
        // change event to GUI - this is the heartbeat of
        // the Sequencer - it doesn't tick over without
        // this call.
        //
        // Also attempt to send the MIDI clock at this point.
        //
        roseSeq->updateClocks();

	switch(roseSeq->getStatus())
	{
	case STARTING_TO_PLAY:
	    if (!roseSeq->startPlaying())
	    {
		// send result failed and stop Sequencer
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(PLAYING);
	    }
	    break;

	case PLAYING:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// process any async events
		//
		roseSeq->processAsynchronousEvents();
	    }
	    break;

	case STARTING_TO_RECORD_MIDI:
	    if (!roseSeq->startPlaying())
	    {
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(RECORDING_MIDI);
	    }
	    break;

	case STARTING_TO_RECORD_AUDIO:
	    if (!roseSeq->startPlaying())
	    {
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		roseSeq->setStatus(RECORDING_AUDIO);
	    }
	    break;

	case RECORDING_MIDI:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// Now process any incoming MIDI events
		// and return them to the gui
		//
		roseSeq->processRecordedMidi();
	    }
	    break;

	case RECORDING_AUDIO:
	    if (!roseSeq->keepPlaying())
	    {
		// there's a problem or the piece has
		// finished - so stop playing
		roseSeq->setStatus(STOPPING);
	    }
	    else
	    {
		// Now process any incoming audio
		// and return it to the gui
		//
		roseSeq->processRecordedAudio();

		// Still process these so we can send up
		// audio levels as MappedEvents
		//
		roseSeq->processAsynchronousEvents();
	    }
	    break;

	case STOPPING:
	    roseSeq->setStatus(STOPPED);
	    Rosegarden::Profiles::getInstance()->dump(); //!!!
	    SEQUENCER_DEBUG << "RosegardenSequencer - Stopped" << endl;
	    break;

	case RECORDING_ARMED:
	    SEQUENCER_DEBUG << "RosegardenSequencer - "
			    << "Sequencer can't enter \""
			    << "RECORDING_ARMED\" state - "
			    << "internal error"
			    << endl;
	    break;

	case STOPPED:
	default:
	    roseSeq->processAsynchronousEvents();
	    break;
	}

        if (lastSeqStatus != roseSeq->getStatus())
            roseSeq->notifySequencerStatus();

        lastSeqStatus = roseSeq->getStatus();

	app.processEvents();
	roseSeq->sleep(sleepTime);
    }

    return app.exec();

}
