/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <string>

#include <qlistview.h>

#include <kdialogbase.h>

#include "AudioFileManager.h"
#include "rosegardenguidoc.h"
#include "Segment.h"
#include "Selection.h"
#include "Track.h"
#include "dialogs.h"

// This dialog presents and allows editing of the Audio files that
// are in the Composition.  We edit the AudioFileManager directly
// and in this dialog we show previews and other pertinent file
// information.  This dialog also allows the user to preview the
// sounds and (eventually) to perform some operations of the audio
// files (split silence etc).
//
//

#ifndef _AUDIOMANAGERDIALOG_H_
#define _AUDIOMANAGERDIALOG_H_

class KURL;
class QPushButton;
class QPixmap;
class QAccel;
class QTimer;
class KListView;

namespace Rosegarden
{

class AudioFileManager;

class AudioManagerDialog : public KMainWindow
{
    Q_OBJECT

public:
    AudioManagerDialog(QWidget *parent,
                       RosegardenGUIDoc *doc);
    ~AudioManagerDialog();

    // Populate the file list from the AudioFileManager
    //

    // Return a pointer to the currently selected AudioFile -
    // returns 0 if nothing is selected
    //
    AudioFile* getCurrentSelection();

    // Scroll and expand to show this selected item
    //
    void setSelected(Rosegarden::AudioFileId id,
                     Rosegarden::Segment *segment,
                     bool propagate); // if true then we tell the segmentcanvas

    MultiViewCommandHistory *getCommandHistory();

    // Pop down playing dialog if it's currently up
    //
    void closePlayingDialog(Rosegarden::AudioFileId id);

    // Can we playback audio currently?
    //
    void setAudioSubsystemStatus(bool ok);

    // Return the accelerator object
    //
    QAccel* getAccelerators() { return m_accelerators; }

    // Add a new file to the audio file manager
    //
    bool addAudioFile(const QString &filePath);


public slots:
    void slotAdd();
    void slotAddCancel();
    void slotDelete();
    void slotPlayPreview();
    void slotRename();
    void slotInsert();
    void slotDeleteAll();
    void slotExportAudio();

    // get selection
    void slotSelectionChanged(QListViewItem *);

    // Repopulate
    //
    void slotPopulateFileList();

    // Commands
    //
    void slotCommandExecuted(KCommand *);

    /**
     * Accept a list of Segments and highlight accordingly
     * Used to reflect a selection on the main view
     * (when the user selects an audio track, the corresponding item
     * in the audio manager should be selected in turn)
     *
     * We check for embedded audio segments and if we find exactly one
     * we highlight it.  If we don't we unselect everything.
     *
     */
    void slotSegmentSelection(const Rosegarden::SegmentSelection &);

    /**
     * Cancel the currently playing audio file
     */
    void slotCancelPlayingAudioFile();

    void slotClose();

    /**
     * Turn a MIDI segment into a set of audio segments triggered
     * by the MIDI Note Ons
     */
    void slotDistributeOnMidiSegment();

signals:

    // Control signals so we can tell the sequencer about our changes
    // or actions.
    //
    void addAudioFile(Rosegarden::AudioFileId);
    void deleteAudioFile(Rosegarden::AudioFileId);
    void playAudioFile(Rosegarden::AudioFileId,
                       const Rosegarden::RealTime &,
                       const Rosegarden::RealTime &);
    void cancelPlayingAudioFile(Rosegarden::AudioFileId);
    void deleteAllAudioFiles();

    // We've selected a segment here, make the canvas select it too
    //
    void segmentsSelected(const Rosegarden::SegmentSelection&);
    void deleteSegments(const Rosegarden::SegmentSelection&);
    void insertAudioSegment(Rosegarden::AudioFileId,
                            const Rosegarden::RealTime &,
                            const Rosegarden::RealTime &);

    void closing();
protected slots:
    void slotDropped(QDropEvent*, QListViewItem*);
    void slotCancelPlayingAudio();

protected:
    bool addFile(const KURL& kurl);
    bool isSelectedTrackAudio();
    void selectFileListItemNoSignal(QListViewItem*);
    void updateActionState(bool haveSelection);

    virtual void closeEvent(QCloseEvent *);

    //--------------- Data members ---------------------------------

    KListView        *m_fileList;
    RosegardenGUIDoc *m_doc;

    QAccel           *m_accelerators;

    Rosegarden::AudioFileId  m_playingAudioFile;
    AudioPlayingDialog      *m_audioPlayingDialog;
    QTimer                  *m_playTimer;

    static const char* const m_listViewLayoutName;
    static const int         m_maxPreviewWidth;
    static const int         m_previewHeight;

    bool                     m_audiblePreview;

    static const char* const AudioManagerDialogConfigGroup;
};

}

#endif // _AUDIOAMANAGERDIALOG_H_
