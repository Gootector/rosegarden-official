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


#include "ClefDialog.h"

#include "base/NotationTypes.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/BigArrowButton.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPixmap>
#include <QRadioButton>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

ClefDialog::ClefDialog(QWidget *parent,
                       NotePixmapFactory *npf,
                       Clef defaultClef,
                       bool showConversionOptions) :
        QDialog(parent),
        m_notePixmapFactory(npf),
        m_clef(defaultClef)
{
    //setHelp("nv-signatures-clef");

    setModal(true);
    setWindowTitle(tr("Clef"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *clefFrame = new QGroupBox( tr("Clef"), vbox );
    QVBoxLayout *clefFrameLayout = new QVBoxLayout;
    vboxLayout->addWidget(clefFrame);

    QGroupBox *conversionFrame = new QGroupBox( tr("Existing notes following clef change"), vbox );
    QVBoxLayout *conversionFrameLayout = new QVBoxLayout;
    vboxLayout->addWidget(conversionFrame);

    QWidget *clefBox = new QWidget(clefFrame);
    QHBoxLayout *clefBoxLayout = new QHBoxLayout;
    clefFrameLayout->addWidget(clefBox);

    BigArrowButton *clefDown = new BigArrowButton( clefBox , Qt::LeftArrow);
    clefBoxLayout->addWidget(clefDown);
    clefDown->setToolTip(tr("Lower clef"));

    QWidget *clefLabelBox = new QWidget(clefBox);
    QVBoxLayout *clefLabelBoxLayout = new QVBoxLayout;

    m_octaveUp = new BigArrowButton(clefLabelBox, Qt::UpArrow);
    clefLabelBoxLayout->addWidget(m_octaveUp);
    m_octaveUp->setToolTip(tr("Up an Octave"));

    m_clefLabel = new QLabel(tr("Clef"), clefLabelBox);
	clefLabelBoxLayout->addWidget(m_clefLabel, Qt::AlignHCenter | Qt::AlignVCenter );
//    m_clefLabel->setAlignment(AlignVCenter | AlignHCenter);

    m_octaveDown = new BigArrowButton(clefLabelBox, Qt::DownArrow);
    clefLabelBoxLayout->addWidget(m_octaveDown);
    m_octaveDown->setToolTip( tr("Down an Octave") );

    BigArrowButton *clefUp = new BigArrowButton( clefBox , Qt::RightArrow);
    clefBoxLayout->addWidget(clefUp);
    clefBox->setLayout(clefBoxLayout);
    clefUp->setToolTip( tr("Higher clef") );

    m_clefNameLabel = new QLabel(tr("Clef"), clefLabelBox);
	clefLabelBoxLayout->addWidget(m_clefNameLabel, Qt::AlignHCenter | Qt::AlignVCenter );
	clefLabelBox->setLayout(clefLabelBoxLayout);
//    m_clefNameLabel->setAlignment(AlignVCenter | AlignHCenter);

    if (showConversionOptions) {
        m_noConversionButton =
            new QRadioButton
            (tr("Maintain current pitches"), conversionFrame);
        conversionFrameLayout->addWidget(m_noConversionButton);
        m_changeOctaveButton =
            new QRadioButton
            (tr("Transpose into appropriate octave"), conversionFrame);
        conversionFrameLayout->addWidget(m_changeOctaveButton);
        m_transposeButton = 0;

        //!!! why aren't we offering this option? does it not work? too difficult to describe?
        //	m_transposeButton =
        //	    new QRadioButton
        //	    (tr("Maintain current positions on the staff"), conversionFrame);
        m_changeOctaveButton->setChecked(true);
    } else {
        m_noConversionButton = 0;
        m_changeOctaveButton = 0;
        m_transposeButton = 0;
        conversionFrame->hide();
    }

    clefFrame->setLayout(clefFrameLayout);
    conversionFrame->setLayout(conversionFrameLayout);
    vbox->setLayout(vboxLayout);

    QObject::connect(clefUp, SIGNAL(clicked()), this, SLOT(slotClefUp()));
    QObject::connect(clefDown, SIGNAL(clicked()), this, SLOT(slotClefDown()));
    QObject::connect(m_octaveUp, SIGNAL(clicked()), this, SLOT(slotOctaveUp()));
    QObject::connect(m_octaveDown, SIGNAL(clicked()), this, SLOT(slotOctaveDown()));

    redrawClefPixmap();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Clef
ClefDialog::getClef() const
{
    return m_clef;
}

ClefDialog::ConversionType

ClefDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
        return NoConversion;
    } else if (m_changeOctaveButton && m_changeOctaveButton->isChecked()) {
        return ChangeOctave;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
        return Transpose;
    }
    return NoConversion;
}

void
ClefDialog::slotClefUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Clef::ClefList clefs(Clef::getClefs());

    for (Clef::ClefList::iterator i = clefs.begin();
            i != clefs.end(); ++i) {

        if (m_clef.getClefType() == i->getClefType()) {
            if (++i == clefs.end())
                i = clefs.begin();
            m_clef = Clef(i->getClefType(), octaveOffset);
            break;
        }
    }

    redrawClefPixmap();
}

void
ClefDialog::slotClefDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Clef::ClefList clefs(Clef::getClefs());

    for (Clef::ClefList::iterator i = clefs.begin();
            i != clefs.end(); ++i) {

        if (m_clef.getClefType() == i->getClefType()) {
            if (i == clefs.begin())
                i = clefs.end();
            --i;
            m_clef = Clef(i->getClefType(), octaveOffset);
            break;
        }
    }

    redrawClefPixmap();
}

void
ClefDialog::slotOctaveUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == 2)
        return ;

    ++octaveOffset;

    m_octaveDown->setEnabled(true);
    if (octaveOffset == 2) {
        m_octaveUp->setEnabled(false);
    }

    m_clef = Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}

void
ClefDialog::slotOctaveDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == -2)
        return ;

    --octaveOffset;

    m_octaveUp->setEnabled(true);
    if (octaveOffset == 2) {
        m_octaveDown->setEnabled(false);
    }

    m_clef = Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}

void
ClefDialog::redrawClefPixmap()
{
    QPixmap pmap = NotePixmapFactory::toQPixmap
                   (m_notePixmapFactory->makeClefDisplayPixmap(m_clef));
    m_clefLabel->setPixmap(pmap);

    QString name;
    int octave = m_clef.getOctaveOffset();

    switch (octave) {
    case - 1:
        name = tr("%1 down an octave");
        break;
    case - 2:
        name = tr("%1 down two octaves");
        break;
    case 1:
        name = tr("%1 up an octave");
        break;
    case 2:
        name = tr("%1 up two octaves");
        break;
    default:
        name = "%1";
        break;
    }

    std::string type = m_clef.getClefType();
    if (type == Clef::Treble)
        name = name.arg(tr("Treble"));
    else if (type == Clef::French)
        name = name.arg(tr("French violin"));
    else if (type == Clef::Soprano)
        name = name.arg(tr("Soprano"));
    else if (type == Clef::Mezzosoprano)
        name = name.arg(tr("Mezzo-soprano"));
    else if (type == Clef::Alto)
        name = name.arg(tr("Alto"));
    else if (type == Clef::Tenor)
        name = name.arg(tr("Tenor"));
    else if (type == Clef::Baritone)
        name = name.arg(tr("C-baritone"));
    else if (type == Clef::Varbaritone)
        name = name.arg(tr("F-baritone"));
    else if (type == Clef::Bass)
        name = name.arg(tr("Bass"));
    else if (type == Clef::Subbass)
        name = name.arg(tr("Sub-bass"));

    m_clefNameLabel->setText(name);
}

}
#include "ClefDialog.moc"
