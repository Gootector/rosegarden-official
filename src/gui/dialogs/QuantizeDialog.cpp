/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>

namespace Rosegarden
{

QuantizeDialog::QuantizeDialog(QWidget *parent, bool inNotation) :
        QDialog(parent)
{
    //setHelp("quantization");

    setModal(true);
    setWindowTitle(QObject::tr("Quantize"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    m_quantizeFrame = new QuantizeParameters( vbox , inNotation ?
            QuantizeParameters::Notation : QuantizeParameters::Grid,
            true, false, 0);
    vboxLayout->addWidget(m_quantizeFrame);
    vbox->setLayout(vboxLayout);

    setDetailsWidget(m_quantizeFrame->getAdvancedWidget());  //
	
	m_quantizeFrame->getAdvancedWidget()->hide();

    m_quantizeFrame->adjustSize();
    vbox->adjustSize();
    adjustSize();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QPushButton *details = buttonBox->addButton(QObject::tr("Advanced"), QDialogButtonBox::ActionRole);
	details->setObjectName( "detailsButton" );
	connect(details, SIGNAL(clicked()), this, SLOT(slotShowDetails(bool)));
	buttonBox->addButton(QDialogButtonBox::Help);
	
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void QuantizeDialog::setDetailsWidget( QWidget* wid ){
	//QPushButton* butt = this->findChild<QPushButton*>( "detailsButton" );
	m_detailsWidget = wid;
//	wid->setObjectName( "detailsWidget" );
}

void QuantizeDialog::slotShowDetails( bool checked ){
	bool vis = m_detailsWidget->isVisible();
	m_detailsWidget->setVisible( ! vis );
}

Quantizer *
QuantizeDialog::getQuantizer() const
{
    return m_quantizeFrame->getQuantizer();
}

}
#include "QuantizeDialog.moc"
