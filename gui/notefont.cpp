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

#include "notefont.h"

#include <qfileinfo.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qdir.h>
#include <qpainter.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include <iostream>

#include "rosestrings.h"
#include "pixmapfunctions.h"

using std::string;
using std::map;
using std::set;
using std::cout;
using std::cerr;
using std::endl;

NoteFontMap::NoteFontMap(string name) :
    m_name(name),
    m_smooth(false),
    m_characterDestination(0),
    m_hotspotCharName(""),
    m_ok(true)
{
    m_fontDirectory = KGlobal::dirs()->findResource("appdata", "fonts/");

    QString mapFileName = QString("%1/mappings/%2.xml")
        .arg(m_fontDirectory)
        .arg(strtoqstr(name));

    QFileInfo mapFileInfo(mapFileName);

    if (!mapFileInfo.isReadable()) {
        throw MappingFileReadFailed(qstrtostr(i18n("Can't open mapping file %1").
					      arg(mapFileName)));
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
    // empty
}

bool
NoteFontMap::characters(QString &chars)
{
    if (!m_characterDestination) return true;
    *m_characterDestination += qstrtostr(chars);
    return true;
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
        
    } else if (lcName == "font-information") { 

	QString s;

        s = attributes.value("origin");
        if (s) m_origin = qstrtostr(s);

        s = attributes.value("copyright");
        if (s) m_copyright = qstrtostr(s);

        s = attributes.value("mapped-by");
        if (s) m_mappedBy = qstrtostr(s);

        s = attributes.value("type");
        if (s) m_type = qstrtostr(s);

        s = attributes.value("smooth");
        if (s) m_smooth = (s.lower() == "true");

    } else if (lcName == "font-sizes") {

    } else if (lcName == "font-size") {

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = i18n("note-height not found");
            return false;
        }
        int noteHeight = s.toInt();

        SizeData sizeData;

        s = attributes.value("staff-line-thickness");
        if (s) sizeData.setStaffLineThickness(s.toInt());

        s = attributes.value("stem-thickness");
        if (s) sizeData.setStemThickness(s.toInt());

        s = attributes.value("beam-thickness");
        if (s) sizeData.setBeamThickness(s.toInt());

        s = attributes.value("border-x");
        if (s) sizeData.setBorderX(s.toInt());

        s = attributes.value("border-y");
        if (s) sizeData.setBorderY(s.toInt());

        m_sizes[noteHeight] = sizeData;

    } else if (lcName == "font-symbol-map") {

    } else if (lcName == "symbol") {

        QString symbolName = attributes.value("name");
        if (!symbolName) {
            m_errorString = i18n("name is a required attribute of symbol");
            return false;
        }
        SymbolData symbolData;

        QString src = attributes.value("src");
	QString code = attributes.value("code");

	int n = -1;
	bool ok = false;
	if (code) {
	    n = code.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    i18n("invalid code attribute \"%1\" (must be integer >= 0)").
		    arg(code);
		return false;
	    }
	    symbolData.setCode(n);
	}

        if (!src && n < 0) {
            m_errorString = i18n("symbol must have either src or code attribute");
            return false;
        }
        if (src) symbolData.setSrc(qstrtostr(src));

        QString inversionSrc = attributes.value("inversion-src");
        if (inversionSrc) symbolData.setInversionSrc(qstrtostr(inversionSrc));

        QString inversionCode = attributes.value("inversion-code");
        if (inversionCode) {
	    n = inversionCode.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    i18n("invalid inversion code attribute \"%1\" (must be integer >= 0)").
		    arg(inversionCode);
		return false;
	    }
	    symbolData.setInversionCode(n);
	}

	QString fontId = attributes.value("font-id");
	if (fontId) {
	    n = fontId.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    i18n("invalid font-id attribute \"%1\" (must be integer >= 0)").
		    arg(fontId);
		return false;
	    }
	    symbolData.setFontId(n);
	}

        m_data[qstrtostr(symbolName.upper())] = symbolData;

    } else if (lcName == "font-hotspots") {

    } else if (lcName == "hotspot") {

        QString s = attributes.value("name");
        if (!s) {
            m_errorString = i18n("name is a required attribute of hotspot");
            return false;
        }
        m_hotspotCharName = qstrtostr(s.upper());

    } else if (lcName == "when") {

        if (m_hotspotCharName == "") {
            m_errorString = i18n("when-element must be in hotspot-element");
            return false;
        }

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = i18n("note-height is a required attribute of when");
            return false;
        }
        int noteHeight = s.toInt();

        s = attributes.value("x");
        int x = 0;
        if (s) x = s.toInt();

        s = attributes.value("y");
        if (!s) {
            m_errorString = i18n("y is a required attribute of when");
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

    } else if (lcName == "font-requirement") {

	QString id = attributes.value("font-id");
	int n = -1;
	bool ok = false;
	if (id) {
	    n = id.stripWhiteSpace().toInt(&ok);
	    if (!ok) {
		m_errorString =
		    i18n("invalid font-id attribute \"%1\" (must be integer >= 0)").
		    arg(id);
		return false;
	    }
	} else {
	    m_errorString = i18n("font-id is a required attribute of font-requirement");
	    return false;
	}

	QString name = attributes.value("name");
	QString names = attributes.value("names");
	if (name) {

	    if (names) {
		m_errorString =
		    i18n("font-requirement may have name or names attribute, but not both");
		return false;
	    }
	    //!!! need the font-sizes stuff to tell us what pixel size to use for a given line width
	    QFont font;
	    if (checkFont(name, 12, font)) { //!!!
		m_fonts[n] = font;
	    } else {
		cerr << i18n("Warning: Unable to load font \"%1\"").arg(name) << endl;
		m_ok = false;
	    }

	} else if (names) {

	    bool have = false;
	    QStringList list = QStringList::split(",", names, false);
	    for (QStringList::Iterator i = list.begin(); i != list.end(); ++i) {
		QFont font;
		if (checkFont(*i, 12, font)) { //!!!
		    m_fonts[n] = font;
		    have = true;
		    break;
		}
	    }
	    if (!have) {
		cerr << i18n("Warning: Unable to load any of the fonts in \"%1\"").
		    arg(names) << endl;
		m_ok = false;
	    }

	} else {
	    m_errorString =
		i18n("font-requirement must have either name or names attribute");
	    return false;
	}	    

    } else {

    }

    if (m_characterDestination) *m_characterDestination = "";
    return true;
}

bool
NoteFontMap::error(const QXmlParseException& exception)
{
    m_errorString = i18n("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::error(exception);
}

bool
NoteFontMap::fatalError(const QXmlParseException& exception)
{
    m_errorString = i18n("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::fatalError(exception);
}

set<int>
NoteFontMap::getSizes() const
{
    set<int> sizes;

    for (SizeDataMap::const_iterator i = m_sizes.begin();
         i != m_sizes.end(); ++i) {
        sizes.insert(i->first);
    }

    return sizes;
}

set<CharName>
NoteFontMap::getCharNames() const
{
    set<CharName> names;

    for (SymbolDataMap::const_iterator i = m_data.begin();
         i != m_data.end(); ++i) {
        names.insert(i->first);
    }

    return names;
}

bool
NoteFontMap::checkFont(QString name, int size, QFont &font) const
{
    font = QFont(name, size, QFont::Normal);
    font.setPixelSize(size);
    return font.exactMatch();
}

bool
NoteFontMap::checkFile(int size, string &src) const
{
    QString pixmapFileName = QString("%1/%2/%3/%4.xpm")
        .arg(m_fontDirectory)
        .arg(strtoqstr(m_name))
        .arg(size)
        .arg(strtoqstr(src));
    src = qstrtostr(pixmapFileName);

    QFileInfo pixmapFileInfo(pixmapFileName);

    if (!pixmapFileInfo.isReadable()) {
        cerr << "Warning: Unable to open pixmap file " << pixmapFileName
             << endl;
        return false;
    }

    return true;
}

bool
NoteFontMap::hasInversion(int, CharName charName) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;
    return i->second.hasInversion();
}

bool
NoteFontMap::getSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    src = i->second.getSrc();
    if (src == "") return false;
    return checkFile(size, src);
}

bool
NoteFontMap::getInversionSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    if (!i->second.hasInversion()) return false;
    src = i->second.getInversionSrc();
    return checkFile(size, src);
}

bool
NoteFontMap::getFont(int, CharName charName, QFont &font) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    int fontId = i->second.getFontId();
    SystemFontMap::const_iterator fi = m_fonts.find(fontId);

    if (fontId < 0 || fi == m_fonts.end()) return false;

    font = fi->second;
    return true;
}

bool
NoteFontMap::getCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    code = i->second.getCode();
    return (code >= 0);
}

bool
NoteFontMap::getInversionCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    code = i->second.getInversionCode();
    return (code >= 0);
}

bool
NoteFontMap::getStaffLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStaffLineThickness(thickness);
}

bool
NoteFontMap::getStemThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStemThickness(thickness);
}

bool
NoteFontMap::getBeamThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getBeamThickness(thickness);
}

bool
NoteFontMap::getBorderThickness(int size,
                                unsigned int &x, unsigned int &y) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getBorderThickness(x, y);
}

bool
NoteFontMap::getHotspot(int size, CharName charName, int &x, int &y) const
{
    HotspotDataMap::const_iterator i = m_hotspots.find(charName);
    if (i == m_hotspots.end()) return false;

    return i->second.getHotspot(size, x, y);
}

void
NoteFontMap::dump() const
{
    // debug code
    //!!! add code/font stuff

    cout << "Font data:\nName: " << getName() << "\nOrigin: " << getOrigin()
         << "\nCopyright: " << getCopyright() << "\nMapped by: "
         << getMappedBy() << endl;

    set<int> sizes = getSizes();
    set<CharName> names = getCharNames();

    for (set<int>::iterator sizei = sizes.begin(); sizei != sizes.end();
         ++sizei) {

        cout << "\nSize: " << *sizei << "\n" << endl;

        unsigned int t = 0;

        if (getStaffLineThickness(*sizei, t)) {
            cout << "Staff line thickness: " << t << endl;
        }

        if (getStemThickness(*sizei, t)) {
            cout << "Stem thickness: " << t << endl;
        }

        for (set<CharName>::iterator namei = names.begin();
             namei != names.end(); ++namei) {

            cout << "\nCharacter: " << namei->c_str() << endl;

            string s;
            int x, y;

            if (getSrc(*sizei, *namei, s)) {
                cout << "Src: " << s << endl;
            }

            if (getInversionSrc(*sizei, *namei, s)) {
                cout << "Inversion src: " << s << endl;
            }
            
            if (getHotspot(*sizei, *namei, x, y)) {
                cout << "Hot spot: (" << x << "," << y << ")" << endl;
            }
        }
    }
}


NoteFont::FontPixmapMap *NoteFont::m_fontPixmapMap = 0;
QPixmap *NoteFont::m_blankPixmap = 0;

NoteFont::NoteFont(string fontName, int size) :
    m_fontMap(fontName)
{
    // Do the size checks first, to avoid doing the extra work if they fail

    std::set<int> sizes = m_fontMap.getSizes();

    if (sizes.size() > 0) {
        m_currentSize = *sizes.begin();
    } else {
        throw BadFont(qstrtostr(QString("No sizes listed for font \"%1\"")
                      .arg(strtoqstr(fontName))));
    }

    if (size > 0) {
        if (sizes.find(size) == sizes.end()) {
            throw BadFont(qstrtostr(QString("Font \"%1\" not available in size %2")
                          .arg(strtoqstr(fontName))
                          .arg(size)));
        } else {
            m_currentSize = size;
        }
    }

    // Create the global font map and blank pixmap if necessary

    if (m_fontPixmapMap == 0) {
        m_fontPixmapMap = new FontPixmapMap();
    }

    if (m_blankPixmap == 0) {
        m_blankPixmap = new QPixmap(10, 10);
        m_blankPixmap->setMask(QBitmap(10, 10, TRUE));
    }

    // Locate our font's pixmap map in the font map, create if necessary

    string fontKey = qstrtostr(QString("__%1__%2__")
        .arg(strtoqstr(m_fontMap.getName()))
        .arg(m_currentSize));

    FontPixmapMap::iterator i = m_fontPixmapMap->find(fontKey);
    if (i == m_fontPixmapMap->end()) {
        (*m_fontPixmapMap)[fontKey] = new PixmapMap();
    }

    m_map = (*m_fontPixmapMap)[fontKey];
}

NoteFont::~NoteFont()
{ 
    // empty
}


set<string>
NoteFont::getAvailableFontNames()
{
    set<string> names;

    QString mappingDir = 
	KGlobal::dirs()->findResource("appdata", "fonts/mappings/");
    QDir dir(mappingDir);
    if (!dir.exists()) {
        cerr << "NoteFont::getAvailableFontNames: mapping directory \""
	     << mappingDir << "\" not found" << endl;
        return names;
    }

    dir.setFilter(QDir::Files | QDir::Readable);
    QStringList files = dir.entryList();
    for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {
	if ((*i).length() > 4 && (*i).right(4).lower() == ".xml") {
	    // we don't catch MappingFileReadFailed here -- that's a serious
	    // enough exception that we need to handle it further up the stack
	    string name(qstrtostr((*i).left((*i).length() - 4)));
	    NoteFontMap map(name);
	    if (map.ok()) names.insert(name);
	}
    }

    return names;
}


bool
NoteFont::getStemThickness(unsigned int &thickness) const
{
    thickness = 1;
    return m_fontMap.getStemThickness(m_currentSize, thickness);
}

bool
NoteFont::getBeamThickness(unsigned int &thickness) const
{
//    thickness = (m_currentSize + 2) / 3;
    thickness = m_currentSize / 2;
    return m_fontMap.getBeamThickness(m_currentSize, thickness);
}

bool
NoteFont::getStaffLineThickness(unsigned int &thickness) const
{
    thickness = 1;
    return m_fontMap.getStaffLineThickness(m_currentSize, thickness);
}

bool
NoteFont::getBorderThickness(unsigned int &x, unsigned int &y) const
{
    x = 0;
    y = 0;
    return m_fontMap.getBorderThickness(m_currentSize, x, y);
}

QPixmap *
NoteFont::lookup(CharName charName, bool inverted) const
{
    PixmapMap::iterator i = m_map->find(charName);
    if (i != m_map->end()) {
        if (inverted) return i->second.second;
        else return i->second.first;
    }
    return 0;
}

void
NoteFont::add(CharName charName, bool inverted, QPixmap *pixmap) const
{
    PixmapMap::iterator i = m_map->find(charName);
    if (i != m_map->end()) {
        if (inverted) {
            delete i->second.second;
            i->second.second = pixmap;
        } else {
            delete i->second.first;
            i->second.first = pixmap;
        }
    } else {
        if (inverted) {
            (*m_map)[charName] = PixmapPair(0, pixmap);
        } else {
            (*m_map)[charName] = PixmapPair(pixmap, 0);
        }
    }
}

bool
NoteFont::getPixmap(CharName charName, QPixmap &pixmap, bool inverted) const
{
    QPixmap *found = lookup(charName, inverted);
    if (found != 0) {
        pixmap = *found;
        return true;
    }

    if (inverted && !m_fontMap.hasInversion(m_currentSize, charName)) {
	if (!getPixmap(charName, pixmap, false)) return false;
	found = new QPixmap(PixmapFunctions::flipVertical(pixmap));
	add(charName, inverted, found);
	pixmap = *found;
	return true;
    }

    string src;
    bool ok = false;

    if (!inverted) ok = m_fontMap.getSrc(m_currentSize, charName, src);
    else  ok = m_fontMap.getInversionSrc(m_currentSize, charName, src);
    
    if (ok) {
        cerr << "NoteFont::getPixmap: Loading \"" << src << "\"" << endl;

        found = new QPixmap(strtoqstr(src));

        if (!found->isNull()) {

            if (found->mask() == 0) {
                cerr << "NoteFont::getPixmap: Warning: No automatic mask "
                     << "for character \"" << charName << "\"" 
                     << (inverted ? " (inverted)" : "") << " in font \""
                     << m_fontMap.getName() << "-" << m_currentSize
                     << "\"; consider making xpm background transparent"
                     << endl;
		found->setMask(PixmapFunctions::generateMask(*found));
            }

            add(charName, inverted, found);
            pixmap = *found;
            return true;
        }

        cerr << "NoteFont::getPixmap: Warning: Unable to read pixmap file " << src << endl;
    } else {

	int code = -1;
	QFont font;
	if (!inverted) ok = m_fontMap.getCode(m_currentSize, charName, code);
	else  ok = m_fontMap.getInversionCode(m_currentSize, charName, code);

	if (!ok) {
	    cerr << "NoteFont::getPixmap: Warning: No pixmap or code for character \""
		 << charName << "\"" << (inverted ? " (inverted)" : "")
		 << " in font \"" << m_fontMap.getName() << "\"" << endl;
	    pixmap = *m_blankPixmap;
	    return false;
	}

	ok = m_fontMap.getFont(m_currentSize, charName, font);

	if (!ok) {
	    cerr << "NoteFont::getPixmap: Warning: No system font for character \""
		 << charName << "\"" << (inverted ? " (inverted)" : "")
		 << " in font \"" << m_fontMap.getName() << "\"" << endl;
	    pixmap = *m_blankPixmap;
	    return false;
	}

	QFontMetrics metrics(font);
	found = new QPixmap(metrics.width(QChar(code)), metrics.height());
	found->fill();
	QPainter painter;
	painter.begin(found);
	painter.drawText(0, metrics.ascent(), QChar(code));
	painter.end();

	found->setMask(PixmapFunctions::generateMask(*found));
	add(charName, inverted, found);
	pixmap = *found;
	return true;
    }

    pixmap = *m_blankPixmap;
    return false;
}

QPixmap
NoteFont::getPixmap(CharName charName, bool inverted) const
{
    QPixmap p;
    (void)getPixmap(charName, p, inverted);
    return p;
}

QCanvasPixmap*
NoteFont::getCanvasPixmap(CharName charName, bool inverted) const
{
    QPixmap p;
    (void)getPixmap(charName, p, inverted);

    int x, y;
    (void)getHotspot(charName, x, y, inverted);

    return new QCanvasPixmap(p, QPoint(x, y));
}

bool
NoteFont::getColouredPixmap(CharName baseCharName, QPixmap &pixmap,
                            int hue, int minValue, bool inverted) const
{
    CharName charName(getNameWithColour(baseCharName, hue));

    QPixmap *found = lookup(charName, inverted);
    if (found != 0) {
        pixmap = *found;
        return true;
    }

    QPixmap basePixmap;
    bool ok = getPixmap(baseCharName, basePixmap, inverted);

    found = new QPixmap(PixmapFunctions::colourPixmap(basePixmap, hue, minValue));
    add(charName, inverted, found);
    pixmap = *found;
    return ok;
}

QPixmap
NoteFont::getColouredPixmap(CharName charName, int hue, int minValue, bool inverted) const
{
    QPixmap p;
    (void)getColouredPixmap(charName, p, hue, minValue, inverted);
    return p;
}

QCanvasPixmap*
NoteFont::getColouredCanvasPixmap(CharName charName, int hue, int minValue,
                                  bool inverted) const
{
    QPixmap p;
    (void)getColouredPixmap(charName, p, hue, minValue, inverted);

    int x, y;
    (void)getHotspot(charName, x, y, inverted);

    return new QCanvasPixmap(p, QPoint(x, y));
}

CharName
NoteFont::getNameWithColour(CharName base, int hue) const
{
    return qstrtostr(QString("%1__%2").arg(hue).arg(strtoqstr(base)));
}

bool
NoteFont::getDimensions(CharName charName, int &x, int &y, bool inverted) const
{
    QPixmap pixmap;
    bool ok = getPixmap(charName, pixmap, inverted);
    x = pixmap.width();
    y = pixmap.height();
    return ok;
}

int
NoteFont::getWidth(CharName charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return x;
}

int
NoteFont::getHeight(CharName charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return y;
}

bool
NoteFont::getHotspot(CharName charName, int &x, int &y, bool inverted) const
{
    bool ok = m_fontMap.getHotspot(m_currentSize, charName, x, y);

    if (!ok) {
        int w, h;
        getDimensions(charName, w, h, inverted);
        x = 0;
        y = h/2;
    }

    if (inverted) {
        y = getHeight(charName) - y;
    }
    
    return ok;
}

QPoint
NoteFont::getHotspot(CharName charName, bool inverted) const
{
    int x, y;
    (void)getHotspot(charName, x, y, inverted);
    return QPoint(x, y);
}

