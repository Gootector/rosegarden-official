// -*- c-basic-offset: 4 -*-

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

#include <string>
#include <vector>

#include "Device.h"
#include "Instrument.h"
#include "MidiProgram.h"


#ifndef _MIDIDEVICE_H_
#define _MIDIDEVICE_H_

namespace Rosegarden
{

typedef std::vector<std::string> StringList;
typedef std::vector<MidiProgram*> ProgramList;
typedef std::vector<MidiBank*> BankList;
typedef std::vector<MidiByte> MidiByteList;

class MidiDevice : public Device
{
public:
    typedef enum
    {
	Play = 0,
	Record = 1
    } DeviceDirection;

    typedef enum
    {
	NoVariations,
	VariationFromLSB,
	VariationFromMSB
    } VariationType;

    MidiDevice();
    MidiDevice(DeviceId id,
               const std::string &name,
               DeviceDirection dir);
    MidiDevice(DeviceId id,
               const std::string &name,
	       const std::string &label,
               DeviceDirection dir);
    virtual ~MidiDevice();

    // Copy constructor
    MidiDevice(const MidiDevice &);

    // Assignment
    MidiDevice &operator=(const MidiDevice &);

    virtual void addInstrument(Instrument*);

    void removeMetronome();
    void setMetronome(InstrumentId instrument,
		      const MidiMetronome &metronome);
    const MidiMetronome* getMetronome() const { return m_metronome; }

    void addProgram(MidiProgram *program);
    void addBank(MidiBank *bank);

    void clearBankList();
    void clearProgramList();

    const BankList getBanks(bool percussion) const;
    const BankList getBanksByMSB(bool percussion, MidiByte msb) const; 
    const BankList getBanksByLSB(bool percussion, MidiByte lsb) const;
    MidiByteList getDistinctMSBs(bool percussion, int lsb = -1) const;
    MidiByteList getDistinctLSBs(bool percussion, int msb = -1) const;

    const ProgramList getPrograms(const MidiBank &bank) const;

    std::string getBankName(const MidiBank &bank) const;
    std::string getProgramName(const MidiProgram &program) const;

    virtual std::string toXmlString();

    virtual InstrumentList getAllInstruments() const;
    virtual InstrumentList getPresentationInstruments() const;

    // Return a copy of banks and programs
    //
    std::vector<MidiBank> getBanks() const;
    std::vector<MidiProgram> getPrograms() const;

    void replaceBankList(const std::vector<Rosegarden::MidiBank> &bank);
    void replaceProgramList(const std::vector<Rosegarden::MidiProgram> &program);

    void mergeBankList(const std::vector<Rosegarden::MidiBank> &bank);
    void mergeProgramList(const std::vector<Rosegarden::MidiProgram> &program);

    // Retrieve Librarian details
    //
    const std::string getLibrarianName() const { return m_librarian.first; }
    const std::string getLibrarianEmail() const { return m_librarian.second; }
    std::pair<std::string, std::string> getLibrarian() const
        { return m_librarian; }

    // Set Librarian details
    //
    void setLibrarian(const std::string &name, const std::string &email)
        { m_librarian = std::pair<std::string, std::string>(name, email); }

    DeviceDirection getDirection() const { return m_direction; }
    void setDirection(DeviceDirection dir) { m_direction = dir; }

    VariationType getVariationType() const { return m_variationType; }
    void setVariationType(VariationType v) { m_variationType = v; }

protected:
    void generatePresentationList();

    ProgramList   *m_programList;
    BankList      *m_bankList;
    MidiMetronome *m_metronome;

    // used when we're presenting the instruments
    InstrumentList  m_presentationInstrumentList;

    // Is this device Play or Record?
    //
    DeviceDirection m_direction; 

    // Should we present LSB or MSB of bank info as a Variation number?
    //
    VariationType m_variationType;

    // Librarian contact details
    //
    std::pair<std::string, std::string> m_librarian; // name. email
};

}

#endif // _MIDIDEVICE_H_
