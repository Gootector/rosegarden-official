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

#ifndef _ROSEGARDENCONFIGUREPAGE_H_
#define _ROSEGARDENCONFIGUREPAGE_H_

#include <qspinbox.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlineedit.h>

#include <klocale.h>

#include <string>

class RosegardenGUIDoc;
class QTabWidget;
class QLineEdit;
class QPushButton;
class QLabel;

namespace Rosegarden
{

/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    ConfigurationPage(RosegardenGUIDoc *doc,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(doc), m_cfg(0), m_pageIndex(0) {}

    ConfigurationPage(KConfig *cfg,
                      QWidget *parent=0, const char *name=0)
        : QWidget(parent, name), m_doc(0), m_cfg(cfg), m_pageIndex(0) {}

    virtual ~ConfigurationPage() {};

    /**
     * Should set the page up (ie. read the setting from the @ref
     * KConfig object into the widgets) after creating it in the
     * constructor. Called from @ref ConfigureDialog.
    */
//     virtual void setup() = 0;

    /**
     * Should apply the changed settings (ie. read the settings from
     * the widgets into the @ref KConfig object). Called from @ref
     * ConfigureDialog.
     */
    virtual void apply() = 0;

    /**
     * Should cleanup any temporaries after cancel. The default
     * implementation does nothing. Called from @ref
     * ConfigureDialog.
     */
    virtual void dismiss() {}

    void setPageIndex( int aPageIndex ) { m_pageIndex = aPageIndex; }
    int pageIndex() const { return m_pageIndex; }

protected:

    //--------------- Data members ---------------------------------

    RosegardenGUIDoc* m_doc;
    KConfig* m_cfg;

    int m_pageIndex;
};

/**
 * This class borrowed from KMail
 * (c) 2000 The KMail Development Team
 */
class TabbedConfigurationPage : public ConfigurationPage
{
    Q_OBJECT

public:
    TabbedConfigurationPage(RosegardenGUIDoc *doc,
                            QWidget *parent=0, const char *name=0);

    TabbedConfigurationPage(KConfig *cfg,
                            QWidget *parent=0, const char *name=0);

    static QString iconName() { return "misc"; }
    
protected:
    void init();
    void addTab(QWidget *tab, const QString &title);

    //--------------- Data members ---------------------------------

    QTabWidget *m_tabWidget;

};

/**
 * General Rosegarden Configuration page
 *
 * (application-wide settings)
 */
class GeneralConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    enum DoubleClickClient
    {
        NotationView,
        MatrixView
    };

    GeneralConfigurationPage(KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("General"); }
    static QString title()     { return i18n("General Configuration"); }

    int getCountInSpin()            { return m_countIn->value(); }
    int getDblClickClient()         { return m_client->currentItem(); }
    int getMIDIPitch2StringOffset() { return m_midiPitchOffset->value(); }
    QString getExternalAudioEditor() { return m_externalAudioEditorPath->text(); }

public slots:
    void slotFileDialog();


protected:
    QComboBox* m_client;
    QSpinBox* m_countIn;
    QSpinBox* m_midiPitchOffset;
    QLineEdit* m_externalAudioEditorPath;
};

/**
 * Notation Configuration page
 */
class NotationConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT;

public:
    NotationConfigurationPage(KConfig *cfg,
			      QWidget *parent = 0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Notation"); }
    static QString title()     { return i18n("Notation"); }

public slots:
    void slotFontComboChanged(const QString &);

protected:
    QComboBox *m_font;
    QComboBox *m_singleStaffSize;
    QComboBox *m_multiStaffSize;
    QLabel *m_fontOriginLabel;
    QLabel *m_fontCopyrightLabel;
    QLabel *m_fontMappedByLabel;
    QLabel *m_fontTypeLabel;

    void populateSizeCombo(QComboBox *combo, std::string font, int dfltSize);
};


/**
 * Latency Configuration page
 *
 * (application-wide settings)
 */
class LatencyConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    LatencyConfigurationPage(RosegardenGUIDoc *doc,
                             KConfig *cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Latency"); }
    static QString title()     { return i18n("Sequencer Latency"); }

    int getReadAheadValue() { return m_readAhead->value(); }
    int getPlaybackValue()  { return m_playback->value(); }

    int getJACKPlaybackValue() { return m_jackPlayback->value(); }
    int getJACKRecordValue() { return m_jackRecord->value(); }

public slots:
    // Get the latest latency values from the sequencer
    //
    void slotFetchLatencyValues();

protected:
    QSlider* m_readAhead;
    QSlider* m_playback;

    QSlider* m_jackPlayback;
    QSlider* m_jackRecord;

    QPushButton* m_fetchLatencyValues;

    RosegardenGUIDoc *m_doc;
};


/**
 * Document Meta-information page
 *
 * (document-wide settings)
 */
class DocumentMetaConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    DocumentMetaConfigurationPage(RosegardenGUIDoc *doc,
				  QWidget *parent = 0, const char *name = 0);
    virtual void apply();

    static QString iconLabel() { return i18n("About"); }
    static QString title() { return i18n("About"); }

public slots:
    
protected:
    QLineEdit *m_copyright;
};


/**
 * Audio Configuration page
 *
 * (document-wide settings)
 */
class AudioConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioConfigurationPage(RosegardenGUIDoc *doc,
                           QWidget *parent=0, const char *name=0);
    virtual void apply();

    static QString iconLabel() { return i18n("Audio"); }
    static QString title()     { return i18n("Audio Settings"); }

public slots:
    void slotFileDialog();

protected:
    QLineEdit        *m_path;
    QPushButton      *m_changePathButton;
};

 
}

#endif // _ROSEGARDENCONFIGUREPAGE_H_
