
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

#include "HeadersConfigurationPage.h"

#include "document/ConfigGroups.h"
#include "document/RosegardenGUIDoc.h"
#include "document/io/LilyPondExporter.h"
#include "gui/widgets/CollapsingFrame.h"
#include "misc/Strings.h"

#include <QApplication>
#include <QSettings>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QFont>

#include <klocale.h>

namespace Rosegarden
{

HeadersConfigurationPage::HeadersConfigurationPage(QWidget *parent,
	RosegardenGUIDoc *doc) :
	QWidget(parent),
	m_doc(doc)
{
	QVBoxLayout *layout = new QVBoxLayout;
	
    //
    // LilyPond export: Printable headers
    //

//old:	QGroupBox *headersBox = new QGroupBox(1, Qt::Horizontal,
//										  i18n("Printable headers"), this);
	QGroupBox *headersBox = new QGroupBox( i18n("Printable headers"), this );
    
    layout->addWidget(headersBox);
    QFrame *frameHeaders = new QFrame(headersBox);
    QGridLayout *layoutHeaders = new QGridLayout(frameHeaders, 10, 6, 10, 5);

    // grab user headers from metadata
    Configuration metadata = (&m_doc->getComposition())->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();
    std::vector<PropertyName> fixedKeys =
	CompositionMetadataKeys::getFixedKeys();

    std::set<std::string> shown;

    for (unsigned int index = 0; index < fixedKeys.size(); index++) {
	std::string key = fixedKeys[index].getName();
	std::string header = "";
	for (unsigned int i = 0; i < propertyNames.size(); ++i) {
	    std::string property = propertyNames [i];
	    if (property == key) {
		header = metadata.get<String>(property);
	    }
	}

	unsigned int row = 0, col = 0, width = 1;
	QLineEdit *editHeader = new QLineEdit(strtoqstr( header ), frameHeaders);
	QString trName;
	if (key == headerDedication) {  
	    m_editDedication = editHeader;
	    row = 0; col = 2; width = 2;
	    trName = i18n("Dedication");
	} else if (key == headerTitle) {       
	    m_editTitle = editHeader;	
	    row = 1; col = 1; width = 4;
	    trName = i18n("Title");
	} else if (key == headerSubtitle) {
	    m_editSubtitle = editHeader;
	    row = 2; col = 1; width = 4;
	    trName = i18n("Subtitle");
	} else if (key == headerSubsubtitle) { 
	    m_editSubsubtitle = editHeader;
	    row = 3; col = 2; width = 2;
	    trName = i18n("Subsubtitle");
	} else if (key == headerPoet) {        
	    m_editPoet = editHeader;
	    row = 4; col = 0; width = 2;
	    trName = i18n("Poet");
	} else if (key == headerInstrument) {  
	    m_editInstrument = editHeader;
	    row = 4; col = 2; width = 2;
	    trName = i18n("Instrument");
	} else if (key == headerComposer) {    
	    m_editComposer = editHeader;
	    row = 4; col = 4; width = 2; 
	    trName = i18n("Composer");
	} else if (key == headerMeter) {       
	    m_editMeter = editHeader;
	    row = 5; col = 0; width = 3; 
	    trName = i18n("Meter");
	} else if (key == headerArranger) {    
	    m_editArranger = editHeader;
	    row = 5; col = 3; width = 3; 
	    trName = i18n("Arranger");
	} else if (key == headerPiece) {       
	    m_editPiece = editHeader;
	    row = 6; col = 0; width = 3; 
	    trName = i18n("Piece");
	} else if (key == headerOpus) {        
	    m_editOpus = editHeader;
	    row = 6; col = 3; width = 3; 
	    trName = i18n("Opus");
	} else if (key == headerCopyright) {   
	    m_editCopyright = editHeader;
	    row = 8; col = 1; width = 4; 
	    trName = i18n("Copyright");
	} else if (key == headerTagline) {     
	    m_editTagline = editHeader;
	    row = 9; col = 1; width = 4; 
	    trName = i18n("Tagline");
	}

	// editHeader->setReadOnly( true );
	editHeader->setAlignment( (col == 0 ? Qt::AlignLeft : (col >= 3 ? Qt::AlignRight : Qt::AlignCenter) ));

	layoutHeaders->addMultiCellWidget(editHeader, row, row, col, col+(width-1) );

	//
	// ToolTips
	//
	QToolTip::add( editHeader, trName );

	shown.insert(key);
    }
    QLabel *separator = new QLabel(i18n("The composition comes here."), frameHeaders);
    separator->setAlignment( Qt::AlignCenter );
    layoutHeaders->addWidget(separator, 7, 1, 1, 4 - 2);

    //
    // LilyPond export: Non-printable headers
    //

    // set default expansion to false for this group -- what a faff

    //@@@ JAS next 2 lines not needed. Commented out
    //@@@ QSettings settings ; // was: confq4
    //@@@ QString groupTemp = settings.group();
    QSettings settings;
    settings.beginGroup( "CollapsingFrame" );

    bool expanded = qStrToBool( settings.value("nonprintableheaders", "false" ) ) ;
    settings.setValue("nonprintableheaders", expanded);
    settings.endGroup();

    CollapsingFrame *otherHeadersBox = new CollapsingFrame
        (i18n("Non-printable headers"), this, "nonprintableheaders");
        
    layout->addWidget(otherHeadersBox);
    setLayout(layout);        
    
    QFrame *frameOtherHeaders = new QFrame(otherHeadersBox);
    otherHeadersBox->setWidgetFill(true);
    QFont font(otherHeadersBox->font());
    font.setBold(false);
    otherHeadersBox->setFont(font);
    otherHeadersBox->setWidget(frameOtherHeaders);

    QGridLayout *layoutOtherHeaders = new QGridLayout(frameOtherHeaders, 2, 2, 10, 5);

	m_metadata = new QTableWidget( 2, 2, frameOtherHeaders ); // rows, columns
	
	m_metadata->setItem( 0, 0, new QTableWidgetItem(i18n("Name"))  ); // row, column, item
	m_metadata->setItem( 0, 2, new QTableWidgetItem(i18n("Value"))  ); // row, column, item
//    m_metadata->setFullWidth(true);
	m_metadata->setMinimumSize( 40, 40 ); // width, height
//    m_metadata->setItemsRenameable(true);  //&&& disabled item renaming  ||use: openPersistentEditor ( QTableWidgetItem * item )
//    m_metadata->setRenameable(0);
//    m_metadata->setRenameable(1);
//    m_metadata->setItemMargin(5);
//    m_metadata->setDefaultRenameAction(QListWidget::Accept);
//    m_metadata->setShowSortIndicator(true);

    std::vector<std::string> names(metadata.getPropertyNames());
	
	QTableWidgetItem* tabItem;
	int row = m_metadata->rowCount();
	int col = 0;
	m_metadata->setRowCount( row + 1 );
    
	for (unsigned int i = 0; i < names.size(); ++i) {

        if (shown.find(names[i]) != shown.end())
            continue;

        QString name(strtoqstr(names[i]));

        // property names stored in lower case
        name = name.left(1).toUpper() + name.right(name.length() - 1);

//		new QListWidgetItem( m_metadata, name, strtoqstr(metadata.get<String>(names[i])) );
		// qt4: icon, text, type
		tabItem = new QTableWidgetItem( strtoqstr(metadata.get<String>(names[i])) );
		m_metadata->setItem( row, col, tabItem );

        shown.insert(names[i]);
    }

    layoutOtherHeaders->addWidget(m_metadata, 0, 0, 0- 0+1, 1- 1);

    QPushButton* addPropButton = new QPushButton(i18n("Add New Property"),
                                 frameOtherHeaders);
    layoutOtherHeaders->addWidget(addPropButton, 1, 0, Qt::AlignHCenter);

    QPushButton* deletePropButton = new QPushButton(i18n("Delete Property"),
                                    frameOtherHeaders);
    layoutOtherHeaders->addWidget(deletePropButton, 1, 1, Qt::AlignHCenter);

    connect(addPropButton, SIGNAL(clicked()),
            this, SLOT(slotAddNewProperty()));

    connect(deletePropButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteProperty()));
}

void
HeadersConfigurationPage::slotAddNewProperty()
{
    QString propertyName;
    int i = 0;
	
	QList<QTableWidgetItem*> foundItems;
    while (1) {
        propertyName =
            (i > 0 ? i18n("{new property %1}", i) : i18n("{new property}"));
		//<QTableWidgetItem*>
		foundItems = m_metadata->findItems( propertyName, Qt::MatchContains | Qt::MatchCaseSensitive );
	
		if ( !m_doc->getComposition().getMetadata().has(qstrtostr(propertyName)) &&
					 foundItems.isEmpty() ){
            break;
		}
        ++i;
    }

	//new QTableWidgetItem(m_metadata, propertyName, i18n("{undefined}"));
	int rc = m_metadata->rowCount();
	int col = 0;
	m_metadata->setRowCount( rc + 1 );
	m_metadata->setItem( rc, col, new QTableWidgetItem(propertyName) );
}

void
HeadersConfigurationPage::slotDeleteProperty()
{
	//delete m_metadata->currentIndex();
	m_metadata->removeRow( m_metadata->currentRow() );
}

void HeadersConfigurationPage::apply()
{
    //@@@ Next two lines not needed in this function.  Commented out
    //@@@ QSettings settings;
    //@@@ settings.beginGroup( NotationViewConfigGroup );

    // If one of the items still has focus, it won't remember edits.
    // Switch between two fields in order to lose the current focus.
    m_editTitle->setFocus();
    m_metadata->setFocus();

    //
    // Update header fields
    //

    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    metadata.clear();

    metadata.set<String>(CompositionMetadataKeys::Dedication, qstrtostr(m_editDedication->text()));
    metadata.set<String>(CompositionMetadataKeys::Title, qstrtostr(m_editTitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subtitle, qstrtostr(m_editSubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Subsubtitle, qstrtostr(m_editSubsubtitle->text()));
    metadata.set<String>(CompositionMetadataKeys::Poet, qstrtostr(m_editPoet->text()));
    metadata.set<String>(CompositionMetadataKeys::Composer, qstrtostr(m_editComposer->text()));
    metadata.set<String>(CompositionMetadataKeys::Meter, qstrtostr(m_editMeter->text()));
    metadata.set<String>(CompositionMetadataKeys::Opus, qstrtostr(m_editOpus->text()));
    metadata.set<String>(CompositionMetadataKeys::Arranger, qstrtostr(m_editArranger->text()));
    metadata.set<String>(CompositionMetadataKeys::Instrument, qstrtostr(m_editInstrument->text()));
    metadata.set<String>(CompositionMetadataKeys::Piece, qstrtostr(m_editPiece->text()));
    metadata.set<String>(CompositionMetadataKeys::Copyright, qstrtostr(m_editCopyright->text()));
    metadata.set<String>(CompositionMetadataKeys::Tagline, qstrtostr(m_editTagline->text()));

//	for (QTableWidgetItem *item = m_metadata->firstChild();
//			item != 0; item = item->nextSibling()) {
	
	QTableWidgetItem* tabItem;
	QTableWidgetItem* tabItem2;
	int c = 0;
	for( int r=0; r < m_metadata->rowCount(); r++ ){
		//for( int c=0; c < m_metadata->columnCount(); c++ ){
			
		tabItem = m_metadata->item( r, c );
		tabItem2 = m_metadata->item( r, c+1 );
			
		metadata.set<String>( qstrtostr(tabItem->text().toLower()),
							  qstrtostr(tabItem2->text()) );
//		metadata.set<String>(qstrtostr(item->text(0).toLower()),
//							 qstrtostr(item->text(1)));
		//}//end for(c)
	}//end for(r)

    m_doc->slotDocumentModified();
}

}
#include "HeadersConfigurationPage.moc"
