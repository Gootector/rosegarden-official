/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TEMPOLISTITEM_H_
#define _RG_TEMPOLISTITEM_H_

#include <klistview.h>

#include "base/Event.h"

namespace Rosegarden {

class Composition;

class TempoListItem : public KListViewItem
{
public:
    enum Type { TimeSignature, Tempo };

    TempoListItem(Composition *composition,
		  Type type,
		  timeT time,
		  int index,
		  KListView *parent,
		  QString label1,
		  QString label2,
		  QString label3,
		  QString label4 = QString::null) :
	KListViewItem(parent, label1, label2, label3, label4),
	m_composition(composition),
	m_type(type),
	m_time(time),
	m_index(index) { }

    Rosegarden::Composition *getComposition() { return m_composition; }
    Type getType() const { return m_type; }
    Rosegarden::timeT getTime() const { return m_time; }
    int getIndex() const { return m_index; }

    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:
    Rosegarden::Composition *m_composition;
    Type m_type;
    Rosegarden::timeT m_time;
    int m_index;
};

}

#endif
