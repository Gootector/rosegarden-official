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


// Accepts an AudioFIle and turns the sample data into peak data for
// storage in a peak file or a BWF format peak chunk.  Pixmaps or
// sample data is returned to callers on demand using these cached
// values.
//
//

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <qpixmap.h>


#ifndef _PEAKFILEMANAGER_H_
#define _PEAKFILEMANAGER_H_

namespace Rosegarden
{

class AudioFile;
class RealTime;


class PeakFileManager
{
public:
    // updatePercentage tells this object how often to throw a
    // percentage complete message - active between 0-100 only
    // if it's set to 5 then we send an update exception every
    // five percent.  The percentage complete is sent with 
    // each exception.
    //
    PeakFileManager();
    ~PeakFileManager();

    // Check that a given audio file has a valid and up to date
    // peak file or peak chunk.
    //
    //
    bool hasValidPeaks(AudioFile *audioFile);

    // Generate a peak file from file details - if the peak file already
    // exists _and_ it's up to date then we don't do anything.  For BWF
    // files we generate an internal peak chunk.
    //
    //
    void generatePeaks(AudioFile *audioFile,
                       unsigned short updatePercentage);

    // Generate a QPixmap to a given resolution - this function
    // makes full use of the peak files of course to render
    // a pixmap quickly for a given resolution.
    //
    QPixmap getPreview(AudioFile *audioFile,
                       const RealTime &startIndex,
                       const RealTime &endIndex,
                       int resolution, // samples per pixel
                       int height);    // pixels

    // Get a vector of floats as the preview
    //
    std::vector<float> getPreview(AudioFile *audioFile,
                                  const RealTime &startIndex,
                                  const RealTime &endIndex,
                                  int resolution);
                    
protected:

    // Calculate actual peaks from in file handle and write out -
    // based on interal file specs (channels, bits per sample).
    //
    void calculatePeaks(std::ifstream *in, std::ofstream *out);

    void readHeader();
    void writeHeader();

    std::string    m_riffFileName;
    std::streampos      m_dataChunk;
    unsigned int   m_bitsPerSample;
    unsigned int   m_channels;
    unsigned short m_updatePercentage;  // how often we send updates 


};


}


#endif // _PEAKFILEMANAGER_H_


