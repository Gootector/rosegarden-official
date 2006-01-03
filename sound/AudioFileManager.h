// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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

#ifndef _AUDIOFILEMANAGER_H_
#define _AUDIOFILEMANAGER_H_

#include <string>
#include <vector>
#include <map>

#include <qpixmap.h>
#include <qobject.h>

#include "AudioFile.h"
#include "XmlExportable.h"
#include "PeakFileManager.h"
#include "PeakFile.h"
#include "Exception.h"

// AudioFileManager loads and maps audio files to their
// internal references (ids).  A point of contact for
// AudioFile information - loading a Composition should
// use this class to pick up the AudioFile references,
// editing the AudioFiles in a Composition will be
// made through this manager.

// This is in the sound library because it's so closely
// connected to other sound classes like the AudioFile
// ones.  However, the audio file manager itself within
// Rosegarden is stored in the GUI process.  This class
// is not (and should not be) used elsewhere within the
// sound or sequencer libraries.

namespace Rosegarden
{

typedef std::vector<AudioFile*>::const_iterator AudioFileManagerIterator;

class AudioFileManager : public QObject, public XmlExportable
{
    Q_OBJECT
public:
    AudioFileManager();
    virtual ~AudioFileManager();
    
    class BadAudioPathException : public Exception
    {
    public:
	BadAudioPathException(std::string path) :
	    Exception("Bad audio file path"), m_path(path) { }
	BadAudioPathException(std::string path, std::string file, int line) :
	    Exception("Bad audio file path", file, line), m_path(path) { }

	~BadAudioPathException() throw() { }

	std::string getPath() const { return m_path; }

    private:
	std::string m_path;
    };

private:
    AudioFileManager(const AudioFileManager &aFM);
    AudioFileManager& operator=(const AudioFileManager &);

public:

    // Create an audio file from an absolute path - we use this interface
    // to add an actual file.
    //
    AudioFileId addFile(const std::string &filePath);

    // Insert an audio file into the AudioFileManager and get the
    // first allocated id for it.  Used from the RG file as we already
    // have both name and filename/path.
    //
    AudioFileId insertFile(const std::string &name,
                           const std::string &fileName);

    // And insert an AudioFile and specify an id
    //
    bool insertFile(const std::string &name, const std::string &fileName,
                    AudioFileId id);

    // Remove a file from the AudioManager by id
    //
    bool removeFile(AudioFileId id);

    // Does a specific file id exist?
    //
    bool fileExists(AudioFileId id);

    // Does a specific file path exist?  Return ID or -1.
    //
    int fileExists(const std::string &path);

    // get audio file by id
    //
    AudioFile* getAudioFile(AudioFileId id);

    // Get the list of files
    //
    std::vector<AudioFile*>::const_iterator begin() const
        { return m_audioFiles.begin(); }

    std::vector<AudioFile*>::const_iterator end() const
        { return m_audioFiles.end(); }

    // Clear down all audio file references
    //
    void clear();

    // Get and set the record path
    //
    std::string getAudioPath() const { return m_audioPath; }
    void setAudioPath(const std::string &path);

    // Throw if the current audio path does not exist or is not writable
    //
    void testAudioPath() throw(BadAudioPathException);

    // Get a new audio filename at the audio record path
    //
    AudioFile *createRecordingAudioFile();

    // Get a set of new audio filenames at the audio record path
    //
    std::vector<std::string> createRecordingAudioFiles(unsigned int number);

    // return the last file in the vector - the last created
    //
    AudioFile* getLastAudioFile();

    // Export to XML
    //
    virtual std::string toXmlString();

    // Convenience function generate all previews on the audio file.
    //
    void generatePreviews();

    // Generate for a single audio file
    //
    bool generatePreview(AudioFileId id);

    // Get a preview for an AudioFile adjusted to Segment start and
    // end parameters (assuming they fall within boundaries).
    // 
    // We can get back a set of values (floats) or a Pixmap if we 
    // supply the details.
    //
    std::vector<float> getPreview(AudioFileId id,
                                  const RealTime &startTime, 
                                  const RealTime &endTime,
                                  int width,
                                  bool withMinima);

    // Draw a fixed size (fixed by QPixmap) preview of an audio file
    //
    void drawPreview(AudioFileId id,
                     const RealTime &startTime, 
                     const RealTime &endTime,
                     QPixmap *pixmap);

    // Usually used to show how an audio Segment makes up part of
    // an audio file.
    //
    void drawHighlightedPreview(AudioFileId it,
                                const RealTime &startTime,
                                const RealTime &endTime,
                                const RealTime &highlightStart,
                                const RealTime &highlightEnd,
                                QPixmap *pixmap);

    // Get a short file name from a long one (with '/'s)
    //
    std::string getShortFilename(const std::string &fileName);

    // Get a directory from a full file path
    //
    std::string getDirectory(const std::string &path);

    // Attempt to subsititute a tilde '~' for a home directory
    // to make paths a little more generic when saving.  Also
    // provide the inverse function as convenience here.
    //
    std::string substituteHomeForTilde(const std::string &path);
    std::string substituteTildeForHome(const std::string &path);

    // Show entries for debug purposes
    //
    void print(); 

    // Get a split point vector from a peak file
    //
    std::vector<SplitPointPair> 
        getSplitPoints(AudioFileId id,
                       const RealTime &startTime,
                       const RealTime &endTime,
                       int threshold,
                       const RealTime &minTime = RealTime(0, 100000000));

    // Get the peak file manager
    //
    const PeakFileManager& getPeakFileManager() const { return m_peakManager; }

    // Get the peak file manager
    //
    PeakFileManager& getPeakFileManager() { return m_peakManager; }

    // Cancel a running preview
    //
    void stopPreview();

signals:
    void setProgress(int);

private:
    std::string getFileInPath(const std::string &file);

    AudioFileId getFirstUnusedID();

    std::vector<AudioFile*>                       m_audioFiles;
    std::string                                   m_audioPath;

    PeakFileManager                               m_peakManager;

};

}

#endif // _AUDIOFILEMANAGER_H_
