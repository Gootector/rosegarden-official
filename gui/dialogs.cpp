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

#include <qlineedit.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qobjectlist.h>
#include <qgrid.h>
#include <qbitmap.h>
#include <qspinbox.h>
#include <qvalidator.h>
#include <qvbuttongroup.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qtextedit.h>
#include <qaccel.h>

#include <klocale.h>
#include <karrowbutton.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kcombobox.h>

#include "RealTime.h"
#include "BaseProperties.h"
#include "MidiTypes.h"

#include "dialogs.h"
#include "rgapplication.h"
#include "notepixmapfactory.h"
#include "notationview.h"
#include "rosestrings.h"
#include "notationstrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "segmentcommands.h"
#include "notationcommands.h"
#include "widgets.h"
#include "midipitchlabel.h"
#include "studiocontrol.h"
#include "colours.h"
#include "rosegardendcop.h"
#include "instrumentparameterbox.h"
#include "sequencemanager.h" // for metronomeChanged()

#include "rosedebug.h"

using Rosegarden::Int;
using Rosegarden::RealTimeT;
using Rosegarden::Bool;
using Rosegarden::String;

using Rosegarden::TimeSignature;
using Rosegarden::Note;
using Rosegarden::Text;
using Rosegarden::Segment;
using Rosegarden::timeT;
using Rosegarden::Quantizer;
using Rosegarden::Event;
using Rosegarden::EventSelection;


class BigArrowButton : public KArrowButton
{
public:
    BigArrowButton(QWidget *parent = 0, Qt::ArrowType arrow = Qt::UpArrow,
		   const char *name = 0) :
	KArrowButton(parent, arrow, name) { }
    virtual ~BigArrowButton() { } 

    virtual QSize sizeHint() const {
	return QSize(20, 20);
    }
};
    

TimeSignatureDialog::TimeSignatureDialog(QWidget *parent,
					 Rosegarden::TimeSignature sig,
					 int barNo, bool atStartOfBar,
					 QString explanatoryText) :
    KDialogBase(parent, 0, true, i18n("Time Signature"), Ok | Cancel),
    m_timeSignature(sig),
    m_explanatoryLabel(0),
    m_commonTimeButton(0),
    m_hideSignatureButton(0),
    m_normalizeRestsButton(0),
    m_asGivenButton(0),
    m_startOfBarButton(0),
    m_barNo(barNo),
    m_atStartOfBar(atStartOfBar)
{
    static QFont *timeSigFont = 0;

    if (timeSigFont == 0) {
	timeSigFont = new QFont("new century schoolbook", 8, QFont::Bold);
	timeSigFont->setPixelSize(20);
    }

    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
	(1, Horizontal, i18n("Time signature"), vbox);
    QHBox *numBox = new QHBox(groupBox);
    QHBox *denomBox = new QHBox(groupBox);

    QLabel *explanatoryLabel = 0;
    if (explanatoryText) {
	explanatoryLabel = new QLabel(explanatoryText, groupBox);
    }

    BigArrowButton *numDown   = new BigArrowButton(numBox, Qt::LeftArrow);
    BigArrowButton *denomDown = new BigArrowButton(denomBox, Qt::LeftArrow);

    m_numLabel   = new QLabel
	(QString("%1").arg(m_timeSignature.getNumerator()), numBox);
    m_denomLabel = new QLabel
	(QString("%1").arg(m_timeSignature.getDenominator()), denomBox);

    m_numLabel->setAlignment(AlignHCenter | AlignVCenter);
    m_denomLabel->setAlignment(AlignHCenter | AlignVCenter);

    m_numLabel->setFont(*timeSigFont);
    m_denomLabel->setFont(*timeSigFont);

    BigArrowButton *numUp     = new BigArrowButton(numBox, Qt::RightArrow);
    BigArrowButton *denomUp   = new BigArrowButton(denomBox, Qt::RightArrow);

    QObject::connect(numDown,   SIGNAL(clicked()), this, SLOT(slotNumDown()));
    QObject::connect(numUp,     SIGNAL(clicked()), this, SLOT(slotNumUp()));
    QObject::connect(denomDown, SIGNAL(clicked()), this, SLOT(slotDenomDown()));
    QObject::connect(denomUp,   SIGNAL(clicked()), this, SLOT(slotDenomUp()));

    groupBox = new QButtonGroup(1, Horizontal, i18n("Scope"), vbox);

    if (!m_atStartOfBar) {

	// let's forget about start-of-composition for the moment
	QString scopeText;
	if (m_barNo != 0 || !m_atStartOfBar) {
	    if (m_atStartOfBar) {
		scopeText = QString
		    (i18n("Insertion point is at start of bar %1."))
		    .arg(m_barNo + 1);
	    } else {
		scopeText = QString
		    (i18n("Insertion point is in the middle of bar %1."))
		    .arg(m_barNo + 1);
	    }
	} else {
	    scopeText = QString
		(i18n("Insertion point is at start of composition."));
	}
	
	new QLabel(scopeText, groupBox);
	m_asGivenButton = new QRadioButton
	    (i18n("Start bar %1 here").arg(m_barNo + 2), groupBox);

	if (!m_atStartOfBar) {
	    m_startOfBarButton = new QRadioButton
		(i18n("Change time from start of bar %1")
		 .arg(m_barNo + 1), groupBox);
	    m_startOfBarButton->setChecked(true);
	} else {
	    m_asGivenButton->setChecked(true);
	}
    } else {
	new QLabel(i18n("Time change will take effect at the start of bar %1.")
		   .arg(barNo + 1), groupBox);
    }

    groupBox = new QGroupBox(1, Horizontal, i18n("Options"), vbox);
    m_hideSignatureButton = new QCheckBox
	(i18n("Make the new time signature hidden"), groupBox);
    m_hideSignatureButton->setChecked(false);
    m_commonTimeButton = new QCheckBox
	(i18n("Show as common time"), groupBox);
    m_commonTimeButton->setChecked(true);
    m_normalizeRestsButton = new QCheckBox
	(i18n("Normalize subsequent rests"), groupBox);
    m_normalizeRestsButton->setChecked(true);
    QObject::connect(m_hideSignatureButton, SIGNAL(clicked()), this,
		     SLOT(slotUpdateCommonTimeButton()));
    slotUpdateCommonTimeButton();
    m_explanatoryLabel = explanatoryLabel;
}

Rosegarden::TimeSignature
TimeSignatureDialog::getTimeSignature() const
{
    TimeSignature ts(m_timeSignature.getNumerator(),
		     m_timeSignature.getDenominator(),
		     (m_commonTimeButton &&
		      m_commonTimeButton->isEnabled() &&
		      m_commonTimeButton->isChecked()),
		     (m_hideSignatureButton &&
		      m_hideSignatureButton->isEnabled() &&
		      m_hideSignatureButton->isChecked()));
    return ts;
}


void
TimeSignatureDialog::slotNumDown()
{
    int n = m_timeSignature.getNumerator();
    if (--n >= 1) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotNumUp()
{
    int n = m_timeSignature.getNumerator();
    if (++n <= 99) {
	m_timeSignature = TimeSignature(n, m_timeSignature.getDenominator());
	m_numLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomDown()
{
    int n = m_timeSignature.getDenominator();
    if ((n /= 2) >= 1) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotDenomUp()
{
    int n = m_timeSignature.getDenominator();
    if ((n *= 2) <= 64) {
	m_timeSignature = TimeSignature(m_timeSignature.getNumerator(), n);
	m_denomLabel->setText(QString("%1").arg(n));
    }
    slotUpdateCommonTimeButton();
}

void
TimeSignatureDialog::slotUpdateCommonTimeButton()
{
    if (m_explanatoryLabel) m_explanatoryLabel->hide();
    if (!m_hideSignatureButton || !m_hideSignatureButton->isChecked()) {
	if (m_timeSignature.getDenominator() == m_timeSignature.getNumerator()) {
	    if (m_timeSignature.getNumerator() == 4) {
		m_commonTimeButton->setText(i18n("Display as common time"));
		m_commonTimeButton->setEnabled(true);
		return;
	    } else if (m_timeSignature.getNumerator() == 2) {
		m_commonTimeButton->setText(i18n("Display as cut common time"));
		m_commonTimeButton->setEnabled(true);
		return;
	    }
	}
    }
    m_commonTimeButton->setEnabled(false);
}


TimeSignatureDialog::Location
TimeSignatureDialog::getLocation() const
{
    if (m_asGivenButton && m_asGivenButton->isChecked()) {
	return AsGiven;
    } else if (m_startOfBarButton && m_startOfBarButton->isChecked()) {
	return StartOfBar;
    }
    return AsGiven;
}

bool
TimeSignatureDialog::shouldNormalizeRests() const
{
    return (m_normalizeRestsButton && m_normalizeRestsButton->isEnabled() &&
	    m_normalizeRestsButton->isChecked());
}


KeySignatureDialog::KeySignatureDialog(QWidget *parent,
				       NotePixmapFactory *npf,
				       Rosegarden::Clef clef,
				       Rosegarden::Key defaultKey,
				       bool showApplyToAll,
				       bool showConversionOptions,
				       QString explanatoryText) :
    KDialogBase(parent, 0, true, i18n("Key Change"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_key(defaultKey),
    m_clef(clef),
    m_valid(true),
    m_ignoreComboChanges(false),
    m_explanatoryLabel(0)
{
    QVBox *vbox = makeVBoxMainWidget();

    QHBox *keyBox = 0;
    QHBox *nameBox = 0;

    QGroupBox *keyFrame = new QGroupBox
	(1, Horizontal, i18n("Key signature"), vbox);

    QGroupBox *buttonFrame = new QButtonGroup
	(1, Horizontal, i18n("Scope"), vbox);
    
    QButtonGroup *conversionFrame = new QButtonGroup
	(1, Horizontal, i18n("Existing notes following key change"), vbox);
	
    keyBox = new QHBox(keyFrame);
    nameBox = new QHBox(keyFrame);

    QLabel *explanatoryLabel = 0;
    if (explanatoryText) {
	explanatoryLabel = new QLabel(explanatoryText, keyFrame);
    }
    
    BigArrowButton *keyDown = new BigArrowButton(keyBox, Qt::LeftArrow);
    QToolTip::add(keyDown, i18n("Flatten"));

    m_keyLabel = new QLabel(i18n("Key"), keyBox);
    m_keyLabel->setAlignment(AlignVCenter | AlignHCenter);

    BigArrowButton *keyUp = new BigArrowButton(keyBox, Qt::RightArrow);
    QToolTip::add(keyUp, i18n("Sharpen"));

    m_keyCombo = new RosegardenComboBox(true, nameBox);
    m_majorMinorCombo = new KComboBox(nameBox);
    m_majorMinorCombo->insertItem("Major");
    m_majorMinorCombo->insertItem("Minor");
    if (m_key.isMinor()) {
	m_majorMinorCombo->setCurrentItem(m_majorMinorCombo->count() - 1);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
    m_explanatoryLabel = explanatoryLabel;

    m_keyLabel->setMinimumWidth(m_keyLabel->pixmap()->width());
    m_keyLabel->setMinimumHeight(m_keyLabel->pixmap()->height());

    if (showApplyToAll) {
	QRadioButton *applyToOneButton =
	    new QRadioButton(i18n("Apply to current segment only"),
			     buttonFrame);
	m_applyToAllButton =
	    new QRadioButton(i18n("Apply to all segments at this time"),
			     buttonFrame);
	applyToOneButton->setChecked(true);
    } else {
	m_applyToAllButton = 0;
	buttonFrame->hide();
    }
    
    if (showConversionOptions) {
	m_noConversionButton =
	    new QRadioButton
	    (i18n("Maintain current pitches"), conversionFrame);
	m_convertButton =
	    new QRadioButton
	    (i18n("Maintain current accidentals"), conversionFrame);
	m_transposeButton =
	    new QRadioButton
	    (i18n("Transpose into this key"), conversionFrame);
	m_noConversionButton->setChecked(true);
    } else {
	m_noConversionButton = 0;
	m_convertButton = 0;
	m_transposeButton = 0;
	conversionFrame->hide();
    }
    
    QObject::connect(keyUp, SIGNAL(clicked()), this, SLOT(slotKeyUp()));
    QObject::connect(keyDown, SIGNAL(clicked()), this, SLOT(slotKeyDown()));
    QObject::connect(m_keyCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotKeyNameChanged(const QString &)));
    QObject::connect(m_keyCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotKeyNameChanged(const QString &)));
    QObject::connect(m_majorMinorCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotMajorMinorChanged(const QString &)));
}

KeySignatureDialog::ConversionType
KeySignatureDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
	return NoConversion;
    } else if (m_convertButton && m_convertButton->isChecked()) {
	return Convert;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
	return Transpose;
    }
    return NoConversion;
}

bool
KeySignatureDialog::shouldApplyToAll() const
{
    return m_applyToAllButton && m_applyToAllButton->isChecked();
}

void
KeySignatureDialog::slotKeyUp()
{
    bool sharp = m_key.isSharp();
    int ac = m_key.getAccidentalCount();
    if (sharp) {
	if (++ac > 7) ac = 7;
    } else {
	if (--ac < 1) {
	    ac = 0;
	    sharp = true;
	}
    }
    
    try {
	m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
	setValid(true);
    } catch (Rosegarden::Key::BadKeySpec) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

void
KeySignatureDialog::slotKeyDown()
{
    bool sharp = m_key.isSharp();
    int ac = m_key.getAccidentalCount();
    if (sharp) {
	if (--ac < 0) {
	    ac = 1;
	    sharp = false;
	}
    } else {
	if (++ac > 7) ac = 7;
    }
    
    try {
	m_key = Rosegarden::Key(ac, sharp, m_key.isMinor());
	setValid(true);
    } catch (Rosegarden::Key::BadKeySpec) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

struct KeyNameComparator
{
    bool operator()(const Rosegarden::Key &k1, const Rosegarden::Key &k2) {
	return (k1.getName() < k2.getName());
    }
};

void
KeySignatureDialog::regenerateKeyCombo()
{
    if (m_explanatoryLabel) m_explanatoryLabel->hide();

    m_ignoreComboChanges = true;
    QString currentText = m_keyCombo->currentText();
    Rosegarden::Key::KeyList keys(Rosegarden::Key::getKeys(m_key.isMinor()));
    m_keyCombo->clear();

    std::sort(keys.begin(), keys.end(), KeyNameComparator());
    bool textSet = false;

    for (Rosegarden::Key::KeyList::iterator i = keys.begin();
	 i != keys.end(); ++i) {

	QString name(strtoqstr(i->getName()));
	int space = name.find(' ');
	if (space > 0) name = name.left(space);

	m_keyCombo->insertItem(name);

	if (m_valid && (*i == m_key)) {
	    m_keyCombo->setCurrentItem(m_keyCombo->count() - 1);
	    textSet = true;
	}
    }

    if (!textSet) {
	m_keyCombo->setEditText(currentText);
    }
    m_ignoreComboChanges = false;
}

bool
KeySignatureDialog::isValid() const
{
    return m_valid;
}

Rosegarden::Key
KeySignatureDialog::getKey() const
{
    return m_key;
}

void
KeySignatureDialog::redrawKeyPixmap()
{
    if (m_valid) {
	QPixmap pmap =
	    NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeKeyDisplayPixmap(m_key, m_clef));
	m_keyLabel->setPixmap(pmap);
    } else {
	m_keyLabel->setText(i18n("No such key"));
    }
}

void
KeySignatureDialog::slotKeyNameChanged(const QString &s)
{
    if (m_ignoreComboChanges) return;

    std::string name(getKeyName(s, m_key.isMinor()));
    
    try {
	m_key = Rosegarden::Key(name);
	setValid(true);

	int space = name.find(' ');
	if (space > 0) name = name.substr(0, space);
	m_keyCombo->setEditText(strtoqstr(name));

    } catch (Rosegarden::Key::BadKeyName) {
	setValid(false);
    }

    redrawKeyPixmap();
}

void
KeySignatureDialog::slotMajorMinorChanged(const QString &s)
{
    if (m_ignoreComboChanges) return;

    std::string name(getKeyName(m_keyCombo->currentText(), s == "Minor"));

    try {
	m_key = Rosegarden::Key(name);
	setValid(true);
    } catch (Rosegarden::Key::BadKeyName) {
	setValid(false);
    }

    regenerateKeyCombo();
    redrawKeyPixmap();
}

void
KeySignatureDialog::setValid(bool valid)
{
    m_valid = valid;
    enableButton(Ok, m_valid);
}

std::string
KeySignatureDialog::getKeyName(const QString &s, bool minor)
{
    QString u((s.length() >= 1) ? (s.left(1).upper() + s.right(s.length() - 1))
			        :  s);
    
    std::string name(qstrtostr(u));
    name = name + " " + (minor ? "minor" : "major");
    return name;
}


PasteNotationDialog::PasteNotationDialog(QWidget *parent,
					 PasteEventsCommand::PasteType defaultType) :
    KDialogBase(parent, 0, true, i18n("Paste"), Ok | Cancel),
    m_defaultType(defaultType)
{
    QVBox *vbox = makeVBoxMainWidget();

    QButtonGroup *pasteTypeGroup = new QButtonGroup
	(1, Horizontal, i18n("Paste type"), vbox);

    PasteEventsCommand::PasteTypeMap pasteTypes =
	PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
	 i != pasteTypes.end(); ++i) {

	QRadioButton *button = new QRadioButton(i->second, pasteTypeGroup);
	button->setChecked(m_defaultType == i->first);
	QObject::connect(button, SIGNAL(clicked()),
			 this, SLOT(slotPasteTypeChanged()));

	m_pasteTypeButtons.push_back(button);
    }

    QButtonGroup *setAsDefaultGroup = new QButtonGroup
	(1, Horizontal, i18n("Options"), vbox);

    m_setAsDefaultButton = new QCheckBox
	(i18n("Make this the default paste type"), setAsDefaultGroup);
    m_setAsDefaultButton->setChecked(true);
}

PasteEventsCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    for (unsigned int i = 0; i < m_pasteTypeButtons.size(); ++i) {
	if (m_pasteTypeButtons[i]->isChecked()) {
	    return (PasteEventsCommand::PasteType)i;
	}
    }

    return PasteEventsCommand::Restricted;
}

bool
PasteNotationDialog::setAsDefault() const
{
    return m_setAsDefaultButton->isChecked();
}

void
PasteNotationDialog::slotPasteTypeChanged()
{
    m_setAsDefaultButton->setChecked(m_defaultType == getPasteType());
}



TupletDialog::TupletDialog(QWidget *parent, Note::Type defaultUnitType,
			   timeT maxDuration) :
    KDialogBase(parent, 0, true, i18n("Tuplet"), Ok | Cancel),
    m_maxDuration(maxDuration)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *timingBox = new QGroupBox
	(1, Horizontal, i18n("New timing for tuplet group"), vbox);

    if (m_maxDuration > 0) {

	// bit of a sanity check
	if (maxDuration < Note(Note::Semiquaver).getDuration()) {
	    maxDuration = Note(Note::Semiquaver).getDuration();
	}

	Note::Type maxUnitType =
	    Note::getNearestNote(maxDuration/2, 0).getNoteType();
	if (defaultUnitType > maxUnitType) defaultUnitType = maxUnitType;
    }

    QGrid *timingGrid = new QGrid(3, QGrid::Horizontal, timingBox);

    new QLabel(i18n("Play "), timingGrid);
    m_untupledCombo = new RosegardenComboBox(true, timingGrid);

    m_unitCombo = new KComboBox(timingGrid);
    NotePixmapFactory npf;

    for (Note::Type t = Note::Shortest; t <= Note::Longest; ++t) {
	Note note(t);
	timeT duration(note.getDuration());
	if (maxDuration > 0 && (2 * duration > maxDuration)) break;
	timeT e; // error factor, ignore
	m_unitCombo->insertItem(NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(duration, e)),
				NotationStrings::makeNoteMenuLabel(duration, false, e, true));
	if (defaultUnitType == t) {
	    m_unitCombo->setCurrentItem(m_unitCombo->count() - 1);
	}
    }
    
    updateUntupledCombo();

    new QLabel(i18n("in the time of  "), timingGrid);
    m_tupledCombo = new RosegardenComboBox(true, timingGrid);
    updateTupledCombo();

    QGroupBox *timingDisplayBox = new QGroupBox
	(1, Horizontal, i18n("Timing calculations"), vbox);

    QGrid *timingDisplayGrid = new QGrid(3, QGrid::Horizontal, timingDisplayBox);

    if (maxDuration > 0) {

	new QLabel(i18n("Selected region:"), timingDisplayGrid);
	new QLabel("", timingDisplayGrid);
	m_selectionDurationDisplay = new QLabel("x", timingDisplayGrid);
	m_selectionDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						     QLabel::AlignRight));
    } else {
	m_selectionDurationDisplay = 0;
    }
    
    new QLabel(i18n("Group with current timing:"), timingDisplayGrid);
    m_untupledDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_untupledDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_untupledDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						QLabel::AlignRight));

    new QLabel(i18n("Group with new timing:"), timingDisplayGrid);
    m_tupledDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_tupledDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_tupledDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
					      QLabel::AlignRight));

    new QLabel(i18n("Gap created by timing change:"), timingDisplayGrid);
    m_newGapDurationCalculationDisplay = new QLabel("x", timingDisplayGrid);
    m_newGapDurationDisplay = new QLabel("x", timingDisplayGrid);
    m_newGapDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
					      QLabel::AlignRight));

    if (maxDuration > 0) {

	new QLabel(i18n("Unchanged at end of selection:"), timingDisplayGrid);
	m_unchangedDurationCalculationDisplay = new QLabel
	    ("x", timingDisplayGrid);
	m_unchangedDurationDisplay = new QLabel("x", timingDisplayGrid);
	m_unchangedDurationDisplay->setAlignment(int(QLabel::AlignVCenter |
						     QLabel::AlignRight));

    } else {
	m_unchangedDurationDisplay = 0;
    }

    updateTimingDisplays();

    QObject::connect(m_unitCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotUnitChanged(const QString &)));

    QObject::connect(m_untupledCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotUntupledChanged(const QString &)));
    QObject::connect(m_untupledCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotUntupledChanged(const QString &)));

    QObject::connect(m_tupledCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotTupledChanged(const QString &)));
    QObject::connect(m_tupledCombo, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotTupledChanged(const QString &)));
}

Note::Type
TupletDialog::getUnitType() const
{
    return Note::Shortest + m_unitCombo->currentItem();
}

int
TupletDialog::getUntupledCount() const
{
    bool isNumeric = true;
    int count = m_untupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric) return 1;
    else return count;
}

int
TupletDialog::getTupledCount() const
{
    bool isNumeric = true;
    int count = m_tupledCombo->currentText().toInt(&isNumeric);
    if (count == 0 || !isNumeric) return 1;
    else return count;
}



void
TupletDialog::updateUntupledCombo()
{
    // Untupled combo can contain numbers up to the maximum
    // duration divided by the unit duration.  If there's no
    // maximum, we'll have to put in some likely values and
    // allow the user to edit it.  Both the numerical combos
    // should possibly be spinboxes, except I think I like
    // being able to "suggest" a few values

    int maxValue = 12;

    if (m_maxDuration) {
	maxValue = m_maxDuration / Note(getUnitType()).getDuration();
    }

    QString previousText = m_untupledCombo->currentText();
    if (previousText.toInt() == 0) {
	if (maxValue < 3) previousText = QString("%1").arg(maxValue);
	else previousText = "3";
    }

    m_untupledCombo->clear();
    bool setText = false;

    for (int i = 1; i <= maxValue; ++i) {
	QString text = QString("%1").arg(i);
	m_untupledCombo->insertItem(text);
	if (text == previousText) {
	    m_untupledCombo->setCurrentItem(m_untupledCombo->count() - 1);
	    setText = true;
	}
    }

    if (!setText) {
	m_untupledCombo->setEditText(previousText);
    }
}

void
TupletDialog::updateTupledCombo()
{
    // should contain all positive integers less than the
    // largest value in the untupled combo.  In principle
    // we can support values larger, but we can't quite
    // do the tupleting transformation yet

    int untupled = getUntupledCount();

    QString previousText = m_tupledCombo->currentText();
    if (previousText.toInt() == 0 ||
	previousText.toInt() > untupled) {
	if (untupled < 2) previousText = QString("%1").arg(untupled);
	else previousText = "2";
    }

    m_tupledCombo->clear();

    for (int i = 1; i < untupled; ++i) {
	QString text = QString("%1").arg(i);
	m_tupledCombo->insertItem(text);
	if (text == previousText) {
	    m_tupledCombo->setCurrentItem(m_tupledCombo->count() - 1);
	}
    }
}

void
TupletDialog::updateTimingDisplays()
{
    timeT unitDuration = Note(getUnitType()).getDuration();

    int untupledCount = getUntupledCount();
    int tupledCount = getTupledCount();

    timeT untupledDuration = unitDuration * untupledCount;
    timeT tupledDuration = unitDuration * tupledCount;

    if (m_selectionDurationDisplay) {
	m_selectionDurationDisplay->setText(QString("%1").arg(m_maxDuration));
    }

    m_untupledDurationCalculationDisplay->setText
	(QString("  %1 x %2 = ").arg(untupledCount).arg(unitDuration));
    m_untupledDurationDisplay->setText
	(QString("%1").arg(untupledDuration));

    m_tupledDurationCalculationDisplay->setText
	(QString("  %1 x %2 = ").arg(tupledCount).arg(unitDuration));
    m_tupledDurationDisplay->setText
	(QString("%1").arg(tupledDuration));

    m_newGapDurationCalculationDisplay->setText
	(QString("  %1 - %2 = ").arg(untupledDuration).arg(tupledDuration));
    m_newGapDurationDisplay->setText
	(QString("%1").arg(untupledDuration - tupledDuration));

    if (m_selectionDurationDisplay && m_unchangedDurationDisplay) {
	if (m_maxDuration != untupledDuration) {
	    m_unchangedDurationCalculationDisplay->setText
		(QString("  %1 - %2 = ").arg(m_maxDuration).arg(untupledDuration));
	} else {
	    m_unchangedDurationCalculationDisplay->setText("");
	}
	m_unchangedDurationDisplay->setText
	    (QString("%1").arg(m_maxDuration - untupledDuration));
    }
}

void
TupletDialog::slotUnitChanged(const QString &)
{
    updateUntupledCombo();
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotUntupledChanged(const QString &)
{
    updateTupledCombo();
    updateTimingDisplays();
}

void
TupletDialog::slotTupledChanged(const QString &)
{
    updateTimingDisplays();
}



TextEventDialog::TextEventDialog(QWidget *parent,
				 NotePixmapFactory *npf,
				 Text defaultText,
				 int maxLength) :
    KDialogBase(parent, 0, true, i18n("Text"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_styles(Text::getUserStyles())
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *entryBox = new QGroupBox
	(1, Horizontal, i18n("Specification"), vbox);
    QGroupBox *exampleBox = new QGroupBox
	(1, Horizontal, i18n("Preview"), vbox);

    QGrid *entryGrid = new QGrid(2, QGrid::Horizontal, entryBox);

    new QLabel(i18n("Text:  "), entryGrid);
    m_text = new QLineEdit(entryGrid);
    m_text->setText(strtoqstr(defaultText.getText()));
    if (maxLength > 0) m_text->setMaxLength(maxLength);

    new QLabel(i18n("Style:  "), entryGrid);
    m_typeCombo = new KComboBox(entryGrid);

    for (unsigned int i = 0; i < m_styles.size(); ++i) {

	std::string style = m_styles[i];

	// if the style is in this list, we can i18n it (kludgy):

	if (style == Text::Dynamic) {
	    m_typeCombo->insertItem(i18n("Dynamic"));

	} else if (style == Text::Direction) {
	    m_typeCombo->insertItem(i18n("Direction"));

	} else if (style == Text::LocalDirection) {
	    m_typeCombo->insertItem(i18n("Local Direction"));

	} else if (style == Text::Tempo) {
	    m_typeCombo->insertItem(i18n("Tempo"));

	} else if (style == Text::LocalTempo) {
	    m_typeCombo->insertItem(i18n("Local Tempo"));

	} else if (style == Text::Lyric) {
	    m_typeCombo->insertItem(i18n("Lyric"));

	} else if (style == Text::Annotation) {
	    m_typeCombo->insertItem(i18n("Annotation"));

	} else {
	    // not i18n()-able

	    std::string styleName;
	    styleName += (char)toupper(style[0]);
	    styleName += style.substr(1);
	    
	    int uindex = styleName.find('_');
	    if (uindex > 0) {
		styleName =
		    styleName.substr(0, uindex) + " " +
		    styleName.substr(uindex + 1);
	    }
	    
	    m_typeCombo->insertItem(strtoqstr(styleName));
	}
	
	if (style == defaultText.getTextType()) {
	    m_typeCombo->setCurrentItem(m_typeCombo->count() - 1);
	}
    }

    QVBox *exampleVBox = new QVBox(exampleBox);
    
    int ls = m_notePixmapFactory->getLineSpacing();

    int mapWidth = 200;
    QPixmap map(mapWidth, ls * 5 + 1);
    QBitmap mask(mapWidth, ls * 5 + 1);

    map.fill();
    mask.fill(Qt::color0);

    QPainter p, pm;

    p.begin(&map);
    pm.begin(&mask);

    p.setPen(Qt::black);
    pm.setPen(Qt::white);
    
    for (int i = 0; i < 5; ++i) {
	p.drawLine(0, ls * i, mapWidth-1, ls * i);
	pm.drawLine(0, ls * i, mapWidth-1, ls * i);
    }

    p.end();
    pm.end();

    map.setMask(mask);

    m_staffAboveLabel = new QLabel("staff", exampleVBox);
    m_staffAboveLabel->setPixmap(map);

    m_textExampleLabel = new QLabel(i18n("Example"), exampleVBox);

    m_staffBelowLabel = new QLabel("staff", exampleVBox);
    m_staffBelowLabel->setPixmap(map);

    QObject::connect(m_text, SIGNAL(textChanged(const QString &)),
		     this, SLOT(slotTextChanged(const QString &)));
    QObject::connect(m_typeCombo, SIGNAL(activated(const QString &)),
		     this, SLOT(slotTypeChanged(const QString &)));

    m_text->setFocus();
    slotTypeChanged(strtoqstr(getTextType()));
}

std::string
TextEventDialog::getTextType() const
{
    return m_styles[m_typeCombo->currentItem()];
}

std::string
TextEventDialog::getTextString() const
{
    return std::string(qstrtostr(m_text->text()));
}

void
TextEventDialog::slotTextChanged(const QString &qtext)
{
    std::string type(getTextType());

    QString qtrunc(qtext);
    if (qtrunc.length() > 20) qtrunc = qtrunc.left(20) + "...";
    std::string text(qstrtostr(qtrunc));
    if (text == "") text = "Sample";

    Text rtext(text, type);
    m_textExampleLabel->setPixmap
	(NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeTextPixmap(rtext)));
}

void
TextEventDialog::slotTypeChanged(const QString &)
{
    std::string type(getTextType());

    QString qtrunc(m_text->text());
    if (qtrunc.length() > 20) qtrunc = qtrunc.left(20) + "...";
    std::string text(qstrtostr(qtrunc));
    if (text == "") text = "Sample";

    Text rtext(text, type);
    m_textExampleLabel->setPixmap
	(NotePixmapFactory::toQPixmap(m_notePixmapFactory->makeTextPixmap(rtext)));

    if (type == Text::Dynamic ||
	type == Text::LocalDirection ||
	type == Text::UnspecifiedType ||
	type == Text::Lyric ||
	type == Text::Annotation) {

	m_staffAboveLabel->show();
	m_staffBelowLabel->hide();

    } else {

	m_staffAboveLabel->hide();
	m_staffBelowLabel->show();
    }
}


EventEditDialog::EventEditDialog(QWidget *parent,
				 const Event &event,
				 bool editable) :
    KDialogBase(parent, 0, true, i18n(editable ? "Advanced Event Edit" : "Advanced Event Viewer"),
		(editable ? (Ok | Cancel) : Ok)),
    m_durationDisplay(0),
    m_durationDisplayAux(0),
    m_persistentGrid(0),
    m_nonPersistentGrid(0),
    m_nonPersistentView(0),
    m_originalEvent(event),
    m_event(event),
    m_type(event.getType()),
    m_absoluteTime(event.getAbsoluteTime()),
    m_duration(event.getDuration()),
    m_subOrdering(event.getSubOrdering()),
    m_modified(false)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *intrinsicBox = new QGroupBox
	(1, Horizontal, i18n("Intrinsics"), vbox);

    QGrid *intrinsicGrid = new QGrid(4, QGrid::Horizontal, intrinsicBox);

    new QLabel(i18n("Event type: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QLineEdit *lineEdit = new QLineEdit(intrinsicGrid);
    lineEdit->setText(strtoqstr(event.getType()));

    new QLabel(i18n("Absolute time: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    QSpinBox *absoluteTime = new QSpinBox
	(INT_MIN, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    absoluteTime->setValue(event.getAbsoluteTime());
    QObject::connect(absoluteTime, SIGNAL(valueChanged(int)),
		     this, SLOT(slotAbsoluteTimeChanged(int)));
    slotAbsoluteTimeChanged(event.getAbsoluteTime());

    new QLabel(i18n("Duration: "), intrinsicGrid);
    m_durationDisplay = new QLabel("(note)", intrinsicGrid);
    m_durationDisplay->setMinimumWidth(20);
    m_durationDisplayAux = new QLabel("(note)", intrinsicGrid);
    m_durationDisplayAux->setMinimumWidth(20);

    QSpinBox *duration = new QSpinBox
	(0, INT_MAX, Note(Note::Shortest).getDuration(), intrinsicGrid);
    duration->setValue(event.getDuration());
    QObject::connect(duration, SIGNAL(valueChanged(int)),
		     this, SLOT(slotDurationChanged(int)));
    slotDurationChanged(event.getDuration());

    new QLabel(i18n("Sub-ordering: "), intrinsicGrid);
    new QLabel("", intrinsicGrid);
    new QLabel("", intrinsicGrid);
    
    QSpinBox *subOrdering = new QSpinBox(-100, 100, 1, intrinsicGrid);
    subOrdering->setValue(event.getSubOrdering());
    QObject::connect(subOrdering, SIGNAL(valueChanged(int)),
		     this, SLOT(slotSubOrderingChanged(int)));
    slotSubOrderingChanged(event.getSubOrdering());

    QGroupBox *persistentBox = new QGroupBox
	(1, Horizontal, i18n("Persistent properties"), vbox);
    m_persistentGrid = new QGrid(4, QGrid::Horizontal, persistentBox);

    QLabel *label = new QLabel(i18n("Name"), m_persistentGrid);
    QFont font(label->font());
    font.setItalic(true);
    label->setFont(font);
    
    label = new QLabel(i18n("Type"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel(i18n("Value"), m_persistentGrid);
    label->setFont(font);
    label = new QLabel("", m_persistentGrid);
    label->setFont(font);

    Event::PropertyNames p = event.getPersistentPropertyNames();

    for (Event::PropertyNames::iterator i = p.begin();
	 i != p.end(); ++i) {
	addPersistentProperty(*i);
    }

    p = event.getNonPersistentPropertyNames();

    if (p.begin() == p.end()) {
	m_nonPersistentView = 0;
	m_nonPersistentGrid = 0;
    } else {

	QGroupBox *nonPersistentBox = new QGroupBox
	    (1, Horizontal, i18n("Non-persistent properties"), vbox);
	new QLabel(i18n("These are cached values, lost if the event is modified."),
		   nonPersistentBox);
	
	m_nonPersistentView = new QScrollView(nonPersistentBox);
	//m_nonPersistentView->setHScrollBarMode(QScrollView::AlwaysOff);
	m_nonPersistentView->setResizePolicy(QScrollView::AutoOneFit);
	
	m_nonPersistentGrid = new QGrid
	    (4, QGrid::Horizontal, m_nonPersistentView->viewport());
	m_nonPersistentView->addChild(m_nonPersistentGrid);
	
	m_nonPersistentGrid->setSpacing(4);
	m_nonPersistentGrid->setMargin(5);
	
	label = new QLabel(i18n("Name       "), m_nonPersistentGrid);
	label->setFont(font);
	label = new QLabel(i18n("Type       "), m_nonPersistentGrid);
	label->setFont(font);
	label = new QLabel(i18n("Value      "), m_nonPersistentGrid);
	label->setFont(font);
	label = new QLabel("", m_nonPersistentGrid);
	label->setFont(font);
	
	for (Event::PropertyNames::iterator i = p.begin();
	     i != p.end(); ++i) {
	    
	    new QLabel(strtoqstr(*i), m_nonPersistentGrid, strtoqstr(*i));
	    new QLabel(strtoqstr(event.getPropertyTypeAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
	    new QLabel(strtoqstr(event.getAsString(*i)), m_nonPersistentGrid, strtoqstr(*i));
	    QPushButton *button = new QPushButton("P", m_nonPersistentGrid, strtoqstr(*i));
	    button->setFixedSize(QSize(24, 24));
	    QToolTip::add(button, i18n("Make persistent"));
	    QObject::connect(button, SIGNAL(clicked()),
			     this, SLOT(slotPropertyMadePersistent()));
	}
    }
}

void
EventEditDialog::addPersistentProperty(const Rosegarden::PropertyName &name)
{
    QLabel *label = new QLabel(strtoqstr(name), m_persistentGrid, strtoqstr(name));
    label->show();
    label = new QLabel(strtoqstr(m_originalEvent.getPropertyTypeAsString(name)),
		       m_persistentGrid, strtoqstr(name));
    label->show();

    Rosegarden::PropertyType type(m_originalEvent.getPropertyType(name));
    switch (type) {
	
    case Int:
    {
	QSpinBox *spinBox = new QSpinBox
	    (INT_MIN, INT_MAX, 1, m_persistentGrid, strtoqstr(name));
	spinBox->setValue(m_originalEvent.get<Int>(name));
	QObject::connect(spinBox, SIGNAL(valueChanged(int)),
			 this, SLOT(slotIntPropertyChanged(int)));
	spinBox->show();
	break;
    }
    
    case RealTimeT:
    {
        Rosegarden::RealTime realTime = m_originalEvent.get<RealTimeT>(name);

        QHBox* hbox = new QHBox(m_persistentGrid);

        // seconds
        //
	QSpinBox *spinBox = new QSpinBox
	    (INT_MIN, INT_MAX, 1,
             hbox, strtoqstr(name) + "%sec");
	spinBox->setValue(realTime.sec);

	QObject::connect(spinBox, SIGNAL(valueChanged(int)),
			 this, SLOT(slotRealTimePropertyChanged(int)));

        // useconds
        //
	spinBox = new QSpinBox
	    (INT_MIN, INT_MAX, 1,
             hbox, strtoqstr(name) + "%usec");
	spinBox->setValue(realTime.usec);

	QObject::connect(spinBox, SIGNAL(valueChanged(int)),
			 this, SLOT(slotRealTimePropertyChanged(int)));
	spinBox->show();
	break;
    }

    case Bool:
    {
	QCheckBox *checkBox = new QCheckBox
	    ("", m_persistentGrid, strtoqstr(name));
	checkBox->setChecked(m_originalEvent.get<Bool>(name));
	QObject::connect(checkBox, SIGNAL(activated()),
			 this, SLOT(slotBoolPropertyChanged()));
	checkBox->show();
	break;
    }
    
    case String:
    {
	QLineEdit *lineEdit = new QLineEdit
	    (strtoqstr(m_originalEvent.get<String>(name)),
	     m_persistentGrid,
	     strtoqstr(name));
	QObject::connect(lineEdit, SIGNAL(textChanged(const QString &)),
			 this, SLOT(slotStringPropertyChanged(const QString &)));
	lineEdit->show();
	break;
    }
    }
    
    QPushButton *button = new QPushButton("X", m_persistentGrid,
					  strtoqstr(name));
    button->setFixedSize(QSize(24, 24));
    QToolTip::add(button, i18n("Delete this property"));
    QObject::connect(button, SIGNAL(clicked()),
		     this, SLOT(slotPropertyDeleted()));
    button->show();
}


Event
EventEditDialog::getEvent() const
{
    return Event(m_event, m_absoluteTime, m_duration, m_subOrdering);
}


void
EventEditDialog::slotEventTypeChanged(const QString &type)
{
    std::string t(qstrtostr(type));
    if (t != m_type) {
	m_modified = true;
	m_type = t;
    }
}

void
EventEditDialog::slotAbsoluteTimeChanged(int value)
{
    if (value == m_absoluteTime) return;
    m_modified = true;
    m_absoluteTime = value;
}

void
EventEditDialog::slotDurationChanged(int value)
{
    timeT error = 0;
    m_durationDisplay->setPixmap
	(NotePixmapFactory::toQPixmap(m_notePixmapFactory.makeNoteMenuPixmap(timeT(value), error)));

    if (error >= value / 2) {
	m_durationDisplayAux->setText("++ ");
    } else if (error > 0) {
	m_durationDisplayAux->setText("+ ");
    } else if (error < 0) {
	m_durationDisplayAux->setText("- ");
    } else {
	m_durationDisplayAux->setText(" ");
    }

    if (timeT(value) == m_duration) return;

    m_modified = true;
    m_duration = value;
}

void
EventEditDialog::slotSubOrderingChanged(int value)
{
    if (value == m_subOrdering) return;
    m_modified = true;
    m_subOrdering = value;
}

void
EventEditDialog::slotIntPropertyChanged(int value) 
{
    const QObject *s = sender();
    const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(s);
    if (!spinBox) return;

    m_modified = true;
    QString propertyName = spinBox->name();
    m_event.set<Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotRealTimePropertyChanged(int value) 
{
    const QObject *s = sender();
    const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(s);
    if (!spinBox) return;

    m_modified = true;
    QString propertyFullName = spinBox->name();

    QString propertyName = propertyFullName.section('%', 0, 0),
        usecOrSec =  propertyFullName.section('%', 1, 1);

    Rosegarden::RealTime realTime = m_event.get<RealTimeT>(qstrtostr(propertyName));

    if (usecOrSec == "sec")
        realTime.sec = value;
    else 
        realTime.usec = value;

    m_event.set<Int>(qstrtostr(propertyName), value);
}

void
EventEditDialog::slotBoolPropertyChanged()
{ 
    const QObject *s = sender();
    const QCheckBox *checkBox = dynamic_cast<const QCheckBox *>(s);
    if (!checkBox) return;

    m_modified = true;
    QString propertyName = checkBox->name();
    bool checked = checkBox->isChecked();

    m_event.set<Bool>(qstrtostr(propertyName), checked);
}

void
EventEditDialog::slotStringPropertyChanged(const QString &value)
{
    const QObject *s = sender();
    const QLineEdit *lineEdit = dynamic_cast<const QLineEdit *>(s);
    if (!lineEdit) return;
    
    m_modified = true;
    QString propertyName = lineEdit->name();
    m_event.set<String>(qstrtostr(propertyName), qstrtostr(value));
}

void
EventEditDialog::slotPropertyDeleted()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton) return;

    QString propertyName = pushButton->name();

    if (KMessageBox::warningContinueCancel
	(this,
	 i18n("Are you sure you want to delete the \"%1\" property?\n\n"
	      "Removing necessary properties may cause unexpected behaviour.").
	 arg(propertyName),
	 i18n("Edit Event"),
	 i18n("&Delete")) != KMessageBox::Continue) return;

    m_modified = true;
    QObjectList *list = m_persistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
	++i;
	delete obj;
    }
    delete list;
    
    m_event.unset(qstrtostr(propertyName));
}

void
EventEditDialog::slotPropertyMadePersistent()
{
    const QObject *s = sender();
    const QPushButton *pushButton = dynamic_cast<const QPushButton *>(s);
    if (!pushButton) return;

    QString propertyName = pushButton->name();

    if (KMessageBox::warningContinueCancel
	(this,
	 i18n("Are you sure you want to make the \"%1\" property persistent?\n\n"
	      "This could cause problems if it overrides a different "
	      "computed value later on.").
	 arg(propertyName),
	 i18n("Edit Event"),
	 i18n("Make &Persistent")) != KMessageBox::Continue) return;

    QObjectList *list = m_nonPersistentGrid->queryList(0, propertyName, false);
    QObjectListIt i(*list);
    QObject *obj;
    while ((obj = i.current()) != 0) {
	++i;
	delete obj;
    }
    delete list;

    m_modified = true;
    addPersistentProperty(qstrtostr(propertyName));

    Rosegarden::PropertyType type =
	m_originalEvent.getPropertyType(qstrtostr(propertyName));

    switch (type) {

    case Int:
	m_event.set<Int>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<Int>
	     (qstrtostr(propertyName)));
	break;

    case RealTimeT:
	m_event.set<RealTimeT>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<RealTimeT>
	     (qstrtostr(propertyName)));
	break;

    case Bool:
	m_event.set<Bool>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<Bool>
	     (qstrtostr(propertyName)));
	break;

    case String:
	m_event.set<String>
	    (qstrtostr(propertyName),
	     m_originalEvent.get<String>
	     (qstrtostr(propertyName)));
	break;
    }
}

// ----------------------- SimpleEventEditDialog ------------------------
//
//
SimpleEventEditDialog::SimpleEventEditDialog(QWidget *parent,
                                            RosegardenGUIDoc *doc,
				            const Event &event,
				            bool inserting) :
    KDialogBase(parent, 0, true, 
                i18n(inserting ? "Insert Event" : "Edit Event"), Ok | Cancel),
    m_originalEvent(event),
    m_event(event),
    m_doc(doc),
    m_type(event.getType()),
    m_absoluteTime(event.getAbsoluteTime()),
    m_duration(event.getDuration()),
    m_modified(false)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
	(1, Horizontal, i18n("Event Properties"), vbox);

    QFrame *frame = new QFrame(groupBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Event type:"), frame), 0, 0);

    m_typeCombo =  new RosegardenComboBox(true, frame);
    layout->addWidget(m_typeCombo, 0, 1);

    m_typeCombo->insertItem(strtoqstr(Rosegarden::Note::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Controller::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::KeyPressure::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::ChannelPressure::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::ProgramChange::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::SystemExclusive::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::PitchBend::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Indication::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Text::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Note::EventRestType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Clef::EventType));
    m_typeCombo->insertItem(strtoqstr(Rosegarden::Key::EventType));

    // Connect up the combos
    //
    connect(m_typeCombo, SIGNAL(activated(int)),
            SLOT(slotEventTypeChanged(int)));

    m_timeLabel = new QLabel(i18n("Absolute time:"), frame);
    layout->addWidget(m_timeLabel, 1, 0);
    m_timeSpinBox = new QSpinBox(INT_MIN, INT_MAX, Note(Note::Shortest).getDuration(), frame);
    layout->addWidget(m_timeSpinBox, 1, 1);

    connect(m_timeSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotAbsoluteTimeChanged(int)));

    m_durationLabel = new QLabel(i18n("Duration:"), frame);
    layout->addWidget(m_durationLabel, 2, 0);
    m_durationSpinBox = new QSpinBox(0, INT_MAX, Note(Note::Shortest).getDuration(), frame);
    layout->addWidget(m_durationSpinBox, 2, 1);

    connect(m_durationSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotDurationChanged(int)));

    m_pitchLabel = new QLabel(i18n("Pitch:"), frame);
    layout->addWidget(m_pitchLabel, 3, 0);
    m_pitchSpinBox = new QSpinBox(frame);
    layout->addWidget(m_pitchSpinBox, 3, 1);

    connect(m_pitchSpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotPitchChanged(int)));

    m_pitchSpinBox->setMinValue(Rosegarden::MidiMinValue);
    m_pitchSpinBox->setMaxValue(Rosegarden::MidiMaxValue);

    m_controllerLabel = new QLabel(i18n("Controller name:"), frame);
    m_controllerLabelValue = new QLabel(i18n("<none>"), frame);
    m_controllerLabelValue->setAlignment(QLabel::AlignRight);

    layout->addWidget(m_controllerLabel, 4, 0);
    layout->addWidget(m_controllerLabelValue, 4, 1);

    m_velocityLabel = new QLabel(i18n("Velocity:"), frame);
    layout->addWidget(m_velocityLabel, 5, 0);
    m_velocitySpinBox = new QSpinBox(frame);
    layout->addWidget(m_velocitySpinBox, 5, 1);

    connect(m_velocitySpinBox, SIGNAL(valueChanged(int)),
            SLOT(slotVelocityChanged(int)));

    m_velocitySpinBox->setMinValue(Rosegarden::MidiMinValue);
    m_velocitySpinBox->setMaxValue(Rosegarden::MidiMaxValue);

    m_metaLabel = new QLabel(i18n("Meta string:"), frame);
    layout->addWidget(m_metaLabel, 6, 0);
    m_metaEdit = new QLineEdit(frame);
    layout->addWidget(m_metaEdit, 6, 1);

    setupForEvent();
}

void
SimpleEventEditDialog::setupForEvent()
{
    using Rosegarden::BaseProperties::PITCH;
    using Rosegarden::BaseProperties::VELOCITY;

    m_typeCombo->blockSignals(true);
    m_timeSpinBox->blockSignals(true);
    m_durationSpinBox->blockSignals(true);
    m_pitchSpinBox->blockSignals(true);
    m_velocitySpinBox->blockSignals(true);
    m_metaEdit->blockSignals(true);

    // Some common settings
    //
    m_durationLabel->setText(i18n("Duration:"));
    m_timeLabel->show();
    m_timeSpinBox->show();


    if (m_type == Rosegarden::Note::EventType)
    {
        m_durationLabel->show();
        m_durationSpinBox->show();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Note pitch:"));
        m_pitchSpinBox->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(i18n("Note velocity:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());
        m_durationSpinBox->setValue(m_event.getDuration());

        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>(PITCH));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }

        try
        {
            m_velocitySpinBox->setValue(m_event.get<Rosegarden::Int>(VELOCITY));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_velocitySpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(0);
    }
    else if (m_type == Rosegarden::Controller::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Controller number:"));
        m_pitchSpinBox->show();

        m_controllerLabel->show();
        m_controllerLabelValue->show();
        m_controllerLabel->setText(i18n("Controller name:"));

        m_velocityLabel->show();
        m_velocityLabel->setText(i18n("Controller value:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::Controller::NUMBER));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }

        try
        {
            m_velocitySpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::Controller::VALUE));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_velocitySpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(1);
    }
    else if (m_type == Rosegarden::KeyPressure::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Key pitch:"));
        m_pitchSpinBox->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(i18n("Key pressure:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());
        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::KeyPressure::PITCH));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }
        
        try
        {
            m_velocitySpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::KeyPressure::PRESSURE));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_velocitySpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(2);
    }
    else if (m_type == Rosegarden::ChannelPressure::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Channel pressure:"));
        m_pitchSpinBox->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::ChannelPressure::PRESSURE));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(3);
    }
    else if (m_type == Rosegarden::ProgramChange::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Program change:"));
        m_pitchSpinBox->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::ProgramChange::PROGRAM));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(4);
    }
    else if (m_type == Rosegarden::SystemExclusive::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_controllerLabel->setText(i18n("Data length:"));
        try
        {
            Rosegarden::SystemExclusive sysEx(m_event);
            m_controllerLabelValue->setText(QString("%1").
                    arg(sysEx.getRawData().length()));
        }
        catch(...)
        {
            m_controllerLabelValue->setText("0");
        }

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());
        m_typeCombo->setCurrentItem(5);
    }
    else if (m_type == Rosegarden::PitchBend::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->show();
        m_pitchLabel->setText(i18n("Pitchbend MSB:"));
        m_pitchSpinBox->show();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->show();
        m_velocityLabel->setText(i18n("Pitchbend LSB:"));
        m_velocitySpinBox->show();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        try
        {
            m_pitchSpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::PitchBend::MSB));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_pitchSpinBox->setValue(0);
        }

        try
        {
            m_velocitySpinBox->setValue(m_event.get<Rosegarden::Int>
                    (Rosegarden::PitchBend::LSB));
        }
        catch(Rosegarden::Event::NoData)
        {
            m_velocitySpinBox->setValue(0);
        }

        m_typeCombo->setCurrentItem(6);
    }
    else if (m_type == Rosegarden::Indication::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_typeCombo->setCurrentItem(7);
    }
    else if (m_type == Rosegarden::Text::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->show();
        m_metaEdit->show();

        m_controllerLabel->setText(i18n("Text type:"));
        m_metaLabel->setText(i18n("Text:"));

        // get the text event
        try
        {
            Rosegarden::Text text(m_event);
            m_controllerLabelValue->setText(strtoqstr(text.getTextType()));
            m_metaEdit->setText(strtoqstr(text.getText()));
        }
        catch(...)
        {
            m_controllerLabelValue->setText(i18n("<none>"));
            m_metaEdit->setText(i18n("<none>"));
        }

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        // getTextType();

        m_typeCombo->setCurrentItem(8);
    }
    else if (m_type == Rosegarden::Note::EventRestType)
    {
        m_durationLabel->show();
        m_durationSpinBox->show();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->hide();

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());
        m_durationSpinBox->setValue(m_event.getDuration());

        m_typeCombo->setCurrentItem(9);
    }
    else if (m_type == Rosegarden::Clef::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_controllerLabel->setText(i18n("Clef type:"));

        try
        {
            Rosegarden::Clef clef(m_event);
            m_controllerLabelValue->setText(strtoqstr(clef.getClefType()));
        }
        catch(...)
        {
            m_controllerLabelValue->setText(i18n("<none>"));
        }

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        m_typeCombo->setCurrentItem(10);
    }
    else if (m_type == Rosegarden::Key::EventType)
    {
        m_durationLabel->hide();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->show();
        m_controllerLabelValue->show();

        m_controllerLabel->setText(i18n("Key name:"));

        try
        {
            Rosegarden::Key key(m_event);
            m_controllerLabelValue->setText(strtoqstr(key.getName()));
        }
        catch(...)
        {
            m_controllerLabelValue->setText(i18n("<none>"));
        }

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_timeSpinBox->setValue(m_event.getAbsoluteTime());

        m_typeCombo->setCurrentItem(11);
    }
    else
    {
        m_durationLabel->setText(i18n("Unsupported event type:"));
        m_durationLabel->show();
        m_durationSpinBox->hide();

        m_pitchLabel->hide();
        m_pitchSpinBox->hide();

        m_controllerLabel->hide();
        m_controllerLabelValue->show();
        m_controllerLabelValue->setText(strtoqstr(m_type));

        m_velocityLabel->hide();
        m_velocitySpinBox->hide();

        m_metaLabel->hide();
        m_metaEdit->hide();

        m_typeCombo->setEnabled(false);
        m_timeSpinBox->setValue(m_event.getAbsoluteTime());
    }

    m_typeCombo->blockSignals(false);
    m_timeSpinBox->blockSignals(false);
    m_durationSpinBox->blockSignals(false);
    m_pitchSpinBox->blockSignals(false);
    m_velocitySpinBox->blockSignals(false);
    m_metaEdit->blockSignals(false);

}


Rosegarden::Event
SimpleEventEditDialog::getEvent() const
{
    // return event selected against type
    Event event(m_type, m_absoluteTime, m_duration);
    int data1 = m_pitchSpinBox->value(), data2 = m_velocitySpinBox->value();

    switch(m_typeCombo->currentItem())
    {
        case 0: // Note
            try
            {
                data1 = m_event.get<Int>(Rosegarden::BaseProperties::PITCH);
                data2 = m_event.get<Int>(Rosegarden::BaseProperties::VELOCITY);
            }
            catch(Rosegarden::Event::NoData)
            {
            }

            event.set<Int>(Rosegarden::BaseProperties::PITCH, data1);
            event.set<Int>(Rosegarden::BaseProperties::VELOCITY, data2);

            break;

        case 1: // Controller

            try
            {
                data1 = m_event.get<Int>(Rosegarden::Controller::NUMBER);
                data2 = m_event.get<Int>(Rosegarden::Controller::VALUE);
            }
            catch(Rosegarden::Event::NoData)
            {
                RG_DEBUG  << "SimpleEventEditDialog::getEvent - "
                          << "can't create Controller event" << endl;
            }

            event.set<Int>(Rosegarden::Controller::NUMBER, data1);
            event.set<Int>(Rosegarden::Controller::VALUE, data2);

            break;

        case 2: // KeyPressure

            try
            {
                data1 = m_event.get<Int>(Rosegarden::KeyPressure::PITCH);
                data2 = m_event.get<Int>(Rosegarden::KeyPressure::PRESSURE);
            }
            catch(Rosegarden::Event::NoData)
            {
                RG_DEBUG << "SimpleEventEditDialog::getEvent - "
                         << "can't create KeyPressure event" << endl;
            }

            event.set<Int>(Rosegarden::KeyPressure::PITCH, data1);
            event.set<Int>(Rosegarden::KeyPressure::PRESSURE, data2);

            break;

        case 3: // ChannelPressure

            try
            {
                data1 = m_event.get<Int>(Rosegarden::ChannelPressure::PRESSURE);
            }
            catch(Rosegarden::Event::NoData)
            {
            }

            event.set<Int>(Rosegarden::ChannelPressure::PRESSURE, data1);

            break;

        case 4: // ProgramChange

            try
            {
                data1 = m_event.get<Int>(Rosegarden::ProgramChange::PROGRAM);
            }
            catch(Rosegarden::Event::NoData)
            {
            }

            event.set<Int>(Rosegarden::ProgramChange::PROGRAM, data1);

            break;

        case 5: // SystemExclusive

            // we should copy across raw data
            break;

        case 6: // PitchBend

            try
            {
                event.set<Int>(Rosegarden::PitchBend::MSB,
                    m_event.get<Int>(Rosegarden::PitchBend::MSB));
                event.set<Int>(Rosegarden::PitchBend::LSB,
                    m_event.get<Int>(Rosegarden::PitchBend::LSB));
            }
            catch(Rosegarden::Event::NoData)
            {
                RG_DEBUG << "SimpleEventEditDialog::getEvent - "
                         << "can't create PitchBend event" << endl;
            }
            break;

        case 7: // Indication

            // nothing yet
            break;

        case 8: // Text

            // nothing yet
            break;

        case 9: //  Rest
            break;

        case 10: // Clef
            break;

        case 11: // Key
            break;

        default:
            break;
    }

    return event;
}


void
SimpleEventEditDialog::slotEventTypeChanged(int value)
{
    m_type = qstrtostr(m_typeCombo->text(value));
    m_modified = true;

    setupForEvent();
}

void 
SimpleEventEditDialog::slotAbsoluteTimeChanged(int value)
{
    m_absoluteTime = value;
    m_modified = true;
}

void 
SimpleEventEditDialog::slotDurationChanged(int value)
{
    m_duration = value;
    m_modified = true;
}

void
SimpleEventEditDialog::slotPitchChanged(int value)
{
    m_modified = true;

    // set properties according to type
    switch (m_typeCombo->currentItem())
    {
        case 0:
            m_event.set<Int>(Rosegarden::BaseProperties::PITCH, value);
            break;

        case 1:
            m_event.set<Int>(Rosegarden::Controller::NUMBER, value);
            break;

        case 2:
            m_event.set<Int>(Rosegarden::KeyPressure::PITCH, value);
            break;

        case 3:
            m_event.set<Int>(Rosegarden::ChannelPressure::PRESSURE, value);
            break;

        case 4:
            m_event.set<Int>(Rosegarden::ProgramChange::PROGRAM, value);
            break;

        case 5:
            // SysEx
            //m_event.set<Int>();
            break;

        case 6:
            m_event.set<Int>(Rosegarden::PitchBend::MSB, value);
            break;

        case 7:
            // Indication
            break;

        case 8:
            // Text
            break;

        default:
            break;
    }

}

void
SimpleEventEditDialog::slotVelocityChanged(int value)
{
    m_modified = true;

    // set properties according to type
    switch (m_typeCombo->currentItem())
    {
        case 0:
            m_event.set<Int>(Rosegarden::BaseProperties::VELOCITY, value);
            break;

        case 1:
            m_event.set<Int>(Rosegarden::Controller::VALUE, value);
            break;

        case 2:
            m_event.set<Int>(Rosegarden::KeyPressure::PRESSURE, value);
            break;

        case 3:
            // none for ChannelPressure
            break;

        case 4:
            // none for ProgramChange
            break;

        case 5:
            // none for SysEx
            break;

        case 6:
            m_event.set<Int>(Rosegarden::PitchBend::LSB, value);
            break;

        case 7:
            // Indication
            break;

        case 8:
            // Text
            break;

        default:
            break;
    }
}

void
SimpleEventEditDialog::slotMetaChanged(const QString &)
{
    m_modified = true;
}



// ----------------------------- TempoValidtor ----------------------------
//
//
class TempoValidator : public QDoubleValidator
{
public:
    TempoValidator(QWidget *parent, const char *name = 0):
        QDoubleValidator(parent, name) {;}

    TempoValidator(double bottom, double top, int decimals,
                   QWidget *parent, const char *name = 0):
        QDoubleValidator(bottom, top, decimals, parent, name) {;}
    virtual ~TempoValidator() {;}

    /*
    virtual void fixup(QString &input) const
    {
        // do nothing for the moment
    }
    */

private:
};



TempoDialog::TempoDialog(QWidget *parent, RosegardenGUIDoc *doc):
    KDialogBase(parent, 0, true, i18n("Insert Tempo Change"), Ok | Cancel),
    m_doc(doc),
    m_tempoTime(0),
    m_tempoValue(0.0)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox(1, Horizontal, i18n("Tempo"), vbox);
    QHBox *tempoBox = new QHBox(groupBox);

    // Set tempo
    new QLabel(i18n("New tempo"), tempoBox);
    m_tempoValueSpinBox = new RosegardenSpinBox(tempoBox);
    m_tempoValueSpinBox->setMinValue(1);
    m_tempoValueSpinBox->setMaxValue(1000);
//    new QLabel(i18n("bpm"), tempoBox);

    // create a validator
    TempoValidator *validator = new TempoValidator(1.0, 1000.0, 6, this);
    m_tempoValueSpinBox->setValidator(validator);

    connect(m_tempoValueSpinBox, SIGNAL(valueChanged(const QString &)),
            SLOT(slotTempoChanged(const QString &)));

    m_tempoBeatLabel = new QLabel(tempoBox);
    m_tempoBeat = new QLabel(tempoBox);
    m_tempoBeatsPerMinute = new QLabel(tempoBox);

    // Scope Box
    QButtonGroup *scopeBox = new QButtonGroup(1, Horizontal,
					      i18n("Scope"), vbox);

    new QLabel(scopeBox);

    QHBox *currentBox = new QHBox(scopeBox);
    new QLabel(i18n("The pointer is currently at "), currentBox);
    m_tempoTimeLabel = new QLabel(currentBox);
    m_tempoBarLabel = new QLabel(currentBox);
    m_tempoStatusLabel = new QLabel(scopeBox);

    new QLabel(scopeBox);

    m_tempoChangeHere = new QRadioButton
	(i18n("Apply this tempo from here onwards"), scopeBox);

    m_tempoChangeBefore = new QRadioButton
	(i18n("Replace the last tempo change"), scopeBox);
    m_tempoChangeBeforeAt = new QLabel(scopeBox);
    m_tempoChangeBeforeAt->hide();

    m_tempoChangeStartOfBar = new QRadioButton
	(i18n("Apply this tempo from the start of this bar"), scopeBox);

    m_tempoChangeGlobal = new QRadioButton
	(i18n("Apply this tempo to the whole composition"), scopeBox);

    QHBox *optionHBox = new QHBox(scopeBox);
    new QLabel(optionHBox);
    m_defaultBox = new QCheckBox
	(i18n("Also make this the default tempo"), optionHBox);
    new QLabel(optionHBox);

    new QLabel(scopeBox);

    connect(m_tempoChangeHere, SIGNAL(clicked()),
            SLOT(slotActionChanged()));
    connect(m_tempoChangeBefore, SIGNAL(clicked()),
            SLOT(slotActionChanged()));
    connect(m_tempoChangeStartOfBar, SIGNAL(clicked()),
            SLOT(slotActionChanged()));
    connect(m_tempoChangeGlobal, SIGNAL(clicked()),
            SLOT(slotActionChanged()));

    m_tempoChangeHere->setChecked(true);

    // disable initially
    m_defaultBox->setEnabled(false);

    populateTempo();
}

TempoDialog::~TempoDialog()
{
}

void
TempoDialog::setTempoPosition(Rosegarden::timeT time)
{
    m_tempoTime = time;
    populateTempo();
}

// Using current m_tempoTime
//
void
TempoDialog::populateTempo()
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    double tempo = comp.getTempoAt(m_tempoTime);
    QString tempoString;
    tempoString.sprintf("%4.6f", tempo);

    // We have to frig this by setting both integer tempo
    // and the special text field.
    //
    m_tempoValueSpinBox->setValue((int)tempo);
    m_tempoValueSpinBox->setSpecialValueText(tempoString);

    updateBeatLabels(tempo);

    Rosegarden::RealTime tempoTime = comp.getElapsedRealTime(m_tempoTime);
    QString milliSeconds;
    milliSeconds.sprintf("%03ld", tempoTime.usec / 1000);
    m_tempoTimeLabel->setText(i18n("%1.%2 s,").arg(tempoTime.sec)
			      .arg(milliSeconds));

    int barNo = comp.getBarNumber(m_tempoTime);
    if (comp.getBarStart(barNo) == m_tempoTime) {
	m_tempoBarLabel->setText
	    (i18n("at the start of bar %1.").arg(barNo+1));
	m_tempoChangeStartOfBar->setEnabled(false);
    } else {
	m_tempoBarLabel->setText(
	    i18n("in the middle of bar %1.").arg(barNo+1));
	m_tempoChangeStartOfBar->setEnabled(true);
    }

    m_tempoChangeBefore->setEnabled(false);
    m_tempoChangeBeforeAt->setEnabled(false);

    bool havePrecedingTempo = false;

    int tempoChangeNo = comp.getTempoChangeNumberAt(m_tempoTime);
    if (tempoChangeNo >= 0) {

	timeT lastTempoTime = comp.getRawTempoChange(tempoChangeNo).first;
	if (lastTempoTime < m_tempoTime) {

	    Rosegarden::RealTime lastRT = comp.getElapsedRealTime(lastTempoTime);
	    QString lastms;
	    lastms.sprintf("%03ld", lastRT.usec / 1000);
	    int lastBar = comp.getBarNumber(lastTempoTime);
	    m_tempoChangeBeforeAt->setText
		(i18n("        (at %1.%2 s, in bar %3)").arg(lastRT.sec)
		 .arg(lastms).arg(lastBar+1));
	    m_tempoChangeBeforeAt->show();

	    m_tempoChangeBefore->setEnabled(true);
	    m_tempoChangeBeforeAt->setEnabled(true);

	    havePrecedingTempo = true;
	}
    }

    if (comp.getTempoChangeCount() > 0) {

	if (havePrecedingTempo) {
	    m_tempoStatusLabel->hide();
	} else {
	    m_tempoStatusLabel->setText
		(i18n("There are no preceding tempo changes."));
	}

	m_tempoChangeGlobal->setEnabled(true);

    } else {
	
	m_tempoStatusLabel->setText
	    (i18n("There are no other tempo changes."));

	m_tempoChangeGlobal->setEnabled(false);
    }

    m_defaultBox->setEnabled(false);
}

void
TempoDialog::updateBeatLabels(double tempo)
{
    Rosegarden::Composition &comp = m_doc->getComposition();

    // If the time signature's beat is not a crotchet, need to show
    // bpm separately

    timeT beat = comp.getTimeSignatureAt(m_tempoTime).getBeatDuration();
    if (beat == Note(Note::Crotchet).getDuration()) {
	m_tempoBeatLabel->setText(" bpm");
	m_tempoBeatLabel->show();
	m_tempoBeat->hide();
	m_tempoBeatsPerMinute->hide();
    } else {
	m_tempoBeatLabel->setText(" (");

	NotePixmapFactory npf;

	timeT error = 0;
	m_tempoBeat->setPixmap(NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(beat, error)));
	if (error) m_tempoBeat->setPixmap(NotePixmapFactory::toQPixmap(npf.makeUnknownPixmap()));

	m_tempoBeatsPerMinute->setText
	    (QString("= %1 )").arg
	     (int(tempo * Note(Note::Crotchet).getDuration() / beat)));
	m_tempoBeatLabel->show();
	m_tempoBeat->show();
	m_tempoBeatsPerMinute->show();
    }
}

void
TempoDialog::slotTempoChanged(const QString &)
{
    updateBeatLabels(m_tempoValueSpinBox->getDoubleValue());
}

void
TempoDialog::slotActionChanged()
{
    m_defaultBox->setEnabled(m_tempoChangeGlobal->isChecked());
}

void
TempoDialog::slotOk()
{
    double tempoDouble = m_tempoValueSpinBox->getDoubleValue();

    // Check for freakiness in the returned results - 
    // if we can't believe the double value then use
    // the value on the SpinBox itself - we just have 
    // to lose the precision.
    //
    if ((int)tempoDouble != m_tempoValueSpinBox->value())
        tempoDouble = m_tempoValueSpinBox->value();

    TempoDialogAction action = AddTempo;

    if (m_tempoChangeBefore->isChecked()) {
	action = ReplaceTempo;
    } else if (m_tempoChangeStartOfBar->isChecked()) {
	action = AddTempoAtBarStart;
    } else if (m_tempoChangeGlobal->isChecked()) {
	action = GlobalTempo;
	if (m_defaultBox->isChecked()) {
	    action = GlobalTempoWithDefault;
	}
    }

    emit changeTempo(m_tempoTime,
                     tempoDouble,
                     action);

    KDialogBase::slotOk();
}




ClefDialog::ClefDialog(QWidget *parent,
		       NotePixmapFactory *npf,
		       Rosegarden::Clef defaultClef,
		       bool showConversionOptions) :
    KDialogBase(parent, 0, true, i18n("Clef"), Ok | Cancel),
    m_notePixmapFactory(npf),
    m_clef(defaultClef)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *clefFrame = new QGroupBox
	(1, Horizontal, i18n("Clef"), vbox);
    
    QButtonGroup *conversionFrame = new QButtonGroup
	(1, Horizontal, i18n("Existing notes following clef change"), vbox);
	
    QHBox *clefBox = new QHBox(clefFrame);
    
    BigArrowButton *clefDown = new BigArrowButton(clefBox, Qt::LeftArrow);
    QToolTip::add(clefDown, i18n("Lower clef"));

    QHBox *clefLabelBox = new QVBox(clefBox);

    m_octaveUp = new BigArrowButton(clefLabelBox, Qt::UpArrow);
    QToolTip::add(m_octaveUp, i18n("Up an Octave"));
    
    m_clefLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefLabel->setAlignment(AlignVCenter | AlignHCenter);

    m_octaveDown = new BigArrowButton(clefLabelBox, Qt::DownArrow);
    QToolTip::add(m_octaveDown, i18n("Down an Octave"));

    BigArrowButton *clefUp = new BigArrowButton(clefBox, Qt::RightArrow);
    QToolTip::add(clefUp, i18n("Higher clef"));

    m_clefNameLabel = new QLabel(i18n("Clef"), clefLabelBox);
    m_clefNameLabel->setAlignment(AlignVCenter | AlignHCenter);
      
    if (showConversionOptions) {
	m_noConversionButton =
	    new QRadioButton
	    (i18n("Maintain current pitches"), conversionFrame);
	m_changeOctaveButton =
	    new QRadioButton
	    (i18n("Transpose into appropriate octave"), conversionFrame);
	m_transposeButton = 0;

	//!!! why aren't we offering this option? does it not work? too difficult to describe?
//	m_transposeButton =
//	    new QRadioButton
//	    (i18n("Maintain current positions on the staff"), conversionFrame);
	m_changeOctaveButton->setChecked(true);
    } else {
	m_noConversionButton = 0;
	m_changeOctaveButton = 0;
	m_transposeButton = 0;
	conversionFrame->hide();
    }
    
    QObject::connect(clefUp, SIGNAL(clicked()), this, SLOT(slotClefUp()));
    QObject::connect(clefDown, SIGNAL(clicked()), this, SLOT(slotClefDown()));
    QObject::connect(m_octaveUp, SIGNAL(clicked()), this, SLOT(slotOctaveUp()));
    QObject::connect(m_octaveDown, SIGNAL(clicked()), this, SLOT(slotOctaveDown()));

    redrawClefPixmap();
}

Rosegarden::Clef
ClefDialog::getClef() const
{
    return m_clef;
}

ClefDialog::ConversionType
ClefDialog::getConversionType() const
{
    if (m_noConversionButton && m_noConversionButton->isChecked()) {
	return NoConversion;
    } else if (m_changeOctaveButton && m_changeOctaveButton->isChecked()) {
	return ChangeOctave;
    } else if (m_transposeButton && m_transposeButton->isChecked()) {
	return Transpose;
    }
    return NoConversion;
}


void
ClefDialog::slotClefUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Rosegarden::Clef::ClefList clefs(Rosegarden::Clef::getClefs());

    for (Rosegarden::Clef::ClefList::iterator i = clefs.begin();
	 i != clefs.end(); ++i) {

	if (m_clef.getClefType() == i->getClefType()) {
	    if (++i == clefs.end()) i = clefs.begin();
	    m_clef = Rosegarden::Clef(i->getClefType(), octaveOffset);
	    break;
	}
    }

    redrawClefPixmap();
}


void
ClefDialog::slotClefDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    Rosegarden::Clef::ClefList clefs(Rosegarden::Clef::getClefs());

    for (Rosegarden::Clef::ClefList::iterator i = clefs.begin();
	 i != clefs.end(); ++i) {

	if (m_clef.getClefType() == i->getClefType()) {
	    if (i == clefs.begin()) i = clefs.end();
	    --i;
	    m_clef = Rosegarden::Clef(i->getClefType(), octaveOffset);
	    break;
	}
    }

    redrawClefPixmap();
}


void
ClefDialog::slotOctaveUp()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == 2) return;

    ++octaveOffset;

    m_octaveDown->setEnabled(true);
    if (octaveOffset == 2) {
	m_octaveUp->setEnabled(false);
    }

    m_clef = Rosegarden::Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}


void
ClefDialog::slotOctaveDown()
{
    int octaveOffset = m_clef.getOctaveOffset();
    if (octaveOffset == -2) return;

    --octaveOffset;

    m_octaveUp->setEnabled(true);
    if (octaveOffset == 2) {
	m_octaveDown->setEnabled(false);
    }

    m_clef = Rosegarden::Clef(m_clef.getClefType(), octaveOffset);
    redrawClefPixmap();
}


void
ClefDialog::redrawClefPixmap()
{
    QPixmap pmap = NotePixmapFactory::toQPixmap
	(m_notePixmapFactory->makeClefDisplayPixmap(m_clef));
    m_clefLabel->setPixmap(pmap);

    QString name;
    int octave = m_clef.getOctaveOffset();

    switch (octave) {
    case -1: name = "%1 down an octave"; break;
    case -2: name = "%1 down two octaves"; break;
    case  1: name = "%1 up an octave"; break;
    case  2: name = "%1 up two octaves"; break;
    default: name = "%1"; break;
    }

    std::string type = m_clef.getClefType();
    if (type == Rosegarden::Clef::Treble) name = name.arg(i18n("Treble"));
    else if (type == Rosegarden::Clef::Alto) name = name.arg(i18n("Alto"));
    else if (type == Rosegarden::Clef::Tenor) name = name.arg(i18n("Tenor"));
    else if (type == Rosegarden::Clef::Bass) name = name.arg(i18n("Bass"));

    m_clefNameLabel->setText(name);
}


QuantizeDialog::QuantizeDialog(QWidget *parent, bool inNotation) :
    KDialogBase(parent, 0, true, i18n("Quantize"), Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();

    m_quantizeFrame =
	new RosegardenQuantizeParameters(vbox, inNotation, 0);
}

Quantizer *
QuantizeDialog::getQuantizer() const
{
    return m_quantizeFrame->getQuantizer();
}

RescaleDialog::RescaleDialog(QWidget *parent) :
    KDialogBase(parent, 0, true, i18n("Rescale"), Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *ratioBox = new QGroupBox
	(1, Horizontal, i18n("Rescale ratio"), vbox);

    QHBox *notesBox = new QHBox(ratioBox);

    new QLabel(i18n("Play "), notesBox);
    KComboBox *fromCombo = new KComboBox(notesBox);

    new QLabel(i18n(" beats in time of "), notesBox);
    KComboBox *toCombo = new KComboBox(notesBox);

    for (int i = 1; i <= 16; ++i) {
	fromCombo->insertItem(QString("%1").arg(i));
	toCombo->insertItem(QString("%1").arg(i));
    }
    fromCombo->setCurrentItem(7);
    toCombo->setCurrentItem(7);
    m_from = m_to = 8;

    QHBox *percentBox = new QHBox(ratioBox);
    new QLabel(i18n("As percentage: "), percentBox);
    m_percent = new QLabel("100.00%", percentBox);

    QObject::connect(fromCombo, SIGNAL(activated(int)),
		     this, SLOT(slotFromChanged(int)));
    QObject::connect(toCombo, SIGNAL(activated(int)),
		     this, SLOT(slotToChanged(int)));
}

int
RescaleDialog::getMultiplier()
{
    return m_to;
}

int
RescaleDialog::getDivisor()
{
    return m_from;
}

void
RescaleDialog::slotFromChanged(int i)
{
    m_from = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
		       arg(perTenThou/100).
		       arg(perTenThou%100));
}

void
RescaleDialog::slotToChanged(int i)
{
    m_to = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
		       arg(perTenThou/100).
		       arg(perTenThou%100));
}


FileMergeDialog::FileMergeDialog(QWidget *parent,
				 QString /*fileName*/,
				 bool timingsDiffer) :
    KDialogBase(parent, 0, true, i18n("Merge File"), Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();

    QHBox *hbox = new QHBox(vbox);
    new QLabel(i18n("Merge new file  "), hbox);

    m_choice = new RosegardenComboBox(hbox);
    m_choice->insertItem(i18n("At start of existing composition"));
    m_choice->insertItem(i18n("From end of existing composition"));
    m_useTimings = 0;

    if (timingsDiffer) {
	new QLabel(i18n("The file has different time signatures or tempos."), vbox);
	m_useTimings = new QCheckBox(i18n("Import these as well"), vbox);
	m_useTimings->setChecked(false);
    }
}

int
FileMergeDialog::getMergeOptions()
{
    int options = MERGE_KEEP_OLD_TIMINGS | MERGE_IN_NEW_TRACKS;

    if (m_choice->currentItem() == 1) {
	options |= MERGE_AT_END;
    }

    if (m_useTimings && m_useTimings->isChecked()) {
	options |= MERGE_KEEP_NEW_TIMINGS;
    }

    return options;
}


FileLocateDialog::FileLocateDialog(QWidget *parent,
                                   const QString &file,
                                   const QString & /*path*/):
    KDialogBase(parent, 0, true,
                i18n("Locate audio file"),
                User1|User2,
                Ok,
                false,
                i18n("&Skip"),
                i18n("&Locate")),
                m_file(file)
{
    QHBox *w = makeHBoxMainWidget();
    QString label =
        QString(i18n("Can't find file \"%1\".\n"
                     "Would you like to try and locate this file or skip it?")).arg(m_file);

    QLabel *labelW = new QLabel(label, w);
    labelW->setAlignment(Qt::AlignHCenter);
    labelW->setMinimumHeight(100);
}


// Locate a file
//
void
FileLocateDialog::slotUser2()
{
    m_file = KFileDialog::getOpenFileName(":WAVS",
                                          QString(i18n("*.wav|WAV files (*.wav)")),
                                          this, i18n("Select an Audio File"));

    if (!m_file.isEmpty()) {
        QFileInfo fileInfo(m_file);
        m_path = fileInfo.dirPath();
        accept();
    } else
        reject();
}

// Skip this file
//
void
FileLocateDialog::slotUser1()
{
    reject();
}


AudioPlayingDialog::AudioPlayingDialog(QWidget *parent,
                                       const QString &name):
                                       KDialogBase(parent, 0, true,
                                       i18n("Playing audio file"),
                                       Cancel)
{
    QHBox *w = makeHBoxMainWidget();
    QLabel *label = new
        QLabel(i18n("Playing audio file \"%1\"").arg(name), w);

    label->setMinimumHeight(80);


}

// ----------------  AudioSplitDialog -----------------------
//

AudioSplitDialog::AudioSplitDialog(QWidget *parent,
                                   Segment *segment,
                                   RosegardenGUIDoc *doc):
            KDialogBase(parent, 0, true,
                        i18n("Autosplit Audio Segment"), Ok|Cancel),
            m_doc(doc),
            m_segment(segment),
            m_canvasWidth(500),
            m_canvasHeight(200),
            m_previewWidth(400),
            m_previewHeight(100)
{
    if (!segment || segment->getType() != Segment::Audio)
        reject();

    QVBox *w = makeVBoxMainWidget();

    new QLabel(i18n("AutoSplit Segment \"") +
	       strtoqstr(m_segment->getLabel()) + QString("\""), w);

    m_canvas = new QCanvas(w);
    m_canvas->resize(m_canvasWidth, m_canvasHeight);
    m_canvasView = new QCanvasView(m_canvas, w);

    m_canvasView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_canvasView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_canvasView->setDragAutoScroll(false);

    QHBox *hbox = new QHBox(w);
    new QLabel(i18n("Threshold"), hbox);
    m_thresholdSpin = new QSpinBox(hbox);
    m_thresholdSpin->setSpecialValueText("%");
    connect(m_thresholdSpin, SIGNAL(valueChanged(int)),
            SLOT(slotThresholdChanged(int)));

    // ensure this is cleared
    m_previewBoxes.clear();

    // Set thresholds
    //
    int threshold = 1;
    m_thresholdSpin->setValue(threshold);
    drawPreview();
    drawSplits(1);

}

void 
AudioSplitDialog::drawPreview()
{
    // Delete everything on the canvas
    //
    QCanvasItemList list = m_canvas->allItems();
    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); it++)
        delete *it;

    // empty the preview boxes
    m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());

    // Draw a bounding box
    //
    int border = 5;
    QCanvasRectangle *rect = new QCanvasRectangle(m_canvas);
    rect->setSize(m_canvasWidth - border * 2, m_canvasHeight - border * 2);
    rect->setX(border);
    rect->setY(border);
    rect->setZ(1);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Dark));
    rect->setBrush(kapp->palette().color(QPalette::Active, QColorGroup::Base));
    rect->setVisible(true);

    // Get preview in vector form
    //
    Rosegarden::AudioFileManager &aFM = m_doc->getAudioFileManager();
    int channels = aFM.getAudioFile(m_segment->getAudioFileId())->getChannels();

    std::vector<float> values;
   
    try
    {
        values = aFM.getPreview(m_segment->getAudioFileId(),
                                m_segment->getAudioStartTime(),
                                m_segment->getAudioEndTime(),
                                m_previewWidth,
                                false);
    }
    catch(std::string e)
    {
        QCanvasText *text = new QCanvasText(m_canvas);
        text->setColor(kapp->palette().
                color(QPalette::Active, QColorGroup::Shadow));
        text->setText(i18n("<no preview generated for this audio file>"));
        text->setX(30);
        text->setY(30);
        text->setZ(4);
        text->setVisible(true);
        m_canvas->update();
        return;
    }

    int startX = (m_canvasWidth - m_previewWidth) / 2;
    int halfHeight = m_canvasHeight / 2;
    float h1, h2;
    std::vector<float>::iterator it = values.begin();

    // Draw preview
    //
    for (int i = 0; i < m_previewWidth; i++)
    {
        if (channels == 1)
        {
            h1 = *(it++);
            h2 = h1;
        }
        else
        {
            h1 = *(it++);
            h2 = *(it++);
        }


        int startY = halfHeight + int(h1 * float(m_previewHeight / 2));
        int endY = halfHeight - int(h2 * float(m_previewHeight / 2));

        if ( startY < 0 )
        {
            RG_DEBUG << "AudioSplitDialog::AudioSplitDialog - "
                     << "startY - out of negative range"
                     << endl;
            startY = 0;
        }

        if (endY < 0)
        {
            RG_DEBUG << "AudioSplitDialog::AudioSplitDialog - "
                     << "endY - out of negative range"
                     << endl;
            endY = 0;
        }

        QCanvasLine *line = new QCanvasLine(m_canvas);
        line->setPoints(startX + i,
                        startY,
                        startX + i,
                        endY);
        line->setZ(3);
        line->setPen(kapp->
                palette().color(QPalette::Active, QColorGroup::Shadow));
        line->setBrush(kapp->
                palette().color(QPalette::Active, QColorGroup::Shadow));
        line->setVisible(true);

    }

    // Draw zero dc line
    //
    rect = new QCanvasRectangle(m_canvas);
    rect->setX(startX);
    rect->setY(halfHeight - 1);
    rect->setSize(m_previewWidth, 2);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setBrush(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(4);
    rect->setVisible(true);

    // Start time
    //
    char usecs[100];
    sprintf(usecs, "%03ld", m_segment->getAudioStartTime().usec / 1000);
    QString startText = QString("%1.%2s")
                              .arg(m_segment->getAudioStartTime().sec)
                              .arg(usecs);
    QCanvasText *text = new QCanvasText(m_canvas);
    text->setColor(
            kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    text->setText(startText);
    text->setX(startX - 20);
    text->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 35);
    text->setZ(3);
    text->setVisible(true);

    rect = new QCanvasRectangle(m_canvas);
    rect->setX(startX - 1);
    rect->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 14);
    rect->setSize(1, m_previewHeight + 28);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(3);
    rect->setVisible(true);
    
    // End time
    //
    sprintf(usecs, "%03ld", m_segment->getAudioEndTime().usec / 1000);
    QString endText = QString("%1.%2s")
                            .arg(m_segment->getAudioEndTime().sec)
                            .arg(usecs);
    text = new QCanvasText(m_canvas);
    text->setColor(
            kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    text->setText(endText);
    text->setX(startX + m_previewWidth - 20);
    text->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 35);
    text->setZ(3);
    text->setVisible(true);

    rect = new QCanvasRectangle(m_canvas);
    rect->setX(startX + m_previewWidth - 1);
    rect->setY(m_canvasHeight / 2 - m_previewHeight / 2 - 14);
    rect->setSize(1, m_previewHeight + 28);
    rect->setPen(kapp->palette().color(QPalette::Active, QColorGroup::Shadow));
    rect->setZ(3);
    rect->setVisible(true);

    m_canvas->update();
}

// Retreive and display split points on the envelope
//
void
AudioSplitDialog::drawSplits(int threshold)
{
    // Now get the current split points and paint them
    //
    Rosegarden::AudioFileManager &aFM = m_doc->getAudioFileManager();
    std::vector<Rosegarden::SplitPointPair> splitPoints =
        aFM.getSplitPoints(m_segment->getAudioFileId(),
                           m_segment->getAudioStartTime(),
                           m_segment->getAudioEndTime(),
                           threshold);

    std::vector<Rosegarden::SplitPointPair>::iterator it;
    std::vector<QCanvasRectangle*> tempRects;

    Rosegarden::RealTime length = m_segment->getAudioEndTime() - 
        m_segment->getAudioStartTime();
    double ticksPerUsec = double(m_previewWidth) /
                          double((length.sec * 1000000.0) + length.usec);

    int startX = (m_canvasWidth - m_previewWidth) / 2;
    int halfHeight = m_canvasHeight / 2;
    int x1, x2;
    int overlapHeight = 10;

    for (it = splitPoints.begin(); it != splitPoints.end(); it++)
    {
        x1 = int(ticksPerUsec * double(double(it->first.sec) *
                    1000000.0 + (double)it->first.usec));

        x2 = int(ticksPerUsec * double(double(it->second.sec) *
                    1000000.0 + double(it->second.usec)));

        QCanvasRectangle *rect = new QCanvasRectangle(m_canvas);
        rect->setX(startX + x1);
        rect->setY(halfHeight - m_previewHeight / 2 - overlapHeight / 2);
        rect->setZ(2);
        rect->setSize(x2 - x1, m_previewHeight + overlapHeight);
        rect->setPen(kapp->
                palette().color(QPalette::Active, QColorGroup::Dark));
        rect->setBrush(kapp->
                palette().color(QPalette::Active, QColorGroup::Dark));
        rect->setVisible(true);
        tempRects.push_back(rect);
    }

    std::vector<QCanvasRectangle*>::iterator pIt;

    // We've written the new Rects, now delete the old ones
    //
    if (m_previewBoxes.size())
    {
        // clear any previous preview boxes
        //
        for (pIt = m_previewBoxes.begin(); pIt != m_previewBoxes.end(); pIt++)
        {
            //(*pIt)->setVisible(false);
            delete (*pIt);
        }
        m_previewBoxes.erase(m_previewBoxes.begin(), m_previewBoxes.end());
        m_canvas->update();
    }
    m_canvas->update();

    // Now store the new ones
    //
    for (pIt = tempRects.begin(); pIt != tempRects.end(); pIt++)
        m_previewBoxes.push_back(*pIt);
}


// When we've got a value change redisplay the split positions
//
void
AudioSplitDialog::slotThresholdChanged(int threshold)
{
    drawSplits(threshold);
}


// Lyric editor.  We store lyrics internally as individual events for
// individual syllables, just as one might enter them (laboriously)
// without the help of a lyric editor.  The editor scans the segment
// for lyric events and unparses them into a string format for
// editing; when the user is done, the editor parses the string and
// completely recreates the lyric events in the segment.

// The string format is pretty basic at the moment.  A bar with notes
// and lyrics somewhat like:
//
//    o     o     o  o     |    o         o   
//    who   by    fi-re    |    and who   by  [etc]
//
// where the word fire is divided across two notes and the second bar
// has two syllables on the same note, would be represented as "who
// by fi-re / and~who by".  I'm sure we can work on this, as the text
// version is not what's actually saved anyway.

LyricEditDialog::LyricEditDialog(QWidget *parent,
				 Segment *segment) :
    KDialogBase(parent, 0, true, i18n("Edit Lyrics"), Ok | Cancel),
    m_segment(segment)
{    
    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
	(1, Horizontal, i18n("Lyrics for this segment"), vbox);

    m_textEdit = new QTextEdit(groupBox);
    m_textEdit->setTextFormat(Qt::PlainText);

    m_textEdit->setMinimumWidth(300);
    m_textEdit->setMinimumHeight(200);

    unparse();
}

void
LyricEditDialog::unparse()
{
    // This and SetLyricsCommand::execute() are opposites that will
    // need to be kept in sync with any changes in one another.  (They
    // should really both be in a common lyric management class.)

    Rosegarden::Composition *comp = m_segment->getComposition();

    QString text;

    timeT lastTime = m_segment->getStartTime();
    int lastBarNo = comp->getBarNumber(lastTime);
    bool haveLyric = false;

    text += QString("[%1] ").arg(lastBarNo + 1);

    for (Segment::iterator i = m_segment->begin();
	 m_segment->isBeforeEndMarker(i); ++i) {

	bool isNote = (*i)->isa(Note::EventType);
	bool isLyric = false;
	
	if (!isNote) {
	    if ((*i)->isa(Text::EventType)) {
		std::string textType;
		if ((*i)->get<String>(Text::TextTypePropertyName, textType) &&
		    textType == Text::Lyric) {
		    isLyric = true;
		}
	    }
	} else {
	    if ((*i)->has(Rosegarden::BaseProperties::TIED_BACKWARD) &&
		(*i)->get<Bool>(Rosegarden::BaseProperties::TIED_BACKWARD))
		continue;
	}

	if (!isNote && !isLyric) continue;

	timeT myTime = (*i)->getNotationAbsoluteTime();
	int myBarNo = comp->getBarNumber(myTime);

	if (myBarNo > lastBarNo) {

	    while (myBarNo > lastBarNo) {
		text += " /";
		++lastBarNo;
	    }
	    text += QString("\n[%1] ").arg(myBarNo + 1);
	}

	if (myTime > lastTime && isNote) {
	    if (!haveLyric) text += " .";
	    lastTime = myTime;
	    haveLyric = false;
	}

	if (isLyric) {
	    std::string ssyllable;
	    (*i)->get<String>(Text::TextPropertyName, ssyllable);
	    QString syllable(strtoqstr(ssyllable));
	    syllable.replace(QRegExp("\\s+"), "~");
	    text += " " + syllable;
	    haveLyric = true;
	}
    }

    m_textEdit->setText(text);
}

QString
LyricEditDialog::getLyricData()
{
    return m_textEdit->text();
}


// ------------------- EventParameterDialog ----------------------------
//
//

EventParameterDialog::EventParameterDialog(
        QWidget *parent,
        const QString &name,
        const Rosegarden::PropertyName &property,
        int startValue):
            KDialogBase(parent, 0, true, name, Ok | Cancel),
            m_property(property)
{
    QVBox *vBox = makeVBoxMainWidget();

    QHBox *topBox = new QHBox(vBox);
    QLabel *explainLabel = new QLabel(topBox);
    QString text = i18n("Set the %1 property of the event selection:").
	arg(strtoqstr(property));
    explainLabel->setText(text);

    QHBox *patternBox = new QHBox(vBox);
    /*QLabel *patternLabel =*/ new QLabel(i18n("Pattern"), patternBox);
    m_patternCombo = new RosegardenComboBox(true, patternBox);

    // create options
    text = i18n("Flat - set %1 to value").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    text = i18n("Alternating - set %1 to max and min on alternate events").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    text = i18n("Crescendo - set %1 rising from min to max").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    text = i18n("Diminuendo - set %1 falling from max to min").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    text = i18n("Ringing - set %1 alternating from max to min with both dying to zero").arg(strtoqstr(property));
    m_patternCombo->insertItem(text);

    connect(m_patternCombo, SIGNAL(activated(int)),
            this, SLOT(slotPatternSelected(int)));

    QHBox *value1Box = new QHBox(vBox);
    m_value1Label = new QLabel(i18n("Value"), value1Box);
    m_value1Combo = new RosegardenComboBox(true, value1Box);

    QHBox *value2Box = new QHBox(vBox);
    m_value2Label = new QLabel(i18n("Value"), value2Box);
    m_value2Combo = new RosegardenComboBox(true, value2Box);

    for (unsigned int i = 0; i < 128; i++)
    {
        m_value1Combo->insertItem(QString("%1").arg(i));
        m_value2Combo->insertItem(QString("%1").arg(i));
    }
    m_value1Combo->setCurrentItem(127);

    slotPatternSelected(0);

    // start value
    m_value1Combo->setCurrentItem(startValue);
    m_value2Combo->setCurrentItem(startValue);

}

void
EventParameterDialog::slotPatternSelected(int value)
{
    switch(value)
    {
        case 0:
            m_value1Label->setText(i18n("Value"));
            m_value1Label->show();
            m_value1Combo->show();
            m_value2Label->hide();
            m_value2Combo->hide();
            break;

        case 1:
            m_value1Label->setText(i18n("Start Value"));
            m_value2Label->setText(i18n("End Value"));
            m_value1Label->show();
            m_value1Combo->show();
            m_value2Label->show();
            m_value2Combo->show();
            break;

        case 2:
            m_value1Label->setText(i18n("Start Value"));
            m_value2Label->setText(i18n("End Value"));
            m_value1Label->show();
            m_value1Combo->show();
            m_value2Label->show();
            m_value2Combo->show();
            break;

        case 3:
            m_value1Label->setText(i18n("End Value"));
            m_value2Label->setText(i18n("Start Value"));
            m_value1Label->show();
            m_value1Combo->show();
            m_value2Label->show();
            m_value2Combo->show();
            break;

        case 4:
            m_value1Label->setText(i18n("First Value"));
            m_value2Label->setText(i18n("Second Value"));
            m_value1Label->show();
            m_value1Combo->show();
            m_value2Label->show();
            m_value2Combo->show();
            break;

        default:
            RG_DEBUG << "EventParameterDialog::slotPatternSelected - "
                     << "unrecognised pattern number" << endl;
            break;
    }

}

Rosegarden::PropertyPattern
EventParameterDialog::getPattern()
{
    return Rosegarden::PropertyPattern(m_patternCombo->currentItem());
}

int
EventParameterDialog::getValue1()
{
    return m_value1Combo->currentItem();
}

int
EventParameterDialog::getValue2()
{
    return m_value2Combo->currentItem();
}

// --------------------- CompositionLengthDialog ---------------------
//

CompositionLengthDialog::CompositionLengthDialog(
        QWidget *parent,
        Rosegarden::Composition *composition):
        KDialogBase(parent, 0, true, i18n("Change Composition Length"),
                    Ok | Cancel),
        m_composition(composition)
{
    QVBox *vBox = makeVBoxMainWidget();

    new QLabel(i18n("Set the Start and End bar markers for this Composition"),
               vBox);

    QHBox *startBox = new QHBox(vBox);
    new QLabel(i18n("Start Bar"), startBox);
    m_startMarkerSpinBox = new QSpinBox(startBox);
    m_startMarkerSpinBox->setMinValue(-10);
    m_startMarkerSpinBox->setMaxValue(1000);
    m_startMarkerSpinBox->setValue(
            m_composition->getBarNumber(m_composition->getStartMarker()));

    QHBox *endBox = new QHBox(vBox);
    new QLabel(i18n("End Bar"), endBox);
    m_endMarkerSpinBox = new QSpinBox(endBox);
    m_endMarkerSpinBox->setMinValue(-10);
    m_endMarkerSpinBox->setMaxValue(1000);
    m_endMarkerSpinBox->setValue(
            m_composition->getBarNumber(m_composition->getEndMarker()));

}

Rosegarden::timeT
CompositionLengthDialog::getStartMarker()
{
    return m_composition->getBarStart(m_startMarkerSpinBox->value());
}

Rosegarden::timeT
CompositionLengthDialog::getEndMarker()
{
    return m_composition->getBarStart(m_endMarkerSpinBox->value());
}


SplitByPitchDialog::SplitByPitchDialog(QWidget *parent) :
    KDialogBase(parent, 0, true, i18n("Split by Pitch"), Ok | Cancel)
{
    QVBox *vBox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    m_pitch = new RosegardenPitchChooser(i18n("Starting split pitch"), frame, 60);
    layout->addMultiCellWidget(m_pitch, 0, 0, 0, 2, Qt::AlignHCenter);
    
    m_range = new QCheckBox(i18n("Range up and down to follow music"), frame);
    layout->addMultiCellWidget(m_range,
			       1, 1, // fromRow, toRow
			       0, 2  // fromCol, toCol
	);

    m_duplicate = new QCheckBox(i18n("Duplicate non-note events"), frame);
    layout->addMultiCellWidget(m_duplicate, 2, 2, 0, 2);

    layout->addWidget(new QLabel(i18n("Clef handling:"), frame), 3, 0);

    m_clefs = new KComboBox(frame);
    m_clefs->insertItem(i18n("Leave clefs alone"));
    m_clefs->insertItem(i18n("Guess new clefs"));
    m_clefs->insertItem(i18n("Use treble and bass clefs"));
    layout->addMultiCellWidget(m_clefs, 3, 3, 1, 2);

    m_range->setChecked(true);
    m_duplicate->setChecked(true);
    m_clefs->setCurrentItem(2);
}

int
SplitByPitchDialog::getPitch()
{
    return m_pitch->getPitch();
}

bool
SplitByPitchDialog::getShouldRange()
{
    return m_range->isChecked();
}

bool
SplitByPitchDialog::getShouldDuplicateNonNoteEvents()
{
    return m_duplicate->isChecked();
}

int
SplitByPitchDialog::getClefHandling()
{
    switch (m_clefs->currentItem()) {
    case 0:  return (int)SegmentSplitByPitchCommand::LeaveClefs;
    case 1:  return (int)SegmentSplitByPitchCommand::RecalculateClefs;
    default: return (int)SegmentSplitByPitchCommand::UseTrebleAndBassClefs;
    }
}



InterpretDialog::InterpretDialog(QWidget *parent) :
    KDialogBase(parent, 0, true, i18n("Interpret"), Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
	(1, Horizontal, i18n("Interpretations to apply"), vbox);

    m_applyTextDynamics = new QCheckBox
	(i18n("Apply text dynamics (p, mf, ff etc)"), groupBox);
    m_applyHairpins = new QCheckBox
	(i18n("Apply hairpin dynamics"), groupBox);
    m_stressBeats = new QCheckBox
	(i18n("Stress beats"), groupBox);
    m_articulate = new QCheckBox
	(i18n("Articulate slurs, staccato, tenuto etc"), groupBox);
    m_allInterpretations = new QCheckBox
	(i18n("All available interpretations"), groupBox);

    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    
    m_allInterpretations->setChecked
	(config->readBoolEntry("interpretall", true));
    m_applyTextDynamics->setChecked
	(config->readBoolEntry("interprettextdynamics", true));
    m_applyHairpins->setChecked
	(config->readBoolEntry("interprethairpins", true));
    m_stressBeats->setChecked
	(config->readBoolEntry("interpretstressbeats", true));
    m_articulate->setChecked
	(config->readBoolEntry("interpretarticulate", true));

    connect(m_allInterpretations,
	    SIGNAL(clicked()), this, SLOT(slotAllBoxChanged()));

    slotAllBoxChanged();
}

void
InterpretDialog::slotAllBoxChanged()
{
    bool all = m_allInterpretations->isChecked();
    m_applyTextDynamics->setEnabled(!all);
    m_applyHairpins->setEnabled(!all);
    m_stressBeats->setEnabled(!all);
    m_articulate->setEnabled(!all);
}

int
InterpretDialog::getInterpretations()
{
    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
    
    config->writeEntry("interpretall", m_allInterpretations->isChecked());
    config->writeEntry("interprettextdynamics", m_applyTextDynamics->isChecked());
    config->writeEntry("interprethairpins", m_applyHairpins->isChecked());
    config->writeEntry("interpretstressbeats", m_stressBeats->isChecked());
    config->writeEntry("interpretarticulate", m_articulate->isChecked());

    if (m_allInterpretations->isChecked()) {
	return TransformsMenuInterpretCommand::AllInterpretations;
    } else {
	int in = 0;
	if (m_applyTextDynamics->isChecked())
	    in |= TransformsMenuInterpretCommand::ApplyTextDynamics;
	if (m_applyHairpins->isChecked())
	    in |= TransformsMenuInterpretCommand::ApplyHairpins;
	if (m_stressBeats->isChecked()) 
	    in |= TransformsMenuInterpretCommand::StressBeats;
	if (m_articulate->isChecked()) {
	    in |= TransformsMenuInterpretCommand::Articulate;
	}
	return in;
    }
}




ShowSequencerStatusDialog::ShowSequencerStatusDialog(QWidget *parent) :
    KDialogBase(parent, 0, true, i18n("Sequencer status"), Close)
{
    QVBox *vbox = makeVBoxMainWidget();

    new QLabel(i18n("Sequencer status:"), vbox);

    QString status(i18n("Status not available."));

    QCString replyType;
    QByteArray replyData;
    QByteArray data;

    if (!rgapp->sequencerCall("getStatusLog()", replyType, replyData)) {
	status = i18n("Sequencer is not running or is not responding.");
    }
    
    QDataStream streamIn(replyData, IO_ReadOnly);
    QString result;
    streamIn >> result;
    if (!result) {
        status = i18n("Sequencer is not returning a valid status report.");
    } else {
        status = result;
    }

    QTextEdit *text = new QTextEdit(vbox);
    text->setTextFormat(Qt::PlainText);
    text->setReadOnly(true);
    text->setMinimumWidth(500);
    text->setMinimumHeight(200);

    text->setText(status);
}

// ------------------ CountdownDialog -------------------
//
//

CountdownDialog::CountdownDialog(QWidget *parent, int seconds):
    QDialog(parent, "", false, WStyle_StaysOnTop | WStyle_DialogBorder),
    m_totalTime(seconds),
    m_progressBarWidth(150),
    m_progressBarHeight(15)
{
    QBoxLayout *layout = new QBoxLayout(this, QBoxLayout::TopToBottom, 10, 14);
    setCaption(i18n("Recording audio"));

    QHBox *hBox = new QHBox(this);
    m_label = new QLabel(hBox);
    m_time = new QLabel(hBox);

    layout->addWidget(hBox, 0, AlignCenter);

    m_label->setText(i18n("Audio recording time left:  "));
    m_progressBar =
        new CountdownBar(this, m_progressBarWidth, m_progressBarHeight);

    m_progressBar->setFixedSize(m_progressBarWidth, m_progressBarHeight);

    // Simply re-emit from Stop button
    //
    m_stopButton = new QPushButton(i18n("Stop"), this);
    m_stopButton->setFixedWidth(60);

    layout->addWidget(m_progressBar, 0, AlignCenter);
    layout->addWidget(m_stopButton, 0, AlignRight);

    connect (m_stopButton, SIGNAL(released()), this, SIGNAL(stopped()));

    // Set the total time to show the bar in initial position
    //
    setElapsedTime(0);

    m_accelerators = new QAccel(this);

}


void
CountdownDialog::setLabel(const QString &label)
{
    m_label->setText(label);
}

void
CountdownDialog::setTotalTime(int seconds)
{
    m_totalTime = seconds;
    setElapsedTime(0); // clear
}


void
CountdownDialog::setElapsedTime(int elapsedSeconds)
{
    int seconds = m_totalTime - elapsedSeconds;

    if (seconds < 0)
    {
        m_time->setText(i18n("<timing has gone astray>"));
        return;
    }

    QString h, m, s;
    h.sprintf("%02d", seconds/3600);
    m.sprintf("%02d", seconds/60);
    s.sprintf("%02d", seconds % 60);

    if (seconds < 3600) // less than an hour
    {
        m_time->setText(QString("%1:%2").arg(m).arg(s));
    }
    else if (seconds < 86400) // less than a day
    {
        m_time->setText(QString("%1:%2:%3").arg(h).arg(m).arg(s));
    }
    else
    {
        m_time->setText(i18n("Just how big is your hard disk?"));
    }

    // Draw the progress bar
    //
    int barPosition = m_progressBarWidth - 
        (elapsedSeconds * m_progressBarWidth) / m_totalTime;
    m_progressBar->setPosition(barPosition);

    // Dialog complete if the display time is zero
    if (seconds == 0) emit completed();

}


// --- CountdownBar ---
//
CountdownBar::CountdownBar(QWidget *parent, int width, int height):
    QFrame(parent), m_width(width), m_height(height), m_position(0)
{
    resize(m_width, m_height);
    repaint();
}

void
CountdownBar::paintEvent(QPaintEvent *e)
{
    QPainter p(this);

    p.setClipRegion(e->region());
    p.setClipRect(e->rect().normalize());

    p.setPen(RosegardenGUIColours::AudioCountdownBackground);
    p.setBrush(RosegardenGUIColours::AudioCountdownBackground);
    p.drawRect(0, 0, m_position, m_height);
    p.setPen(RosegardenGUIColours::AudioCountdownForeground);
    p.setBrush(RosegardenGUIColours::AudioCountdownForeground);
    p.drawRect(m_position, 0, m_width, m_height);
}

void
CountdownBar::setPosition(int position)
{
    m_position = position;
    repaint();
}


ManageMetronomeDialog::ManageMetronomeDialog(QWidget *parent,
					     RosegardenGUIDoc *doc) :
    KDialogBase(parent, 0, true, i18n("Metronome"), Ok | Apply | Close),
    m_doc(doc)
{
    QHBox *hbox = makeHBoxMainWidget();

    // I think having this as well probably just overcomplicates things
    m_instrumentParameterBox = 0;
//    m_instrumentParameterBox = new InstrumentParameterBox(doc, hbox);

    QVBox *vbox = new QVBox(hbox);

    QGroupBox *deviceBox = new QGroupBox
	(1, Horizontal, i18n("Metronome Instrument"), vbox);

    QFrame *frame = new QFrame(deviceBox);
    QGridLayout *layout = new QGridLayout(frame, 2, 2, 10, 5);

    Rosegarden::Configuration &config = m_doc->getConfiguration();

    layout->addWidget(new QLabel(i18n("Device"), frame), 0, 0);
    m_metronomeDevice = new RosegardenComboBox(frame);
    layout->addWidget(m_metronomeDevice, 0, 1);

    Rosegarden::DeviceList *devices = doc->getStudio().getDevices();
    Rosegarden::DeviceListConstIterator it;

    Rosegarden::DeviceId deviceId = config.get<Int>("metronomedevice", 0);

    for (it = devices->begin(); it != devices->end(); it++)
    {
        Rosegarden::MidiDevice *dev =
            dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev && dev->getDirection() == Rosegarden::MidiDevice::Play)
        {
	    QString label = strtoqstr(dev->getName());
	    QString connection = strtoqstr(dev->getConnection());
	    label += " - ";
	    if (connection == "") label += i18n("No connection");
	    else label += connection;
	    m_metronomeDevice->insertItem(label);
	    if (dev->getId() == deviceId) {
		m_metronomeDevice->setCurrentItem(m_metronomeDevice->count() - 1);
	    }
        }
    }

    layout->addWidget(new QLabel(i18n("Instrument"), frame), 1, 0);
    m_metronomeInstrument = new RosegardenComboBox(frame);
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotSetModified()));
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotInstrumentChanged(int)));
    layout->addWidget(m_metronomeInstrument, 1, 1);

    QGroupBox *beatBox = new QGroupBox
	(1, Horizontal, i18n("Beats"), vbox);

    frame = new QFrame(beatBox);
    layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Resolution"), frame), 0, 0);
    m_metronomeResolution = new RosegardenComboBox(frame);
    m_metronomeResolution->insertItem(i18n("Bars only"));
    m_metronomeResolution->insertItem(i18n("Bars and beats"));
    m_metronomeResolution->insertItem(i18n("Bars, beats, and divisions"));
    connect(m_metronomeResolution, SIGNAL(activated(int)), this, SLOT(slotResolutionChanged(int)));
    layout->addWidget(m_metronomeResolution, 0, 1);

    layout->addWidget(new QLabel(i18n("Bar velocity"), frame), 1, 0);
    m_metronomeBarVely = new QSpinBox(frame);
    m_metronomeBarVely->setMinValue(0);
    m_metronomeBarVely->setMaxValue(127);
    connect(m_metronomeBarVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBarVely, 1, 1);

    layout->addWidget(new QLabel(i18n("Beat velocity"), frame), 2, 0);
    m_metronomeBeatVely = new QSpinBox(frame);
    m_metronomeBeatVely->setMinValue(0);
    m_metronomeBeatVely->setMaxValue(127);
    connect(m_metronomeBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBeatVely, 2, 1);

    layout->addWidget(new QLabel(i18n("Sub-beat velocity"), frame), 3, 0);
    m_metronomeSubBeatVely = new QSpinBox(frame);
    m_metronomeSubBeatVely->setMinValue(0);
    m_metronomeSubBeatVely->setMaxValue(127);
    connect(m_metronomeSubBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeSubBeatVely, 3, 1);

    vbox = new QVBox(hbox);

    m_metronomePitch = new RosegardenPitchChooser(i18n("Pitch"), vbox);
    connect(m_metronomePitch, SIGNAL(pitchChanged(int)), this, SLOT(slotSetModified()));
    connect(m_metronomePitch, SIGNAL(preview(int)), this, SLOT(slotPreviewPitch(int)));

    // populate the dialog
    populate(m_metronomeDevice->currentItem());

    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(populate(int)));
    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(slotSetModified()));

    setModified(false);
}

void
ManageMetronomeDialog::slotResolutionChanged(int depth)
{
    m_metronomeBeatVely->setEnabled(depth > 0);
    m_metronomeSubBeatVely->setEnabled(depth > 1);
    slotSetModified();
}

void
ManageMetronomeDialog::populate(int deviceIndex)
{
    m_metronomeInstrument->clear();

    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    Rosegarden::DeviceListConstIterator it;
    int count = 0;
    Rosegarden::MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev && dev->getDirection() == Rosegarden::MidiDevice::Play)
        {
            if (count == deviceIndex)
                break;

            count++;
        }
    }

    // sanity
    if (count < 0 || dev == 0) {
	if (m_instrumentParameterBox)
	    m_instrumentParameterBox->useInstrument(0);
	return;
    }

    // populate instrument list
    Rosegarden::InstrumentList list = dev->getPresentationInstruments();
    Rosegarden::InstrumentList::iterator iit;

    Rosegarden::MidiMetronome *metronome = dev->getMetronome();

    // if we've got no metronome against this device then create one
    if (metronome == 0)
    {
        Rosegarden::InstrumentId id = Rosegarden::SystemInstrumentBase;

        for (iit = list.begin(); iit != list.end(); ++iit)
        {
            if ((*iit)->isPercussion()) { id = (*iit)->getId(); break; }
        }

        dev->setMetronome(Rosegarden::MidiMetronome(id));

        metronome = dev->getMetronome();
    }

    // metronome should now be set but we still check it
    if (metronome)
    {
        int position = 0;
        int count = 0;
        for (iit = list.begin(); iit != list.end(); ++iit)
        {
	    QString iname(strtoqstr((*iit)->getPresentationName()));
	    QString pname(strtoqstr((*iit)->getProgramName()));
	    if (pname != "") iname += " (" + pname + ")";

	    bool used = false;
	    for (Rosegarden::Composition::trackcontainer::iterator tit =
		     m_doc->getComposition().getTracks().begin();
		 tit != m_doc->getComposition().getTracks().end(); ++tit) {

		if (tit->second->getInstrument() == (*iit)->getId()) {
		    used = true;
		    break;
		}
	    }

//	    if (used) iname = i18n("%1 [used]").arg(iname);

            m_metronomeInstrument->insertItem(iname);
    
            if ((*iit)->getId() == metronome->getInstrument())
            {
                position = count;
            }
            count++;
        }
        m_metronomeInstrument->setCurrentItem(position);
	slotInstrumentChanged(position);

        m_metronomePitch->slotSetPitch(metronome->getPitch());
	m_metronomeResolution->setCurrentItem(metronome->getDepth() - 1);
        m_metronomeBarVely->setValue(metronome->getBarVelocity());
        m_metronomeBeatVely->setValue(metronome->getBeatVelocity());
        m_metronomeSubBeatVely->setValue(metronome->getSubBeatVelocity());
	slotResolutionChanged(metronome->getDepth() - 1);
    }
}

void
ManageMetronomeDialog::slotInstrumentChanged(int i)
{
    if (!m_instrumentParameterBox) return;

    int deviceIndex = m_metronomeDevice->currentItem();

    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    Rosegarden::DeviceListConstIterator it;
    int count = 0;
    Rosegarden::MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev && dev->getDirection() == Rosegarden::MidiDevice::Play)
        {
            if (count == deviceIndex)
                break;

            count++;
        }
    }

    // sanity
    if (count < 0 || dev == 0) {
	m_instrumentParameterBox->useInstrument(0);
	return;
    }

    // populate instrument list
    Rosegarden::InstrumentList list = dev->getPresentationInstruments();

    if (i < 0 || i >= list.size()) return;

    m_instrumentParameterBox->useInstrument(list[i]);
}

void
ManageMetronomeDialog::slotOk()
{
    slotApply();
    accept();
}

void
ManageMetronomeDialog::slotSetModified()
{
    setModified(true);
}

void
ManageMetronomeDialog::setModified(bool value)
{
    if (m_modified == value) return;

    if (value)
    {
        enableButtonApply(true);
    }
    else
    {
        enableButtonApply(false);
    }

    m_modified = value;
}

void
ManageMetronomeDialog::slotApply()
{
    Rosegarden::Configuration &config = m_doc->getConfiguration();

    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    Rosegarden::DeviceListConstIterator it;
    int count = 0;
    Rosegarden::MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev && dev->getDirection() == Rosegarden::MidiDevice::Play)
        {
            if (count == m_metronomeDevice->currentItem())
                break;

            count++;
        }
    }

    if (!dev) {
	std::cerr << "Warning: ManageMetronomeDialog::slotApply: no " << m_metronomeDevice->currentItem() << "th device" << std::endl;
	return;
    }

    Rosegarden::DeviceId deviceId = dev->getId();
    config.set<Int>("metronomedevice", deviceId);

    Rosegarden::MidiMetronome *metronome = dev->getMetronome();
    if (metronome == 0) return;

    // get instrument
    Rosegarden::InstrumentList list = dev->getPresentationInstruments();
    
    Rosegarden::Instrument *inst = 
	list[m_metronomeInstrument->currentItem()];
    
    if (inst)
    {
	metronome->setInstrument(inst->getId());
    }

    metronome->setPitch(
            Rosegarden::MidiByte(m_metronomePitch->getPitch()));

    metronome->setDepth(
	    m_metronomeResolution->currentItem() + 1);

    metronome->setBarVelocity(
            Rosegarden::MidiByte(m_metronomeBarVely->value()));

    metronome->setBeatVelocity(
            Rosegarden::MidiByte(m_metronomeBeatVely->value()));

    metronome->setSubBeatVelocity(
            Rosegarden::MidiByte(m_metronomeSubBeatVely->value()));

    m_doc->getSequenceManager()->metronomeChanged(inst->getId(), true);
    m_doc->slotDocumentModified();
    setModified(false);
}

void
ManageMetronomeDialog::slotPreviewPitch(int pitch)
{
    RG_DEBUG << "ManageMetronomeDialog::slotPreviewPitch: pitch " << pitch << endl;

    Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
    Rosegarden::DeviceListConstIterator it;
    int count = 0;
    Rosegarden::MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++)
    {
        dev = dynamic_cast<Rosegarden::MidiDevice*>(*it);

        if (dev && dev->getDirection() == Rosegarden::MidiDevice::Play)
        {
            if (count == m_metronomeDevice->currentItem())
                break;

            count++;
        }
    }

    if (!dev) return;

    Rosegarden::MidiMetronome *metronome = dev->getMetronome();
    if (metronome == 0) return;

    Rosegarden::InstrumentList list = dev->getPresentationInstruments();
    
    Rosegarden::Instrument *inst = 
	list[m_metronomeInstrument->currentItem()];
    
    if (inst)
    {
    RG_DEBUG << "ManageMetronomeDialog::slotPreviewPitch: instrument " << inst->getId() << endl;

	Rosegarden::MappedEvent *mE = 
	    new Rosegarden::MappedEvent
	    (inst->getId(),
	     Rosegarden::MappedEvent::MidiNoteOneShot,
	     pitch,
	     Rosegarden::MidiMaxValue,
	     Rosegarden::RealTime(0,0),
	     Rosegarden::RealTime(0, 10000),
	     Rosegarden::RealTime(0, 0));

	Rosegarden::StudioControl::sendMappedEvent(mE);
    }
}
