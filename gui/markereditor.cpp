// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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


#include <kstdaction.h>
#include <kaction.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qaccel.h>

#include "markereditor.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "editcommands.h"
#include "widgets.h"

MarkerEditorDialog::MarkerEditorDialog(QWidget *parent,
                                         RosegardenGUIDoc *doc):
    KMainWindow(parent, "markereditordialog"),
    m_doc(doc),
    m_modified(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Markers"));

    m_listView = new KListView(mainFrame);
    m_listView->addColumn(i18n("Marker time  "));
    m_listView->addColumn(i18n("Marker name  "));
    m_listView->addColumn(i18n("Marker description "));

    // Align centrally
    for (int i = 0; i < 3; ++i)
        m_listView->setColumnAlignment(i, Qt::AlignHCenter);

    QGroupBox *posGroup = new QGroupBox(2, Horizontal,
                                        i18n("Pointer position"), mainFrame);
    
    new QLabel(i18n("Absolute time:"), posGroup);
    m_absoluteTime = new QLabel(posGroup);

    new QLabel(i18n("Real time:"), posGroup);
    m_realTime = new QLabel(posGroup);

    new QLabel(i18n("In bar:"), posGroup);
    m_barTime = new QLabel(posGroup);

    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);
    m_deleteAllButton = new QPushButton(i18n("Delete All"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    QToolTip::add(m_addButton,
                  i18n("Add a Marker"));

    QToolTip::add(m_deleteButton,
                  i18n("Delete a Marker"));

    QToolTip::add(m_deleteAllButton,
                  i18n("Delete All Markers"));

    QToolTip::add(m_closeButton,
                  i18n("Close the Marker Editor"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_deleteAllButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_deleteAllButton, SIGNAL(released()),
            SLOT(slotDeleteAll()));

    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QListViewItem *)),
            SLOT(slotEdit(QListViewItem *)));

    connect(m_listView, SIGNAL(pressed(QListViewItem *)),
            this, SLOT(slotItemClicked(QListViewItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
    m_listView->setSelectionMode(QListView::Extended);
    m_listView->setItemsRenameable(true);

    initDialog();

    setAutoSaveSettings(MarkerEditorConfigGroup, true);

    m_accelerators = new QAccel(this);
}

void
MarkerEditorDialog::updatePosition()
{
    Rosegarden::timeT pos = m_doc->getComposition().getPosition();
    m_absoluteTime->setText(QString("%1").arg(pos));

    Rosegarden::RealTime rT = m_doc->getComposition().getElapsedRealTime(pos);
    long hours = rT.sec / (60 * 60);
    long mins = rT.sec / 60;
    long secs = rT.sec;
    long msecs = rT.usec / 1000;

    QString realTime, secsStr;
    if (hours) realTime += QString("%1h ").arg(hours);
    if (mins) realTime += QString("%1m ").arg(hours);
    secsStr.sprintf("%ld.%03lds", secs, msecs);
    realTime += secsStr;

    // only update if we need to to try and avoid flickering
    if (m_realTime->text() != realTime) m_realTime->setText(realTime);

    QString barTime = 
        QString("%1").arg(m_doc->getComposition().getBarNumber(pos) + 1);

    // again only update if needed
    if (m_barTime->text() != barTime) m_barTime->setText(barTime);

    /*
    // Don't allow us to add another marker if there's already one
    // at the current position.
    //
    if (m_doc->getComposition().
            isMarkerAtPosition(m_doc->getComposition().getPosition()))
        m_addButton->setEnabled(false);
    else
        m_addButton->setEnabled(true);
        */
}



MarkerEditorDialog::~MarkerEditorDialog()
{
    RG_DEBUG << "MarkerEditorDialog::~MarkerEditorDialog" << endl;

    m_listView->saveLayout(kapp->config(), MarkerEditorConfigGroup);

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
}

void 
MarkerEditorDialog::initDialog()
{
    RG_DEBUG << "MarkerEditorDialog::initDialog" << endl;
    slotUpdate();
}

void
MarkerEditorDialog::slotUpdate()
{
    RG_DEBUG << "MarkerEditorDialog::slotUpdate" << endl;

    //QPtrList<QListViewItem> selection = m_listView->selectedItems();

    QListViewItem *item;

    m_listView->clear();

    Rosegarden::Composition::markercontainer markers =
       m_doc->getComposition().getMarkers();

    Rosegarden::Composition::markerconstiterator it;

    for (it = markers.begin(); it != markers.end(); ++it)
    {
        item = new QListViewItem(m_listView,
                                 QString("%1").arg((*it)->getTime()),
                                 strtoqstr((*it)->getName()),
                                 strtoqstr((*it)->getDescription()));

        m_listView->insertItem(item);
    }

    if (m_listView->childCount() == 0)
    {
        QListViewItem *item = new QListViewItem(m_listView,
                                                i18n("<none>"));
        m_listView->insertItem(item);

        m_listView->setSelectionMode(QListView::NoSelection);
    }
    else
    {
        m_listView->setSelectionMode(QListView::Extended);
    }

    updatePosition();

}

void 
MarkerEditorDialog::slotDeleteAll()
{
    RG_DEBUG << "MarkerEditorDialog::slotDeleteAll" << endl;
    KMacroCommand *command = new KMacroCommand(i18n("Remove all markers"));

    QListViewItem *item = m_listView->firstChild();

    do
    {
        RemoveMarkerCommand *rc = 
            new RemoveMarkerCommand(&m_doc->getComposition(),
                                    item->text(0).toInt(),
                                    qstrtostr(item->text(1)),
                                    qstrtostr(item->text(2)));
        command->addCommand(rc);
    }
    while((item = item->nextSibling()));

    addCommandToHistory(command);
}

void
MarkerEditorDialog::slotAdd()
{
    RG_DEBUG << "MarkerEditorDialog::slotAdd" << endl;

    AddMarkerCommand *command =
        new AddMarkerCommand(&m_doc->getComposition(),
                             m_doc->getComposition().getPosition(),
                             std::string("new marker"),
                             std::string("no description"));

    addCommandToHistory(command);
}


void
MarkerEditorDialog::slotDelete()
{
    RG_DEBUG << "MarkerEditorDialog::slotDelete" << endl;
    QListViewItem *item = m_listView->currentItem();

    if (!item) return;

    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&m_doc->getComposition(),
                                item->text(0).toInt(),
                                qstrtostr(item->text(1)),
                                qstrtostr(item->text(2)));

    addCommandToHistory(command);
                                             
}

void
MarkerEditorDialog::slotClose()
{
    RG_DEBUG << "MarkerEditorDialog::slotClose" << endl;

    if (m_doc) m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
MarkerEditorDialog::setupActions()
{
    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    // some adjustments
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    createGUI("markereditor.rc");
}

void 
MarkerEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory* 
MarkerEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


void
MarkerEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "MarkerEditorDialog::setModified(" << modified << ")" << endl;

    if (modified)
    {
    }
    else
    {
    }

    m_modified = modified;
}

void
MarkerEditorDialog::checkModified()
{
    RG_DEBUG << "MarkerEditorDialog::checkModified(" << m_modified << ")" 
             << endl;

}

void
MarkerEditorDialog::slotEdit(QListViewItem *i)
{
    RG_DEBUG << "MarkerEditorDialog::slotEdit" << endl;

    MarkerModifyDialog *dialog = 
        new MarkerModifyDialog(this, i->text(0).toInt(),
                               i->text(1), i->text(2));

    if (dialog->exec() == QDialog::Accepted)
    {
        ModifyMarkerCommand *command =
            new ModifyMarkerCommand(&m_doc->getComposition(),
                                    dialog->getOriginalTime(),
                                    dialog->getTime(),
                                    qstrtostr(dialog->getName()),
                                    qstrtostr(dialog->getDescription()));

        addCommandToHistory(command);
    }


}

void
MarkerEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

// Reset the document
//
void
MarkerEditorDialog::setDocument(RosegardenGUIDoc *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}


MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       int time,
                                       const QString &name,
                                       const QString &des):
    KDialogBase(parent, 0, true, i18n("Edit Marker"), Ok | Cancel),
    m_originalTime(time)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
        (1, Horizontal, i18n("Marker Properties"), vbox);

    QFrame *frame = new QFrame(groupBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Absolute Time:"), frame), 0, 0);
    m_timeEdit = new QSpinBox(frame);
    layout->addWidget(m_timeEdit, 0, 1);

    m_timeEdit->setMinValue(INT_MIN);
    m_timeEdit->setMaxValue(INT_MAX);
    m_timeEdit->setLineStep(
            Rosegarden::Note(Rosegarden::Note::Shortest).getDuration());
    m_timeEdit->setValue(time);

    layout->addWidget(new QLabel(i18n("Name:"), frame), 1, 0);
    m_nameEdit = new QLineEdit(name, frame);
    layout->addWidget(m_nameEdit, 1, 1);

    layout->addWidget(new QLabel(i18n("Description:"), frame), 2, 0);
    m_desEdit = new QLineEdit(des, frame);
    layout->addWidget(m_desEdit, 2, 1);
}

void
MarkerEditorDialog::slotItemClicked(QListViewItem *item)
{
    RG_DEBUG << "MarkerEditorDialog::slotItemClicked" << endl;

    if (item)
    {
        RG_DEBUG << "MarkerEditorDialog::slotItemClicked - "
                 << "jump to marker at " << item->text(0).toInt() << endl;

        emit jumpToMarker(Rosegarden::timeT(item->text(0).toInt()));
    }
}

const char* const MarkerEditorDialog::MarkerEditorConfigGroup = "Marker Editor";
