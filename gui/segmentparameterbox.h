// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <vector>

#include <qframe.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>

#include "Quantizer.h"


// Provides a mechanism for viewing and modifying the parameters
// associated with a Rosegarden Segment.
//
//

namespace Rosegarden { class Segment; }

#ifndef _SEGMENTPARAMETERBOX_H_
#define _SEGMENTPARAMETERBOX_H_

// Create out own check box which is always Tristate 
// and allows us to click only between on and off
// and only to _show_ the third ("Some") state 
//
//
class RosegardenTristateCheckBox : public QCheckBox
{
public:
    RosegardenTristateCheckBox(QWidget *parent=0,
                               const char *name=0):QCheckBox(parent, name)
        { setTristate(true) ;}

    virtual ~RosegardenTristateCheckBox() {;}

protected:
    // don't emit when the button is released
    virtual void mouseReleaseEvent(QMouseEvent *) {;}

private:
};

// Turn a normal QComboBox into one that accepts mouse wheel
// events to change the value
//
class RosegardenComboBox : public QComboBox
{
public:
    RosegardenComboBox(QWidget *parent=0, const char *name=0):
        QComboBox(parent, name) {;}

    RosegardenComboBox(bool rw, QWidget *parent=0, const char *name=0):
        QComboBox(rw, parent, name) {;}


protected:
    virtual void wheelEvent(QWheelEvent *e)
    {  e->accept();
       
        if (e->delta() < 0)
        {
            if (currentItem() < count() - 1)
                setCurrentItem(currentItem() + 1);
        }
        else
        {
            if (currentItem() > 0)
                setCurrentItem(currentItem() - 1);
        }
    }

private:

};



class SegmentParameterBox : public QFrame
{
Q_OBJECT

public:

    typedef enum
    {
        None,
        Some,
        All
    } Tristate;

    SegmentParameterBox(QWidget *parent=0, const char *name=0, WFlags f=0);
    ~SegmentParameterBox();

    // Use Segments to update GUI parameters
    //
    void useSegment(Rosegarden::Segment *segment);
    void useSegments(std::vector<Rosegarden::Segment*> segments);

public slots:

    void repeatPressed();

private:
    void initBox();
    void populateBoxFromSegments();

    RosegardenTristateCheckBox *m_repeatValue;
    RosegardenComboBox         *m_quantizeValue;
    RosegardenComboBox         *m_transposeValue;
    RosegardenComboBox         *m_delayValue;

    std::vector<Rosegarden::Segment*> m_segments;

    std::vector<Rosegarden::StandardQuantization>
    m_standardQuantizations;
};


#endif // _SEGMENTPARAMETERBOX_H_
