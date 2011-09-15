/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ShowSequencerStatusDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QByteArray>
#include <QDataStream>
#include <QLabel>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QVBoxLayout>
#include "sequencer/RosegardenSequencer.h"


namespace Rosegarden
{

ShowSequencerStatusDialog::ShowSequencerStatusDialog(QWidget *parent) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Sequencer status"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    new QLabel(tr("Sequencer status:"), vbox);

    QString status = RosegardenSequencer::getInstance()->getStatusLog();

    QTextEdit *text = new QTextEdit( vbox );
    vboxLayout->addWidget(text);
    vbox->setLayout(vboxLayout);
    text->setReadOnly(true);
    text->setMinimumWidth(500);
    text->setMinimumHeight(200);

    text->setPlainText(status);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

}
#include "ShowSequencerStatusDialog.moc"
