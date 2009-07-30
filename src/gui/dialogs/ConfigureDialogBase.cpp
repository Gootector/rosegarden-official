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


#include "ConfigureDialogBase.h"

#include "gui/configuration/ConfigurationPage.h"
#include "gui/widgets/IconStackedWidget.h"
#include "misc/Debug.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace Rosegarden
{


ConfigureDialogBase::ConfigureDialogBase(QWidget *parent, QString label, const char *name  )
: QDialog(parent)
{
    this->setAttribute( Qt::WA_DeleteOnClose );
    
    this->setWindowTitle( tr("Configure Rosegarden") );
    this->setObjectName( (name) );

    QVBoxLayout *dlgLayout = new QVBoxLayout(this);

    m_iconWidget = new IconStackedWidget(this);
    dlgLayout->addWidget(m_iconWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply  |
                                                       QDialogButtonBox::Ok     |
                                                       QDialogButtonBox::Cancel |
                                                       QDialogButtonBox::Help);
    dlgLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(slotCancelOrClose()));

    QPushButton *m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
    m_applyButton->setEnabled(false);
    connect(m_applyButton, SIGNAL(clicked()), this, SLOT(slotApply()));

}

// ConfigureDialogBase::addPage(const QString& name, const QString& title, const QPixmap& icon, QWidget *page)
// where:
// name - The name written in the page select icon button
// title - The title placed at the top of the configuration page
// icon - The icon shown on the page select icon button
// page - pointer to a configuration page widget
void ConfigureDialogBase::addPage(const QString& name, const QString& title, const QPixmap& icon, QWidget *page)
{
    // New widget to hold the title, dividing line and configuration page
    QWidget * titledPage = new QWidget(this);
    QLayout * pageLayout = new QVBoxLayout(titledPage);

    // Create the title label widget for the configration page
    QLabel * titleLabel = new QLabel(titledPage,title);
    QFont font;
    font.setBold(true);
    font.setPixelSize(12);
    titleLabel->setFont(font);
    
    // Create the dividing line
    QFrame * divideLine = new QFrame(titledPage);
    divideLine->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    divideLine->setLineWidth(2);

    // Add these widgets to the layout
    pageLayout->addWidget(titleLabel);
    pageLayout->addWidget(divideLine);
    pageLayout->addWidget(page);
    
    // Add the page to the IconStackedWidgget (creating the select button)
    m_iconWidget->addPage(name, titledPage, icon);
}

//QWidget* ConfigureDialogBase::addPage( const QString& iconLabel, const QString& label, const QIcon& icon ){
    ///**     add a configuration options tab to the tabWidget ; return the tab-page <QWidget*>  */
    //QWidget *page = new QWidget();
    //if( ! m_tabWidget ){
        //std::cerr << "ERROR: m_tabWidget is NULL in ConfigureDialogBase::addPage " << std::endl;
        //return 0;
    //}
    //m_tabWidget->addTab( page, icon, label );
    //return page;
//}

ConfigureDialogBase::~ConfigureDialogBase()
{}


//void ConfigureDialogBase::slotButtonBoxButtonClicked(QAbstractButton * button){
    
    //QDialogButtonBox::ButtonRole bRole = m_dialogButtonBox->buttonRole( button );
    
    //if( bRole == QDialogButtonBox::ApplyRole ){
        //slotApply();
////        close();
    //}
    //else if( bRole == QDialogButtonBox::AcceptRole ){
        //accept();
    //}
    //else if( bRole == QDialogButtonBox::HelpRole ){
////         slotHelp();
    //}
    //else if( bRole == QDialogButtonBox::ResetRole ){
////         slotRestoreDefaults();
    //}else{
        //// cancel
        ////### do we need to reset/restore anything here, before closing the conf dialog ?
        //slotCancelOrClose();
        ////close();
    //}
//}


void
ConfigureDialogBase::slotApply()
{
    RG_DEBUG << "CONFIGUREDIALOGBASE SLOTAPPLY()" << endl;
    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i)
        (*i)->apply();

    m_applyButton->setEnabled(false);
}

void
ConfigureDialogBase::slotActivateApply()
{
    m_applyButton->setEnabled(true);
}

void
ConfigureDialogBase::accept()
{
    slotApply();
    QDialog::accept();
    close();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
    close();
}


}
#include "ConfigureDialogBase.moc"
