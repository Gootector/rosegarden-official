// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2008 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef _MAPPEDCOMMON_H_
#define _MAPPEDCOMMON_H_

// Some Mapped types that gui and sound libraries use to communicate
// plugin and Studio information.  Putting them here so we can change
// MappedStudio regularly without having to rebuild the gui.
//
#include <vector>

#include <qstring.h>
#include <qdatastream.h>

namespace Rosegarden
{

typedef int          MappedObjectId;
typedef QString      MappedObjectProperty;
typedef float        MappedObjectValue;

// typedef QValueVector<MappedObjectProperty> MappedObjectPropertyList;
// replaced with a std::vector<> for Qt2 compatibility

typedef std::vector<MappedObjectId> MappedObjectIdList;
typedef std::vector<MappedObjectProperty> MappedObjectPropertyList;
typedef std::vector<MappedObjectValue> MappedObjectValueList;

// The direction in which a port operates.
//
typedef enum
{
    ReadOnly,  // input port
    WriteOnly, // output port
    Duplex
} PortDirection;

QDataStream& operator>>(QDataStream& s, MappedObjectIdList&);
QDataStream& operator<<(QDataStream&, const MappedObjectIdList&);

QDataStream& operator>>(QDataStream& s, MappedObjectPropertyList&);
QDataStream& operator<<(QDataStream&, const MappedObjectPropertyList&);

QDataStream& operator>>(QDataStream& s, MappedObjectValueList&);
QDataStream& operator<<(QDataStream&, const MappedObjectValueList&);

}

#endif // _MAPPEDCOMMON_H_
