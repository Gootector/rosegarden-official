
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

#ifndef _RG_DEVICEMANAGERDIALOG_H_
#define _RG_DEVICEMANAGERDIALOG_H_

#include "base/Device.h"
#include <kmainwindow.h>
#include <qstring.h>
#include <qstringlist.h>
#include <vector>


class QWidget;
class QTable;
class QPushButton;
class QCloseEvent;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;
class MidiDevice;


class DeviceManagerDialog : public KMainWindow
{
    Q_OBJECT
public:
    DeviceManagerDialog(QWidget *parent, RosegardenGUIDoc *document);
    ~DeviceManagerDialog();

    void setModified(bool value);

signals:
    void deviceNamesChanged();

    void editBanks(DeviceId);
    void editControllers(DeviceId);

    void closing();

protected slots:
    void slotClose();
    void slotAddPlayDevice();
    void slotAddRecordDevice();
    void slotDeletePlayDevice();
    void slotDeleteRecordDevice();
    void slotPlayValueChanged(int row, int col);
    void slotRecordValueChanged(int row, int col);
    void slotPlayDeviceSelected(int row, int col);
    void slotRecordDeviceSelected(int row, int col);

    // for play devices only:
    void slotImport();
    void slotExport();
    void slotSetBanks();
    void slotSetControllers();

    void slotDevicesResyncd();
    void populate();

protected:
    virtual void closeEvent(QCloseEvent *);

private:
    RosegardenGUIDoc *m_document;
    Studio *m_studio;

    QPushButton *m_deletePlayButton;
    QPushButton *m_deleteRecordButton;
    QPushButton *m_importButton;
    QPushButton *m_exportButton;
    QPushButton *m_banksButton;
    QPushButton *m_controllersButton;

    QStringList m_playConnections;
    QStringList m_recordConnections;
    void makeConnectionList(unsigned int direction, QStringList &list);
    
    QTable *m_playTable;
    QTable *m_recordTable;

    typedef std::vector<MidiDevice *> MidiDeviceList;
    MidiDeviceList m_playDevices;
    MidiDeviceList m_recordDevices;

    DeviceId getPlayDeviceIdAt(int row); // NO_DEVICE = not found
    DeviceId getRecordDeviceIdAt(int row); // NO_DEVICE = not found

    QString m_noConnectionString;
};


}

#endif
