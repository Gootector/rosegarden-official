/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "QuantizeDialog.h"

#include <klocale.h>
#include "base/Quantizer.h"
#include "gui/widgets/QuantizeParameters.h"
#include <kdialogbase.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

QuantizeDialog::QuantizeDialog(QWidget *parent, bool inNotation) :
        KDialogBase(parent, 0, true, i18n("Quantize"), Ok | Cancel | Details | Help)
{
    setHelp("quantization");

    QVBox *vbox = makeVBoxMainWidget();

    m_quantizeFrame =
        new QuantizeParameters
        (vbox, inNotation ? QuantizeParameters::Notation :
         QuantizeParameters::Grid,
         true, false, 0);

    setButtonText(Details, i18n("Advanced"));
    setDetailsWidget(m_quantizeFrame->getAdvancedWidget());
    m_quantizeFrame->getAdvancedWidget()->hide();

    m_quantizeFrame->adjustSize();
    vbox->adjustSize();
    adjustSize();
}

Quantizer *
QuantizeDialog::getQuantizer() const
{
    return m_quantizeFrame->getQuantizer();
}

}
#include "QuantizeDialog.moc"
