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

#include <algorithm>

#include <qobjectlist.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qdir.h>

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kio/netaccess.h>

#include "bankeditor.h"
#include "widgets.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "studiocommands.h"
#include "rosedebug.h"

#include "Studio.h"
#include "MidiDevice.h"

#include "SF2PatchExtractor.h"

using Rosegarden::SF2PatchExtractor;


MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListView* parent, QString name)
    : QListViewItem(parent, name),
      m_deviceId(deviceId)
{
}

MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListViewItem* parent, QString name,
                                               QString msb, QString lsb)
    : QListViewItem(parent, name, msb, lsb),
      m_deviceId(deviceId)
{
}

//--------------------------------------------------

MidiBankListViewItem::MidiBankListViewItem(Rosegarden::DeviceId deviceId,
                                           int bankNb,
                                           QListViewItem* parent,
                                           QString name, QString msb, QString lsb)
    : MidiDeviceListViewItem(deviceId, parent, name, msb, lsb),
      m_bankNb(bankNb)
{
}

//--------------------------------------------------

MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog* bankEditor,
                                       QWidget* parent,
                                       const char* name)
    : QVGroupBox(i18n("Bank and Program details"),
                 parent, name),
      m_bankEditor(bankEditor),
      m_mainFrame(new QFrame(this)),
      m_bankName(new QLabel(m_mainFrame)),
      m_msb(new QSpinBox(m_mainFrame)),
      m_lsb(new QSpinBox(m_mainFrame)),
      m_bankList(bankEditor->getBankList()),
      m_programList(bankEditor->getProgramList())
{

    QGridLayout *gridLayout = new QGridLayout(m_mainFrame,
                                              1,  // rows
                                              6,  // cols
                                              2); // margin
 


    /*
    gridLayout->addWidget(new QLabel(i18n("Bank Name"), m_mainFrame),
                          0, 0, AlignLeft);
                          */
    QFont boldFont = m_bankName->font();
    boldFont.setBold(true);
    m_bankName->setFont(boldFont);

    gridLayout->addWidget(m_bankName, 0, 0, AlignLeft);

    gridLayout->addWidget(new QLabel(i18n("MSB Value"), m_mainFrame),
                          1, 0, AlignLeft);
    m_msb->setMinValue(0);
    m_msb->setMaxValue(127);
    gridLayout->addWidget(m_msb, 1, 1, AlignLeft);

    QToolTip::add(m_msb,
            i18n("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    QToolTip::add(m_lsb,
            i18n("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame),
                          2, 0, AlignLeft);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 2, 1, AlignLeft);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(2,
                                        Qt::Horizontal,
                                        i18n("Librarian"),
                                        m_mainFrame);
    gridLayout->addMultiCellWidget(groupBox, 0, 2, 3, 5);


    new QLabel(i18n("Name"), groupBox);
    m_librarian = new QLabel(groupBox);

    new QLabel(i18n("Email"), groupBox);
    m_librarianEmail = new QLabel(groupBox);

    QToolTip::add(groupBox,
                  i18n("The librarian maintains the generic Bank and Program information for this device.\nIf you've made modifications to a Bank to suit your own device it might be worth\nliasing with the librarian in order to publish your Bank information for the benefit\nof others."));

    QTabWidget* tab = new QTabWidget(this);

    tab->setMargin(10);

    QHBox *progHBox = new QHBox(tab);
    QVBox *progVBox;
    QHBox *numBox;
    QLabel *label;

    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            unsigned int labelId = j*16 + i;

            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(labelId + 1), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            KLineEdit* lineEdit = new KLineEdit(numBox, label->text().data());
            lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
            lineEdit->setCompletionObject(&m_completion);
            m_programNames.push_back(lineEdit);

            connect(m_programNames[labelId],
                    SIGNAL(textChanged(const QString&)),
                    this,
                    SLOT(slotProgramChanged(const QString&)));
        }
    }
    tab->addTab(progHBox, i18n("Programs 1 - 64"));

    progHBox = new QHBox(tab);
    for (unsigned int j = 0; j < 4; j++)
    {
        progVBox = new QVBox(progHBox);

        for (unsigned int i = 0; i < 16; i++)
        {
            unsigned int labelId = 64 + j*16 + i;

            numBox = new QHBox(progVBox);
            label = new QLabel(QString("%1").arg(labelId + 1), numBox);
            label->setFixedWidth(50);
            label->setAlignment(AlignHCenter);

            KLineEdit* lineEdit = new KLineEdit(numBox, label->text().data());
            lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
            lineEdit->setCompletionObject(&m_completion);
            m_programNames.push_back(lineEdit);

            connect(m_programNames[labelId],
                    SIGNAL(textChanged(const QString&)),
                    this,
                    SLOT(slotProgramChanged(const QString&)));

        }
    }
    tab->addTab(progHBox, i18n("Programs 65 - 128"));

}

MidiProgramsEditor::MidiProgramContainer
MidiProgramsEditor::getBankSubset(Rosegarden::MidiByte msb,
                                  Rosegarden::MidiByte lsb)
{
    MidiProgramContainer program;
    MidiProgramContainer::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb)
            program.push_back(*it);
    }

    return program;
}

Rosegarden::MidiBank*
MidiProgramsEditor::getCurrentBank()
{
    return m_currentBank;
}

void
MidiProgramsEditor::modifyCurrentPrograms(int oldMSB, int oldLSB,
                                        int msb, int lsb)
{
    MidiProgramContainer::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->msb == oldMSB && it->lsb == oldLSB)
        {
            it->msb = msb;
            it->lsb = lsb;
        }
    }
}

void
MidiProgramsEditor::clearAll()
{
    blockAllSignals(true);

    for(unsigned int i = 0; i < m_programNames.size(); ++i)
        m_programNames[i]->clear();
    
    m_bankName->clear();
    m_msb->setValue(0);
    m_lsb->setValue(0);
    m_librarian->setText("");
    m_librarianEmail->setText("");
    m_currentBank = 0;
    setEnabled(false);

    blockAllSignals(false);
}

void
MidiProgramsEditor::populateBank(QListViewItem* item)
{
    RG_DEBUG << "MidiProgramsEditor::populateBank" << endl;

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    if (!bankItem) return;
    
    Rosegarden::DeviceId deviceId = bankItem->getDevice();
    Rosegarden::MidiDevice* device = m_bankEditor->getMidiDevice(deviceId);
    if (!device) return;
    
    setEnabled(true);

    setBankName(item->text(0));

    RG_DEBUG << "MidiProgramsEditor::populateBank : bankItem->getBank = "
             << bankItem->getBank() << endl;

    m_currentBank = &(m_bankList[bankItem->getBank()]); // device->getBankByIndex(bankItem->getBank());

    blockAllSignals(true);

    // set the bank values
    m_msb->setValue(m_currentBank->msb);
    m_lsb->setValue(m_currentBank->lsb);

    // Librarian details
    //
    m_librarian->setText(strtoqstr(device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(device->getLibrarianEmail()));

    MidiProgramContainer programSubset = getBankSubset(m_currentBank->msb,
                                                       m_currentBank->lsb);
    MidiProgramContainer::iterator it;

    for (unsigned int i = 0; i < m_programNames.size(); i++) {
        m_programNames[i]->clear();

        for (it = programSubset.begin(); it != programSubset.end(); it++) {
            if (it->program == i) {
                QString programName = strtoqstr(it->name);
                m_completion.addItem(programName);
                m_programNames[i]->setText(programName);
                break;
            }
        }

        // show start of label
        m_programNames[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiProgramsEditor::slotNewMSB(int value)
{
    m_msb->blockSignals(true);

    int msb;

    try
        {
            msb = ensureUniqueMSB(value, value > getCurrentBank()->msb);
        }
    catch(bool)
        {
            msb = getCurrentBank()->msb;
        }

    modifyCurrentPrograms(getCurrentBank()->msb,
                          getCurrentBank()->lsb,
                          msb,
                          getCurrentBank()->lsb);

    m_msb->setValue(msb);
    getCurrentBank()->msb = msb;

    m_msb->blockSignals(false);

    m_bankEditor->setModified(true);
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    m_lsb->blockSignals(true);

    int lsb;

    try
        {
            lsb = ensureUniqueLSB(value, value > getCurrentBank()->lsb);
        }
    catch(bool)
        {
            lsb = getCurrentBank()->lsb;
        }

    modifyCurrentPrograms(getCurrentBank()->msb,
                          getCurrentBank()->lsb,
                          getCurrentBank()->msb,
                          lsb);

    m_lsb->setValue(lsb);
    getCurrentBank()->lsb = lsb;

    m_lsb->blockSignals(false);

    m_bankEditor->setModified(true);
}

struct ProgramCmp
{
    bool operator()(const Rosegarden::MidiProgram &p1,
                    const Rosegarden::MidiProgram &p2)
    {
        if (p1.program == p2.program)
        {
            if (p1.msb == p2.msb)
                return (p1.lsb < p2.lsb);
            else
                return (p1.msb < p2.msb);
        }
        else
            return (p1.program < p2.program);
    }
};

void
MidiProgramsEditor::slotProgramChanged(const QString &programName)
{
    QString senderName = sender()->name();

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;

    RG_DEBUG << "BankEditorDialog::slotProgramChanged("
             << programName << ") : id = " << id << endl;

    Rosegarden::MidiProgram *program =
        getProgram(getCurrentBank()->msb,
                   getCurrentBank()->lsb,
                   id);

    if (program == 0)
    {
        // Do nothing if program name is empty
        if (programName.isEmpty()) return;

        program = new Rosegarden::MidiProgram;
        program->msb = getCurrentBank()->msb;
        program->lsb = getCurrentBank()->lsb;
        program->program = id;
        m_programList.push_back(*program);

        // Sort the program list by id
        std::sort(m_programList.begin(), m_programList.end(), ProgramCmp());

        // Now, get with the program
        //
        program =
            getProgram(getCurrentBank()->msb,
                       getCurrentBank()->lsb,
                       id);
    }
    else
    {
        // If we've found a program and the label is now empty
        // then remove it from the program list.
        //
        if (programName.isEmpty())
        {
            MidiProgramContainer::iterator it = m_programList.begin();
            MidiProgramContainer tmpProg;

            for (; it != m_programList.end(); it++)
            {
                if (((unsigned int)it->program) == id)
                {
                    m_programList.erase(it);
                    m_bankEditor->setModified(true);
                    RG_DEBUG << "deleting empty program (" << id << ")" << endl;
                    return;
                }
            }
        }
    }

    if (qstrtostr(programName) != program->name)
    {
        program->name = qstrtostr(programName);
        m_bankEditor->setModified(true);
    }
}

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
    int newMSB = msb;
    while (banklistContains(newMSB, m_lsb->value())
           && newMSB < 128
           && newMSB > -1)
        if (ascending) newMSB++;
        else newMSB--;

   if (newMSB == -1 || newMSB == 128)
       throw false;

    return newMSB;
}

int
MidiProgramsEditor::ensureUniqueLSB(int lsb, bool ascending)
{
    int newLSB = lsb;
    while (banklistContains(m_msb->value(), newLSB)
           && newLSB < 128
           && newLSB > -1)
        if (ascending) newLSB++;
        else newLSB--;

   if (newLSB == -1 || newLSB == 128)
       throw false;

    return newLSB;
}

bool
MidiProgramsEditor::banklistContains(int msb, int lsb)
{
    MidiBankContainer::iterator it;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        if (it->msb == msb && it->lsb == lsb)
            return true;

    return false;
}

Rosegarden::MidiProgram*
MidiProgramsEditor::getProgram(int msb, int lsb, int program)
{
    MidiProgramsEditor::MidiProgramContainer::iterator it = m_programList.begin();

    for (; it != m_programList.end(); it++)
    {
        if (it->msb == msb && it->lsb == lsb && it->program == program)
            return &(*it);
    }

    return 0;

}

void
MidiProgramsEditor::setBankName(const QString& s)
{
    m_bankName->setText(s);
}

//--------------------------------------------------

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc):
    KDialogBase(parent, "bankeditordialog", true,
                i18n("Manage MIDI Banks and Programs"),
                Ok | Apply | Close,
                Ok, true),
    m_studio(&doc->getStudio()),
    m_doc(doc),
    m_copyBank(Rosegarden::Device::NO_DEVICE, -1),
    m_modified(false),
    m_keepBankList(false),
    m_deleteAll(false),
    m_lastDevice(Rosegarden::Device::NO_DEVICE),
    m_lastMSB(0),
    m_lastLSB(0)
{
    QHBox* mainFrame = makeHBoxMainWidget();

    QSplitter* splitter = new QSplitter(mainFrame);

    //
    // Left-side list view
    //
    QVBox* leftPart = new QVBox(splitter);
    m_listView = new KListView(leftPart);
    m_listView->addColumn(i18n("MIDI Device"));
    m_listView->addColumn(i18n("MSB"));
    m_listView->addColumn(i18n("LSB"));
    m_listView->setRootIsDecorated(true);
    m_listView->setShowSortIndicator(true);
    m_listView->setItemsRenameable(true);

    QGroupBox *bankBox  = new QGroupBox(3,
                                        Qt::Horizontal,
                                        i18n("Manage Banks..."),
                                        leftPart);

    m_addBank        = new QPushButton(i18n("Add Bank"), bankBox);
    m_deleteBank     = new QPushButton(i18n("Delete Bank"), bankBox);
    m_deleteAllBanks = new QPushButton(i18n("Delete All Banks"), bankBox);

    // Tips
    //
    QToolTip::add(m_addBank,
                  i18n("Add a Bank to the current device"));

    QToolTip::add(m_deleteBank,
                  i18n("Delete the current Bank"));

    QToolTip::add(m_deleteAllBanks,
                  i18n("Delete all Banks from the current Device"));

    m_importBanks = new QPushButton(i18n("Import Banks"), bankBox);
    m_exportBanks = new QPushButton(i18n("Export Banks"), bankBox);
    new QLabel(bankBox); // spacer

    // Tips
    //
    QToolTip::add(m_importBanks,
            i18n("Import Bank and Program data from a Rosegarden file to the current Device"));
    QToolTip::add(m_exportBanks,
            i18n("Export all Device and Bank information to a Rosegarden format  interchange file"));

    m_copyPrograms = new QPushButton(i18n("Copy Programs"), bankBox);
    m_pastePrograms = new QPushButton(i18n("Paste Programs"), bankBox);

    // Tips
    //
    QToolTip::add(m_copyPrograms,
            i18n("Copy all Program names from current Bank to clipboard"));

    QToolTip::add(m_pastePrograms,
            i18n("Paste Program names from clipboard to current Bank"));

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            this,       SLOT(slotPopulateDevice(QListViewItem*)));

    m_programEditor = new MidiProgramsEditor(this, splitter);


    // default buttons states
    enableButtonOK(false);
    enableButtonApply(false);

    // device/bank modification
    connect(m_listView, SIGNAL(itemRenamed             (QListViewItem*,const QString&,int)),
            this,       SLOT(slotModifyDeviceOrBankName(QListViewItem*,const QString&,int)));

    connect(m_addBank, SIGNAL(clicked()),
            this, SLOT(slotAddBank()));

    connect(m_deleteBank, SIGNAL(clicked()),
            this, SLOT(slotDeleteBank()));

    connect(m_deleteAllBanks, SIGNAL(clicked()),
            this, SLOT(slotDeleteAllBanks()));

    connect(m_importBanks, SIGNAL(clicked()),
            this, SLOT(slotImport()));

    connect(m_exportBanks, SIGNAL(clicked()),
            this, SLOT(slotExport()));

    connect(m_copyPrograms, SIGNAL(clicked()),
            this, SLOT(slotCopy()));

    connect(m_pastePrograms, SIGNAL(clicked()),
            this, SLOT(slotPaste()));

    // Initialise the dialog
    //
    initDialog();

    // Check for no Midi devices and disable everything
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    bool haveMidiPlayDevice = false;
    for (it = devices->begin(); it != devices->end(); ++it) {
	Rosegarden::MidiDevice *md =
	    dynamic_cast<Rosegarden::MidiDevice *>(*it);
	if (md && md->getDirection() == Rosegarden::MidiDevice::Play) {
	    haveMidiPlayDevice = true;
	    break;
	}
    }
    if (!haveMidiPlayDevice) {
        leftPart->setDisabled(true);
        m_programEditor->setDisabled(true);
    }

};

void
BankEditorDialog::initDialog()
{
    // Clear down
    //
    m_deviceNameMap.clear();
    m_listView->clear();

    // Fill list view
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;

    for (it = devices->begin(); it != devices->end(); ++it)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            Rosegarden::MidiDevice* midiDevice =
		dynamic_cast<Rosegarden::MidiDevice*>(*it);
	    if (!midiDevice) continue;
            
	    // skip read-only devices
	    if (midiDevice->getDirection() == Rosegarden::MidiDevice::Record)
		continue;

            m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog : adding "
                     << itemName << endl;

            QListViewItem* deviceItem = new MidiDeviceListViewItem
		(midiDevice->getId(), m_listView, itemName);
            deviceItem->setOpen(true);

            MidiProgramsEditor::MidiBankContainer banks = midiDevice->getBanks();
            // add banks for this device
            for (unsigned int i = 0; i < banks.size(); ++i) {
                RG_DEBUG << "BankEditorDialog : adding "
                         << itemName << " - " << strtoqstr(banks[i].name)
                         << endl;
                new MidiBankListViewItem(midiDevice->getId(), i, deviceItem,
                                         strtoqstr(banks[i].name),
                                         QString("%1").arg(banks[i].msb),
                                         QString("%1").arg(banks[i].lsb));
            }
            
            
        }
    }

    // Select the first Device
    //
    slotPopulateDevice(m_listView->firstChild());
    m_listView->setSelected(m_listView->firstChild(), true);

}


Rosegarden::MidiDevice*
BankEditorDialog::getCurrentMidiDevice()
{
    return getMidiDevice(m_listView->currentItem());
}

void
BankEditorDialog::checkModified()
{
    if (m_modified)
    {
        // then ask if we want to apply the changes

        int reply = KMessageBox::questionYesNo(this,
                                               i18n("Apply pending changes?"));

        if (reply == KMessageBox::Yes)
        {
            ModifyDeviceCommand *command;

            if (m_bankList.size() == 0 && m_programList.size() == 0)
            {
                Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

                std::vector<Rosegarden::MidiBank>
                    tempBank = device->getBanks();
                std::vector<Rosegarden::MidiProgram> tempProg =
                    device->getPrograms();

                command = new ModifyDeviceCommand(m_studio,
                                                  m_lastDevice,
                                                  m_deviceNameMap[m_lastDevice],
                                                  device->getLibrarianName(),
                                                  device->getLibrarianEmail(),
                                                  tempBank,
                                                  tempProg,
                                                  true); // overwrite
            }
            else
            {
                Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

                command = new ModifyDeviceCommand(m_studio,
                                                  m_lastDevice,
                                                  m_deviceNameMap[m_lastDevice],
                                                  device->getLibrarianName(),
                                                  device->getLibrarianEmail(),
                                                  m_bankList,
                                                  m_programList,
                                                  true);
            }

            addCommandToHistory(command);
        }
        setModified(false);
    }

}

void
BankEditorDialog::slotPopulateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::slotPopulateDevice\n";

    if (!item) return;

    checkModified();
    
    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);

    if (!bankItem) {

        // Ensure we fill these lists for the new device
        //
        MidiDeviceListViewItem* deviceItem = getParentDeviceItem(item);
        Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);
        m_bankList = device->getBanks();
        m_programList = device->getPrograms();

        RG_DEBUG << "BankEditorDialog::slotPopulateDevice : not a bank item - disabling\n";
        m_deleteBank->setEnabled(false);
        m_copyPrograms->setEnabled(false);
        m_pastePrograms->setEnabled(false);
        m_programEditor->clearAll();
        return;
    }
    
    m_deleteBank->setEnabled(true);
    m_copyPrograms->setEnabled(true);

    if (m_copyBank.first != Rosegarden::Device::NO_DEVICE)
        m_pastePrograms->setEnabled(true);

    Rosegarden::MidiDevice *device = getMidiDevice(bankItem->getDevice());

    if (!m_keepBankList || m_bankList.size() == 0)
        m_bankList    = device->getBanks();
    else
        m_keepBankList = false;

    m_programList = device->getPrograms();

    m_lastMSB = m_bankList[bankItem->getBank()].msb;
    m_lastLSB = m_bankList[bankItem->getBank()].lsb;

    m_programEditor->populateBank(item);

    m_lastDevice = bankItem->getDevice();
}

void
BankEditorDialog::slotOk()
{
    slotApply();
    accept();
}

void
BankEditorDialog::slotApply()
{
    ModifyDeviceCommand *command;

    // Make sure that we don't delete all the banks and programs
    // if we've not populated them here yet.
    //
    if (m_bankList.size() == 0 && m_programList.size() == 0 &&
            m_deleteAll == false)
    {
        Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

        std::vector<Rosegarden::MidiBank> tempBank = device->getBanks();
        std::vector<Rosegarden::MidiProgram> tempProg =
            device->getPrograms();

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
                                          tempBank,
                                          tempProg,
                                          true);
    }
    else
    {
        /*
        MidiProgramsEditor::MidiProgramContainer::iterator it =
            m_programList.begin();
            */

        Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
                                          m_bankList,
                                          m_programList,
                                          true);

    }
    addCommandToHistory(command);

    setModified(false);
    initDialog();
}
void
BankEditorDialog::slotClose()
{
    checkModified();
    reject();
}

MidiDeviceListViewItem*
BankEditorDialog::getParentDeviceItem(QListViewItem* item)
{
    if (!item) return 0;

    if (dynamic_cast<MidiBankListViewItem*>(item)) // this is a bank item,
        // go up to the parent device item
        item = item->parent();

    if (!item) {
        RG_DEBUG << "BankEditorDialog::getParentDeviceItem : missing parent device item for bank item - this SHOULD NOT HAPPEN\n";
        return 0;
    }

    return dynamic_cast<MidiDeviceListViewItem*>(item);
}


void
BankEditorDialog::slotAddBank()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);
   
    if (device)
    {
        // If the bank and program lists are empty then try to
        // populate them.
        //
        if (m_bankList.size() == 0 && m_programList.size() == 0)
        {
            m_bankList = device->getBanks();
            m_programList = device->getPrograms();
        }

        std::pair<int, int> bank = getFirstFreeBank(m_listView->currentItem());

        Rosegarden::MidiBank newBank;
        newBank.msb = bank.first;
        newBank.lsb = bank.second;
        newBank.name = "<new bank>";
        m_bankList.push_back(newBank);

        QListViewItem* newBankItem =
            new MidiBankListViewItem(deviceItem->getDevice(),
                                     m_bankList.size() - 1,
                                     deviceItem,
                                     strtoqstr(newBank.name),
                                     QString("%1").arg(newBank.msb),
                                     QString("%1").arg(newBank.lsb));
        keepBankListForNextPopulate();
        m_listView->setCurrentItem(newBankItem);

        slotApply();
        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotDeleteBank()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(currentItem);

    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);

    if (device)
    {
        int currentBank = bankItem->getBank();

        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this bank?"));

        if (reply == KMessageBox::Yes)
        {
            int newBank = currentBank - 1;
        
            if (newBank < 0) newBank = 0;

            int msb = m_bankList[currentBank].msb;
            int lsb = m_bankList[currentBank].lsb;

            // Copy across all programs that aren't in the doomed bank
            //
            MidiProgramsEditor::MidiProgramContainer::iterator it;
            MidiProgramsEditor::MidiProgramContainer tempList;
            for (it = m_programList.begin(); it != m_programList.end(); it++)
                if (it->msb != msb || it->lsb != lsb)
                    tempList.push_back(*it);

            // Erase the bank and repopulate
            //
            MidiProgramsEditor::MidiBankContainer::iterator er =
                m_bankList.begin();
            er += currentBank;
            m_bankList.erase(er);
            m_programList = tempList;
            keepBankListForNextPopulate();

            // the listview automatically selects a new current item
            delete currentItem;

            // Don't allow pasting from this defunct device
            //
            if (m_copyBank.first == bankItem->getDevice() &&
                m_copyBank.second == bankItem->getBank())
            {
                m_pastePrograms->setEnabled(false);
                m_copyBank = std::pair<Rosegarden::DeviceId, int>
		    (Rosegarden::Device::NO_DEVICE, -1);
            }

            slotApply();
            selectDeviceItem(device);

        }
    }
}

void
BankEditorDialog::slotDeleteAllBanks()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();
    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);

    QString question = i18n("Really delete all banks for ") +
                       strtoqstr(device->getName()) + QString(" ?");

    int reply = KMessageBox::warningYesNo(this, question);

    if (reply == KMessageBox::Yes)
    {

        // erase all bank items
        QListViewItem* child = 0;
        while((child = deviceItem->firstChild())) delete child;

        m_bankList.clear();
        m_programList.clear();

        // Don't allow pasting from this defunct device
        //
        if (m_copyBank.first == deviceItem->getDevice())
        {
            m_pastePrograms->setEnabled(false);
            m_copyBank = std::pair<Rosegarden::DeviceId, int>
		(Rosegarden::Device::NO_DEVICE, -1);
        }

        // Urgh, we have this horrible flag that we're using to frig this.
        // (we might not need this anymore but I'm too scared to remove it
        // now).
        //
        m_deleteAll = true;
        slotApply();
        m_deleteAll = false;

        selectDeviceItem(device);

    }
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(Rosegarden::DeviceId id)
{
    return m_studio->getMidiDevice(id);
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(QListViewItem* item)
{
    MidiDeviceListViewItem* deviceItem =
	dynamic_cast<MidiDeviceListViewItem*>(item);
    if (!deviceItem) return 0;

    return m_studio->getMidiDevice(deviceItem->getDevice());
}

// Try to find a unique MSB/LSB pair for a new bank
//
std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QListViewItem* item)
{
    int msb = 0;
    int lsb = 0;

    Rosegarden::MidiDevice *device = getMidiDevice(item);

    if (device)
    {
        do
        {
            lsb = 0;
            while(m_programEditor->banklistContains(msb, lsb) && lsb < 128)
                lsb++;
        }
        while (lsb == 128 && msb++);
    }

    return std::pair<int, int>(msb, lsb);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QListViewItem* item, const QString &label, int)
{
    RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName\n";

    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName : modify bank name to "
                 << label << endl;

        if (m_bankList[bankItem->getBank()].name != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].name = qstrtostr(label);
            setModified(true);
        }
        
    } else if (deviceItem) {

        // renaming a device item

        RG_DEBUG << "MidiProgramsEditor::slotModifyDeviceorBankName : modify device name to "
                 << label << endl;

        if (m_deviceNameMap[deviceItem->getDevice()] != qstrtostr(label)) {
            m_deviceNameMap[deviceItem->getDevice()] = qstrtostr(label);
            setModified(true);
        }
    }
    
}

void
BankEditorDialog::selectDeviceItem(Rosegarden::MidiDevice *device)
{
    QListViewItem *child = m_listView->firstChild();
    MidiDeviceListViewItem *midiDeviceItem;
    Rosegarden::MidiDevice *midiDevice;

    do
    {
        midiDeviceItem = dynamic_cast<MidiDeviceListViewItem*>(child);

        if (midiDeviceItem)
        {
            midiDevice = getMidiDevice(midiDeviceItem);

            if (midiDevice == device)
            {
                m_listView->setSelected(child, true);
                return;
            }
        }

    }
    while ((child = child->nextSibling()));
}
void
BankEditorDialog::selectDeviceBankItem(Rosegarden::DeviceId deviceId,
				       int bank)
{
    QListViewItem *deviceChild = m_listView->firstChild();
    QListViewItem *bankChild;
    int deviceCount = 0, bankCount = 0;

    do
    {
        bankChild = deviceChild->firstChild();

        MidiDeviceListViewItem *midiDeviceItem =
	    dynamic_cast<MidiDeviceListViewItem*>(deviceChild);

	if (midiDeviceItem && bankChild)
        {
            do
            {
                if (deviceId == midiDeviceItem->getDevice() &
		    bank == bankCount)
                {
                    m_listView->setSelected(bankChild, true);
                    return;
                }
                bankCount++;

            } while ((bankChild = bankChild->nextSibling()));
        }

        deviceCount++;
        bankCount = 0;
    }
    while ((deviceChild = deviceChild->nextSibling()));
}

void
BankEditorDialog::setModified(bool value)
{
    RG_DEBUG << "BankEditorDialog::setModified("
             << value << ")\n";

    if (m_modified == value) return;

    if (value == true)
    {
        enableButtonOK(true);
        enableButtonApply(true);
    }
    else
    {
        enableButtonOK(false);
        enableButtonApply(false);
    }

    m_modified = value;
}

void
BankEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}


MultiViewCommandHistory*
BankEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
BankEditorDialog::slotImport()
{
    QString deviceDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(deviceDir);
    if (!dir.exists()) {
	deviceDir = ":ROSEGARDENDEVICE";
    } else {
	deviceDir = "file://" + deviceDir;
    }

    KURL url = KFileDialog::getOpenURL
	(deviceDir,
	 "*.rgd|Rosegarden device file\n*.rg|Rosegarden file\n*.sf2|SoundFont file",
	 this, i18n("Import Banks from Device in File"));

    if (url.isEmpty()) return;

    QString target;
    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, QString(i18n("Cannot download file %1"))
                           .arg(url.prettyURL()));
        return;
    }

    if (SF2PatchExtractor::isSF2File(target.data())) {
	importFromSF2(target);
	return;
    }

    RosegardenGUIDoc *doc = new RosegardenGUIDoc(this, 0);

    // Add some dummy devices for bank population when we open the document.
    // We guess that the file won't have more than 8 devices.
    //
    for (unsigned int i = 0; i < 8; i++)
    {
        QString label = QString("MIDI Device %1").arg(i + 1);
        doc->getStudio().addDevice(qstrtostr(label),
                                   i,
                                   Rosegarden::Device::Midi);

    }

    if (doc->openDocument(target))
    {
        Rosegarden::DeviceList *list = doc->getStudio().getDevices();
        Rosegarden::DeviceListIterator it = list->begin();

        if (list->size() == 0)
        {
             KMessageBox::sorry(this, i18n("No Devices found in file"));
             delete doc;
             return;
        }
        else
        {
            std::vector<QString> importList;
            int count = 0;
            
            for (; it != list->end(); ++it)
            {
                Rosegarden::MidiDevice *device = 
                    dynamic_cast<Rosegarden::MidiDevice*>(*it);

                if (device)
                {
                    std::vector<Rosegarden::MidiBank> banks =
                        device->getBanks();

                    // We've got a bank on a Device fom this file
                    //
                    if (banks.size())
                    {
                        if (device->getName() == "")
                        {
                            QString deviceNo =
                                QString("Device %1").arg(count++);
                            importList.push_back(deviceNo);
                        }
                        else
                        {
                            importList.push_back(strtoqstr(device->getName()));
                        }
                    }
                }
            }


            // If we have our devices then we offer the selection otherwise
            // we just 
            //
            if (importList.size())
            {
                ImportDeviceDialog *dialog =
                    new ImportDeviceDialog(this, importList);

                int res = dialog->exec();

                // Check for overwrite flag and reset res as necessary
                //
                bool overwrite = false;
                if (res >> 24)
                {
                    overwrite = true;
                    res &= 0xffffff;
                }

                if (res > -1)
                {
                    count = 0;
                    bool found = false;
                    std::vector<Rosegarden::MidiBank> banks;
                    std::vector<Rosegarden::MidiProgram> programs;
                    std::string librarianName, librarianEmail;

                    for (it = list->begin(); it != list->end(); ++it)
                    {
                        Rosegarden::MidiDevice *device = 
                            dynamic_cast<Rosegarden::MidiDevice*>(*it);

                        if (device)
                        {
                            if (count == res)
                            {
                                banks = device->getBanks();
                                programs = device->getPrograms();
                                librarianName = device->getLibrarianName();
                                librarianEmail = device->getLibrarianEmail();
                                found = true;
                                break;
                            }
                            else
                                count++;
                        }

                    }

                    if (found)
                    {
                        MidiDeviceListViewItem* deviceItem =
                            dynamic_cast<MidiDeviceListViewItem*>
                                (m_listView->selectedItem());

                        if (deviceItem)
                        {
                            ModifyDeviceCommand *command =
                                new ModifyDeviceCommand(
                                        m_studio,
                                        deviceItem->getDevice(),
                                        qstrtostr(importList[res]),
                                        librarianName,
                                        librarianEmail,
                                        banks,
                                        programs,
                                        overwrite);
                            addCommandToHistory(command);

			    setModified(true);

                            // Redraw the dialog
                            initDialog();
                        }
                    }
                }
            }
        }
    }

    delete doc;
}

void
BankEditorDialog::importFromSF2(QString filename)
{
    SF2PatchExtractor::Device sf2device;
    try {
	sf2device = SF2PatchExtractor::read(filename.data());

    // These exceptions shouldn't happen -- the isSF2File call before this
    // one should have weeded them out
    } catch (SF2PatchExtractor::FileNotFoundException e) {
	return;
    } catch (SF2PatchExtractor::WrongFileFormatException e) {
	return;
    }

    std::vector<Rosegarden::MidiBank> banks;
    std::vector<Rosegarden::MidiProgram> programs;

    for (SF2PatchExtractor::Device::const_iterator i = sf2device.begin();
	 i != sf2device.end(); ++i) {

	int bankNumber = i->first;
	const SF2PatchExtractor::Bank &sf2bank = i->second;

	Rosegarden::MidiBank bank;
	bank.msb = bankNumber / 128;
	bank.lsb = bankNumber % 128;
	bank.name = qstrtostr(QString("Bank %1:%1").arg(bank.msb).arg(bank.lsb));
	banks.push_back(bank);

	for (SF2PatchExtractor::Bank::const_iterator j = sf2bank.begin();
	     j != sf2bank.end(); ++j) {

	    int programNumber = j->first;
	    Rosegarden::MidiProgram program;
	    program.msb = bank.msb;
	    program.lsb = bank.lsb;
	    program.name = j->second;
	    program.program = programNumber;
	    programs.push_back(program);
	}
    }

    MidiDeviceListViewItem* deviceItem =
	dynamic_cast<MidiDeviceListViewItem*>
	(m_listView->selectedItem());
    
    if (deviceItem)
    {
	Rosegarden::DeviceId deviceId = deviceItem->getDevice();
	Rosegarden::Device *device = m_studio->getDevice(deviceId);

	std::vector<QString> importList;
	importList.push_back(filename);
	ImportDeviceDialog *dialog = new ImportDeviceDialog(this, importList);

	int res = dialog->exec();

	// Check for overwrite flag and reset res as necessary
	//
	bool overwrite = false;
	if (res >> 24) {
	    overwrite = true;
	    res &= 0xffffff;
	}

	if (res > -1) {

	    if (device) {
		ModifyDeviceCommand *command =
		    new ModifyDeviceCommand(
			m_studio,
			deviceId,
			device->getName(),
			"",
			"",
			banks,
			programs,
			overwrite);
		addCommandToHistory(command);
		
		setModified(true);
	
		// Redraw the dialog
		initDialog();
	    }
	}
    }
}


// Store the current bank for copy
//
void
BankEditorDialog::slotCopy()
{
    MidiBankListViewItem* bankItem
        = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem)
    {
        m_copyBank = std::pair<Rosegarden::DeviceId, int>(bankItem->getDevice(),
							  bankItem->getBank());
        m_pastePrograms->setEnabled(true);
    }
}

void
BankEditorDialog::slotPaste()
{
    MidiBankListViewItem* bankItem
        = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem)
    {
        // Get the full program and bank list for the source device
        //
        Rosegarden::MidiDevice *device = getMidiDevice(m_copyBank.first);
        std::vector<Rosegarden::MidiBank> tempBank = device->getBanks();

        MidiProgramsEditor::MidiProgramContainer::iterator it;
        std::vector<Rosegarden::MidiProgram> tempProg;

        // Remove programs that will be overwritten
        //
        for (it = m_programList.begin(); it != m_programList.end(); it++)
        {
            if (it->msb != m_lastMSB || it->lsb != m_lastLSB)
                tempProg.push_back(*it);
        }
        m_programList = tempProg;

        // Now get source list and msb/lsb
        //
        tempProg = device->getPrograms();
        int sourceMSB = tempBank[m_copyBank.second].msb;
        int sourceLSB = tempBank[m_copyBank.second].lsb;

        // Add the new programs
        //
        for (it = tempProg.begin(); it != tempProg.end(); it++)
        {
            if (it->msb == sourceMSB && it->lsb == sourceLSB)
            {
                // Insert with new MSB and LSB
                //
                Rosegarden::MidiProgram copyProgram;
                copyProgram.name = it->name;
                copyProgram.program = it->program;
                copyProgram.msb = m_lastMSB;
                copyProgram.lsb = m_lastLSB;

                m_programList.push_back(copyProgram);
            }
        }

        // Save these for post-apply 
        //
        int devPos = bankItem->getDevice();
        int bankPos = bankItem->getBank();

        slotApply();

        // Select same bank
        //
        selectDeviceBankItem(devPos, bankPos);
    }
}

void
BankEditorDialog::slotExport()
{
    QString extension = "rgd";

    QString name =
        KFileDialog::getSaveFileName(":ROSEGARDEN",
               (extension.isEmpty() ? QString("*") : ("*." + extension)),
               this,
               i18n("Export Device as..."));

    // Check for the existence of the name
    if (name.isEmpty()) return;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty())
    {
        QRegExp rgFile("\\." + extension + "$");
        if (rgFile.match(name) == -1)
        {
            name += "." + extension;
        }
    }

    QFileInfo info(name);

    if (info.isDir())
    {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    if (info.exists())
    {
        int overwrite = KMessageBox::questionYesNo
            (this, i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes) return;

    }


    //std::cout << "GOT FILENAME = " << name << std::endl;
    m_doc->saveDocument(name, "deviceExport");


}


void MidiProgramsEditor::blockAllSignals(bool block)
{
    RG_DEBUG << "MidiProgramsEditor : Blocking all signals = " << block << endl;

    const QObjectList* allChildren = queryList("KLineEdit", "[0-9]+");
    QObjectListIt it(*allChildren);
    QObject *obj;

    while ( (obj = it.current()) != 0 ) {
//         RG_DEBUG << "Blocking signals for " << obj->name()
//                  << ", class " << obj->className() << endl;
        obj->blockSignals(block);
        ++it;
    }

    m_msb->blockSignals(block);
    m_lsb->blockSignals(block);
}

// ------------------------ RemapInstrumentDialog -----------------------
//
RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
                                             RosegardenGUIDoc *doc):
    KDialogBase(parent, "", true, i18n("Remap Instrument assigments..."),
                Ok | Apply | Cancel),
    m_doc(doc)
{
    QVBox *vBox = makeVBoxMainWidget();

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Device or Instrument"),
                                     vBox);

    new QLabel(i18n("Remap Tracks by all Instruments on a Device or by single Instrument"), m_buttonGroup);
    m_deviceButton = new QRadioButton(i18n("Device"), m_buttonGroup);
    m_instrumentButton = new QRadioButton(i18n("Instrument"), m_buttonGroup);


    connect(m_buttonGroup, SIGNAL(released(int)),
            this, SLOT(slotRemapReleased(int)));

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Choose Source and Destination"),
                                        vBox);

    new QLabel(i18n("From"), groupBox);
    new QLabel(i18n("To"), groupBox);
    m_fromCombo = new RosegardenComboBox(false, groupBox);
    m_toCombo = new RosegardenComboBox(false, groupBox);

    /*
    QGridLayout *gridLayout = new QGridLayout(frame,
                                              2,  // rows
                                              2,  // cols
                                              2); // margin
    m_fromCombo = new RosegardenComboBox(true, frame);
    m_toCombo = new RosegardenComboBox(true, frame);

    gridLayout->addWidget(new QLabel(i18n("From:"), frame), 0, 0, AlignLeft);
    gridLayout->addWidget(new QLabel(i18n("To:"), frame), 0, 1, AlignLeft);
    gridLayout->addWidget(m_fromCombo, 1, 0, AlignLeft);
    gridLayout->addWidget(m_toCombo, 1, 1, AlignLeft);
    */

    m_buttonGroup->setButton(0);
    populateCombo(0);
}

void
RemapInstrumentDialog::populateCombo(int id)
{
    m_fromCombo->clear();
    m_toCombo->clear();
    Rosegarden::Studio *studio = &m_doc->getStudio();

    if (id == 0)
    {
        Rosegarden::DeviceList *devices = studio->getDevices();
        Rosegarden::DeviceListIterator it;

        for (it = devices->begin(); it != devices->end(); it++)
        {
            m_fromCombo->insertItem(strtoqstr((*it)->getName()));
            m_toCombo->insertItem(strtoqstr((*it)->getName()));
        }

        if (devices->size() == 0)
        {
            m_fromCombo->insertItem(i18n("<no devices>"));
            m_toCombo->insertItem(i18n("<no devices>"));
        }
    }
    else 
    {
        //Rosegarden::DeviceList *devices = studio->getDevices();
        Rosegarden::InstrumentList list = studio->getPresentationInstruments();
        Rosegarden::InstrumentList::iterator it = list.begin();

        for (; it != list.end(); it++)
        {
            m_fromCombo->insertItem(strtoqstr((*it)->getName()));
            m_toCombo->insertItem(strtoqstr((*it)->getName()));
        }
    }

    /*
    for (int i = 0; m_fromCombo->count(); i++)
    {
    }
    */

}


void
RemapInstrumentDialog::slotRemapReleased(int id)
{
    populateCombo(id);
}

void
RemapInstrumentDialog::slotOk()
{
    slotApply();
    accept();
}

void
RemapInstrumentDialog::slotApply()
{
    if (m_buttonGroup->id(m_buttonGroup->selected()) == 0) // devices
    {
        ModifyDeviceMappingCommand *command =
            new ModifyDeviceMappingCommand(m_doc,
                                           m_fromCombo->currentItem(),
                                           m_toCombo->currentItem());
        addCommandToHistory(command);
    }
    else // instruments
    {
        ModifyInstrumentMappingCommand *command =
            new ModifyInstrumentMappingCommand(m_doc,
                                               m_fromCombo->currentItem(),
                                               m_toCombo->currentItem());

        addCommandToHistory(command);
    }

    emit applyClicked();
}

void
RemapInstrumentDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}


MultiViewCommandHistory*
RemapInstrumentDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


// ------------------- ImportDeviceDialog --------------------
//
ImportDeviceDialog::ImportDeviceDialog(QWidget *parent,
                                       std::vector<QString> devices):
    KDialogBase(parent, "importdevicedialog", true,
                i18n("Import Banks from Device..."),
                Ok | Cancel, Ok, true)
{
    QVBox* mainFrame = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Source device"),
                                        mainFrame);

    if (devices.size() > 1) {

	m_deviceCombo = new RosegardenComboBox(groupBox);

	// Create the combo
	//
	std::vector<QString>::iterator it = devices.begin();
	for (; it != devices.end(); it++)
	    m_deviceCombo->insertItem(*it);

	m_label = 0;

    } else {
	
	m_deviceCombo = 0;
	m_label = new QLabel(devices[0], groupBox);
    }

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Import behaviour"),
                                     mainFrame);
    m_mergeBanks = new QRadioButton(i18n("Merge Banks"), m_buttonGroup);
    m_overwriteBanks =
        new QRadioButton(i18n("Overwrite Banks"), m_buttonGroup);

    KConfig *config = kapp->config();
    config->setGroup("General Options");
    bool overwrite = config->readBoolEntry("importbanksoverwrite", false);
    if (overwrite) m_buttonGroup->setButton(1);
    else m_buttonGroup->setButton(0);
}

void
ImportDeviceDialog::slotOk()
{
    int v = m_buttonGroup->id(m_buttonGroup->selected());
    KConfig *config = kapp->config();
    config->setGroup("General Options");
    config->writeEntry("importbanksoverwrite", v == 1);
    v <<= 24;
    if (m_deviceCombo) v += m_deviceCombo->currentItem();
    done (v);
}

void
ImportDeviceDialog::slotCancel()
{
    done(-1);
}


