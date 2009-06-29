/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "misc/ConfigGroups.h"
#include "document/io/LilyPondExporter.h"
#include "document/RosegardenDocument.h"
#include "gui/configuration/HeadersConfigurationPage.h"
#include "LilyPondOptionsDialog.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QSettings>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <QLocale>

#include <iostream>


namespace Rosegarden
{

LilyPondOptionsDialog::LilyPondOptionsDialog(QWidget *parent,
	RosegardenDocument *doc,
        QString windowCaption,
        QString heading) :
        QDialog(parent),
	m_doc(doc)
{
    //setHelp("file-printing");

    setModal(true);
    setWindowTitle((windowCaption = "" ? tr("LilyPond Export/Preview") : windowCaption));

    QGridLayout *metaGridLayout = new QGridLayout;

    QWidget *mainbox = new QWidget(this);
    QVBoxLayout *mainboxLayout = new QVBoxLayout;
    metaGridLayout->addWidget(mainbox, 0, 0);

    //
    // Arrange options in "Layout" and "Headers" tabs.
    //

    QTabWidget *tabWidget = new QTabWidget(mainbox);
    mainboxLayout->addWidget(tabWidget);

    QFrame *layoutFrame = new QFrame();
    tabWidget->addTab(layoutFrame, tr("Layout"));

    layoutFrame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layoutGrid = new QGridLayout;
    layoutGrid->setSpacing(5);

    m_headersPage = new HeadersConfigurationPage(this, m_doc);
    tabWidget->addTab(m_headersPage, tr("Headers"));
//     m_headersPage->setSpacing(5);
//     m_headersPage->setMargin(5);
	
	
    //
    // LilyPond export: Basic options
    //

    QGroupBox *basicOptionsBox = new QGroupBox(tr("Basic options"), layoutFrame);
    QVBoxLayout *basicOptionsBoxLayout = new QVBoxLayout;

    layoutGrid->addWidget(basicOptionsBox, 0, 0);

    QFrame *frameBasic = new QFrame(basicOptionsBox);
    frameBasic->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutBasic = new QGridLayout;
    layoutBasic->setSpacing(5);
    basicOptionsBoxLayout->addWidget(frameBasic);

    layoutBasic->addWidget(new QLabel(
                          tr("Export content"), frameBasic), 0, 0);

    m_lilyExportSelection = new QComboBox(frameBasic);
    m_lilyExportSelection->setToolTip(tr("<qt>Choose which tracks to export.</qt>"));
    m_lilyExportSelection->addItem(tr("All tracks"));
    m_lilyExportSelection->addItem(tr("Non-muted tracks"));
    m_lilyExportSelection->addItem(tr("Selected tracks"));
    m_lilyExportSelection->addItem(tr("Selected segments"));

    layoutBasic->addWidget(m_lilyExportSelection, 0, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Compatibility level"), frameBasic), 1, 0);

    m_lilyLanguage = new QComboBox(frameBasic);
    m_lilyLanguage->setToolTip(tr("<qt>Set the LilyPond version you have installed. Rosegarden only caters for stable releases of LilyPond.</qt>"));

    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.6")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.8")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.10")));
    m_lilyLanguage->addItem(tr("LilyPond %1").arg(tr("2.12")));
    layoutBasic->addWidget(m_lilyLanguage, 1, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Paper size"), frameBasic), 2, 0);

    QHBoxLayout *hboxPaper = new QHBoxLayout;
    m_lilyPaperSize = new QComboBox(frameBasic);
    m_lilyPaperSize->setToolTip(tr("<qt>Set the paper size.</qt>"));
    m_lilyPaperSize->addItem(tr("A3"));
    m_lilyPaperSize->addItem(tr("A4"));
    m_lilyPaperSize->addItem(tr("A5"));
    m_lilyPaperSize->addItem(tr("A6"));
    m_lilyPaperSize->addItem(tr("Legal"));
    m_lilyPaperSize->addItem(tr("US Letter"));
    m_lilyPaperSize->addItem(tr("Tabloid"));
    m_lilyPaperSize->addItem(tr("do not specify"));

    m_lilyPaperLandscape = new QCheckBox(tr("Landscape"), frameBasic);
    m_lilyPaperLandscape->setToolTip(tr("<qt>Check this if you want your score set in landscape.</qt>"));

    hboxPaper->addWidget(m_lilyPaperSize);
    hboxPaper->addWidget(new QLabel(" ", frameBasic)); // fixed-size spacer
    hboxPaper->addWidget(m_lilyPaperLandscape);
    layoutBasic->addLayout(hboxPaper, 2, 1);

    layoutBasic->addWidget(new QLabel(
                          tr("Font size"), frameBasic), 3, 0);

    m_lilyFontSize = new QComboBox(frameBasic);
    m_lilyFontSize->setToolTip(tr("<qt>Choose the font size of the score.</qt>"));
    for (unsigned int i = 0; i < 24; i++) {
        bool recommended = false;
        int printSize = i + FONT_OFFSET;
        switch (printSize) {
            case 11:
            case 13:
            case 16:
            case 19:
            case 20:
            case 23:
            case 26: recommended = true; break;
            default: recommended = false;
        }
        QString fontString = tr("%1 pt %2").arg(printSize).arg(recommended ? tr("(recommended)") : "");
        m_lilyFontSize->addItem(fontString);
    }
    layoutBasic->addWidget(m_lilyFontSize, 3, 1);

    //
    // LilyPond export: Notation options
    //

    QGroupBox *specificOptionsBox = new QGroupBox(tr("Specific options"), layoutFrame);
    QVBoxLayout *specificOptionsBoxLayout = new QVBoxLayout;
    layoutGrid->addWidget(specificOptionsBox, 2, 0);

    QFrame *frameNotation = new QFrame(specificOptionsBox);
    frameNotation->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layoutNotation = new QGridLayout;
    layoutNotation->setSpacing(5);
    specificOptionsBoxLayout->addWidget(frameNotation);

    m_lilyTempoMarks = new QComboBox(frameNotation);
    m_lilyTempoMarks->addItem(tr("None"));
    m_lilyTempoMarks->addItem(tr("First"));
    m_lilyTempoMarks->addItem(tr("All"));

    layoutNotation->addWidget(new QLabel(
			 tr("Export tempo marks "), frameNotation), 0, 0);
    layoutNotation->addWidget(m_lilyTempoMarks, 0, 1);
    m_lilyTempoMarks->setToolTip(tr("<qt>Choose how often to show Tempo Marks in your score.</qt>"));

    layoutNotation->addWidget(new QLabel(
			 tr("Export lyrics"), frameNotation), 1, 0);
    m_lilyExportLyrics = new QComboBox(frameNotation);
    m_lilyExportLyrics->addItem(tr("None"));
    m_lilyExportLyrics->addItem(tr("Left"));
    m_lilyExportLyrics->addItem(tr("Center"));
    m_lilyExportLyrics->addItem(tr("Right"));
    layoutNotation->addWidget(m_lilyExportLyrics, 1, 1);
    m_lilyExportLyrics->setToolTip(tr("<qt>Set the position of the <b>lyrics/words</b> in relation to the notes.</qt>"));


    m_lilyExportBeams = new QCheckBox(
                            tr("Export beamings"), frameNotation);
    layoutNotation->addWidget(m_lilyExportBeams, 2, 0, 0+1, 1- 0+1);
    m_lilyExportBeams->setToolTip(tr("<qt>Check to export Rosegarden beamings to your score.</qt>"));

    // recycle this for a new option to ignore the track brackets (so it is less
    // obnoxious to print single parts where brackets are in place)
    m_lilyExportStaffGroup = new QCheckBox(
                                 tr("Export track staff brackets"), frameNotation);
    layoutNotation->addWidget(m_lilyExportStaffGroup, 3, 0, 0+1, 1- 0+1);
    m_lilyExportStaffGroup->setToolTip(tr("<qt>If you have only one staff in this score, you wouldn't want to export brackets.</qt>"));

    layoutGrid->setRowStretch(4, 10);

    m_lilyChordNamesMode = new QCheckBox(
                           tr("Interpret chord texts as lead sheet chord names"), frameNotation);
    layoutNotation->addWidget(m_lilyChordNamesMode, 4, 0, 0+1, 1- 0+1);
    m_lilyChordNamesMode->setToolTip(tr("<qt><p>There is a tutorial on how to use this feature at http://www.rosegardenmusic.com/tutorials/supplemental/chordnames/index.html</p></qt>"));

    m_lilyRaggedBottom = new QCheckBox(
                           tr("Ragged bottom (systems will not be spread vertically across the page)"), frameNotation);
    layoutNotation->addWidget(m_lilyRaggedBottom, 5, 0, 0+1, 1- 0+1);
    m_lilyRaggedBottom->setToolTip(tr("<qt><p>Useful for multipage scores: this will prevent ugly final pages.</p></qt>"));

    m_lilyMarkerMode = new QComboBox(frameNotation);
    m_lilyMarkerMode->addItem(tr("No markers"));
    m_lilyMarkerMode->addItem(tr("Rehearsal marks"));
    m_lilyMarkerMode->addItem(tr("Marker text"));

    layoutNotation->addWidget(new QLabel(
                                   tr("Export markers"), frameNotation),6, 0);
    layoutNotation->addWidget(m_lilyMarkerMode, 6, 1);
    m_lilyMarkerMode->setToolTip(tr("<qt>Set different score mark options.</qt>"));

    basicOptionsBox->setLayout(basicOptionsBoxLayout);
    specificOptionsBox->setLayout(specificOptionsBoxLayout);

    layoutFrame->setLayout(layoutGrid);

    frameNotation->setLayout(layoutNotation);
    frameBasic->setLayout(layoutBasic);

    mainbox->setLayout(mainboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metaGridLayout->addWidget(buttonBox, 1, 0);
    metaGridLayout->setRowStretch(0, 10);

    setLayout(metaGridLayout);


    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    populateDefaultValues();

    resize(minimumSizeHint());
}

void
LilyPondOptionsDialog::populateDefaultValues()
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    m_lilyLanguage->setCurrentIndex(settings.value("lilylanguage", 0).toUInt());
    // See also setDefaultLilyPondVersion below
    int defaultPaperSize = 1; // A4
    if (QLocale::system().country() == QLocale::UnitedStates) {
        defaultPaperSize = 5; // Letter
    }
    m_lilyPaperSize->setCurrentIndex(settings.value("lilypapersize", defaultPaperSize).toUInt());
    m_lilyPaperLandscape->setChecked(qStrToBool(settings.value("lilypaperlandscape", "false")));
    m_lilyFontSize->setCurrentIndex(settings.value("lilyfontsize", FONT_20).toUInt());
    m_lilyRaggedBottom->setChecked(qStrToBool(settings.value("lilyraggedbottom", "false")));
    m_lilyChordNamesMode->setChecked(qStrToBool(settings.value("lilychordnamesmode", "false")));
    m_lilyExportLyrics->setCurrentIndex(settings.value("lilyexportlyrics", 1).toUInt());
    m_lilyTempoMarks->setCurrentIndex(settings.value("lilyexporttempomarks", 0).toUInt());
    m_lilyExportSelection->setCurrentIndex(settings.value("lilyexportselection", 1).toUInt());
    m_lilyExportBeams->setChecked(settings.value("lilyexportbeamings", "false").toBool());
    m_lilyExportStaffGroup->setChecked(settings.value("lilyexportstaffbrackets", "true").toBool());
    m_lilyMarkerMode->setCurrentIndex(settings.value("lilyexportmarkermode", 0).toUInt());

    std::cerr << "QSettings for LilyPond (populateDefaultValues):" << std::endl
              << "  lilyexportmarkermode: " << settings.value("lilyexportmarkermode").toUInt() << std::endl
              << "  lilyraggedbottom: " << (settings.value("lilyraggedbottom").toBool() ? "true" : "false") << std::endl
              << std::endl;

    settings.endGroup();
}


void
LilyPondOptionsDialog::slotApply()
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    settings.setValue("lilylanguage", m_lilyLanguage->currentIndex());
    settings.setValue("lilypapersize", m_lilyPaperSize->currentIndex());
    settings.setValue("lilypaperlandscape", m_lilyPaperLandscape->isChecked());
    settings.setValue("lilyfontsize", m_lilyFontSize->currentIndex());
    settings.setValue("lilyraggedbottom", m_lilyRaggedBottom->isChecked());
    settings.setValue("lilychordnamesmode", m_lilyChordNamesMode->isChecked());
    settings.setValue("lilyexportlyrics", m_lilyExportLyrics->currentIndex());
    settings.setValue("lilyexporttempomarks", m_lilyTempoMarks->currentIndex());
    settings.setValue("lilyexportselection", m_lilyExportSelection->currentIndex());
    settings.setValue("lilyexportbeamings", m_lilyExportBeams->isChecked());
    settings.setValue("lilyexportstaffbrackets", m_lilyExportStaffGroup->isChecked());
    settings.setValue("lilyexportmarkermode", m_lilyMarkerMode->currentIndex());

    std::cerr << "QSettings for LilyPond (slotApply):" << std::endl
              << "  lilyexportmarkermode: " << settings.value("lilyexportmarkermode").toUInt() << std::endl
              << "  lilyraggedbottom: " << (settings.value("lilyraggedbottom").toBool() ? "true" : "false") << std::endl
              << std::endl;

    settings.endGroup();

    m_headersPage->apply();
}
 
void
LilyPondOptionsDialog::accept()
{
    slotApply();
    QDialog::accept();
}

void
LilyPondOptionsDialog::setDefaultLilyPondVersion(QString version)
{
    QSettings settings;
    settings.beginGroup(LilyPondExportConfigGroup);

    int index = -1;
    bool unstable = false;
    if (version == "2.6" || version.startsWith("2.6.")) {
        index = 0;
    } else if (version == "2.7" || version.startsWith("2.7.")) {
        unstable = true;
        index = 1;
    } else if (version == "2.8" || version.startsWith("2.8.")) {
        index = 1;
    } else if (version == "2.9" || version.startsWith("2.9.")) {
        unstable = true;
        index = 2;
    } else if (version == "2.10" || version.startsWith("2.10.")) {
        index = 2;
    } else if (version == "2.11" || version.startsWith("2.11.")) {
        unstable = true;
        index = 3;
    } else if (version == "2.12" || version.startsWith("2.12.")) {
        index = 3;
    }
    if (unstable) {
        std::cerr << "\nWARNING: Unstable LilyPond version detected, selecting next language version up\n" << std::endl;
    }
    if (index >= 0) {
        settings.setValue("lilylanguage", index);
    }

    settings.endGroup();
}

}
#include "LilyPondOptionsDialog.moc"
