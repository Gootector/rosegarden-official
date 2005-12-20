// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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
 
#ifndef _NOTE_FONT_H_
#define _NOTE_FONT_H_

#include <string>
#include <set>
#include <map>

#include <qstring.h>
#include <qstringlist.h>
#include <qpixmap.h>
#include <qcanvas.h>
#include <qxml.h>

#include "notecharname.h"
#include "NotationTypes.h"
#include "Exception.h"


typedef std::pair<QString, int> SystemFontSpec;

class SystemFont
{
public:
    enum Strategy {
	PreferGlyphs, PreferCodes, OnlyGlyphs, OnlyCodes
    };

    virtual QPixmap renderChar(CharName charName,
			       int glyph, int code,
			       Strategy strategy,
			       bool &success) = 0;

    static SystemFont *loadSystemFont(const SystemFontSpec &spec);
};


// Helper class for looking up information about a font

class NoteFontMap : public QXmlDefaultHandler
{
public:
    typedef Rosegarden::Exception MappingFileReadFailed;

    NoteFontMap(std::string name); // load and parse the XML mapping file
    ~NoteFontMap();

    /**
     * ok() returns false if the file read succeeded but the font
     * relies on system fonts that are not available.  (If the file
     * read fails, the constructor throws MappingFileReadFailed.)
     */
    bool ok() const { return m_ok; }

    std::string getName() const { return m_name; }
    std::string getOrigin() const { return m_origin; }
    std::string getCopyright() const { return m_copyright; }
    std::string getMappedBy() const { return m_mappedBy; }
    std::string getType() const { return m_type; }
    bool isSmooth() const { return m_smooth; }

    std::set<int> getSizes() const;
    std::set<CharName> getCharNames() const;

    bool getStaffLineThickness(int size, unsigned int &thickness) const;
    bool getLegerLineThickness(int size, unsigned int &thickness) const;
    bool getStemThickness(int size, unsigned int &thickness) const;
    bool getBeamThickness(int size, unsigned int &thickness) const;
    bool getStemLength(int size, unsigned int &length) const;
    bool getFlagSpacing(int size, unsigned int &spacing) const;
    
    bool hasInversion(int size, CharName charName) const;

    bool getSrc(int size, CharName charName, std::string &src) const;
    bool getInversionSrc(int size, CharName charName, std::string &src) const;

    SystemFont *getSystemFont(int size, CharName charName, int &charBase) const;
    SystemFont::Strategy getStrategy(int size, CharName charName) const;
    bool getCode(int size, CharName charName, int &code) const;
    bool getInversionCode(int size, CharName charName, int &code) const;
    bool getGlyph(int size, CharName charName, int &glyph) const;
    bool getInversionGlyph(int size, CharName charName, int &glyph) const;

    bool getHotspot(int size, CharName charName, int width, int height,
		    int &x, int &y) const;

    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);

    virtual bool characters(QString &);

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

    void dump() const;

    // Not for general use, but very handy for diagnostic display
    QStringList getSystemFontNames() const;

    // want this to be private, but need access from HotspotData
    static int toSize(int baseSize, double factor, bool limitAtOne);

private:
    class SymbolData
    {
    public:
        SymbolData() : m_fontId(0),
		       m_src(""), m_inversionSrc(""),
		       m_code(-1), m_inversionCode(-1),
		       m_glyph(-1), m_inversionGlyph(-1) { }
        ~SymbolData() { }

	void setFontId(int id) { m_fontId = id; }
	int  getFontId() const { return m_fontId; }

        void setSrc(std::string src) { m_src = src; }
        std::string getSrc() const { return m_src; }

	void setCode(int code) { m_code = code; }
	int  getCode() const { return m_code; }

	void setGlyph(int glyph) { m_glyph = glyph; }
	int  getGlyph() const { return m_glyph; }

        void setInversionSrc(std::string inversion) { m_inversionSrc = inversion; }
        std::string getInversionSrc() const { return m_inversionSrc; }

        void setInversionCode(int code) { m_inversionCode = code; }
        int  getInversionCode() const { return m_inversionCode; }

        void setInversionGlyph(int glyph) { m_inversionGlyph = glyph; }
        int  getInversionGlyph() const { return m_inversionGlyph; }

        bool hasInversion() const {
	    return m_inversionGlyph >= 0 ||
		   m_inversionCode  >= 0 ||
		   m_inversionSrc   != "";
	}

    private:
	int m_fontId;
        std::string m_src;
        std::string m_inversionSrc;
	int m_code;
	int m_inversionCode;
	int m_glyph;
	int m_inversionGlyph;
    };

    class HotspotData
    {
    private:
        typedef std::pair<int, int> Point;
        typedef std::pair<double, double> ScaledPoint;
        typedef std::map<int, Point> DataMap;

    public:
        HotspotData() : m_scaled(-1.0, -1.0) { }
        ~HotspotData() { }
        
        void addHotspot(int size, int x, int y) {
            m_data[size] = Point(x, y);
        }

	void setScaledHotspot(double x, double y) {
	    m_scaled = ScaledPoint(x, y);
	}

        bool getHotspot(int size, int width, int height, int &x, int &y) const;

    private:
        DataMap m_data;
	ScaledPoint m_scaled;
    };

    class SizeData
    {
    public:
        SizeData() : m_stemThickness(-1),
                     m_beamThickness(-1),
                     m_stemLength(-1),
                     m_flagSpacing(-1),
                     m_staffLineThickness(-1),
                     m_legerLineThickness(-1) { }
        ~SizeData() { }

        void setStemThickness(unsigned int i) {
            m_stemThickness = (int)i;
        }
        void setBeamThickness(unsigned int i) {
            m_beamThickness = (int)i;
        }
        void setStemLength(unsigned int i) {
            m_stemLength = (int)i;
        }
        void setFlagSpacing(unsigned int i) {
            m_flagSpacing = (int)i;
        }
        void setStaffLineThickness(unsigned int i) {
            m_staffLineThickness = (int)i;
        }
        void setLegerLineThickness(unsigned int i) {
            m_legerLineThickness = (int)i;
        }
	void setFontHeight(int fontId, unsigned int h) {
	    m_fontHeights[fontId] = (int)h;
	}

        bool getStemThickness(unsigned int &i) const {
            if (m_stemThickness >= 0) {
                i = (unsigned int)m_stemThickness;
                return true;
            } else return false;
        }

        bool getBeamThickness(unsigned int &i) const {
            if (m_beamThickness >= 0) {
                i = (unsigned int)m_beamThickness;
                return true;
            } else return false;
        }

        bool getStemLength(unsigned int &i) const {
            if (m_stemLength >= 0) {
                i = (unsigned int)m_stemLength;
                return true;
            } else return false;
        }

        bool getFlagSpacing(unsigned int &i) const {
            if (m_flagSpacing >= 0) {
                i = (unsigned int)m_flagSpacing;
                return true;
            } else return false;
        }

        bool getStaffLineThickness(unsigned int &i) const {
            if (m_staffLineThickness >= 0) {
                i = (unsigned int)m_staffLineThickness;
                return true;
            } else return false;
        }

        bool getLegerLineThickness(unsigned int &i) const {
            if (m_legerLineThickness >= 0) {
                i = (unsigned int)m_legerLineThickness;
                return true;
            } else return false;
        }

	bool getFontHeight(int fontId, unsigned int &h) const {
	    std::map<int, int>::const_iterator fhi = m_fontHeights.find(fontId);
	    if (fhi != m_fontHeights.end()) {
		h = (unsigned int)fhi->second;
		return true;
	    }
	    return false;
	}	
       
    private:
        int m_stemThickness;
        int m_beamThickness;
        int m_stemLength;
        int m_flagSpacing;
        int m_staffLineThickness;
        int m_legerLineThickness;
	std::map<int, int> m_fontHeights; // per-font-id
    };

    //--------------- Data members ---------------------------------

    std::string m_name;
    std::string m_origin;
    std::string m_copyright;
    std::string m_mappedBy;
    std::string m_type;
    bool m_smooth;

    std::string m_srcDirectory;

    typedef std::map<CharName, SymbolData> SymbolDataMap;
    SymbolDataMap m_data;

    typedef std::map<CharName, HotspotData> HotspotDataMap;
    HotspotDataMap m_hotspots;

    typedef std::map<int, SizeData> SizeDataMap;
    SizeDataMap m_sizes;

    typedef std::map<int, QString> SystemFontNameMap;
    SystemFontNameMap m_systemFontNames;

    typedef std::map<int, SystemFont::Strategy> SystemFontStrategyMap;
    SystemFontStrategyMap m_systemFontStrategies;

    typedef std::map<SystemFontSpec, SystemFont *> SystemFontMap;
    mutable SystemFontMap m_systemFontCache;

    typedef std::map<int, int> CharBaseMap;
    CharBaseMap m_bases;

    // For use when reading the XML file:
    bool m_expectingCharacters;
    std::string *m_characterDestination;
    std::string m_hotspotCharName;
    QString m_errorString;

    bool checkFile(int size, std::string &src) const;
    QString m_fontDirectory;

    bool m_ok;
};


class NoteCharacterDrawRep;

/**
 * NoteCharacter knows how to draw a character from a font.  It may be
 * optimised for screen (using QPixmap underneath to produce
 * low-resolution colour or greyscale glyphs) or printer (using some
 * internal representation to draw in high-resolution monochrome on a
 * print device).  You can use screen characters on a printer and vice
 * versa, but the performance and quality might not be as good.
 *
 * NoteCharacter objects are always constructed by the NoteFont, never
 * directly.
 */

class NoteCharacter
{
public:
    NoteCharacter();
    NoteCharacter(const NoteCharacter &);
    NoteCharacter &operator=(const NoteCharacter &);
    ~NoteCharacter();

    int getWidth() const;
    int getHeight() const;
    
    QPoint getHotspot() const;

    QPixmap *getPixmap() const;
    QCanvasPixmap *getCanvasPixmap() const;

    void draw(QPainter *painter, int x, int y) const;
    void drawMask(QPainter *painter, int x, int y) const;

private:
    friend class NoteFont;
    NoteCharacter(QPixmap pixmap, QPoint hotspot, NoteCharacterDrawRep *rep);

    QPoint m_hotspot;
    QPixmap *m_pixmap; // I own this
    NoteCharacterDrawRep *m_rep; // I don't own this, it's a reference to a static in the NoteFont
};
    

// Encapsulates NoteFontMap, and loads pixmaps etc on demand

class NoteFont
{
public:
    enum CharacterType { Screen, Printer };

    typedef Rosegarden::Exception BadNoteFont;
    ~NoteFont();

    std::string getName() const { return m_fontMap.getName(); }
    int getSize() const { return m_size; }
    bool isSmooth() const { return m_fontMap.isSmooth(); }
    const NoteFontMap &getNoteFontMap() const { return m_fontMap; }

    /// Returns false + thickness=1 if not specified
    bool getStemThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable thickness if not specified
    bool getBeamThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable length if not specified
    bool getStemLength(unsigned int &length) const;

    /// Returns false + a guess at suitable spacing if not specified
    bool getFlagSpacing(unsigned int &spacing) const;

    /// Returns false + thickness=1 if not specified
    bool getStaffLineThickness(unsigned int &thickness) const;

    /// Returns false + thickness=1 if not specified
    bool getLegerLineThickness(unsigned int &thickness) const;

    /// Returns false if not available
    bool getCharacter(CharName charName,
		      NoteCharacter &character,
		      CharacterType type = Screen,
		      bool inverted = false);

    /// Returns an empty character if not available
    NoteCharacter getCharacter(CharName charName,
			       CharacterType type = Screen,
			       bool inverted = false);

    /// Returns false if not available
    bool getCharacterColoured(CharName charName,
			      int hue, int minValue,
			      NoteCharacter &character,
			      CharacterType type = Screen,
			      bool inverted = false);

    /// Returns an empty character if not available
    NoteCharacter getCharacterColoured(CharName charName,
				       int hue, int minValue,
				       CharacterType type = Screen,
				       bool inverted = false);

    /// Returns false if not available
    bool getCharacterShaded(CharName charName,
			    NoteCharacter &character,
			    CharacterType type = Screen,
			    bool inverted = false);

    /// Returns an empty character if not available
    NoteCharacter getCharacterShaded(CharName charName,
				     CharacterType type = Screen,
				     bool inverted = false);

    /// Returns false + dimensions of blank pixmap if none found
    bool getDimensions(CharName charName, int &x, int &y,
                       bool inverted = false) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getWidth(CharName charName) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getHeight(CharName charName) const;

    /// Returns false + centre-left of pixmap if no hotspot specified
    bool getHotspot(CharName charName, int &x, int &y,
                    bool inverted = false) const;

    /// Ignores problems, returns centre-left of pixmap if necessary
    QPoint getHotspot(CharName charName, bool inverted = false) const;

private:
    /// Returns false + blank pixmap if it can't find the right one
    bool getPixmap(CharName charName, QPixmap &pixmap,
                   bool inverted = false) const;

    /// Returns false + blank pixmap if it can't find the right one
    bool getColouredPixmap(CharName charName, QPixmap &pixmap,
                           int hue, int minValue,
			   bool inverted = false) const;

    /// Returns false + blank pixmap if it can't find the right one
    bool getShadedPixmap(CharName charName, QPixmap &pixmap,
			 bool inverted = false) const;

    friend class NoteFontFactory;
    NoteFont(std::string fontName, int size = 0);
    std::set<int> getSizes() const { return m_fontMap.getSizes(); }

    bool lookup(CharName charName, bool inverted, QPixmap *&pixmap) const;
    void add(CharName charName, bool inverted, QPixmap *pixmap) const;

    NoteCharacterDrawRep *lookupDrawRep(QPixmap *pixmap) const;

    CharName getNameWithColour(CharName origName, int hue) const;
    CharName getNameShaded(CharName origName) const;

    typedef std::pair<QPixmap *, QPixmap *>    PixmapPair;
    typedef std::map<CharName, PixmapPair>     PixmapMap;
    typedef std::map<std::string, PixmapMap *> FontPixmapMap;

    typedef std::map<QPixmap *, NoteCharacterDrawRep *> DrawRepMap;

    //--------------- Data members ---------------------------------

    int m_size;
    NoteFontMap m_fontMap;

    mutable PixmapMap *m_map; // pointer at a member of m_fontPixmapMap

    static FontPixmapMap *m_fontPixmapMap;
    static DrawRepMap *m_drawRepMap;

    static QPixmap *m_blankPixmap;
};


class NoteFontFactory
{
public:
    typedef Rosegarden::Exception NoFontsAvailable;

    // Any method passed a fontName argument may throw BadFont or
    // MappingFileReadFailed; any other method may throw NoFontsAvailable

    static NoteFont *getFont(std::string fontName, int size);

    static std::set<std::string> getFontNames(bool onStartup = false);
    static std::vector<int> getAllSizes(std::string fontName); // sorted
    static std::vector<int> getScreenSizes(std::string fontName); // sorted

    static std::string getDefaultFontName();
    static int getDefaultSize(std::string fontName);
    static bool isAvailableInSize(std::string fontName, int size);

private:
    static std::set<std::string> m_fontNames;
    static std::map<std::pair<std::string, int>, NoteFont *> m_fonts;
};


#endif

