#ifndef ROSEGARDENGUIIFACE_H
#define ROSEGARDENGUIIFACE_H

#include <MappedComposition.h>
#include <dcopobject.h>

class RosegardenGUIIface : virtual public DCOPObject
{
    K_DCOP
public:
k_dcop:
    virtual int  openFile(const QString &file) = 0;
    virtual int  importRG21File(const QString &file) = 0;
    virtual int  importMIDIFile(const QString &file) = 0;
    virtual void fileNew()                       = 0;
    virtual void fileSave()                      = 0;
    virtual void fileClose()                     = 0;
    virtual void quit()                          = 0;

    virtual const Rosegarden::MappedComposition&
            getSequencerSlice(const int &sliceStart, const int &sliceEnd) = 0;

    virtual void setPointerPosition(const int &position) = 0;
};

#endif
