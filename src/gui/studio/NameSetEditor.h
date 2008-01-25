
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NAMESETEDITOR_H_
#define _RG_NAMESETEDITOR_H_

#include <kcompletion.h>
#include <qstring.h>
#include <qvgroupbox.h>
#include <vector>


class QWidget;
class QPushButton;
class QListViewItem;
class QLabel;
class QGridLayout;
class QFrame;
class KLineEdit;


namespace Rosegarden
{

class BankEditorDialog;


class NameSetEditor : public QVGroupBox
{
    Q_OBJECT
public:
    virtual void clearAll() = 0;

    virtual void populate(QListViewItem *) = 0;
    virtual void reset() = 0;

public slots:
    virtual void slotNameChanged(const QString&) = 0;
    virtual void slotEntryButtonPressed() = 0;
    void slotToggleInitialLabel();

protected:
    NameSetEditor(BankEditorDialog *bankEditor,
                  QString title,
                  QWidget *parent,
                  const char *name,
                  QString headingPrefix = "",
                  bool showEntryButtons = false);

    QPushButton *getEntryButton(int n) { return m_entryButtons[n]; }
    const QPushButton *getEntryButton(int n) const { return m_entryButtons[n]; }

    QGridLayout             *m_mainLayout;
    BankEditorDialog*        m_bankEditor;
    KCompletion              m_completion;
    QPushButton             *m_initialLabel;
    std::vector<QLabel*>     m_labels;
    std::vector<KLineEdit*>  m_names;
    QFrame                  *m_mainFrame;
    QLabel                  *m_librarian;
    QLabel                  *m_librarianEmail;
    std::vector<QPushButton *> m_entryButtons;
};


}

#endif
