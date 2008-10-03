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


#include "ControlEditorDialog.h"

#include <klocale.h>
// #include <kmainwindow.h>
// #include <kstandardshortcut.h>
// #include <kstandardaction.h>

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/Studio.h"
#include "commands/studio/AddControlParameterCommand.h"
#include "commands/studio/ModifyControlParameterCommand.h"
#include "commands/studio/RemoveControlParameterCommand.h"
#include "ControlParameterEditDialog.h"
#include "ControlParameterItem.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "document/Command.h"
#include "gui/kdeext/KTmpStatusMsg.h"

#include <QShortcut>
#include <QMainWindow>
#include <QLayout>
#include <QApplication>
#include <QAction>
#include <QTreeWidget>
#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>

#include <QList>
// #include <qptrlist.h>

namespace Rosegarden
{

const QString notShowing(i18n("<not showing>"));

ControlEditorDialog::ControlEditorDialog
		(
			QWidget *parent,
			RosegardenGUIDoc *doc,
			DeviceId device
		):
        QMainWindow(parent, "controleditordialog"),
        m_studio(&doc->getStudio()),
        m_doc(doc),
        m_device(device),
        m_modified(false)
{
    RG_DEBUG << "ControlEditorDialog::ControlEditorDialog: device is " << m_device << endl;

    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Control Events"));

    QString deviceName(i18n("<no device>"));
    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (md)
        deviceName = strtoqstr(md->getName());

    // spacing hack!
    new QLabel("", mainFrame);
    new QLabel(i18n("  Control Events for %1 (device %2)", deviceName, 
               device), mainFrame);
    new QLabel("", mainFrame);

	
	/*
    m_listView->addColumn(i18n("Control Event name  "));
    m_listView->addColumn(i18n("Control Event type  "));
    m_listView->addColumn(i18n("Control Event value  "));
    m_listView->addColumn(i18n("Description  "));
    m_listView->addColumn(i18n("Min  "));
    m_listView->addColumn(i18n("Max  "));
    m_listView->addColumn(i18n("Default  "));
    m_listView->addColumn(i18n("Color  "));
    m_listView->addColumn(i18n("Position on instrument panel"));
	*/
	
	QStringList sl;
	sl	<< i18n("Control Event name  ")
		<< i18n("Control Event type  ")
		<< i18n("Control Event value  ")
		<< i18n("Description  ")
		<< i18n("Min  ")
		<< i18n("Max  ")
		<< i18n("Default  ")
		<< i18n("Color  ")
		<< i18n("Position on instrument panel");
	
	m_listView = new QTreeWidget( mainFrame );
	m_listView->setHeaderLabels( sl );
	
// 	m_listView->setColumnAlignment(0, Qt::AlignLeft);	//&&& align per item now:
// 	m_listViewItem->setTextAlignment(0, Qt::AlignLeft);	
	
		
	mainFrameLayout->addWidget(m_listView);
	
    // Align remaining columns centrally
//     for (int i = 1; i < 9; ++i)
//         m_listView->setColumnAlignment(i, Qt::AlignHCenter);	//&&& align per item now
	
	
//     m_listView->restoreLayout(ControlEditorConfigGroup);	//&&&
	
    QFrame *btnBox = new QFrame( mainFrame );
    mainFrameLayout->addWidget(btnBox);
    mainFrame->setLayout(mainFrameLayout);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    m_addButton->setToolTip(i18n("Add a Control Parameter to the Studio"));

    m_deleteButton->setToolTip(i18n("Delete a Control Parameter from the Studio"));

    m_closeButton->setToolTip(i18n("Close the Control Parameter editor"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    setupActions();

//     m_doc->getCommandHistory()->attachView( actionCollection() );	//&&&
	
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QTreeWidgetItem *)),
            SLOT(slotEdit(QTreeWidgetItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
	
	m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );
// 	m_listView->setSelectionBehavior( QAbstractItemView::SelectRows );

    initDialog();

//     setAutoSaveSettings(ControlEditorConfigGroup, true);	//&&&
}

ControlEditorDialog::~ControlEditorDialog()
{
    RG_DEBUG << "\n*** ControlEditorDialog::~ControlEditorDialog\n" << endl;

//     m_listView->saveLayout(ControlEditorConfigGroup);	//&&&

//     if (m_doc)
//         m_doc->getCommandHistory()->detachView( actionCollection() );	//&&&
	
}

void
ControlEditorDialog::initDialog()
{
    RG_DEBUG << "ControlEditorDialog::initDialog" << endl;
    slotUpdate();
}

void
ControlEditorDialog::slotUpdate()
{
    RG_DEBUG << "ControlEditorDialog::slotUpdate" << endl;

    //QPtrList<QTreeWidgetItem> selection = m_listView->selectedItems();

    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (!md)
        return ;

    ControlList::const_iterator it = md->beginControllers();
    QTreeWidgetItem *item;
    int i = 0;

    m_listView->clear();

    for (; it != md->endControllers(); ++it) {
        Composition &comp = m_doc->getComposition();

        QString colour =
            strtoqstr(comp.getGeneralColourMap().getNameByIndex(it->getColourIndex()));

        if (colour == "")
            colour = i18n("<default>");

        QString position = QString("%1").arg(it->getIPBPosition());
        if (position.toInt() == -1)
            position = notShowing;

        QString value;
        value.sprintf("%d (0x%x)", it->getControllerValue(),
                      it->getControllerValue());

        if (it->getType() == PitchBend::EventType) {
            item = new ControlParameterItem(
											i++,
											m_listView,
											QStringList()
                                            	<< strtoqstr(it->getName())
                                            	<< strtoqstr(it->getType())
                                            	<< QString("-")
                                            	<< strtoqstr(it->getDescription())
                                            	<< QString("%1").arg(it->getMin())
                                            	<< QString("%1").arg(it->getMax())
                                            	<< QString("%1").arg(it->getDefault())
                                            	<< colour
                                            	<< position 
										   );
        } else {
            item = new ControlParameterItem(
							i++,
							m_listView,
							QStringList()
								<< strtoqstr(it->getName())
								<< strtoqstr(it->getType())
								<< value
								<< strtoqstr(it->getDescription())
								<< QString("%1").arg(it->getMin())
								<< QString("%1").arg(it->getMax())
								<< QString("%1").arg(it->getDefault())
								<< colour
								<< position
							);
        }


        // create and set a colour pixmap
        //
        QPixmap colourPixmap(16, 16);
        Colour c = comp.getGeneralColourMap().getColourByIndex(it->getColourIndex());
        colourPixmap.fill(QColor(c.getRed(), c.getGreen(), c.getBlue()));
		
// 		item->setPixmap(7, colourPixmap);
		item->setIcon(7, QIcon(colourPixmap) );

		m_listView->addTopLevelItem(item);
    }

    if( m_listView->topLevelItemCount() == 0 ) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_listView, QStringList( i18n("<none>")) );
		m_listView->addTopLevelItem(item);

		m_listView->setSelectionMode( QAbstractItemView::NoSelection );
    } else {
		m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    }


}

/*
void
ControlEditorDialog::slotEditCopy()
{
    RG_DEBUG << "ControlEditorDialog::slotEditCopy" << endl;
}

void
ControlEditorDialog::slotEditPaste()
{
    RG_DEBUG << "ControlEditorDialog::slotEditPaste" << endl;
}
*/

void
ControlEditorDialog::slotAdd()
{
    RG_DEBUG << "ControlEditorDialog::slotAdd to device " << m_device << endl;

    AddControlParameterCommand *command =
        new AddControlParameterCommand(m_studio, m_device,
                                       ControlParameter());

    addCommandToHistory(command);
}

void
ControlEditorDialog::slotDelete()
{
    RG_DEBUG << "ControlEditorDialog::slotDelete" << endl;

// 	if (!m_listView->currentIndex())
	if( ! m_listView->currentItem() )
		return ;

    ControlParameterItem *item =
        dynamic_cast<ControlParameterItem*>( m_listView->currentItem() );

    if (item) {
        RemoveControlParameterCommand *command =
            new RemoveControlParameterCommand(m_studio, m_device, item->getId());

        addCommandToHistory(command);
    }
}

void
ControlEditorDialog::slotClose()
{
    RG_DEBUG << "ControlEditorDialog::slotClose" << endl;

//     if (m_doc)
//         m_doc->getCommandHistory()->detachView(actionCollection());	//&&&
    m_doc = 0;

    close();
}

void
ControlEditorDialog::setupActions()
{
//     KAction* close = KStandardAction::close(this,
//                                        SLOT(slotClose()),
//                                        actionCollection());
	
 	QAction* close = new QAction( i18n("&Close"), this );
//  	connect( close, SIGNAL(close()), this, SLOT(slotClose()) );
	
    m_closeButton->setText( close->text() );
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));
	
	
	QAction *tac;
	
    // some adjustments
//     new KToolBarPopupAction(i18n("Und&o"),
//                             "undo",
//                             KStandardShortcut::key(KStandardShortcut::Undo),
//                             actionCollection(),
//                             KStandardAction::stdName(KStandardAction::Undo));
	tac = new QAction( i18n("Und&o"), this );
	tac->setObjectName( "undo" );
	tac->setShortcut( QKeySequence::Undo );

//     new KToolBarPopupAction(i18n("Re&do"),
//                             "redo",
//                             KStandardShortcut::key(KStandardShortcut::Redo),
//                             actionCollection(),
//                             KStandardAction::stdName(KStandardAction::Redo));
	tac = new QAction( i18n("Re&do"), this );
	tac->setObjectName( "redo" );
	tac->setShortcut( QKeySequence::Redo );

    rgTempQtIV->createGUI("controleditor.rc", 0);
}

void
ControlEditorDialog::addCommandToHistory(Command *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory*
ControlEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
ControlEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "ControlEditorDialog::setModified(" << modified << ")" << endl;

    if (modified) {}
    else {}

    m_modified = modified;
}

void
ControlEditorDialog::checkModified()
{
    RG_DEBUG << "ControlEditorDialog::checkModified(" << m_modified << ")"
    << endl;

}

void
ControlEditorDialog::slotEdit()
{}

void
ControlEditorDialog::slotEdit(QTreeWidgetItem *i)
{
    RG_DEBUG << "ControlEditorDialog::slotEdit" << endl;

    ControlParameterItem *item =
        dynamic_cast<ControlParameterItem*>(i);

    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));

    if (item && md) {
        ControlParameterEditDialog dialog
        (this,
         md->getControlParameter(item->getId()), m_doc);

        if (dialog.exec() == QDialog::Accepted) {
            ModifyControlParameterCommand *command =
                new ModifyControlParameterCommand(m_studio,
                                                  m_device,
                                                  dialog.getControl(),
                                                  item->getId());

            addCommandToHistory(command);
        }
    }
}

void
ControlEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
	close();
//     KMainWindow::closeEvent(e);
}

void
ControlEditorDialog::setDocument(RosegardenGUIDoc *doc)
{
    // reset our pointers
    m_doc = doc;
    m_studio = &doc->getStudio();
    m_modified = false;

    slotUpdate();
}

}
#include "ControlEditorDialog.moc"
