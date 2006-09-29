/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "NoteFontMap.h"
#include "misc/Debug.h"

#include <klocale.h>
#include <kstddirs.h>
#include "misc/Strings.h"
#include "base/Exception.h"
#include "SystemFont.h"
#include <kglobal.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qpixmap.h>
#include <qregexp.h>
#include <qstring.h>
#include <qstringlist.h>


namespace Rosegarden
{

NoteFontMap::NoteFontMap(string name) :
        m_name(name),
        m_smooth(false),
        m_srcDirectory(name),
        m_characterDestination(0),
        m_hotspotCharName(""),
        m_errorString(i18n("unknown error")),
        m_ok(true)
{
    m_fontDirectory = KGlobal::dirs()->findResource("appdata", "fonts/");

    QString mapFileName;

    QString mapFileMixedName = QString("%1/mappings/%2.xml")
                               .arg(m_fontDirectory)
                               .arg(strtoqstr(name));

    QFileInfo mapFileMixedInfo(mapFileMixedName);

    if (!mapFileMixedInfo.isReadable()) {

        QString lowerName = strtoqstr(name).lower();
        lowerName.replace(QRegExp(" "), "_");
        QString mapFileLowerName = QString("%1/mappings/%2.xml")
                                   .arg(m_fontDirectory)
                                   .arg(lowerName);

        QFileInfo mapFileLowerInfo(mapFileLowerName);

        if (!mapFileLowerInfo.isReadable()) {
            if (mapFileLowerName != mapFileMixedName) {
                throw MappingFileReadFailed
                (qstrtostr(i18n("Can't open font mapping file %1 or %2").
                           arg(mapFileMixedName).arg(mapFileLowerName)));
            } else {
                throw MappingFileReadFailed
                (qstrtostr(i18n("Can't open font mapping file %1").
                           arg(mapFileMixedName)));
            }
        } else {
            mapFileName = mapFileLowerName;
        }
    } else {
        mapFileName = mapFileMixedName;
    }

    QFile mapFile(mapFileName);

    QXmlInputSource source(mapFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    mapFile.close();

    if (!ok) {
        throw MappingFileReadFailed(qstrtostr(m_errorString));
    }
}

NoteFontMap::~NoteFontMap()
{
    for (SystemFontMap::iterator i = m_systemFontCache.begin();
            i != m_systemFontCache.end(); ++i) {
        delete i->second;
    }
}

bool
NoteFontMap::characters(QString &chars)
{
    if (!m_characterDestination)
        return true;
    *m_characterDestination += qstrtostr(chars);
    return true;
}

int
NoteFontMap::toSize(int baseSize, double factor, bool limitAtOne)
{
    double dsize = factor * baseSize;
    dsize += 0.5;
    if (limitAtOne && dsize < 1.0)
        dsize = 1.0;
    return int(dsize);
}

bool
NoteFontMap::startElement(const QString &, const QString &,
                          const QString &qName,
                          const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();
    m_characterDestination = 0;

    // The element names are actually unique within the whole file;
    // we don't bother checking we're in the right context.  Leave that
    // to the DTD, when we have one.

    if (lcName == "rosegarden-font-encoding") {

        QString s;

        s = attributes.value("name");
        if (s) {
            m_name = qstrtostr(s);
            m_srcDirectory = m_name;
        }

    } else if (lcName == "font-information") {

        QString s;

        s = attributes.value("origin");
        if (s)
            m_origin = qstrtostr(s);

        s = attributes.value("copyright");
        if (s)
            m_copyright = qstrtostr(s);

        s = attributes.value("mapped-by");
        if (s)
            m_mappedBy = qstrtostr(s);

        s = attributes.value("type");
        if (s)
            m_type = qstrtostr(s);

        s = attributes.value("autocrop");
        if (s) {
            cerr << "Warning: autocrop attribute in note font mapping file is no longer supported\n(all fonts are now always autocropped)" << endl;
        }

        s = attributes.value("smooth");
        if (s)
            m_smooth = (s.lower() == "true");

    } else if (lcName == "font-sizes") {
    }
    else if (lcName == "font-size") {

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = "note-height is a required attribute of font-size";
            return false;
        }
        int noteHeight = s.toInt();

        SizeData &sizeData = m_sizes[noteHeight];

        s = attributes.value("staff-line-thickness");
        if (s)
            sizeData.setStaffLineThickness(s.toInt());

        s = attributes.value("leger-line-thickness");
        if (s)
            sizeData.setLegerLineThickness(s.toInt());

        s = attributes.value("stem-thickness");
        if (s)
            sizeData.setStemThickness(s.toInt());

        s = attributes.value("beam-thickness");
        if (s)
            sizeData.setBeamThickness(s.toInt());

        s = attributes.value("stem-length");
        if (s)
            sizeData.setStemLength(s.toInt());

        s = attributes.value("flag-spacing");
        if (s)
            sizeData.setFlagSpacing(s.toInt());

        s = attributes.value("border-x");
        if (s) {
            cerr << "Warning: border-x attribute in note font mapping file is no longer supported\n(use hotspot-x for note head or flag)" << endl;
        }

        s = attributes.value("border-y");
        if (s) {
            cerr << "Warning: border-y attribute in note font mapping file is no longer supported" << endl;
        }

        int fontId = 0;
        s = attributes.value("font-id");
        if (s)
            fontId = s.toInt();

        s = attributes.value("font-height");
        if (s)
            sizeData.setFontHeight(fontId, s.toInt());

    } else if (lcName == "font-scale") {

        double fontHeight = -1.0;
        double beamThickness = -1.0;
        double stemLength = -1.0;
        double flagSpacing = -1.0;
        double staffLineThickness = -1.0;
        double legerLineThickness = -1.0;
        double stemThickness = -1.0;

        QString s;

        s = attributes.value("font-height");
        if (s)
            fontHeight = qstrtodouble(s);
        else {
            m_errorString = "font-height is a required attribute of font-scale";
            return false;
        }

        s = attributes.value("staff-line-thickness");
        if (s)
            staffLineThickness = qstrtodouble(s);

        s = attributes.value("leger-line-thickness");
        if (s)
            legerLineThickness = qstrtodouble(s);

        s = attributes.value("stem-thickness");
        if (s)
            stemThickness = qstrtodouble(s);

        s = attributes.value("beam-thickness");
        if (s)
            beamThickness = qstrtodouble(s);

        s = attributes.value("stem-length");
        if (s)
            stemLength = qstrtodouble(s);

        s = attributes.value("flag-spacing");
        if (s)
            flagSpacing = qstrtodouble(s);

        int fontId = 0;
        s = attributes.value("font-id");
        if (s)
            fontId = s.toInt();

        //!!! need to be able to calculate max size -- checkFont needs
        //to take a size argument; unfortunately Qt doesn't seem to be
        //able to report to us when a scalable font was loaded in the
        //wrong size, so large sizes might be significantly inaccurate
        //as it just stops scaling up any further at somewhere around
        //120px.  We could test whether the metric for the black
        //notehead is noticeably smaller than the notehead should be,
        //and reject if so?  [update -- no, that doesn't work either,
        //Qt just returns the correct metric even if drawing the
        //incorrect size]

        for (int sz = 1; sz <= 30; sz += (sz == 1 ? 1 : 2)) {

            SizeData & sizeData = m_sizes[sz];
            unsigned int temp;

            if (sizeData.getStaffLineThickness(temp) == false &&
                    staffLineThickness >= 0.0)
                sizeData.setStaffLineThickness(toSize(sz, staffLineThickness, true));

            if (sizeData.getLegerLineThickness(temp) == false &&
                    legerLineThickness >= 0.0)
                sizeData.setLegerLineThickness(toSize(sz, legerLineThickness, true));

            if (sizeData.getStemThickness(temp) == false &&
                    stemThickness >= 0.0)
                sizeData.setStemThickness(toSize(sz, stemThickness, true));

            if (sizeData.getBeamThickness(temp) == false &&
                    beamThickness >= 0.0)
                sizeData.setBeamThickness(toSize(sz, beamThickness, true));

            if (sizeData.getStemLength(temp) == false &&
                    stemLength >= 0.0)
                sizeData.setStemLength(toSize(sz, stemLength, true));

            if (sizeData.getFlagSpacing(temp) == false &&
                    flagSpacing >= 0.0)
                sizeData.setFlagSpacing(toSize(sz, flagSpacing, true));

            if (sizeData.getFontHeight(fontId, temp) == false)
                sizeData.setFontHeight(fontId, toSize(sz, fontHeight, true));
        }

    } else if (lcName == "font-symbol-map") {
    }
    else if (lcName == "src-directory") {

        QString d = attributes.value("name");
        if (!d) {
            m_errorString = "name is a required attribute of src-directory";
            return false;
        }

        m_srcDirectory = qstrtostr(d);

    } else if (lcName == "codebase") {

        int bn = 0, fn = 0;
        bool ok;
        QString base = attributes.value("base");
        if (!base) {
            m_errorString = "base is a required attribute of codebase";
            return false;
        }
        bn = base.toInt(&ok);
        if (!ok || bn < 0) {
            m_errorString =
                QString("invalid base attribute \"%1\" (must be integer >= 0)").
                arg(base);
            return false;
        }

        QString fontId = attributes.value("font-id");
        if (!fontId) {
            m_errorString = "font-id is a required attribute of codebase";
            return false;
        }
        fn = fontId.stripWhiteSpace().toInt(&ok);
        if (!ok || fn < 0) {
            m_errorString =
                QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
                arg(fontId);
            return false;
        }

        m_bases[fn] = bn;

    } else if (lcName == "symbol") {

        QString symbolName = attributes.value("name");
        if (!symbolName) {
            m_errorString = "name is a required attribute of symbol";
            return false;
        }
        SymbolData symbolData;

        QString src = attributes.value("src");
        QString code = attributes.value("code");
        QString glyph = attributes.value("glyph");

        int icode = -1;
        bool ok = false;
        if (code) {
            icode = code.stripWhiteSpace().toInt(&ok);
            if (!ok || icode < 0) {
                m_errorString =
                    QString("invalid code attribute \"%1\" (must be integer >= 0)").
                    arg(code);
                return false;
            }
            symbolData.setCode(icode);
        }

        int iglyph = -1;
        ok = false;
        if (glyph) {
            iglyph = glyph.stripWhiteSpace().toInt(&ok);
            if (!ok || iglyph < 0) {
                m_errorString =
                    QString("invalid glyph attribute \"%1\" (must be integer >= 0)").
                    arg(glyph);
                return false;
            }
            symbolData.setGlyph(iglyph);
        }

        if (!src && icode < 0 && iglyph < 0) {
            m_errorString = "symbol must have either src, code, or glyph attribute";
            return false;
        }
        if (src)
            symbolData.setSrc(qstrtostr(src));

        QString inversionSrc = attributes.value("inversion-src");
        if (inversionSrc)
            symbolData.setInversionSrc(qstrtostr(inversionSrc));

        QString inversionCode = attributes.value("inversion-code");
        if (inversionCode) {
            icode = inversionCode.stripWhiteSpace().toInt(&ok);
            if (!ok || icode < 0) {
                m_errorString =
                    QString("invalid inversion code attribute \"%1\" (must be integer >= 0)").
                    arg(inversionCode);
                return false;
            }
            symbolData.setInversionCode(icode);
        }

        QString inversionGlyph = attributes.value("inversion-glyph");
        if (inversionGlyph) {
            iglyph = inversionGlyph.stripWhiteSpace().toInt(&ok);
            if (!ok || iglyph < 0) {
                m_errorString =
                    QString("invalid inversion glyph attribute \"%1\" (must be integer >= 0)").
                    arg(inversionGlyph);
                return false;
            }
            symbolData.setInversionGlyph(iglyph);
        }

        QString fontId = attributes.value("font-id");
        if (fontId) {
            int n = fontId.stripWhiteSpace().toInt(&ok);
            if (!ok || n < 0) {
                m_errorString =
                    QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
                    arg(fontId);
                return false;
            }
            symbolData.setFontId(n);
        }

        m_data[qstrtostr(symbolName.upper())] = symbolData;

    } else if (lcName == "font-hotspots") {
    }
    else if (lcName == "hotspot") {

        QString s = attributes.value("name");
        if (!s) {
            m_errorString = "name is a required attribute of hotspot";
            return false;
        }
        m_hotspotCharName = qstrtostr(s.upper());

    } else if (lcName == "scaled") {

        if (m_hotspotCharName == "") {
            m_errorString = "scaled-element must be in hotspot-element";
            return false;
        }

        QString s = attributes.value("x");
        double x = -1.0;
        if (s)
            x = qstrtodouble(s);

        s = attributes.value("y");
        if (!s) {
            m_errorString = "y is a required attribute of scaled";
            return false;
        }
        double y = qstrtodouble(s);

        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

        i->second.setScaledHotspot(x, y);

    } else if (lcName == "fixed") {

        if (m_hotspotCharName == "") {
            m_errorString = "fixed-element must be in hotspot-element";
            return false;
        }

        QString s = attributes.value("x");
        int x = 0;
        if (s)
            x = s.toInt();

        s = attributes.value("y");
        int y = 0;
        if (s)
            y = s.toInt();

        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

        i->second.addHotspot(0, x, y);

    } else if (lcName == "when") {

        if (m_hotspotCharName == "") {
            m_errorString = "when-element must be in hotspot-element";
            return false;
        }

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = "note-height is a required attribute of when";
            return false;
        }
        int noteHeight = s.toInt();

        s = attributes.value("x");
        int x = 0;
        if (s)
            x = s.toInt();

        s = attributes.value("y");
        if (!s) {
            m_errorString = "y is a required attribute of when";
            return false;
        }
        int y = s.toInt();

        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

        i->second.addHotspot(noteHeight, x, y);

    } else if (lcName == "font-requirements") {
    }
    else if (lcName == "font-requirement") {

        QString id = attributes.value("font-id");
        int n = -1;
        bool ok = false;
        if (id) {
            n = id.stripWhiteSpace().toInt(&ok);
            if (!ok) {
                m_errorString =
                    QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
                    arg(id);
                return false;
            }
        } else {
            m_errorString = "font-id is a required attribute of font-requirement";
            return false;
        }

        QString name = attributes.value("name");
        QString names = attributes.value("names");

        if (name) {
            if (names) {
                m_errorString = "font-requirement may have name or names attribute, but not both";
                return false;
            }

            SystemFont *font = SystemFont::loadSystemFont
                               (SystemFontSpec(name, 12));

            if (font) {
                m_systemFontNames[n] = name;
                delete font;
            } else {
                cerr << QString("Warning: Unable to load font \"%1\"").arg(name) << endl;
                m_ok = false;
            }

        } else if (names) {

            bool have = false;
            QStringList list = QStringList::split(",", names, false);
            for (QStringList::Iterator i = list.begin(); i != list.end(); ++i) {
                SystemFont *font = SystemFont::loadSystemFont
                                   (SystemFontSpec(*i, 12));
                if (font) {
                    m_systemFontNames[n] = *i;
                    have = true;
                    delete font;
                    break;
                }
            }
            if (!have) {
                cerr << QString("Warning: Unable to load any of the fonts in \"%1\"").
                arg(names) << endl;
                m_ok = false;
            }

        } else {
            m_errorString = "font-requirement must have either name or names attribute";
            return false;
        }

        QString s = attributes.value("strategy").lower();
        SystemFont::Strategy strategy = SystemFont::PreferGlyphs;

        if (s) {
            if (s == "prefer-glyphs")
                strategy = SystemFont::PreferGlyphs;
            else if (s == "prefer-codes")
                strategy = SystemFont::PreferCodes;
            else if (s == "only-glyphs")
                strategy = SystemFont::OnlyGlyphs;
            else if (s == "only-codes")
                strategy = SystemFont::OnlyCodes;
            else {
                cerr << "Warning: Unknown strategy value " << s
                << " (known values are prefer-glyphs, prefer-codes,"
                << " only-glyphs, only-codes)" << endl;
            }
        }

        m_systemFontStrategies[n] = strategy;

    } else {
    }

    if (m_characterDestination)
        *m_characterDestination = "";
    return true;
}

bool
NoteFontMap::error(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3: %4")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber())
                    .arg(m_errorString);
    return QXmlDefaultHandler::error(exception);
}

bool
NoteFontMap::fatalError(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3: %4")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber())
                    .arg(m_errorString);
    return QXmlDefaultHandler::fatalError(exception);
}

set
    <int>
    NoteFontMap::getSizes() const
    {
        set
            <int> sizes;

        for (SizeDataMap::const_iterator i = m_sizes.begin();
                i != m_sizes.end(); ++i) {
            sizes.insert(i->first);
        }

        return sizes;
    }

set
    <CharName>
    NoteFontMap::getCharNames() const
    {
        set
            <CharName> names;

        for (SymbolDataMap::const_iterator i = m_data.begin();
                i != m_data.end(); ++i) {
            names.insert(i->first);
        }

        return names;
    }

bool
NoteFontMap::checkFile(int size, string &src) const
{
    QString pixmapFileMixedName = QString("%1/%2/%3/%4.xpm")
                                  .arg(m_fontDirectory)
                                  .arg(strtoqstr(m_srcDirectory))
                                  .arg(size)
                                  .arg(strtoqstr(src));

    QFileInfo pixmapFileMixedInfo(pixmapFileMixedName);

    if (!pixmapFileMixedInfo.isReadable()) {

        QString pixmapFileLowerName = QString("%1/%2/%3/%4.xpm")
                                      .arg(m_fontDirectory)
                                      .arg(strtoqstr(m_srcDirectory).lower())
                                      .arg(size)
                                      .arg(strtoqstr(src));

        QFileInfo pixmapFileLowerInfo(pixmapFileLowerName);

        if (!pixmapFileLowerInfo.isReadable()) {
            if (pixmapFileMixedName != pixmapFileLowerName) {
                cerr << "Warning: Unable to open pixmap file "
                << pixmapFileMixedName << " or " << pixmapFileLowerName
                << endl;
            } else {
                cerr << "Warning: Unable to open pixmap file "
                << pixmapFileMixedName << endl;
            }
            return false;
        } else {
            src = qstrtostr(pixmapFileLowerName);
        }
    } else {
        src = qstrtostr(pixmapFileMixedName);
    }

    return true;
}

bool
NoteFontMap::hasInversion(int, CharName charName) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;
    return i->second.hasInversion();
}

bool
NoteFontMap::getSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    src = i->second.getSrc();
    if (src == "")
        return false;
    return checkFile(size, src);
}

bool
NoteFontMap::getInversionSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    if (!i->second.hasInversion())
        return false;
    src = i->second.getInversionSrc();
    if (src == "")
        return false;
    return checkFile(size, src);
}

SystemFont *
NoteFontMap::getSystemFont(int size, CharName charName, int &charBase)
const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    SizeDataMap::const_iterator si = m_sizes.find(size);
    if (si == m_sizes.end())
        return false;

    int fontId = i->second.getFontId();

    unsigned int fontHeight = 0;
    if (!si->second.getFontHeight(fontId, fontHeight)) {
        if (fontId == 0 || !si->second.getFontHeight(0, fontHeight)) {
            fontHeight = size;
        }
    }

    SystemFontNameMap::const_iterator fni = m_systemFontNames.find(fontId);
    if (fontId < 0 || fni == m_systemFontNames.end())
        return false;
    QString fontName = fni->second;

    CharBaseMap::const_iterator bi = m_bases.find(fontId);
    if (bi == m_bases.end())
        charBase = 0;
    else
        charBase = bi->second;

    SystemFontSpec spec(fontName, fontHeight);
    SystemFontMap::const_iterator fi = m_systemFontCache.find(spec);
    if (fi != m_systemFontCache.end()) {
        return fi->second;
    }

    SystemFont *font = SystemFont::loadSystemFont(spec);
    if (!font)
        return 0;
    m_systemFontCache[spec] = font;

    NOTATION_DEBUG << "NoteFontMap::getFont: loaded font " << fontName
    << " at pixel size " << fontHeight << endl;

    return font;
}

NoteFontMap::getStrategy(int, CharName charName)
const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return SystemFont::PreferGlyphs;

    int fontId = i->second.getFontId();
    SystemFontStrategyMap::const_iterator si =
        m_systemFontStrategies.find(fontId);

    if (si != m_systemFontStrategies.end()) {
        return si->second;
    }

    return SystemFont::PreferGlyphs;
}

bool
NoteFontMap::getCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    code = i->second.getCode();
    return (code >= 0);
}

bool
NoteFontMap::getInversionCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    code = i->second.getInversionCode();
    return (code >= 0);
}

bool
NoteFontMap::getGlyph(int, CharName charName, int &glyph) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    glyph = i->second.getGlyph();
    return (glyph >= 0);
}

bool
NoteFontMap::getInversionGlyph(int, CharName charName, int &glyph) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end())
        return false;

    glyph = i->second.getInversionGlyph();
    return (glyph >= 0);
}

bool
NoteFontMap::getStaffLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getStaffLineThickness(thickness);
}

bool
NoteFontMap::getLegerLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getLegerLineThickness(thickness);
}

bool
NoteFontMap::getStemThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getStemThickness(thickness);
}

bool
NoteFontMap::getBeamThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getBeamThickness(thickness);
}

bool
NoteFontMap::getStemLength(int size, unsigned int &length) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getStemLength(length);
}

bool
NoteFontMap::getFlagSpacing(int size, unsigned int &spacing) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end())
        return false;

    return i->second.getFlagSpacing(spacing);
}

bool
NoteFontMap::getHotspot(int size, CharName charName, int width, int height,
                        int &x, int &y) const
{
    HotspotDataMap::const_iterator i = m_hotspots.find(charName);
    if (i == m_hotspots.end())
        return false;
    return i->second.getHotspot(size, width, height, x, y);
}

bool
NoteFontMap::HotspotData::getHotspot(int size, int width, int height,
                                     int &x, int &y) const
{
    DataMap::const_iterator i = m_data.find(size);
    if (i == m_data.end()) {
        i = m_data.find(0); // fixed-pixel hotspot
        x = 0;
        if (m_scaled.first >= 0) {
            x = toSize(width, m_scaled.first, false);
        } else {
            if (i != m_data.end()) {
                x = i->second.first;
            }
        }
        if (m_scaled.second >= 0) {
            y = toSize(height, m_scaled.second, false);
            return true;
        } else {
            if (i != m_data.end()) {
                y = i->second.second;
                return true;
            }
            return false;
        }
    }
    x = i->second.first;
    y = i->second.second;
    return true;
}

QStringList
NoteFontMap::getSystemFontNames() const
{
    QStringList names;
    for (SystemFontNameMap::const_iterator i = m_systemFontNames.begin();
            i != m_systemFontNames.end(); ++i) {
        names.append(i->second);
    }
    return names;
}

void
NoteFontMap::dump() const
{
    // debug code

    cout << "Font data:\nName: " << getName() << "\nOrigin: " << getOrigin()
    << "\nCopyright: " << getCopyright() << "\nMapped by: "
    << getMappedBy() << "\nType: " << getType()
    << "\nSmooth: " << isSmooth() << endl;

    set
        <int> sizes = getSizes();
    set
        <CharName> names = getCharNames();

    for (set
            <int>::iterator sizei = sizes.begin(); sizei != sizes.end();
            ++sizei) {

        cout << "\nSize: " << *sizei << "\n" << endl;

        unsigned int t = 0;

        if (getStaffLineThickness(*sizei, t)) {
            cout << "Staff line thickness: " << t << endl;
        }

        if (getLegerLineThickness(*sizei, t)) {
            cout << "Leger line thickness: " << t << endl;
        }

        if (getStemThickness(*sizei, t)) {
            cout << "Stem thickness: " << t << endl;
        }

        if (getBeamThickness(*sizei, t)) {
            cout << "Beam thickness: " << t << endl;
        }

        if (getStemLength(*sizei, t)) {
            cout << "Stem length: " << t << endl;
        }

        if (getFlagSpacing(*sizei, t)) {
            cout << "Flag spacing: " << t << endl;
        }

        for (set
                <CharName>::iterator namei = names.begin();
                namei != names.end(); ++namei) {

            cout << "\nCharacter: " << namei->c_str() << endl;

            string s;
            int x, y, c;

            if (getSrc(*sizei, *namei, s)) {
                cout << "Src: " << s << endl;
            }

            if (getInversionSrc(*sizei, *namei, s)) {
                cout << "Inversion src: " << s << endl;
            }

            if (getCode(*sizei, *namei, c)) {
                cout << "Code: " << c << endl;
            }

            if (getInversionCode(*sizei, *namei, c)) {
                cout << "Inversion code: " << c << endl;
            }

            if (getGlyph(*sizei, *namei, c)) {
                cout << "Glyph: " << c << endl;
            }

            if (getInversionGlyph(*sizei, *namei, c)) {
                cout << "Inversion glyph: " << c << endl;
            }

            if (getHotspot(*sizei, *namei, 1, 1, x, y)) {
                cout << "Hot spot: (" << x << "," << y << ")" << endl;
            }
        }
    }
}

}
