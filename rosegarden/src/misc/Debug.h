// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef ROSEDEBUG_H
#define ROSEDEBUG_H

#include <string>
#include <iostream>
#include <kdebug.h>
#include <kdeversion.h>

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
class QDateTime;
class QDate;
class QTime;
class QPoint;
class QSize;
class QRect;
class QRegion;
class KURL;
class QStringList;
class QColor;
class QPen;
class QBrush;
#endif

namespace Rosegarden { class Event; class Segment; class RealTime; class Colour; }

#define KDEBUG_AREA                 200000
#define KDEBUG_AREA_NOTATION        200001
#define KDEBUG_AREA_MATRIX          200002
#define KDEBUG_AREA_SEQUENCER       200003
#define KDEBUG_AREA_SEQUENCEMANAGER 200004

#define RG_DEBUG        kdDebug(KDEBUG_AREA)
#define NOTATION_DEBUG  kdDebug(KDEBUG_AREA_NOTATION)
#define MATRIX_DEBUG    kdDebug(KDEBUG_AREA_MATRIX)
#define SEQUENCER_DEBUG kdDebug(KDEBUG_AREA_SEQUENCER)
#define SEQMAN_DEBUG    kdDebug(KDEBUG_AREA_SEQUENCEMANAGER)

#ifndef NDEBUG

kdbgstream&
operator<<(kdbgstream&, const std::string&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::Event&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::Segment&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::RealTime&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::Colour&);

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
kdbgstream& 
operator << (kdbgstream&, const QDateTime& dateTime );

kdbgstream& 
operator << (kdbgstream&, const QDate& date );

kdbgstream& 
operator << (kdbgstream&, const QTime& time );

kdbgstream& 
operator << (kdbgstream&, const QPoint& point );

kdbgstream& 
operator << (kdbgstream&, const QSize& size );

kdbgstream& 
operator << (kdbgstream&, const QRect& rect);

kdbgstream& 
operator << (kdbgstream&, const QRegion& region);

kdbgstream& 
operator << (kdbgstream&, const KURL& url );

kdbgstream& 
operator << (kdbgstream&, const QStringList& list);

kdbgstream& 
operator << (kdbgstream&, const QColor& color);

kdbgstream& 
operator << (kdbgstream&, const QPen& pen );

kdbgstream& 
operator << (kdbgstream&, const QBrush& brush );
#endif

#else

inline kndbgstream&
operator<<(kndbgstream &s, const std::string&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::Event&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::Segment&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::RealTime&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::Colour&) { return s; }

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
inline kndbgstream& 
operator << (kndbgstream& s, const QDateTime&  ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QDate&  ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QTime& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QPoint& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QSize& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QRect& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QRegion& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const KURL& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QStringList& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QColor& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QPen& ) { return s; }

inline kndbgstream& 
operator << (kndbgstream& s, const QBrush& ) { return s; }

#endif

#endif

#ifndef NO_TIMING

#include <iostream>
#include <ctime>

#define START_TIMING \
  clock_t dbgStart = clock();
#define ELAPSED_TIME \
  ((clock() - dbgStart) * 1000 / CLOCKS_PER_SEC)
#define PRINT_ELAPSED(n) \
  RG_DEBUG << n << ": " << ELAPSED_TIME << "ms elapsed" << endl;

#else

#define START_TIMING
#define ELAPSED_TIME  0
#define PRINT_ELAPSED(n)

#endif




// This doesn't work - keeping it just in case I somehow get it
// working someday

#ifdef NOT_DEFINED

// can't be bothered to even get this to compile with gcc-3.0 at the
// moment

class kdbgostreamAdapter : public std::ostream
{
public:
    kdbgostreamAdapter(kdbgstream &e) : m_kdbgStream(e) {}

    std::ostream& operator<<(bool i);
    std::ostream& operator<<(short i);
    std::ostream& operator<<(unsigned short i);
    std::ostream& operator<<(char i);
    std::ostream& operator<<(unsigned char i);
    std::ostream& operator<<(int i);
    std::ostream& operator<<(unsigned int i);
    std::ostream& operator<<(long i);
    std::ostream& operator<<(unsigned long i);
    std::ostream& operator<<(const QString& str);
    std::ostream& operator<<(const char *str);
    std::ostream& operator<<(const QCString& str);
    std::ostream& operator<<(void * p);
    std::ostream& operator<<(KDBGFUNC f);
    std::ostream& operator<<(double d);

    kdbgstream& dbgStream() { return m_kdbgStream; }

protected:
    kdbgstream &m_kdbgStream;
};

#endif

// std::ostream& endl(std::ostream& s);

void DBCheckThrow();


#endif
