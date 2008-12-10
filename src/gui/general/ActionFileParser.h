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

#ifndef _RG_ACTIONFILEPARSER_H_
#define _RG_ACTIONFILEPARSER_H_

#include <QXmlDefaultHandler>

namespace Rosegarden
{
	
class ActionFileParser : public QXmlDefaultHandler
{
    Q_OBJECT
    
public:
    ActionFileParser(QObject *actionOwner);
    virtual ~ActionFileParser();
    
    bool load(QString actionRcFile);

    // XML handler methods

    virtual bool startDocument();

    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts);

    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName);

    virtual bool characters(const QString& ch);

    virtual bool endDocument();

    bool error(const QXmlParseException &exception);
    bool fatalError(const QXmlParseException &exception);

protected:
    bool setActionText(QString actionName, QString text);
    bool setActionIcon(QString actionName, QString icon);
    bool setActionShortcut(QString actionName, QString shortcut);
    bool setActionGroup(QString actionName, QString group);

    bool setMenuText(QString name, QString text);
    bool addMenuToMenu(QString parent, QString child);
    bool addActionToMenu(QString menuName, QString actionName);
    bool addSeparatorToMenu(QString menuName);
    
    bool setToolbarText(QString name, QString text);
    bool addActionToToolbar(QString toolbarName, QString actionName);
    bool addSeparatorToToolbar(QString toolbarName);

    bool addState(QString name);
    bool enableActionInState(QString stateName, QString actionName);
    bool disableActionInState(QString stateName, QString actionName);
};

}

#endif

    
    
