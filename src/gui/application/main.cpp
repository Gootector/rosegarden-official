// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QDesktopWidget>
#include <Q3Canvas>
#include <QTimer>
#include <QApplication>
#include <sys/time.h>
#include "base/RealTime.h"

//@@@ required ? :
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
//#include <ktip.h>
//#include <kglobalsettings.h>

#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QProcess>

#include <QStringList>
#include <QRegExp>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>

#include "document/ConfigGroups.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/application/RosegardenGUIApp.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/kdeext/KStartupLogo.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"

#include "gui/application/RosegardenApplication.h"

using namespace Rosegarden;

/*! \mainpage Rosegarden global design
 
Rosegarden is split into 3 main parts:
 
\section base Base
 
The base library holds all of the fundamental "music handling"
structures, of which the primary ones are Event, Segment, Track,
Instrument and Composition.  It also contains a selection of utility
and helper classes of a kind that is not specific to any particular
GUI.  Everything here is part of the Rosegarden namespace, and there
are no dependencies on KDE or Qt (although it uses the STL heavily).
 
The keyword for the basic structures in use is "flexibility".  Our
Event objects can be extended arbitrarily for the convenience of GUI
or performance code without having to change their declaration or
modify anything in the base library.  And most of our assumptions
about the use of the container classes can be violated without
disastrous side-effects.
 
\subsection musicstructs Music Structures
 
 - \link Event Event\endlink is the basic musical element.  It's more or less a
    generalization of the MIDI event.  Each note or rest, each key
    change or tempo change, is an event: there's no "note class" or
    "rest class" as such, they are simply represented by events whose
    type happens to be "note" or "rest".
    Each Event has a type code, absolute time (the moment at which the
    Event starts, relative only to the start of the Composition) and
    duration (usually non-zero only for notes and rests), together
    with an arbitrary set of named and typed properties that can be
    assigned and queried dynamically by other parts of the
    application.  So, for example, a note event is likely to have an
    integer property called "pitch", and probably a "velocity", as
    well as potentially many others -- but this is not fixed anywhere,
    and there's no definition of what exactly a note is: client code
    is simply expected to ignore any unrecognised events or properties
    and to cope if properties that should be there are not.
 
 - \link Segment Segment\endlink is a series of consecutive Events found on the same Track,
    automatically ordered by their absolute time.  It's the usual
    container for Events.  A Segment has a starting time that can be
    changed, and a duration that is based solely on the end time of
    the last Event it contains.  Note that in order to facilitate
    musical notation editing, we explicitly store silences as series
    of rest Events; thus a Segment really should contain no gaps
    between its Events.  (This isn't checked anywhere and nothing will
    break very badly if there are gaps, but notation won't quite work
    correctly.)
 
 - \link Track Track \endlink is much the same thing as on a mixing table, usually
    assigned to an instrument, a voice, etc.  Although a Track is not
    a container of Events and is not strictly a container of Segments
    either, it is referred to by a set of Segments that are therefore
    mutually associated with the same instruments and parameters.  In
    GUI terms, the Track is a horizontal row on the main Rosegarden
    window, whereas a Segment is a single blue box within that row, of
    which there may be any number.
 
 - \link Instrument Instrument \endlink corresponds broadly to a MIDI or Audio channel, and is
    the destination for a performed Event.  Each Track is mapped to a
    single Instrument (although many Tracks may have the same
    Instrument), and the Instrument is indicated in the header at the
    left of the Track's row in the GUI.
 
 - \link Composition Composition\endlink is the container for the entire piece of music.  It
    consists of a set of Segments, together with a set of Tracks that
    the Segments may or may not be associated with, a set of
    Instruments, and some information about time signature and tempo
    changes.  (The latter are not stored in Segments; they are only
    stored in the top-level Composition.  You can't have differing
    time signatures or tempos in different Segments.)  Any code that
    wants to know about the locations of bar lines, or request
    real-time calculations based on tempo changes, talks to the
    Composition.
 
 
See also docs/data_struct/units.txt for an explanation of the units we
use for time and pitch values.  See docs/discussion/names.txt for some
name-related discussion.  See docs/code/creating_events.txt for an
explanation of how to create new Events and add properties to them.
 
The base directory also contains various music-related helper classes:
 
 - The NotationTypes.[Ch] files contain classes that help with
    creating and manipulating events.  It's very important to realise
    that these classes are not the events themselves: although there
    is a Note class in this file, and a TimeSignature class, and Clef
    and Key classes, instances of these are rarely stored anywhere.
    Instead they're created on-the-fly in order to do calculation
    related to note durations or time signatures or whatever, and they
    contain getAsEvent() methods that may be used when an event for
    storage is required.  But the class of a stored event is always
    simply Event.
 
    The NotationTypes classes also define important constants for the
    names of common properties in Events.  For example, the Note class
    contains Note::EventType, which is the type of a note Event, and
    Note::EventRestType, the type of a rest Event; and Key contains
    Key::EventType, the type of a key change Event, KeyPropertyName,
    the name of the property that defines the key change, and a set
    of the valid strings for key changes.
 
 - BaseProperties.[Ch] contains a set of "standard"-ish Event
    property names that are not basic enough to go in NotationTypes.
 
 - \link SegmentNotationHelper SegmentNotationHelper\endlink
    and \link SegmentPerformanceHelper SegmentPerformanceHelper\endlink
    do tasks that
    may be useful to notation-type code and performer code
    respectively.  For example, SegmentNotationHelper is used to
    manage rests when inserting and deleting notes in a score editor,
    and to create beamed groups and suchlike; SegmentPerformanceHelper
    generally does calculations involving real performance time of
    notes (taking into account tied notes, tuplets and tempo changes).
    These two lightweight helper classes are also usually constructed
    on-the-fly for use on the events in a given Segment and then
    discarded after use.
 
 - \link Quantizer Quantizer\endlink is used to quantize event timings and set quantized
    timing properties on those events.  Note that quantization is
    non-destructive, as it takes advantage of the ability to set new
    Event properties to simply assign the quantized values as separate
    properties from the original absolute time and duration.
 
 
\section gui GUI
 
The GUI directory builds into a KDE/Qt application. Like most KDE
applications, it follows a document/view model. The document (class
RosegardenGUIDoc, which wraps a Composition) can have several views
(class RosegardenGUIView), although at the moment only a single one is
used.
 
This view is the TrackEditor, which shows all the Composition's
Segments organized in Tracks. Each Segment can be edited in two ways:
notation (score) or matrix (piano roll).
 
All editor views are derived from EditView. An EditView is the class
dealing with the edition per se of the events. It uses several
components:
 
 - Layout classes, horizontal and vertical: these are the classes
    which determine the x and y coordinates of the graphic items
    representing the events (notes or piano-roll rectangles).  They
    are derived from the LayoutEngine base-class in the base library.
 
 - Tools, which implement each editing function at the GUI (such as
    insert, erase, cut and paste). These are the tools which appear on
    the EditView's toolbar.
 
 - Toolbox, which is a simple string => tool map.
 
 - Commands, which are the fundamental implementations of editing
    operations (both menu functions and tool operations) subclassed
    from KDE's Command and used for undo and redo.
 
 - a canvas view.  Although this isn't a part of the EditView's
    definition, both of the existing edit views (notation and matrix)
    use one, because they both use a Q3Canvas to represent data.
 
 - LinedStaff, a staff with lines.  Like the canvas view, this isn't
    part of the EditView definition, but both views use one.
 
 
There are currently two editor views:
 
 - NotationView, with accompanying classes NotationHLayout,
    NotationVLayout, NotationStaff, and all the classes in the
    notationtool and notationcommands files.  These are also closely
    associated with the NotePixmapFactory and NoteFont classes, which
    are used to generate notes from component pixmap files.
 
 - MatrixView, with accompanying classes MatrixHLayout,
    MatrixVLayout, MatrixStaff and other classes in the matrixview
    files.
 
The editing process works as follows:
 
[NOTE : in the following, we're talking both about events as UI events
or user events (mouse button clicks, mouse move, keystrokes, etc...)
and Events (our basic music element).  To help lift the ambiguity,
"events" is for UI events, Events is for Event.]
 
 -# The canvas view gets the user events (see
    NotationCanvasView::contentsMousePressEvent(QMouseEvent*) for an
    example).  It locates where the event occured in terms of musical
    element: which note or staff line the user clicked on, which pitch
    and time this corresponds to, that kind of stuff.  (In the
    Notation and Matrix views, the LinedStaff calculates mappings
    between coordinates and staff lines: the former is especially
    complicated because of its support for page layout.)\n
 -# The canvas view transmits this kind of info as a signal, which is
 connected to a slot in the parent EditView.
 -# The EditView delegates action to the current tool.\n
 -# The tool performs the actual job (inserting or deleting a note,
    etc...).
 
Since this action is usually complex (merely inserting a note requires
dealing with the surrounding Events, rests or notes), it does it
through a SegmentHelper (for instance, base/SegmentNotationHelper)
which "wraps" the complexity into simple calls and performs all the
hidden tasks.
 
The EditView also maintains (obviously) its visual appearance with the
layout classes, applying them when appropriate.
 
\section sequencer Sequencer
 
The sequencer directory also builds into a KDE/Qt application, but one
which doesn't have a gui.  The Sequencer can be started automatically
by the main Rosegarden GUI or manually if testing - it's sometimes
more convenient to do the latter as the Sequencer needs to be connected
up to the underlying sound system every time it is started.
 
The Sequencer interfaces directly with \link AlsaDriver ALSA\endlink
and provides MIDI "play" and "record" ports which can be connected to
other MIDI clients (MIDI IN and OUT hardware ports or ALSA synth devices)
using any ALSA MIDI Connection Manager.  The Sequencer also supports 
playing and recording of Audio sample files using \link JackDriver Jack\endlink 
 
The GUI and Sequencer were originally implemented as separate processes
communicating using the KDE DCOP communication framework, but they have
now been restructured into separate threads of a single process.  The
original design still explains some of the structure of these classes,
however.  Generally, the DCOP functions that the GUI used to call in
the sequencer are now simple public functions of RosegardenSequencer
that are described in the RosegardenSequencerIface parent class (this
class is retained purely for descriptive purposes); calls that the
sequencer used to make back to the GUI have mostly been replaced by
polling from the GUI to sequencer.

The main operations invoked from the GUI involve starting and
stopping the Sequencer, playing and recording, fast forwarding and
rewinding.  Once a play or record cycle is enabled it's the Sequencer
that does most of the hard work.  Events are read from (or written to,
when recording) a set of mmapped files shared between the threads.
 
The Sequencer makes use of two libraries libRosegardenSequencer
and libRosegardenSound:
 
 - libRosegardenSequencer holds everything pertinent to sequencing
   for Rosegarden including the
   Sequencer class itself.  This library is only linked into the
   Rosegarden Sequencer.
 
 - libRosegardenSound holds the MidiFile class (writing and reading
   MIDI files) and the MappedEvent and MappedComposition classes (the
   communication class for transferring events back and forth between
   sequencer and GUI).  This library is needed by the GUI as well as
   the Sequencer.
 
The main Sequencer state machine is a good starting point and clearly
visible at the bottom of rosegarden/sequencer/main.cpp.
*/

static KLocalizedString description =
       ki18n("Rosegarden - A sequencer and musical notation editor");

/*&&& removed options -- we'll want a different set anyway 

static KCmdLineOptions options[] =
    {
        { "nosequencer", I18N_NOOP("Don't use the sequencer (support editing only)"), 0 },
        { "nosplash", I18N_NOOP("Don't show the splash screen"), 0 },
        { "nofork", I18N_NOOP("Don't automatically run in the background"), 0 },
        { "ignoreversion", I18N_NOOP("Ignore installed version - for devs only"), 0 },
        { "+[File]", I18N_NOOP("file to open"), 0 },
        { 0, 0, 0 }
    };
*/

// -----------------------------------------------------------------

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>

static int _x_errhandler( Display *dpy, XErrorEvent *err )
{
    char errstr[256];
    XGetErrorText( dpy, err->error_code, errstr, 256 );
    if ( err->error_code != BadWindow )
	std::cerr << "Rosegarden: detected X Error: " << errstr << " " << err->error_code
		  << "\n  Major opcode:  " << err->request_code << std::endl;
    return 0;
}
#endif

// NOTE: to get a dump of the stack trace from KDE during program execution:
// std::cerr << kdBacktrace() << std::endl
// (see kdebug.h)

//###!!! This should no longer be necessary... should it?

/*&&&
void testInstalledVersion()
{
    QString versionLocation =
	ResourceFinder().getResourcePath("appdata", "version.txt");
    QString installedVersion;

    if (!versionLocation.isEmpty()) {
        QFile versionFile(versionLocation);
        if (versionFile.open(QIODevice::ReadOnly)) {
            QTextStream text(&versionFile);
            QString s = text.readLine().trimmed();
            versionFile.close();
            if (!s.isEmpty()) {
                if (s == VERSION) return;
                installedVersion = s;
            }
        }
    }

    if (!installedVersion.isEmpty()) {

        QMessageBox box = QMessageBox::critical(
          dynamic_cast<QWidget*>(0),
          i18n("Installation problem"),
          i18n("Installation contains the wrong version of Rosegarden."),
          QMessageBox::Ok,
          QMessageBox::Ok);

        QString blather = i18n(
              " The wrong versions of Rosegarden's data files were\n"
              " found.\n"
              " (I am %1, but the installed files are for version %2.)\n\n"
              " This may mean one of the following:\n\n"
              " 1. This is a new upgrade of Rosegarden, and it has not yet been\n"
              "    installed.  If you compiled it yourself, check that you have\n"
              "    run \"make install\" and that the procedure completed\n"
              "    successfully.\n\n"
              " 2. The upgrade was installed in a non-standard directory,\n"
              "    and an old version was found in a standard directory.",
              VERSION, installedVersion);

         box.setDetailedText(blather);

    } else {

        QMessageBox box = QMessageBox::critical(
          dynamic_cast<QWidget*>(0),
          i18n("Installation problem"),
          i18n("Rosegarden does not appear to have been installed."),
          QMessageBox::Ok,
          QMessageBox::Ok);

         QString blather = 
         i18n(" One or more of Rosegarden's data files could not be\n"
              " found. This may mean one of the following:\n\n"
              " 1. Rosegarden has not been correctly installed.  If you compiled\n"
              "    it yourself, check that you have run \"make install\" and that\n"
              "    the procedure completed successfully.\n\n"
              " 2. Rosegarden has been installed in a non-standard directory,\n"
              "    like /usr/local or /opt.");

          box.setDetailedText(blather);
    }

    exit(1);
}
*/

extern const unsigned char qt_resource_data[];

int main(int argc, char *argv[])
{
    setsid(); // acquire shiny new process group

    srandom((unsigned int)time(0) * (unsigned int)getpid());

    KAboutData aboutData( "rosegarden", "", ki18n("Rosegarden"),
                          VERSION, description, KAboutData::License_GPL,
                          ki18n("Copyright 2000 - 2009 Guillaume Laurent, Chris Cannam, Richard Bown\nParts copyright 1994 - 2004 Chris Cannam, Andy Green, Richard Bown, Guillaume Laurent\nLilyPond fonts copyright 1997 - 2005 Han-Wen Nienhuys and Jan Nieuwenhuizen"),
                          KLocalizedString(),
                          "http://www.rosegardenmusic.com/",
                          "rosegarden-devel@lists.sourceforge.net");

    aboutData.addAuthor(ki18n("Guillaume Laurent (lead)"), KLocalizedString(), "glaurent@telegraph-road.org", "http://telegraph-road.org");
    aboutData.addAuthor(ki18n("Chris Cannam (lead)"), KLocalizedString(), "cannam@all-day-breakfast.com", "http://all-day-breakfast.com");
    aboutData.addAuthor(ki18n("Richard Bown (lead)"), KLocalizedString(), "richard.bown@ferventsoftware.com");
    aboutData.addAuthor(ki18n("D. Michael McIntyre"), KLocalizedString(), "dmmcintyr@users.sourceforge.net");
    aboutData.addAuthor(ki18n("Pedro Lopez-Cabanillas"), KLocalizedString(), "plcl@users.sourceforge.net");
    aboutData.addAuthor(ki18n("Heikki Johannes Junes"), KLocalizedString(), "hjunes@users.sourceforge.net");

    aboutData.addCredit(ki18n("Randall Farmer"), ki18n("Chord labelling code"), " rfarme@simons-rock.edu");
    aboutData.addCredit(ki18n("Hans  Kieserman"), ki18n("LilyPond output\nassorted other patches\ni18n-ization"), "hkieserman@mail.com");
    aboutData.addCredit(ki18n("Levi Burton"), ki18n("UI improvements\nbug fixes"), "donburton@sbcglobal.net");
    aboutData.addCredit(ki18n("Mark Hymers"), ki18n("Segment colours\nOther UI and bug fixes"), "<markh@linuxfromscratch.org>");
    aboutData.addCredit(ki18n("Alexandre Prokoudine"), ki18n("Russian translation\ni18n-ization"), "avp@altlinux.ru");
    aboutData.addCredit(ki18n("Jörg Schumann"), ki18n("German translation"), "jrschumann@gmx.de");
    aboutData.addCredit(ki18n("Eckhard Jokisch"), ki18n("German translation"), "e.jokisch@u-code.de");
    aboutData.addCredit(ki18n("Kevin Donnelly"), ki18n("Welsh translation"));
    aboutData.addCredit(ki18n("Didier Burli"), ki18n("French translation"), "didierburli@bluewin.ch");
    aboutData.addCredit(ki18n("Yves Guillemot"), ki18n("French translation\nBug fixes"), "yc.guillemot@wanadoo.fr");
    aboutData.addCredit(ki18n("Daniele Medri"), ki18n("Italian translation"), "madrid@linuxmeeting.net");
    aboutData.addCredit(ki18n("Alessandro Musesti"), ki18n("Italian translation"), "a.musesti@dmf.unicatt.it");
    aboutData.addCredit(ki18n("Stefan Asserhäll"), ki18n("Swedish translation"), "stefan.asserhall@comhem.se");
    aboutData.addCredit(ki18n("Erik Magnus Johansson"), ki18n("Swedish translation"), "erik.magnus.johansson@telia.com");
    aboutData.addCredit(ki18n("Hasso Tepper"), ki18n("Estonian translation"), "hasso@estpak.ee");
    aboutData.addCredit(ki18n("Jelmer Vernooij"), ki18n("Dutch translation"), "jelmer@samba.org");
    aboutData.addCredit(ki18n("Jasper Stein"), ki18n("Dutch translation"), "jasper.stein@12move.nl");
    aboutData.addCredit(ki18n("Arnout Engelen"), ki18n("Transposition by interval"));
    aboutData.addCredit(ki18n("Thorsten Wilms"), ki18n("Original designs for rotary controllers"), "t_w_@freenet.de");
    aboutData.addCredit(ki18n("Oota Toshiya"), ki18n("Japanese translation"), "ribbon@users.sourceforge.net");
    aboutData.addCredit(ki18n("William"), ki18n("Auto-scroll deceleration\nRests outside staves and other bug fixes"), "rosegarden4p AT orthoset.com");
    aboutData.addCredit(ki18n("Liu Songhe"), ki18n("Simplified Chinese translation"), "jackliu9999@msn.com");
    aboutData.addCredit(ki18n("Toni Arnold"), ki18n("LIRC infrared remote-controller support"), "<toni__arnold@bluewin.ch>");
    aboutData.addCredit(ki18n("Vince Negri"), ki18n("MTC slave timing implementation"), "vince.negri@gmail.com");
    aboutData.addCredit(ki18n("Jan Bína"), ki18n("Czech translation"), "jbina@sky.cz");
    aboutData.addCredit(ki18n("Thomas Nagy"), ki18n("SCons/bksys building system"), "tnagy256@yahoo.fr");
    aboutData.addCredit(ki18n("Vladimir Savic"), ki18n("icons, icons, icons"), "vladimir@vladimirsavic.net");
    aboutData.addCredit(ki18n("Marcos Germán Guglielmetti"), ki18n("Spanish translation"), "marcospcmusica@yahoo.com.ar");
    aboutData.addCredit(ki18n("Lisandro Damián Nicanor Pérez Meyer"), ki18n("Spanish translation"), "perezmeyer@infovia.com.ar");
    aboutData.addCredit(ki18n("Javier Castrillo"), ki18n("Spanish translation"), "riverplatense@gmail.com");
    aboutData.addCredit(ki18n("Lucas Godoy"), ki18n("Spanish translation"), "godoy.lucas@gmail.com");
    aboutData.addCredit(ki18n("Feliu Ferrer"), ki18n("Catalan translation"), "mverge2@pie.xtec.es");
    aboutData.addCredit(ki18n("Quim Perez i Noguer"), ki18n("Catalan translation"), "noguer@osona.com");
    aboutData.addCredit(ki18n("Carolyn McIntyre"), ki18n("1.2.3 splash screen photo\nGave birth to D. Michael McIntyre, bought him a good flute once\nupon a time, and always humored him when he came over to play her\nsome new instrument, even though she really hated his playing.\nBorn October 19, 1951, died September 21, 2007, R. I. P."), "DECEASED");
    aboutData.addCredit(ki18n("Stephen Torri"), ki18n("Initial guitar chord editing code"), "storri@torri.org");
    aboutData.addCredit(ki18n("Piotr Sawicki"), ki18n("Polish translation"), "pelle@plusnet.pl");
    aboutData.addCredit(ki18n("David García-Abad"), ki18n("Basque translation"), "davidgarciabad@telefonica.net");

    aboutData.setTranslator(ki18nc("NAME OF TRANSLATORS", "Your names"),
			    ki18nc("EMAIL OF TRANSLATORS", "Your emails"));

//&&&    KCmdLineArgs::init( argc, argv, &aboutData );
//&&&    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
//&&&    KUniqueApplication::addCmdLineOptions(); // Add KUniqueApplication options.

//&&&    if (!RosegardenApplication::start())
//&&&        return 0;

    RosegardenApplication app(argc, argv);

    QApplication::setOrganizationName("rosegardenmusic");
    QApplication::setOrganizationDomain("rosegardenmusic.org");
    QApplication::setApplicationName(i18n("Rosegarden"));

    // Ensure quit on last window close
    //
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    // Parse cmd line args
    //
/*&&&
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (!args->isSet("ignoreversion")) {
        // Give up immediately if we haven't been installed or if the
        // installation is out of date
        //
        testInstalledVersion();
    }
*/
    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    QString lastVersion = settings.value("lastversion", "").toString();
    bool newVersion = (lastVersion != VERSION);
    if (newVersion) {
	std::cerr << "*** This is the first time running this Rosegarden version" << std::endl;
	settings.setValue("lastversion", VERSION);
    }

    // If there is no config setting for the startup window size, set
    // one now.  But base the default on the appropriate desktop size
    // (i.e. not the entire desktop, if Xinerama is in use).  This is
    // obtained from KGlobalSettings::desktopGeometry(), but we can't
    // give it a meaningful point to measure from at this stage so we
    // always use the "leftmost" display (point 0,0).

    // The config keys are "Height X" and "Width Y" where X and Y are
    // the sizes of the available desktop (i.e. the whole shebang if
    // under Xinerama).  These are obtained from QDesktopWidget.

    settings.endGroup();
    settings.beginGroup( "MainView" );

    int windowWidth = 0, windowHeight = 0;

    QDesktopWidget *desktop = QApplication::desktop();
    if (desktop) {
	QRect totalRect(desktop->screenGeometry());
	QRect desktopRect = desktop->availableGeometry();
	QSize startupSize;
	if (desktopRect.height() <= 800) {
	    startupSize = QSize((desktopRect.width() * 6) / 7,
				(desktopRect.height() * 6) / 7);
	} else {
	    startupSize = QSize((desktopRect.width() * 4) / 5,
				(desktopRect.height() * 4) / 5);
	}
	QString widthKey = QString("Width %1").arg(totalRect.width());
	QString heightKey = QString("Height %1").arg(totalRect.height());
	windowWidth = settings.value
	    (widthKey, startupSize.width()).toInt();
	windowHeight = settings.value
	    (heightKey, startupSize.height()).toInt();
    }

    settings.endGroup();
    settings.beginGroup( "KDE Action Restrictions" );

    settings.setValue("action/help_report_bug", false);

    // Show Startup logo
    // (this code borrowed from KDevelop 2.0,
    // (c) The KDevelop Development Team
    //
    settings.endGroup();
    settings.beginGroup( GeneralOptionsConfigGroup );

    KStartupLogo* startLogo = 0L;

    // See if the settings wants us to control JACK
    //
    if ( qStrToBool( settings.value("Logo", "true" ) ) 
//&&&	 && (!qApp->isSessionRestored() && args->isSet("splash"))
	)
    {
        startLogo = KStartupLogo::getInstance();
	startLogo->setShowTip(!newVersion);
        startLogo->show();
    }

    struct timeval logoShowTime;
    gettimeofday(&logoShowTime, 0);

    //
    // Start application
    //
    RosegardenGUIApp *rosegardengui = 0;

/*&&& worry about this later
    if (app.isSessionRestored()) {
        RG_DEBUG << "Restoring from session\n";

        // RESTORE(RosegardenGUIApp);
        int n = 1;
        while (KMainWindow::canBeRestored(n)) {
            // memory leak if more than one can be restored?
            RG_DEBUG << "Restoring from session - restoring app #" << n << endl;
            (rosegardengui = new RosegardenGUIApp)->restore(n);
            n++;
        }

    } else {
*/
/*&&& 
#ifndef NO_SOUND
        app.setNoSequencerMode(!args->isSet("sequencer"));
#else

        app.setNoSequencerMode(true);
#endif // NO_SOUND
*/
        rosegardengui = new RosegardenGUIApp(!app.noSequencerMode(), startLogo);

	rosegardengui->setIsFirstRun(newVersion);

        app.setMainWidget(rosegardengui);

	if (windowWidth != 0 && windowHeight != 0) {
	    rosegardengui->resize(windowWidth, windowHeight);
	}

        rosegardengui->show();

        // raise start logo
        //
        if (startLogo) {
            startLogo->raise();
            startLogo->setHideEnabled(true);
            QApplication::flushX();
        }

/*&&&
        if (args->count()) {
            rosegardengui->openFile(QFile::decodeName(args->arg(0)), RosegardenGUIApp::ImportCheckType);
        } else {
            // rosegardengui->openDocumentFile();
        }

        args->clear();
    }
*/

	//@@@ temporarily
	if (argc > 1) {
	    rosegardengui->openFile(QString::fromLocal8Bit(argv[1]));
	}


    QObject::connect(&app, SIGNAL(aboutToSaveState()),
                     rosegardengui, SLOT(slotDeleteTransport()));

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
        startLogo->setHideEnabled(true);
        QApplication::flushX();
    }

    // Check for sequencer and launch if needed
    //
    try {
        rosegardengui->launchSequencer();
    } catch (std::string e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (QString e) {
        RG_DEBUG << "RosegardenGUI - " << e << endl;
    } catch (Exception e) {
        RG_DEBUG << "RosegardenGUI - " << e.getMessage() << endl;
    }

    settings.endGroup();
    settings.beginGroup( SequencerOptionsConfigGroup );

    // See if the settings wants us to load a soundfont
    //
    if ( qStrToBool( settings.value("sfxloadenabled", "false" ) ) ) {
        QString sfxLoadPath = settings.value("sfxloadpath", "/bin/sfxload").toString();
        QString soundFontPath = settings.value("soundfontpath", "").toString();
        QFileInfo sfxLoadInfo(sfxLoadPath), soundFontInfo(soundFontPath);
        if (sfxLoadInfo.isExecutable() && soundFontInfo.isReadable()) {
            // setup sfxload Process
            QProcess* sfxLoadProcess = new QProcess;

            RG_DEBUG << "Starting sfxload : " << sfxLoadPath << " " << soundFontPath << endl;

            QObject::connect(sfxLoadProcess, SIGNAL(processExited(QProcess*)),
                             &app, SLOT(sfxLoadExited(QProcess*)));

            sfxLoadProcess->start(sfxLoadPath, (QStringList()) << soundFontPath);
        } else {
            RG_DEBUG << "sfxload not executable or soundfont not readable : "
            << sfxLoadPath << " " << soundFontPath << endl;
        }

    } else {
        RG_DEBUG << "sfxload disabled\n";
    }


#ifdef Q_WS_X11
    XSetErrorHandler( _x_errhandler );
#endif

    if (startLogo) {

        // pause to ensure the logo has been visible for a reasonable
        // length of time, just 'cos it looks a bit silly to show it
        // and remove it immediately

        struct timeval now;
        gettimeofday(&now, 0);

        RealTime visibleFor =
            RealTime(now.tv_sec, now.tv_usec * 1000) -
            RealTime(logoShowTime.tv_sec, logoShowTime.tv_usec * 1000);

        if (visibleFor < RealTime(2, 0)) {
            int waitTime = visibleFor.sec * 1000 + visibleFor.msec();
            QTimer::singleShot(2500 - waitTime, startLogo, SLOT(close()));
        } else {
            startLogo->close();
        }

    } else {

        // if the start logo is there, it's responsible for showing this;
        // otherwise we have to

	if (!newVersion) {
	    RosegardenGUIApp::self()->awaitDialogClearance();
	    QString tipResource = ResourceFinder().getResourcePath("", "tips");
	    if (tipResource != "") {
// 			KTipDialog::showTip(tipResource);	//&&&
	    }
	}
    }

    if (newVersion) {
        KStartupLogo::hideIfStillThere();
        CurrentProgressDialog::freeze();

        QDialog *dialog = new QDialog;
        dialog->setModal(true);
        dialog->setWindowTitle(i18n("Welcome!"));
        QGridLayout *metagrid = new QGridLayout;
        dialog->setLayout(metagrid);

        QWidget *hb = new QWidget;
        QHBoxLayout *hbLayout = new QHBoxLayout;
        metagrid->addWidget(hb, 0, 0);

        QLabel *image = new QLabel;
        hbLayout->addWidget(image);
        image->setAlignment(Qt::AlignTop);

	image->setPixmap(IconLoader().loadPixmap("welcome-icon"));

        QLabel *label = new QLabel;
        hbLayout->addWidget(label);
        label->setText(i18n("<h2>Welcome to Rosegarden!</h2><p>Welcome to the Rosegarden audio and MIDI sequencer and musical notation editor.</p><ul><li>If you have not already done so, you may wish to install some DSSI synth plugins, or a separate synth program such as QSynth.  Rosegarden does not synthesize sounds from MIDI on its own, so without these you will hear nothing.</li><br><br><li>Rosegarden uses the JACK audio server for recording and playback of audio, and for playback from DSSI synth plugins.  These features will only be available if the JACK server is running.</li><br><br><li>Rosegarden has comprehensive documentation: see the Help menu for the handbook, tutorials, and other information!</li></ul><p>Rosegarden was brought to you by a team of volunteers across the world.  To learn more, go to <a href=\"http://www.rosegardenmusic.com/\">http://www.rosegardenmusic.com/</a>.</p>"));
        label->setWordWrap(true);

        hb->setLayout(hbLayout);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
        metagrid->addWidget(buttonBox, 1, 0);
        metagrid->setRowStretch(0, 10);
	QObject::connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
	QObject::connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

        rosegardengui->awaitDialogClearance();
        dialog->exec();

	CurrentProgressDialog::thaw();
    }
    settings.endGroup();

    return qApp->exec();
}

