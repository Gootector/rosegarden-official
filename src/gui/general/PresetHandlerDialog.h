
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PRESETHANDLERDIALOG_H_
#define _RG_PRESETHANDLERDIALOG_H_

#include <kdialogbase.h>
#include <qradiobutton.h>
#include <qstring.h>
#include "CategoryElement.h"

class QWidget;
class KConfig;
class KComboBox;


namespace Rosegarden
{

class PresetGroup;


class PresetHandlerDialog : public KDialogBase
{
    Q_OBJECT

public:

    PresetHandlerDialog(QWidget* parent, bool fromNotation = false);
    ~PresetHandlerDialog();

    PresetGroup *m_presets;
    CategoriesContainer m_categories;

    KConfig *m_config;
    bool m_fromNotation;

    //-------[ accessor functions ]------------------------

    QString getName();

    int getClef();
    int getTranspose();
    int getLowRange();
    int getHighRange();
    bool getConvertAllSegments();
    bool getConvertOnlySelectedSegments();

protected:

    //--------[ member functions ]-------------------------
    
    // initialize the dialog
    void initDialog();

    // populate the category combo
    void populateCategoryCombo();


    //---------[ data members ]-----------------------------

    KComboBox   *m_categoryCombo;
    KComboBox   *m_instrumentCombo;
    KComboBox   *m_playerCombo;
    QRadioButton *m_convertSegments;
    QRadioButton *m_convertAllSegments;

protected slots:

    // de-populate and re-populate the Instrument combo when the category
    // changes.
    void slotCategoryIndexChanged(int index);

    // write out settings to kconfig data for next time and call accept()
    void slotOk();

}; // PresetHandlerDialog


}

#endif
