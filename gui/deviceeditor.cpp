// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "deviceeditor.h"

#include <qtable.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qpushbutton.h>

#include <kapp.h>
#include <klocale.h>
#include <dcopclient.h>

#include "rosegardenguidoc.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardendcop.h"

#include "Studio.h"
#include "Device.h"
#include "MidiDevice.h"


DeviceEditorDialog::DeviceEditorDialog(QWidget *parent,
				       RosegardenGUIDoc *document) :
    KDialogBase(parent, "deviceeditordialog", true,
		i18n("Manage MIDI Devices"), Ok | Apply | Close, Ok, true),
    m_document(document),
    m_studio(&document->getStudio())
{
    QVBox *mainBox = makeVBoxMainWidget();

    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;

    Rosegarden::DeviceList validDevices;
    for (it = devices->begin(); it != devices->end(); ++it) {
	if ((*it)->getType() == Rosegarden::Device::Midi) {
	    Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>(*it);
	    validDevices.push_back(md);
	}
    }

    m_table = new QTable(validDevices.size(), 4, mainBox);
    m_table->setSorting(false);
    m_table->setRowMovingEnabled(false);
    m_table->setColumnMovingEnabled(false);
    m_table->setShowGrid(false);
    m_table->horizontalHeader()->setLabel(0, i18n("Device"));
    m_table->horizontalHeader()->setLabel(1, i18n("Name"));
    m_table->horizontalHeader()->setLabel(2, i18n("Type"));
    m_table->horizontalHeader()->setLabel(3, i18n("Connection"));
    m_table->horizontalHeader()->show();
    m_table->verticalHeader()->hide();
    m_table->setLeftMargin(0);
    m_table->setSelectionMode(QTable::SingleRow);
    m_table->setColumnReadOnly(0, true);
    m_table->setColumnReadOnly(2, true);

    int deviceCount = 0;

    for (it = validDevices.begin(); it != validDevices.end(); ++it) {

	// we know we only put MidiDevices in the valid device list above
	Rosegarden::MidiDevice *md = static_cast<Rosegarden::MidiDevice *>(*it);

	Rosegarden::DeviceId id = md->getId();
	QString deviceName = i18n("Device %1").arg(md->getId() + 1);
	QString deviceLabel = strtoqstr(md->getName());
	QString connectionName = strtoqstr(md->getConnection());

	QStringList connectionList;
	
	QByteArray data;
	QByteArray replyData;
	QCString replyType;
	QDataStream arg(data, IO_WriteOnly);
	arg << (unsigned int)id;

	if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
				      ROSEGARDEN_SEQUENCER_IFACE_NAME,
				      "getConnections(unsigned int)",
				      data, replyType, replyData, false)) {
	    RG_DEBUG << "DeviceEditorDialog: can't call Sequencer" << endl;
	    continue;
	}

	QDataStream reply(replyData, IO_ReadOnly);
	unsigned int connections = 0;
	if (replyType == "unsigned int") reply >> connections;
	int currentConnectionIndex = -1;

	for (unsigned int i = 0; i < connections; ++i) {

	    QByteArray data;
	    QByteArray replyData;
	    QCString replyType;
	    QDataStream arg(data, IO_WriteOnly);
	    arg << (unsigned int)id;
	    arg << i;
	    
	    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
					  ROSEGARDEN_SEQUENCER_IFACE_NAME,
					  "getConnection(unsigned int, unsigned int)",
					  data, replyType, replyData, false)) {
		RG_DEBUG << "DeviceEditorDialog: can't call Sequencer" << endl;
		continue;
	    }
	    
	    QDataStream reply(replyData, IO_ReadOnly);
	    QString connection;
	    if (replyType == "QString") {
		reply >> connection;
		connectionList.append(connection);
		if (connection == connectionName) currentConnectionIndex = i;
	    }
	}

	connectionList.append(i18n("No connection"));
	if (currentConnectionIndex == -1)
	    currentConnectionIndex = connections;

	m_table->setText(deviceCount, 0, deviceName);
	m_table->setText(deviceCount, 1, deviceLabel);
	m_table->setText(deviceCount, 2,
			 (md->getDirection() == Rosegarden::MidiDevice::Play ?
			  i18n("Play") : i18n("Record")));

	QComboTableItem *item = new QComboTableItem(m_table, connectionList, false);
	item->setCurrentItem(currentConnectionIndex);
	m_table->setItem(deviceCount, 3, item);

	m_table->adjustRow(deviceCount);
	++deviceCount;
    }

    m_table->adjustColumn(0);
    if (m_table->columnWidth(0) < 120) m_table->setColumnWidth(0, 120);

    m_table->adjustColumn(1);
    if (m_table->columnWidth(1) < 120) m_table->setColumnWidth(1, 120);

    m_table->adjustColumn(2);
    if (m_table->columnWidth(2) < 80) m_table->setColumnWidth(2, 80);

    m_table->adjustColumn(3);
    if (m_table->columnWidth(3) < 200) m_table->setColumnWidth(3, 200);

    QHBox *hbox = new QHBox(mainBox);
    QPushButton *addButton = new QPushButton(i18n("Add Play Device"), hbox);
    QPushButton *addRButton = new QPushButton(i18n("Add Record Device"), hbox);
    QPushButton *deleteButton = new QPushButton(i18n("Delete Device"), hbox);
    connect(addButton, SIGNAL(clicked()), this, SLOT(slotAddPlayDevice()));
    connect(addRButton, SIGNAL(clicked()), this, SLOT(slotAddRecordDevice()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(slotDeleteDevice()));

    setMinimumHeight(250);
}


void
DeviceEditorDialog::slotAddPlayDevice()
{
    //...
}

void
DeviceEditorDialog::slotAddRecordDevice()
{
    //...
}

void
DeviceEditorDialog::slotDeleteDevice()
{
    //...
}

void
DeviceEditorDialog::slotValueChanged(int row, int column)
{
    //...
}

