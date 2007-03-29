
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_LYRICEDITDIALOG_H_
#define _RG_LYRICEDITDIALOG_H_

#include <kdialogbase.h>
#include <qstring.h>
#include <vector>


class QWidget;
class QTextEdit;
class QComboBox;
class QPushButton;


namespace Rosegarden
{

class Segment;


class LyricEditDialog : public KDialogBase
{
    Q_OBJECT

public:
    LyricEditDialog(QWidget *parent, Segment *segment);

    int getVerseCount() const;
    QString getLyricData(int verse) const;

protected slots:
    void slotVerseNumberChanged(int);
    void slotAddVerse();

protected:
    Segment *m_segment;

    int m_currentVerse;
    QComboBox *m_verseNumber;
    QTextEdit *m_textEdit;
    QPushButton *m_verseAddButton;

    int m_verseCount;
    std::vector<QString> m_texts;
    QString m_skeleton;

    void countVerses();
    void unparse();
};

}

#endif
