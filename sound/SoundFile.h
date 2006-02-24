// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*- /*
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


#ifndef _SOUNDFILE_H_
#define _SOUNDFILE_H_

// SoundFile is an abstract base class defining behaviour for both
// MidiFiles and AudioFiles.  The getBytes routine is buffered into
// suitably sized chunks to prevent excessive file reads.
// 
//

#include <iostream>
#include <fstream>
#include <string>

namespace Rosegarden
{


typedef unsigned char FileByte; 

class SoundFile
{
public:
    SoundFile(const std::string &fileName);
    virtual ~SoundFile();

    // All files should be able open, write and close
    virtual bool open() = 0;
    virtual bool write() = 0;
    virtual void close() = 0;

    std::string getShortFilename() const;
    std::string getFilename() const { return m_fileName; }
    void setFilename(const std::string &fileName) { m_fileName = fileName; }

    // Useful methods that operate on our file data
    //
    int getIntegerFromLittleEndian(const std::string &s);
    std::string getLittleEndianFromInteger(unsigned int value,
                                           unsigned int length);

    int getIntegerFromBigEndian(const std::string &s);
    std::string getBigEndianFromInteger(unsigned int value,
                                        unsigned int length);

    // Buffered read - allow this to be public
    //
    std::string getBytes(unsigned int numberOfBytes);

    // Return file size
    //
    unsigned int getSize() const { return m_fileSize; }

    void resetStream() { m_inFile->seekg(0); m_inFile->clear(); }

    // check EOF status
    //
    bool isEof() const 
        { if (m_inFile) return m_inFile->eof(); else return true; }

protected:
    std::string m_fileName;

    // get some bytes from an input stream - unbuffered as we can
    // modify the file stream
    std::string getBytes(std::ifstream *file, unsigned int numberOfBytes);

    // Get n bytes from an input stream and write them into buffer.
    // Return the actual number of bytes read.
    size_t getBytes(std::ifstream *file, char *buffer, size_t n);

    // write some bytes to an output stream
    void putBytes(std::ofstream *file, const std::string outputString);

    // write some bytes to an output stream
    void putBytes(std::ofstream *file, const char *buffer, size_t n);

    // Read buffering - define chunk size and buffer file reading
    //
    int            m_readChunkPtr;
    int            m_readChunkSize;
    std::string    m_readBuffer;

    std::ifstream *m_inFile;
    std::ofstream *m_outFile;
    
    bool           m_loseBuffer; // do we need to dump the read buffer
                                 // and re-fill it?

    unsigned int   m_fileSize;

};

}


#endif // _SOUNDFILE_H_


