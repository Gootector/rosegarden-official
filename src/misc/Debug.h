// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEDEBUG_H
#define ROSEDEBUG_H

#include <QDebug>

namespace Rosegarden {
    class Event;
    class Segment;
    class RealTime;
    class Colour;
    namespace Guitar {
	class Chord;
    }
}

#ifndef NDEBUG

QDebug &operator<<(QDebug &, const std::string &);
QDebug &operator<<(QDebug &, const Rosegarden::Event &);
QDebug &operator<<(QDebug &, const Rosegarden::Segment &);
QDebug &operator<<(QDebug &, const Rosegarden::RealTime &);
QDebug &operator<<(QDebug &, const Rosegarden::Colour &);
QDebug &operator<<(QDebug &, const Rosegarden::Guitar::Chord &);

#define RG_DEBUG        QDebug(QtDebugMsg) << "[generic] "
#define NOTATION_DEBUG  QDebug(QtDebugMsg) << "[notation] "
#define MATRIX_DEBUG    QDebug(QtDebugMsg) << "[matrix] "
#define SEQUENCER_DEBUG QDebug(QtDebugMsg) << "[sequencer] "
#define SEQMAN_DEBUG    QDebug(QtDebugMsg) << "[seqman] "

#else

class RGNoDebug
{
public:
    inline RGNoDebug() {}
    inline ~RGNoDebug(){}

    template <typename T>
    inline RGNoDebug &operator<<(const T &) { return *this; }
};

#define RG_DEBUG        RGNoDebug()
#define NOTATION_DEBUG  RGNoDebug()
#define MATRIX_DEBUG    RGNoDebug()
#define SEQUENCER_DEBUG RGNoDebug()
#define SEQMAN_DEBUG    RGNoDebug()

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

#endif
