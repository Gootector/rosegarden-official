/***************************************************************************
                          rosedebug.cpp  -  description
                             -------------------
    begin                : Fri Aug 18 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "rosedebug.h"

#include "Event.h"

kdbgstream&
operator<<(kdbgstream &dbg, const string &s)
{
    dbg << s.c_str();
    return dbg;
}

kdbgstream&
operator<<(kdbgstream &dbg, const Event &e)
{
    if (e.package().length()) {
        dbg << "Event type : " << e.type() << " - package : "
            << e.package() << endl;
    } else {    
        dbg << "Event type : " << e.type() << endl;
    }

    dbg << "\tDuration : " << e.duration()
        << "\n\tAbsolute Time : " << e.absoluteTime()
        << endl;

//     for (Event::PropertyMap::const_iterator i = e.properties().begin();
//          i != e.properties().end(); ++i) {
//         dbg << "\t\t" << (*i).first << " : "
//             << ((*i).second)->unparse() << '\n';
//     }

    e.dump(cerr);
    
    return dbg;
}


ostream&
kdbgostreamAdapter::operator<<(bool i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(short i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned short i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(char i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned char i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(int i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned int i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(long i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(unsigned long i)
{
    m_kdbgStream << i;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const QString& string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const char *string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(const QCString& string)
{
    m_kdbgStream << string;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(void * p)
{
    m_kdbgStream << p;
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(KDBGFUNC f)
{
    (*f)(m_kdbgStream);
    return *this;
}

ostream&
kdbgostreamAdapter::operator<<(double d)
{
    m_kdbgStream << d;
    return *this;
}


// ostream& endl( ostream &s)
// {
//     s << "\n"; return s;
// }

void DBCheckThrow()
{
    Event ev;
    
    try {
        int pitch = ev.get<Int>("BLAH");

    } catch (Event::NoData) {
        kdDebug(KDEBUG_AREA) << "DBCheckThrow()" << endl;
    }
}

