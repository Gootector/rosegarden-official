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


#ifndef _AUDIOFILE_H_
#define _AUDIOFILE_H_

#include <string>
#include <vector>
#include <math.h>

#include "SoundFile.h"
#include "RealTime.h"

// An AudioFile maintains information pertaining to an audio sample.
// This is an abstract base class from which we derive our actual
// AudioFile types - WAV, BWF, AIFF etc.
//
//

namespace Rosegarden
{

// The different types of audio file we support.
//
typedef enum
{

    UNKNOWN, // not yet known
    WAV,     // RIFF (Resource Interchange File Format) wav file 
    BWF,     // RIFF Broadcast Wave File
    AIFF     // Audio Interchange File Format

} AudioFileType;

class AudioFile : public SoundFile
{
public:
    // The "read" constructor - open a file
    // an assign a name and id to it.
    //
    AudioFile(unsigned int id,
              const std::string &name,
              const std::string &fileName);

    // The "write" constructor - we only need to
    // specify a filename and some parameters and
    // then write it out.
    //
    AudioFile(const std::string &fileName,
              unsigned int channels,
              unsigned int sampleRate,
              unsigned int bitsPerSample);

    ~AudioFile();

    // Id of this audio file (used by AudioFileManager)
    //
    void setId(unsigned int id) { m_id = id; }
    unsigned int getId() const { return m_id; }

    // Name of this sample - in addition to a filename
    //
    void setName(const std::string &name) { m_name = name; }
    std::string getName() const { return m_name; }

    // Useful methods that operate on our file data
    //
    int getIntegerFromLittleEndian(const std::string &s);
    std::string getLittleEndianFromInteger(unsigned int value,
                                           unsigned int length);

    int getIntegerFromBigEndian(const std::string &s);
    std::string getBigEndianFromInteger(unsigned int value,
                                        unsigned int length);

    // Used for waveform interpolation at a point
    //
    float sinc(float value) { return sin(M_PI * value)/ M_PI * value; }

    // Audio file identifier - set in constructor of file type
    //
    AudioFileType getType() { return m_type; }

    unsigned int getBitsPerSample() const { return m_bitsPerSample; }
    unsigned int getSampleRate() const { return m_sampleRate; }
    unsigned int getChannels() const { return m_channels; }
    
    // We must continue our main control abstract methods from SoundFile
    // and add our own close() method that will add any relevant header
    // information to an audio file that has been written and is now
    // being closed.
    //
    virtual bool open() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;

    // Show the information we have on this file
    //
    virtual void printStats() = 0;

    // Move file pointer to relative time in data chunk -
    // shouldn't be less than zero.  Returns true if the
    // scan time was valid and successful.
    // 
    virtual bool scanTo(std::ifstream *file, const RealTime &time) = 0;

    // Scan forward in a file by a certain amount of time
    //
    virtual bool scanForward(std::ifstream *file, const RealTime &time) = 0;

    // Return a number of samples - caller will have to
    // de-interleave n-channel samples themselves.
    //
    virtual std::string getSampleFrames(std::ifstream *file,
                                        unsigned int frames) = 0;

    // Return a number of (possibly) interleaved samples
    // over a time slice from current file pointer position.
    //
    virtual std::string getSampleFrameSlice(std::ifstream *file,
                                            const RealTime &time) = 0;

    // Append a string of samples to an already open (for writing)
    // audio file.
    //
    virtual bool appendSamples(const std::string &buffer) = 0;


    // Get the length of the sample file in RealTime
    //
    virtual RealTime getLength() = 0;

    // Must be accessible from the Driver
    //
    virtual void writeHeader() = 0;

    // Generate the preview
    //
    virtual std::vector<float> getPreview(const RealTime &resolution) = 0;

protected:
    virtual void parseHeader(const std::string &header) = 0;
    virtual void parseBody() = 0;

    AudioFileType  m_type;   // AudioFile type
    unsigned int   m_id;     // AudioFile ID
    std::string    m_name;   // AudioFile name (not filename)

    unsigned int   m_bitsPerSample;
    unsigned int   m_sampleRate;
    unsigned int   m_channels;
    unsigned int   m_fileSize;

    // How many bytes do we read before we get to the data?
    // Could be huge so we make it a long long. -1 means it
    // hasn't been set yet.
    //
    long long      m_dataChunkIndex;

    std::ifstream *m_inFile;
    std::ofstream *m_outFile;
};

}


#endif // _AUDIOFILE_H_
