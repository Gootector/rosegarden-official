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

#ifndef _RG_PLUGINCONTROL_H_
#define _RG_PLUGINCONTROL_H_

#include <QObject>


class QWidget;
class QGridLayout;


namespace Rosegarden
{

class Rotary;
class PluginPort;
class AudioPluginManager;
class Studio;

class PluginControl : public QObject
{
    Q_OBJECT
public:

    typedef enum
    {
        Rotary,
        Slider,
        NumericSlider
    } ControlType;

	//### This is just garbage

    PluginControl(QWidget *parent,
                  QGridLayout *layout,
		  int row,
		  int col,
                  ControlType type,
                  PluginPort *port,
                  AudioPluginManager *pluginManager,
                  int index,
                  float initialValue,
                  bool showBounds,
                  bool hidden);
 
    void setValue(float value, bool emitSignals = true);
    float getValue() const;

    int getIndex() const { return m_index; }

    void show();
    void hide();

public slots:
    void slotValueChanged(float value);

signals:
    void valueChanged(float value);

protected:

    //--------------- Data members ---------------------------------

    QGridLayout         *m_layout;

	int m_row;
	int m_col;

    ControlType          m_type;
    PluginPort          *m_port;

    ::Rosegarden::Rotary              *m_dial; // we have to specify the namespace here otherwise gcc 4.1 thinks it's the enum value above
    AudioPluginManager  *m_pluginManager;

    int                  m_index;

};


}

#endif
