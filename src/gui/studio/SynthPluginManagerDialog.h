
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

#ifndef _RG_SYNTHPLUGINMANAGERDIALOG_H_
#define _RG_SYNTHPLUGINMANAGERDIALOG_H_

#include "base/MidiProgram.h"
#include "gui/general/ActionFileClient.h"
#include <QMainWindow>
#include <vector>


class QWidget;
class QPushButton;
class QCloseEvent;
class QComboBox;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;
class AudioPluginOSCGUIManager;
class AudioPluginManager;


class SynthPluginManagerDialog : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    SynthPluginManagerDialog(QWidget *parent,
                             RosegardenGUIDoc *doc
#ifdef HAVE_LIBLO
                             , AudioPluginOSCGUIManager *guiManager
#endif
        );

    virtual ~SynthPluginManagerDialog();

    void updatePlugin(InstrumentId id, int plugin);

signals:
    void closing();
    void pluginSelected(InstrumentId, int pluginIndex, int plugin);
    void showPluginDialog(QWidget *, InstrumentId, int pluginIndex);
    void showPluginGUI(InstrumentId, int pluginIndex);

protected slots:
    void slotClose();
    void slotPluginChanged(int index);
    void slotControlsButtonClicked();
    void slotGUIButtonClicked();

protected:
    virtual void closeEvent(QCloseEvent *);

protected:
    RosegardenGUIDoc *m_document;
    Studio *m_studio;
    AudioPluginManager *m_pluginManager;
    std::vector<int> m_synthPlugins;
    std::vector<QComboBox *> m_synthCombos;
    std::vector<QPushButton *> m_controlsButtons;
    std::vector<QPushButton *> m_guiButtons;

#ifdef HAVE_LIBLO
    AudioPluginOSCGUIManager *m_guiManager;
#endif
};



}

#endif
