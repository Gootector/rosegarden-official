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


#include "EditToolBox.h"

#include "BaseToolBox.h"
#include "EditTool.h"
#include "EditView.h"
#include <qobject.h>
#include <qstring.h>
#include <kmessagebox.h>

namespace Rosegarden
{

EditToolBox::EditToolBox(EditView *parent)
        : BaseToolBox(parent),
        m_parentView(parent)
{
}

EditTool* EditToolBox::getTool(const QString& toolName)
{
    return dynamic_cast<EditTool*>(BaseToolBox::getTool(toolName));
}

EditTool* EditToolBox::createTool(const QString&)
{
    KMessageBox::error(0, "EditToolBox::createTool called - this should never happen");
    return 0;
}

}
#include "EditToolBox.moc"
