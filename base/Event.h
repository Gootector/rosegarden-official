// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _EVENT_H_
#define _EVENT_H_

#include "Property.h"
#include "PropertyName.h"

#include <string>
#ifndef NDEBUG
#include <iostream>
#include <ctime>
#endif

#if (__GNUC__ < 3)
#include <hash_map>
#else
#include <ext/hash_map>
#endif

namespace Rosegarden 
{

typedef int timeT;

/**
 * The Event class represents an event with some basic attributes and
 * an arbitrary number of properties of dynamically-determined name
 * and type.
 *
 * An Event has a type; a duration, often zero for events other than
 * notes; an absolute time, the time at which the event begins, which
 * is used to order events within a Segment; and a "sub-ordering", used
 * to determine an order for events that have the same absolute time
 * (for example to ensure that the clef always appears before the key
 * signature at the start of a piece).  Besides these, an event can
 * have any number of properties, which are typed values stored and
 * retrieved by name.  Properties may be persistent or non-persistent,
 * depending on whether they are saved to file with the rest of the
 * event data or are considered to be only cached values that can be
 * recomputed at will if necessary.
 */

class Event
{
public:
    friend class ViewElement;

    struct NoData { };
    struct BadType { };

    Event() :
	m_data(new EventData()), m_viewElementRefCount(0) { }
    Event(const std::string &type) :
	m_data(new EventData(type)), m_viewElementRefCount(0) { }
    Event(const Event &e) :
	m_viewElementRefCount(0) { share(e); }
    ~Event() { lose(); }

    Event &operator=(const Event &e) {
	if (&e != this) { lose(); share(e); }
	return *this;
    }

    friend bool operator<(const Event&, const Event&);

    // Accessors
    const std::string &getType() const    { return m_data->m_type; }
    void setType(const std::string &t)    { unshare(); m_data->m_type = t; }

    bool isa(const std::string &t) const  { return (m_data->m_type == t); }

    timeT getAbsoluteTime() const    { return m_data->m_absoluteTime; }
    void  setAbsoluteTime(timeT d)   { unshare(); m_data->m_absoluteTime = d; }
    void  addAbsoluteTime(timeT d)   { unshare(); m_data->m_absoluteTime += d; }

    timeT getDuration()     const    { return m_data->m_duration; }
    void  setDuration(timeT d)       { unshare(); m_data->m_duration = d; }

    int   getSubOrdering()  const    { return m_data->m_subOrdering; }
    void  setSubOrdering(int o)      { unshare(); m_data->m_subOrdering = o; }

    bool  has(const PropertyName &name) const;

    template <PropertyType P>
    PropertyDefn<P>::basic_type get(const PropertyName &name) const;
    // throw (NoData, BadType);

    // no throw, returns bool
    template <PropertyType P>
    bool get(const PropertyName &name, PropertyDefn<P>::basic_type &val) const;

    template <PropertyType P>
    bool isPersistent(const PropertyName &name) const;
    // throw (NoData);

    template <PropertyType P>
    void setPersistence(const PropertyName &name, bool persistent);
    // throw (NoData);

    std::string getPropertyType(const PropertyName &name) const;
    // throw (NoData);

    std::string getAsString(const PropertyName &name) const;
    // throw (NoData);

    template <PropertyType P>
    void set(const PropertyName &name, PropertyDefn<P>::basic_type value,
             bool persistent = true);
    // throw (BadType);

    // set non-persistent, but only if there's no persistent value already
    template <PropertyType P>
    void setMaybe(const PropertyName &name, PropertyDefn<P>::basic_type value);
    // throw (BadType);

    template <PropertyType P>
    void setFromString(const PropertyName &name, std::string value,
                       bool persistent = true);
    // throw (BadType);

    void unset(const PropertyName &name);
    
    typedef std::vector<PropertyName> PropertyNames;
    PropertyNames getPropertyNames() const;
    PropertyNames getPersistentPropertyNames() const;
    PropertyNames getNonPersistentPropertyNames() const;

    struct EventCmp
    {
        bool operator()(const Event &e1, const Event &e2) const {
            return e1 < e2;
        }
        bool operator()(const Event *e1, const Event *e2) const {
            return *e1 < *e2;
        }
    };

    static bool compareEvent2Time(const Event *e, timeT t) {
        return e->getAbsoluteTime() < t;
    }

    static bool compareTime2Event(timeT t, const Event *e) {
        return t <  e->getAbsoluteTime();
    }

    // approximate, for debugging and inspection purposes
    size_t getStorageSize() const;

#ifndef NDEBUG
    void dump(std::ostream&) const;
#else
    void dump(std::ostream&) const {}
#endif
    static void dumpStats(std::ostream&);

    bool hasViewElement() const { return m_viewElementRefCount != 0; }

protected:
    // these are for ViewElement only
    void viewElementRef()   { ++m_viewElementRefCount; }
    void viewElementUnRef() { --m_viewElementRefCount; }

private:
    struct EventData
    {
	EventData();
	EventData(const std::string &type);
	EventData *unshare();
	~EventData();
	unsigned int m_refCount;

	std::string m_type;
	timeT m_duration;
	timeT m_absoluteTime;
	int m_subOrdering;

	typedef std::hash_map<PropertyName, PropertyStoreBase*,
			      PropertyNameHash, PropertyNamesEqual> PropertyMap;
	typedef PropertyMap::value_type PropertyPair;
	PropertyMap m_properties;

    private:
	EventData(const EventData &);
	EventData &operator=(const EventData &);
    };	

    EventData *m_data;
    unsigned int m_viewElementRefCount;

    void share(const Event &e) {
	m_data = e.m_data;
	m_data->m_refCount++;
    }

    void unshare() {
	if (m_data->m_refCount > 1) m_data = m_data->unshare();
    }

    void lose() {
	m_viewElementRefCount = 0;
	if (--m_data->m_refCount == 0) delete m_data;
    }

#ifndef NDEBUG
    static int m_getCount;
    static int m_setCount;
    static int m_setMaybeCount;
    static int m_hasCount;
    static int m_unsetCount;
    static clock_t m_lastStats;
#endif
};


template <PropertyType P>
bool
Event::get(const PropertyName &name, PropertyDefn<P>::basic_type &val) const
{
#ifndef NDEBUG
    ++m_getCount;
#endif

    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            val = (static_cast<PropertyStore<P> *>(sb))->getData();
            return true;
        }
        else {
#ifndef NDEBUG
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            return false;
        }
	    
    } else {
        return false;
    }
}


template <PropertyType P>
PropertyDefn<P>::basic_type
Event::get(const PropertyName &name) const
    // throw (NoData, BadType)
{
#ifdef NDEBUG

    // If debug is off, we just do the quickest possible thing and let
    // the property map implementation worry about absent entries
    //!!! Nah, bad idea -- we really do want to catch the exception in
    // some cases.  Could possible get away with returning a default
    // value though...

    return (static_cast<PropertyStore<P> *>
	    (m_data->m_properties[name]))->getData();
    
#else

    ++m_getCount;

    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) { 

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P)
            return (static_cast<PropertyStore<P> *>(sb))->getData();
        else {
            std::cerr << "Event::get() Error: Attempt to get property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
            throw BadType();
        }
	    
    } else {

        std::cerr << "Event::get(): Error: Attempt to get property \"" << name
             << "\" which doesn't exist for this event\nEvent::get(): Dump follows:" << std::endl;
	dump(std::cerr);
        throw NoData();
    }

#endif
}


template <PropertyType P>
bool
Event::isPersistent(const PropertyName &name) const
    // throw (NoData)
{
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) return i->second->isPersistent();
    else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to get persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::setPersistence(const PropertyName &name, bool persistent)
    // throw (NoData)
{
    unshare();
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) i->second->setPersistence(persistent);
    else {
#ifndef NDEBUG
        std::cerr << "Event::get() Error: Attempt to set persistence of property \""
             << name << "\" which doesn't exist for this element" << std::endl;
#endif
        throw NoData();
    }
}


template <PropertyType P>
void
Event::set(const PropertyName &name, PropertyDefn<P>::basic_type value,
           bool persistent)
    // throw (BadType)
{
#ifndef NDEBUG
    ++m_setCount;
#endif

    unshare();
    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) {

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
            (static_cast<PropertyStore<P> *>(sb))->setData(value);
            sb->setPersistence(persistent);
        } else {
#ifndef NDEBUG
            std::cerr << "Error: Element: Attempt to set property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
	    
    } else {
        PropertyStoreBase *p = new PropertyStore<P>(value, persistent);
        m_data->m_properties.insert(EventData::PropertyPair(name, p));
    }
}



// setMaybe<> is actually called rather more frequently than set<>, so
// it makes sense for best performance to implement it separately
// rather than through calls to has, isPersistent and set<>

template <PropertyType P>
void
Event::setMaybe(const PropertyName &name, PropertyDefn<P>::basic_type value)
    // throw (BadType)
{
#ifndef NDEBUG
    ++m_setMaybeCount;
#endif

    EventData::PropertyMap::const_iterator i = m_data->m_properties.find(name);
    if (i != m_data->m_properties.end()) {

        PropertyStoreBase *sb = i->second;
        if (sb->getType() == P) {
	    if (!sb->isPersistent()) {
		unshare();
		(static_cast<PropertyStore<P> *>(sb))->setData(value);
	    }
        } else {
#ifndef NDEBUG
            std::cerr << "Error: Element: Attempt to setMaybe property \"" << name
                 << "\" as " << PropertyDefn<P>::typeName() <<", actual type is "
                 << sb->getTypeName() << std::endl;
#endif
            throw BadType();
        }
	    
    } else {
	unshare();
        PropertyStoreBase *p = new PropertyStore<P>(value, false);
        m_data->m_properties.insert(EventData::PropertyPair(name, p));
    }
}


template <PropertyType P>
void
Event::setFromString(const PropertyName &name, std::string value, bool persistent)
    // throw (BadType)
{
    set<P>(name, PropertyDefn<P>::parse(value), persistent);
}


//////////////////////////////////////////////////////////////////////

}

#endif
