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


#include "AudioManagerDialog.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "AudioPlayingDialog.h"
#include "base/Composition.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/CommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/application/RosegardenGUIView.h"
#include "sequencer/RosegardenSequencer.h"
#include "gui/widgets/AudioListItem.h"
#include "gui/widgets/AudioListView.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include "gui/widgets/ProgressDialog.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/WAVAudioFile.h"
#include "UnusedAudioSelectionDialog.h"
#include "document/Command.h"
#include "gui/kdeext/KTmpStatusMsg.h"

#include <QApplication>
#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QAction>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QTreeWidget>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QVBoxLayout>
#include <QUrl>
#include <QShortcut>
#include <QKeySequence>

#include <Q3DragObject>
#include <Q3UriDrag>

#include <kurl.h>


namespace Rosegarden
{

const int AudioManagerDialog::m_maxPreviewWidth            = 100;
const int AudioManagerDialog::m_previewHeight              = 30;
const char* const AudioManagerDialog::m_listViewLayoutName = "AudioManagerDialog Layout";

AudioManagerDialog::AudioManagerDialog(QWidget *parent,
                                       RosegardenGUIDoc *doc):
        QMainWindow(parent, "audioManagerDialog"),
        m_doc(doc),
        m_playingAudioFile(0),
        m_audioPlayingDialog(0),
        m_playTimer(new QTimer(this)),
        m_audiblePreview(true)
{
    setCaption(QObject::tr("Audio File Manager"));
//    setWFlags(WDestructiveClose);	
	this->setAttribute( Qt::WA_DeleteOnClose );

    QWidget *box = new QWidget(this);
    QVBoxLayout *boxLayout = new QVBoxLayout;
    setCentralWidget(box);
    boxLayout->setMargin(10);
    boxLayout->setSpacing(5);

    m_sampleRate = RosegardenSequencer::getInstance()->getSampleRate();

    m_fileList = new AudioListView( box );
	m_fileList->setSelectionMode( QAbstractItemView::SingleSelection );
	m_fileList->setSelectionBehavior( QAbstractItemView::SelectRows );
//	m_fileList->setSelectionBehavior( QAbstractItemView::SelectItems );
	
    boxLayout->addWidget(m_fileList);

    m_wrongSampleRates = new QLabel( "...or adjusting the sample rate of the JACK server.", box );	//### FIX: text was cut
    boxLayout->addWidget(m_wrongSampleRates);
    box->setLayout(boxLayout);
    m_wrongSampleRates->hide();

    createAction("add_audio", SLOT(slotAdd()));
    createAction("export_audio", SLOT(slotExportAudio()));
    createAction("remove_audio", SLOT(slotRemove()));
    createAction("remove_all_audio", SLOT(slotRemoveAll()));
    createAction("remove_all_unused_audio", SLOT(slotRemoveAllUnused()));
    createAction("delete_unused_audio", SLOT(slotDeleteUnused()));
    createAction("preview_audio", SLOT(slotPlayPreview()));
    createAction("insert_audio", SLOT(slotInsert()));
    createAction("preview_audio", SLOT(slotPlayPreview()));

    //!!! oh now hang on, does this one work?
    createAction("distribute_audio", SLOT(slotDistributeOnMidiSegment()));

    // Set the column names
    //
	//
	
	//m_fileList->setHorizontalHeaderItem( 0, new QTreeWidgetItem( QObject::tr("File")));           // 6	
	QStringList sl;
	
	sl << QObject::tr("Name");           // 0
	sl << QObject::tr("Duration");       // 1	
	sl << QObject::tr("Envelope");       // 2
	sl << QObject::tr("Sample rate");    // 3
	sl << QObject::tr("Channels");       // 4
	sl << QObject::tr("Resolution");     // 5
	sl << QObject::tr("File");           // 6	
	
	m_fileList->setColumnCount(7);
	m_fileList->setHeaderItem( new QTreeWidgetItem( sl ) );
	
	/*
	//&&& table widget - column alignment
    m_fileList->setColumnAlignment(1, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(2, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(3, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(4, Qt::AlignHCenter);
    m_fileList->setColumnAlignment(5, Qt::AlignHCenter);
	
	*/
	
//	m_fileList->restoreLayout(m_listViewLayoutName);	// &&&

    // a minimum width for the list box
    //m_fileList->setMinimumWidth(300);

    // show focus across all columns
//    m_fileList->setAllColumnsShowFocus(true);

    // show tooltips when columns are partially hidden
//    m_fileList->setShowToolTips(true);

    // connect selection mechanism
    connect(m_fileList, SIGNAL(selectionChanged(QTreeWidgetItem*)),
            SLOT(slotSelectionChanged(QTreeWidgetItem*)));

    connect(m_fileList, SIGNAL(dropped(QDropEvent*, QTreeWidgetItem*)),
            SLOT(slotDropped(QDropEvent*, QTreeWidgetItem*)));

    // setup local shortcuts
    //
	m_shortcuts = new QShortcut( QKeySequence(Qt::Key_Delete), dynamic_cast<QWidget*>(this));

    // delete
    //
//    m_shortcuts->connectItem(m_shortcuts->addItem( Qt::Key_Delete ),
 //                               this,
   //                             SLOT(slotRemove()));
	connect( m_shortcuts, SIGNAL(activated()), this, SLOT(slotRemove()) );

    slotPopulateFileList();

    // Connect command history for updates
    //
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted(Command *)),
            this, SLOT(slotCommandExecuted(Command *)));

    //setInitialSize(configDialogSize(AudioManagerDialogConfigGroup));

    connect(m_playTimer, SIGNAL(timeout()),
            this, SLOT(slotCancelPlayingAudio()));

	
//    KStandardAction::close(this,
						   
	connect( this, SIGNAL(close()), this,
                      SLOT(slotClose()) );

    createGUI( "audiomanager.rc"); //@@@ JAS orig. 0

    updateActionState(false);
}

AudioManagerDialog::~AudioManagerDialog()
{
    RG_DEBUG << "\n*** AudioManagerDialog::~AudioManagerDialog\n" << endl;
	
//    m_fileList->saveLayout(m_listViewLayoutName);	//&&&
    //saveDialogSize(AudioManagerDialogConfigGroup);
}

void
AudioManagerDialog::slotPopulateFileList()
{
    // create pixmap of given size
    QPixmap *audioPixmap = new QPixmap(m_maxPreviewWidth, m_previewHeight);

    // Store last selected item if we have one
    //
   // AudioListItem *selectedItem = dynamic_cast<AudioListItem*>(m_fileList->selectedItem());
	AudioListItem *selectedItem = dynamic_cast<AudioListItem*>( m_fileList->currentItem() );
	
    AudioFileId lastId = 0;
    Segment *lastSegment = 0;
    bool findSelection = false;
    bool foundSelection = false;

    if (selectedItem) {
        lastId = selectedItem->getId();
        lastSegment = selectedItem->getSegment();
        findSelection = true;
    }

    // We don't want the selection changes to be propagated
    // to the main view
    //
    m_fileList->blockSignals(true);

    // clear file list and disable associated action buttons
    m_fileList->clear();
	AudioListItem* auItem;
    if (m_doc->getAudioFileManager().begin() ==
            m_doc->getAudioFileManager().end()) {
        // Turn off selection and report empty list
        //
		//new AudioListItem(m_fileList, QObject::tr("<no audio files>"), 0);
		auItem = new AudioListItem( m_fileList, QStringList( QObject::tr("<no audio files>")) );
		
		m_fileList->setSelectionMode( QAbstractItemView::NoSelection );
//        m_fileList->setRootIsDecorated(false);

        m_fileList->blockSignals(false);
        updateActionState(false);
        return ;
    }

    // show tree hierarchy
//    m_fileList->setRootIsDecorated(true); //&&&

    // enable selection
	m_fileList->setSelectionMode( QAbstractItemView::SingleSelection );

    // for the sample file length
    QString msecs, sRate;
    RealTime length;

    // Create a vector of audio Segments only
    //
    std::vector<Segment*> segments;
    std::vector<Segment*>::const_iterator iit;

    for (Composition::iterator it = m_doc->getComposition().begin();
            it != m_doc->getComposition().end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            segments.push_back(*it);
    }

    // duration
    RealTime segmentDuration;
    bool wrongSampleRates = false;

    for (std::vector<AudioFile*>::const_iterator
            it = m_doc->getAudioFileManager().begin();
            it != m_doc->getAudioFileManager().end();
            ++it) {
        try {
            m_doc->getAudioFileManager().
            drawPreview((*it)->getId(),
                        RealTime::zeroTime,
                        (*it)->getLength(),
                        audioPixmap);
        } catch (Exception e) {
            audioPixmap->fill(); // white
            QPainter p(audioPixmap);
            p.setPen(QColor(Qt::black));
            p.drawText(10, m_previewHeight / 2, QString("<no preview>"));
        }

        //!!! Why isn't the label the label the user assigned to the file?
        // Why do we allow the user to assign a label at all, then?

        QString label = QString((*it)->getShortFilename().c_str());

        // Set the label, duration, envelope pixmap and filename
        //
		
		//AudioListItem *item = new AudioListItem(m_fileList, label, (*it)->getId());
		AudioListItem *item = new AudioListItem( m_fileList, QStringList(label) ); //, (*it)->getId() );
//		m_fileList->setItem(0, item);
		
        // Duration
        //
        length = (*it)->getLength();
        msecs.sprintf("%03d", length.nsec / 1000000);
        //item->setText(1, QString("%1.%2s").arg(length.sec).arg(msecs));
		item->setText( 1,QString("%1.%2s").arg(length.sec).arg(msecs) );	// row, col

        // set start time and duration
        item->setStartTime(RealTime::zeroTime);
        item->setDuration(length);

        // Envelope pixmap
        //
        //item->setPixmap(2, *audioPixmap);
		item->setIcon( 2, QIcon(*audioPixmap) );	// row, col	
		

        // File location
        //
        item->setText( 6, QString(
                          m_doc->getAudioFileManager().
                          substituteHomeForTilde((*it)->getFilename()).c_str()));

        // Resolution
        //
		item->setText( 5, QString("%1 bits").arg((*it)->getBitsPerSample()));

        // Channels
        //
		item->setText( 4, QString("%1").arg((*it)->getChannels()));

        // Sample rate
        //
        if (m_sampleRate != 0 && (*it)->getSampleRate() != m_sampleRate) {
            sRate.sprintf("%.1f KHz *", float((*it)->getSampleRate()) / 1000.0);
            wrongSampleRates = true;
        } else {
            sRate.sprintf("%.1f KHz", float((*it)->getSampleRate()) / 1000.0);
        }
		item->setText( 3, sRate );

        // Test audio file element for selection criteria
        //
        if (findSelection && lastSegment == 0 && lastId == (*it)->getId()) {
			//m_fileList->setSelected(item, true);
			m_fileList->setCurrentItem( item );
			
			findSelection = false;
        }

        // Add children
        //
        for (iit = segments.begin(); iit != segments.end(); iit++) {
            if ((*iit)->getAudioFileId() == (*it)->getId()) {
                AudioListItem *childItem =
                    new AudioListItem( item,
                                      QStringList( QString((*iit)->getLabel().c_str()) ),
                                      (*it)->getId() );
                segmentDuration = (*iit)->getAudioEndTime() -
                                  (*iit)->getAudioStartTime();

                // store the start time
                //
                childItem->setStartTime((*iit)->getAudioStartTime());
                childItem->setDuration(segmentDuration);

                // Write segment duration
                //
                msecs.sprintf("%03d", segmentDuration.nsec / 1000000);
                childItem->setText(1, QString("%1.%2s")
                                   .arg(segmentDuration.sec)
                                   .arg(msecs));

                try {
                    m_doc->getAudioFileManager().
                    drawHighlightedPreview((*it)->getId(),
                                           RealTime::zeroTime,
                                           (*it)->getLength(),
                                           (*iit)->getAudioStartTime(),
                                           (*iit)->getAudioEndTime(),
                                           audioPixmap);
                } catch (Exception e) {
                    // should already be set to "no file"
                }

                // set pixmap
                //
				//childItem->setPixmap(2, *audioPixmap);
				childItem->setIcon(2, QIcon(*audioPixmap) );

                // set segment
                //
                childItem->setSegment(*iit);

                if (findSelection && lastSegment == (*iit)) {
                    m_fileList->setCurrentItem( childItem );	// select
                    findSelection = false;
                    foundSelection = true;
                }

                // Add children
            }
        }
    }

    updateActionState(foundSelection);

    if (wrongSampleRates) {
        m_wrongSampleRates->show();
    } else {
        m_wrongSampleRates->hide();
    }

    m_fileList->blockSignals(false);
}

AudioFile*
AudioManagerDialog::getCurrentSelection()
{
    // try and get the selected item
	QList<QTreeWidgetItem *> til= m_fileList->selectedItems();
	if (til.isEmpty()){
		QMessageBox::warning
				(this, "Error: Selection is empty!", QObject::tr("Please select an audio item in the list!"), QMessageBox::Yes );
		return 0;
	}
    AudioListItem *item = dynamic_cast<AudioListItem*>( til[0] );
    if (item == 0)
        return 0;

    std::vector<AudioFile*>::const_iterator it;

    for (it = m_doc->getAudioFileManager().begin();
            it != m_doc->getAudioFileManager().end();
            ++it) {
        // If we match then return the valid AudioFile
        //
        if (item->getId() == (*it)->getId())
            return (*it);
    }

    return 0;
}

void
AudioManagerDialog::slotExportAudio()
{
    WAVAudioFile *sourceFile
    = dynamic_cast<WAVAudioFile*>(getCurrentSelection());

	QList<QTreeWidgetItem *> til= m_fileList->selectedItems();
	if (til.isEmpty()){
		QMessageBox::warning
				(this, "Error: Selection is empty!", QObject::tr("Please select an audio item in the list!"), QMessageBox::Yes );
		return;
	}
	AudioListItem *item = dynamic_cast<AudioListItem*>( til[0] );

    Segment *segment = item->getSegment();

    QString saveFile =
			QFileDialog::getSaveFileName( this, QObject::tr("Choose a name to save this file as ").arg(":WAVS"), QDir::currentPath(), QObject::tr("*.wav|WAV files (*.wav)" ) );

    if (sourceFile == 0 || item == 0 || saveFile.isEmpty())
        return ;

    // Check for a dot extension and append ".wav" if not found
    //
    if (saveFile.contains(".") == 0)
        saveFile += ".wav";

    ProgressDialog progressDlg(QObject::tr("Exporting audio file..."),
                               100,
                               this);

    progressDlg.progressBar()->setValue(0);

    RealTime clipStartTime = RealTime::zeroTime;
    RealTime clipDuration = sourceFile->getLength();

    if (segment) {
        clipStartTime = segment->getAudioStartTime();
        clipDuration = segment->getAudioEndTime() - clipStartTime;
    }

    WAVAudioFile *destFile
    = new WAVAudioFile(qstrtostr(saveFile),
                       sourceFile->getChannels(),
                       sourceFile->getSampleRate(),
                       sourceFile->getBytesPerSecond(),
                       sourceFile->getBytesPerFrame(),
                       sourceFile->getBitsPerSample());

    if (sourceFile->open() == false) {
        delete destFile;
        return ;
    }

    destFile->write();

    sourceFile->scanTo(clipStartTime);
    destFile->appendSamples(sourceFile->getSampleFrameSlice(clipDuration));

    destFile->close();
    sourceFile->close();
    delete destFile;

    progressDlg.progressBar()->setValue(100);
}

void
AudioManagerDialog::slotRemove()
{
    AudioFile *audioFile = getCurrentSelection();
	QList<QTreeWidgetItem*> til = m_fileList->selectedItems();
	if (til.isEmpty() ){
		QMessageBox::warning
				(this, "Error: Selection is empty!", QObject::tr("Please select an audio item in the list!"), QMessageBox::Yes );
		return;
	}
    AudioListItem *item = dynamic_cast<AudioListItem*>( til[0] );

    if (audioFile == 0 || item == 0)
        return ;

    // If we're on a Segment then delete it at the Composition
    // and refresh the list.
    //
    if (item->getSegment()) {
        // Get the next item to highlight
        //
//		QTreeWidgetItem *newItem = item->itemBelow();
		//QTreeWidgetItem *newItem = m_fileList->item( (item->row()+1), item->column() );
		QTreeWidgetItem *newItem = m_fileList->itemBelow( item );
		
        // Or try above
        //
        if (newItem == 0)
            newItem = m_fileList->itemAbove( item );

        // Or the parent
        //
        if (newItem == 0)
            newItem = item->parent();

        // Get the id and segment of the next item so that we can
        // match against it
        //
        AudioFileId id = 0;
        Segment *segment = 0;
        AudioListItem *aItem = dynamic_cast<AudioListItem*>(newItem);

        if (aItem) {
            segment = aItem->getSegment();
            id = aItem->getId();
        }

        // Jump to new selection
        //
        if (newItem)
            setSelected(id, segment, true); // propagate

        // Do it - will force update
        //
        SegmentSelection selection;
        selection.insert(item->getSegment());
        emit deleteSegments(selection);

        return ;
    }

    // remove segments along with audio file
    //
    AudioFileId id = audioFile->getId();
    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    bool haveSegments = false;
    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio &&
                (*it)->getAudioFileId() == id) {
            haveSegments = true;
            break;
        }
    }

    if (haveSegments) {

        QString question = QObject::tr("This will unload audio file \"%1\" and remove all associated segments.  Are you sure?")
                           .arg(QString(audioFile->getFilename().c_str()));

        // Ask the question
		int reply = QMessageBox::warning(this, "", question, QMessageBox::Yes | QMessageBox::Cancel , QMessageBox::Cancel );
		
        if (reply != QMessageBox::Yes)
            return ;
    }
	
    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio &&
                (*it)->getAudioFileId() == id)
            selection.insert(*it);
    }
    emit deleteSegments(selection);

    m_doc->notifyAudioFileRemoval(id);

    m_doc->getAudioFileManager().removeFile(id);

    // tell the sequencer
    emit deleteAudioFile(id);

    // repopulate
    slotPopulateFileList();
}

void
AudioManagerDialog::slotPlayPreview()
{
    AudioFile *audioFile = getCurrentSelection();
	
	QList<QTreeWidgetItem*> til = m_fileList->selectedItems();
	if (til.isEmpty() ){
		QMessageBox::warning
				(this, "Error: Selection is empty!", QObject::tr("Please select an audio item in the list!"), QMessageBox::Yes );
		return;
	}
	AudioListItem *item = dynamic_cast<AudioListItem*>( til[0] );

    if (item == 0 || audioFile == 0)
        return ;

    // store the audio file we're playing
    m_playingAudioFile = audioFile->getId();

    // tell the sequencer
    emit playAudioFile(audioFile->getId(),
                       item->getStartTime(),
                       item->getDuration());

    // now open up the playing dialog
    //
    m_audioPlayingDialog =
        new AudioPlayingDialog(this, QString(audioFile->getFilename().c_str()));

    // Setup timer to pop down dialog after file has completed
    //
    int msecs = item->getDuration().sec * 1000 +
                item->getDuration().nsec / 1000000;
    m_playTimer->start(msecs, true); // single shot

    // just execute
    //
    if (m_audioPlayingDialog->exec() == QDialog::Rejected)
        emit cancelPlayingAudioFile(m_playingAudioFile);

    delete m_audioPlayingDialog;
    m_audioPlayingDialog = 0;

    m_playTimer->stop();

}

void
AudioManagerDialog::slotCancelPlayingAudio()
{
    //std::cout << "AudioManagerDialog::slotCancelPlayingAudio" << std::endl;
    if (m_audioPlayingDialog) {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = 0;
    }
}

void
AudioManagerDialog::slotAdd()
{
    QString extensionList = QObject::tr("*.wav|WAV files (*.wav)\n*.*|All files");
    
    if (RosegardenGUIApp::self()->haveAudioImporter()) {
	//!!! This list really needs to come from the importer helper program
	// (which has an option to supply it -- we just haven't recorded it)
        extensionList = QObject::tr("*.wav *.flac *.ogg *.mp3|Audio files (*.wav *.flac *.ogg *.mp3)\n*.wav|WAV files (*.wav)\n*.flac|FLAC files (*.flac)\n*.ogg|Ogg files (*.ogg)\n*.mp3|MP3 files (*.mp3)\n*.*|All files");
    }

	QStringList kurlList;
	kurlList = QFileDialog::getOpenFileNames( this, QObject::tr("Select one or more audio files").arg(":WAVS"), QDir::currentPath(), extensionList );
                                 //  QObject::tr("*.wav|WAV files (*.wav)\n*.mp3|MP3 files (*.mp3)"),);

	//KURL::List::iterator it;

	//for (it = kurlList.begin(); it != kurlList.end(); ++it)
	//	addFile(*it);
	for( int i=0; i < kurlList.size(); i++ )
		addFile( kurlList.at(i) );
}

void
AudioManagerDialog::updateActionState(bool haveSelection)
{
    if (m_doc->getAudioFileManager().begin() ==
            m_doc->getAudioFileManager().end()) {
        leaveActionState("have_audio_files"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    } else {
        enterActionState("have_audio_files"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
    }

    if (haveSelection) {

        enterActionState("have_audio_selected"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse

        if (m_audiblePreview) {
            enterActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            leaveActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }

        if (isSelectedTrackAudio()) {
            enterActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateNoReverse
        } else {
            leaveActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        }

    } else {
        leaveActionState("have_audio_selected"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_audio_insertable"); //@@@ JAS orig. KXMLGUIClient::StateReverse
        leaveActionState("have_audible_preview"); //@@@ JAS orig. KXMLGUIClient::StateReverse
    }
}

void
AudioManagerDialog::slotInsert()
{
    AudioFile *audioFile = getCurrentSelection();
    if (audioFile == 0)
        return ;

    RG_DEBUG << "AudioManagerDialog::slotInsert\n";

    emit insertAudioSegment(audioFile->getId(),
                            RealTime::zeroTime,
                            audioFile->getLength());
}

void
AudioManagerDialog::slotRemoveAll()
{
    QString question =
        QObject::tr("This will unload all audio files and remove their associated segments.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

	int reply = QMessageBox::warning(this, "", question, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

    if (reply != QMessageBox::Yes)
        return ;

    SegmentSelection selection;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            selection.insert(*it);
    }
    // delete segments
    emit deleteSegments(selection);

    for (std::vector<AudioFile*>::const_iterator
            aIt = m_doc->getAudioFileManager().begin();
            aIt != m_doc->getAudioFileManager().end(); ++aIt) {
        m_doc->notifyAudioFileRemoval((*aIt)->getId());
    }

    m_doc->getAudioFileManager().clear();

    // and now the audio files
    emit deleteAllAudioFiles();

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
}

void
AudioManagerDialog::slotRemoveAllUnused()
{
    QString question =
        QObject::tr("This will unload all audio files that are not associated with any segments in this composition.\nThis action cannot be undone, and associations with these files will be lost.\nFiles will not be removed from your disk.\nAre you sure?");

	int reply = QMessageBox::warning(this, "", question,QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel );

    if (reply != QMessageBox::Yes)
        return ;

    std::set
        <AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<AudioFileId> toDelete;
    for (std::vector<AudioFile*>::const_iterator
            aIt = m_doc->getAudioFileManager().begin();
            aIt != m_doc->getAudioFileManager().end(); ++aIt) {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end())
            toDelete.push_back((*aIt)->getId());
    }

    // Delete the audio files from the AFM
    //
    for (std::vector<AudioFileId>::iterator dIt = toDelete.begin();
            dIt != toDelete.end(); ++dIt) {
        
        m_doc->notifyAudioFileRemoval(*dIt);
        m_doc->getAudioFileManager().removeFile(*dIt);
        emit deleteAudioFile(*dIt);
    }

    // clear the file list
    m_fileList->clear();
    slotPopulateFileList();
}

void
AudioManagerDialog::slotDeleteUnused()
{
    std::set
        <AudioFileId> audioFiles;
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator it = comp.begin(); it != comp.end(); ++it) {
        if ((*it)->getType() == Segment::Audio)
            audioFiles.insert((*it)->getAudioFileId());
    }

    std::vector<QString> toDelete;
    std::map<QString, AudioFileId> nameMap;

    for (std::vector<AudioFile*>::const_iterator
            aIt = m_doc->getAudioFileManager().begin();
            aIt != m_doc->getAudioFileManager().end(); ++aIt) {
        if (audioFiles.find((*aIt)->getId()) == audioFiles.end()) {
            toDelete.push_back(strtoqstr((*aIt)->getFilename()));
            nameMap[strtoqstr((*aIt)->getFilename())] = (*aIt)->getId();
        }
    }

    UnusedAudioSelectionDialog *dialog = new UnusedAudioSelectionDialog
                                         (this,
                                          QObject::tr("The following audio files are not used in the current composition.\n\nPlease select the ones you wish to delete permanently from the hard disk.\n"),
                                          toDelete);

    if (dialog->exec() == QDialog::Accepted) {

        std::vector<QString> names = dialog->getSelectedAudioFileNames();

        if (names.size() > 0) {

            QString question =
                QObject::tr("<qt>About to delete %n audio file(s) permanently from the hard disk.<br>This action cannot be undone, and there will be no way to recover the files.<br>Are you sure?</qt>", "", names.size());

			int reply = QMessageBox::warning(this, "", question, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel );

            if (reply != QMessageBox::Yes) {
                delete dialog;
                return ;
            }

            for (int i = 0; i < names.size(); ++i) {
                std::cerr << i << ": " << names[i] << std::endl;
                QFile file(names[i]);
                if (!file.remove()) {
                    QMessageBox::critical(this, "", QObject::tr("File %1 could not be deleted.").arg(names[i]));
                } else {
                    if (nameMap.find(names[i]) != nameMap.end()) {
                        m_doc->getAudioFileManager().removeFile(nameMap[names[i]]);
                        emit deleteAudioFile(nameMap[names[i]]);
                    } else {
                        std::cerr << "WARNING: Audio file name " << names[i] << " not in name map" << std::endl;
                    }

                    QFile peakFile(QString("%1.pk").arg(names[i]));
                    peakFile.remove();
                }
            }
        }
    }

    m_fileList->clear();
    slotPopulateFileList();

    delete dialog;
}

void
AudioManagerDialog::slotRename()
{
    AudioFile *audioFile = getCurrentSelection();

    if (audioFile == 0)
        return ;

    bool ok = false;

    QString newText = QInputDialog::getText( this, 
                          QObject::tr("Change Audio File label"),
							   QObject::tr("Enter new label"), 
							QLineEdit::Normal, 
                          QString(audioFile->getName().c_str()),
                          &ok );

    if ( ok && !newText.isEmpty() )
        audioFile->setName(qstrtostr(newText));

    slotPopulateFileList();
}

void
AudioManagerDialog::slotSelectionChanged(QTreeWidgetItem *item)
{
    AudioListItem *aItem = dynamic_cast<AudioListItem*>(item);

    // If we're on a segment then send a "select" signal
    // and enable appropriate buttons.
    //
    if (aItem && aItem->getSegment()) {
        SegmentSelection selection;
        selection.insert(aItem->getSegment());
        emit segmentsSelected(selection);
    }

    updateActionState(aItem != 0);
}

void
AudioManagerDialog::setSelected(AudioFileId id,
                                const Segment *segment,
                                bool propagate)
{
    QTreeWidgetItem *it = m_fileList->itemAt(0,0); //firstChild();
    QTreeWidgetItem *chIt = 0;
    AudioListItem *aItem;
	int nn;
    while (it) {
        // If we're looking for a top level audio file
        if (segment == 0) {
            aItem = dynamic_cast<AudioListItem*>(it);

            if (aItem->getId() == id) {
                selectFileListItemNoSignal(it);
                return ;
            }
        } else // look for a child
        {
            if (it->childCount() > 0)
				//chIt = it->firstChild();
				//chIt = it->itemAt(0,0);
				chIt = it->child(0);
			nn = 0;
			while (chIt && (chIt->childCount() < nn) ) {
				nn +=1;
                aItem = dynamic_cast<AudioListItem*>(chIt);

                if (aItem) {
                    if (aItem->getId() == id && aItem->getSegment() == segment) {
                        selectFileListItemNoSignal(chIt);

                        // Only propagate to compositionview if asked to
                        if (propagate) {
                            SegmentSelection selection;
                            selection.insert(aItem->getSegment());
                            emit segmentsSelected(selection);
                        }

                        return ;
                    }
                }
				//chIt = chIt->nextSibling();
				//chIt = it->itemBelow( chIt );
				chIt = it->child( nn );
			}
        }

        //it = it->nextSibling();
		chIt = m_fileList->itemBelow( chIt );
	}

}

void
AudioManagerDialog::selectFileListItemNoSignal(QTreeWidgetItem* it)
{
    m_fileList->blockSignals(true);

    if (it) {
//        m_fileList->ensureItemVisible(it);
		m_fileList->scrollToItem(it, QAbstractItemView::PositionAtTop );
//		m_fileList->setSelected(it, true);
		m_fileList->setCurrentItem( it );
	} else {
        m_fileList->clearSelection();
    }

    m_fileList->blockSignals(false);
}

void
AudioManagerDialog::slotCommandExecuted(Command*)
{
    slotPopulateFileList();
}

void
AudioManagerDialog::slotSegmentSelection(
    const SegmentSelection &segments)
{
    const Segment *segment = 0;

    for (SegmentSelection::const_iterator it = segments.begin();
            it != segments.end(); ++it) {
        if ((*it)->getType() == Segment::Audio) {
            // Only get one audio segment
            if (segment == 0)
                segment = *it;
            else
                segment = 0;
        }

    }

    if (segment) {
        // We don't propagate this segment setting to the canvas
        // as we probably got called from there.
        //
        setSelected(segment->getAudioFileId(), segment, false);
    } else {
        selectFileListItemNoSignal(0);
    }

}

void
AudioManagerDialog::slotCancelPlayingAudioFile()
{
    emit cancelPlayingAudioFile(m_playingAudioFile);
}

void
AudioManagerDialog::closePlayingDialog(AudioFileId id)
{
    //std::cout << "AudioManagerDialog::closePlayingDialog" << std::endl;
    if (m_audioPlayingDialog && id == m_playingAudioFile) {
        m_playTimer->stop();
        delete m_audioPlayingDialog;
        m_audioPlayingDialog = 0;
    }

}

bool
AudioManagerDialog::addFile(const QUrl& kurl)
{
    AudioFileId id = 0;

    AudioFileManager &aFM = m_doc->getAudioFileManager();

    if (! QFile::exists(kurl.toLocalFile()) ) {
		
		if (!RosegardenGUIApp::self()->testAudioPath(QObject::tr("importing a remote audio file"))) return false;
    } else if (aFM.fileNeedsConversion(qstrtostr(kurl.path()), m_sampleRate)) {
        if (!RosegardenGUIApp::self()->testAudioPath(QObject::tr("importing an audio file that needs to be converted or resampled"))) return false;
    }
	
    ProgressDialog progressDlg(QObject::tr("Adding audio file..."),
                               100,
                               this);

    CurrentProgressDialog::set(&progressDlg);
    progressDlg.progressBar()->hide();
    progressDlg.show();

    // Connect the progress dialog
    //
    connect(&aFM, SIGNAL(setValue(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));
    connect(&aFM, SIGNAL(setOperationName(QString)),
            &progressDlg, SLOT(slotSetOperationName(QString)));
    connect(&progressDlg, SIGNAL(cancelClicked()),
            &aFM, SLOT(slotStopImport()));

    try {
        id = aFM.importURL(kurl, m_sampleRate);
    } catch (AudioFileManager::BadAudioPathException e) {
        CurrentProgressDialog::freeze();
        QString errorString = QObject::tr("Failed to add audio file. ") + strtoqstr(e.getMessage());
        /* was sorry */ QMessageBox::warning(this, "", errorString);
        return false;
    } catch (SoundFile::BadSoundFileException e) {
        CurrentProgressDialog::freeze();
        QString errorString = QObject::tr("Failed to add audio file. ") + strtoqstr(e.getMessage());
        /* was sorry */ QMessageBox::warning(this, "", errorString);
        return false;
    }
            
    disconnect(&progressDlg, SIGNAL(cancelClicked()),
               &aFM, SLOT(slotStopImport()));
    connect(&progressDlg, SIGNAL(cancelClicked()),
            &aFM, SLOT(slotStopPreview()));
    progressDlg.progressBar()->show();
    progressDlg.slotSetOperationName(QObject::tr("Generating audio preview..."));

    try {
        aFM.generatePreview(id);
    } catch (Exception e) {
        CurrentProgressDialog::freeze();

        QString message = strtoqstr(e.getMessage()) + "\n\n" +
                          QObject::tr("Try copying this file to a directory where you have write permission and re-add it");
        QMessageBox::information(this, "", message);
    }

    disconnect(&progressDlg, SIGNAL(cancelClicked()),
               &aFM, SLOT(slotStopPreview()));

    slotPopulateFileList();

    // tell the sequencer
    emit addAudioFile(id);

    return true;
}

void
AudioManagerDialog::slotDropped(QDropEvent *event, QTreeWidgetItem*)
		// dropEvent
{
	//QStrList uri;
	QList<QString> uri;

    // see if we can decode a URI.. if not, just ignore it
//    if (QUriDrag::decode(event, uri)) {			//&&& QUriDrag, implement drag/drop
		
        // okay, we have a URI.. process it
//        for (QString url = uri.first(); !url.isEmpty(); url = uri.next()) { //!!! this one is really weird and uncertain
		for( int i=0; i < uri.size(); i++ ){
			QString url = uri.at(i);
			
            RG_DEBUG << "AudioManagerDialog::dropEvent() : got "
            << url << endl;

            addFile( QUrl(url) );
        }

//    }// end if QUriDrag
}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    RG_DEBUG << "AudioManagerDialog::closeEvent()\n";
    emit closing();
    QMainWindow::closeEvent(e);
}

void
AudioManagerDialog::slotClose()
{
    emit closing();
    close();
    //KDockMainWindow::slotClose();
    //     delete this;
}

void
AudioManagerDialog::setAudioSubsystemStatus(bool ok)
{
    // We can do something more fancy in the future but for the moment
    // this will suffice.
    //
    m_audiblePreview = ok;
}

bool
AudioManagerDialog::addAudioFile(const QString &filePath)
{
	QString fp = QFileInfo(filePath).absFilePath();
	RG_DEBUG << "\\_AudioFilePath : " << fp << endl;
	return addFile( fp );
}

bool
AudioManagerDialog::isSelectedTrackAudio()
{
    Composition &comp = m_doc->getComposition();
    Studio &studio = m_doc->getStudio();

    TrackId currentTrackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(currentTrackId);

    if (track) {
        InstrumentId ii = track->getInstrument();
        Instrument *instrument = studio.getInstrumentById(ii);

        if (instrument &&
                instrument->getType() == Instrument::Audio)
            return true;
    }

    return false;

}

void
AudioManagerDialog::slotDistributeOnMidiSegment()
{
    RG_DEBUG << "AudioManagerDialog::slotDistributeOnMidiSegment" << endl;

    //Composition &comp = m_doc->getComposition();

    QList<RosegardenGUIView*> viewList_ = m_doc->getViewList();
	QListIterator<RosegardenGUIView*> viewList( viewList_ );

    RosegardenGUIView *w = 0;
    SegmentSelection selection;

	viewList.toFront();
	while( viewList.hasNext() ){
		w = viewList.next();
        selection = w->getSelection();
    }

    // Store the insert times in a local vector
    //
    std::vector<timeT> insertTimes;

    for (SegmentSelection::iterator i = selection.begin();
            i != selection.end(); ++i) {
        // For MIDI (Internal) Segments only of course
        //
        if ((*i)->getType() == Segment::Internal) {
            for (Segment::iterator it = (*i)->begin(); it != (*i)->end(); ++it) {
                if ((*it)->isa(Note::EventType))
                    insertTimes.push_back((*it)->getAbsoluteTime());
            }
        }
    }

    for (unsigned int i = 0; i < insertTimes.size(); ++i) {
        RG_DEBUG << "AudioManagerDialog::slotDistributeOnMidiSegment - "
        << "insert audio segment at " << insertTimes[i]
        << endl;
    }
}

}
#include "AudioManagerDialog.moc"
