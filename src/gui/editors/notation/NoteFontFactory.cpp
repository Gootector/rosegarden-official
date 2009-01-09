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


#include "NoteFontFactory.h"
#include "misc/Debug.h"
#include <QApplication>

#include <klocale.h>
#include <QDir>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Exception.h"
#include "gui/kdeext/KStartupLogo.h"
#include "NoteFont.h"
#include "NoteFontMap.h"
#include <QSettings>
#include "gui/general/ResourceFinder.h"
#include <QMessageBox>
#include <QDir>
#include <QString>
#include <QStringList>
#include <algorithm>


namespace Rosegarden
{

std::set<std::string>
NoteFontFactory::getFontNames(bool forceRescan)
{
    NOTATION_DEBUG << "NoteFontFactory::getFontNames: forceRescan = " << forceRescan << endl;

    if (forceRescan)
        m_fontNames.clear();
    if (!m_fontNames.empty())
        return m_fontNames;

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    QString fontNameList = "";
    if (!forceRescan) {
        fontNameList = settings.value("notefontlist", "").toString();
    }
    settings.endGroup();

    NOTATION_DEBUG << "NoteFontFactory::getFontNames: read from cache: " << fontNameList << endl;

    QStringList names = QStringList::split(",", fontNameList);

    ResourceFinder rf;

    if (names.empty()) {

        NOTATION_DEBUG << "NoteFontFactory::getFontNames: No names available, rescanning..." << endl;

        QStringList files = rf.getResourceFiles("fonts/mappings", "xml");

        for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {

            QString filepath = *i;
            QString qname = QFileInfo(filepath).baseName();
            std::string name = qstrtostr(qname);

            try {
                NoteFontMap map(name);
                if (map.ok()) names.append(strtoqstr(map.getName()));
            } catch (Exception e) {
                KStartupLogo::hideIfStillThere();
                QMessageBox::critical(0, "", strtoqstr(e.getMessage()));
                throw;
            }
        }
    }

    QString savedNames = "";

    for (QStringList::Iterator i = names.begin(); i != names.end(); ++i) {
        m_fontNames.insert(qstrtostr(*i));
        if (i != names.begin()) savedNames += ",";
        savedNames += *i;
    }

    settings.beginGroup( NotationViewConfigGroup );
    settings.setValue("notefontlist", savedNames);
    settings.endGroup();

    return m_fontNames;
}

std::vector<int>
NoteFontFactory::getAllSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font)
        return std::vector<int>();

    std::set
        <int> s(font->getSizes());
    std::vector<int> v;
    for (std::set
                <int>::iterator i = s.begin(); i != s.end(); ++i) {
            v.push_back(*i);
        }

    std::sort(v.begin(), v.end());
    return v;
}

std::vector<int>
NoteFontFactory::getScreenSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font)
        return std::vector<int>();

    std::set
        <int> s(font->getSizes());
    std::vector<int> v;
    for (std::set
                <int>::iterator i = s.begin(); i != s.end(); ++i) {
            if (*i <= 16)
                v.push_back(*i);
        }
    std::sort(v.begin(), v.end());
    return v;
}

NoteFont *
NoteFontFactory::getFont(std::string fontName, int size)
{
    std::map<std::pair<std::string, int>, NoteFont *>::iterator i =
        m_fonts.find(std::pair<std::string, int>(fontName, size));

    if (i == m_fonts.end()) {
        try {
            NoteFont *font = new NoteFont(fontName, size);
            m_fonts[std::pair<std::string, int>(fontName, size)] = font;
            return font;
        } catch (Exception e) {
            KStartupLogo::hideIfStillThere();
            QMessageBox::critical(0, "", strtoqstr(e.getMessage()));
            throw;
        }
    } else {
        return i->second;
    }
}

std::string
NoteFontFactory::getDefaultFontName()
{
    static std::string defaultFont = "";
    if (defaultFont != "")
        return defaultFont;

    std::set
        <std::string> fontNames = getFontNames();

    if (fontNames.find("Feta") != fontNames.end())
        defaultFont = "Feta";
    else {
        fontNames = getFontNames(true);
        if (fontNames.find("Feta") != fontNames.end())
            defaultFont = "Feta";
        else if (fontNames.find("Feta Pixmaps") != fontNames.end())
            defaultFont = "Feta Pixmaps";
        else if (fontNames.size() > 0)
            defaultFont = *fontNames.begin();
        else {
            QString message = i18n("Can't obtain a default font -- no fonts found");
            KStartupLogo::hideIfStillThere();
            QMessageBox::critical(0, "", message);
            throw NoFontsAvailable(qstrtostr(message));
        }
    }

    return defaultFont;
}

int
NoteFontFactory::getDefaultSize(std::string fontName)
{
    // always return 8 if it's supported!
    std::vector<int> sizes(getScreenSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == 8)
            return sizes[i];
    }
    return sizes[sizes.size() / 2];
}

bool
NoteFontFactory::isAvailableInSize(std::string fontName, int size)
{
    std::vector<int> sizes(getAllSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size)
            return true;
    }
    return false;
}

std::set<std::string> NoteFontFactory::m_fontNames;
std::map<std::pair<std::string, int>, NoteFont *> NoteFontFactory::m_fonts;

}
