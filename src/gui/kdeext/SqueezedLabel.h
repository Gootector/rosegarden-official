/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDEN_SQUEEZED_LABEL_H_
#define _ROSEGARDEN_SQUEEZED_LABEL_H_

#include <QLabel>

namespace Rosegarden {

//### This is not in any way a replacement for KSqueezedTextLabel, but
// that's what it must become

class SqueezedLabel : public QLabel
{
    Q_OBJECT

public:
    SqueezedLabel( QWidget* parent=0 );
    SqueezedLabel( QString label="", QWidget* parent=0 );
};

}

#endif
