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

#ifndef _RG_ACTIONFILEPARSER_H_
#define _RG_ACTIONFILEPARSER_H_

#include <QXmlDefaultHandler>
#include <QMap>
#include <QObject>

class QAction;
class QActionGroup;
class QMenu;
class QToolBar;

namespace Rosegarden
{

class ActionFileParser : public QObject, public QXmlDefaultHandler
{
    Q_OBJECT

public:
    ActionFileParser(QObject *actionOwner);
    ActionFileParser(QObject *actionOwner, QWidget *menuOwner);
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
    
    void enterActionState(QString stateName);
    void leaveActionState(QString stateName);

protected slots:
    void slotObjectDestroyed();

protected:
    enum Position { Top, Bottom, Left, Right, Default };

    bool setActionText(QString actionName, QString text);
    bool setActionIcon(QString actionName, QString icon);
    bool setActionShortcut(QString actionName, QString shortcut);
    bool setActionGroup(QString actionName, QString group);
    bool setActionChecked(QString actionName, bool);

    bool setMenuText(QString name, QString text);
    bool addMenuToMenu(QString parent, QString child);
    bool addMenuToMenubar(QString menuName);
    bool addActionToMenu(QString menuName, QString actionName);
    bool addSeparatorToMenu(QString menuName);
    
    bool setToolbarText(QString name, QString text);
    bool addActionToToolbar(QString toolbarName, QString actionName);
    bool addSeparatorToToolbar(QString toolbarName);
    void addToolbarBreak(Position);

    bool enableActionInState(QString stateName, QString actionName);
    bool disableActionInState(QString stateName, QString actionName);

    QString translate(QString actionName, QString text, QString purpose);

    QString findRcFile(QString name);

    QAction *findAction(QString name);
    QAction *findStandardAction(QString name);
    QActionGroup *findGroup(QString name);
    QMenu *findMenu(QString name);
    QToolBar *findToolbar(QString name, Position position);

    typedef QMap<QString, QList<QAction *> > StateMap;
    StateMap m_stateEnableMap;
    StateMap m_stateDisableMap;

    QObject *m_actionOwner;
    QWidget *m_menuOwner;
    bool m_inMenuBar;
    bool m_inText;
    bool m_inEnable;
    bool m_inDisable;
    QStringList m_currentMenus;
    QString m_currentToolbar;
    QString m_currentState;
    QString m_currentText;
    QString m_currentFile;
    Position m_lastToolbarPosition;
};

// A QMenu needs a QWidget as its parent, but the action file client
// will not necessarily be a QWidget -- for example, it might be a
// tool object.  In this case, we need to make a menu that has no
// parent, but we need to wrap it in something that is parented by the
// action file client so that we can find it later and it shares the
// scope of the client.  This is that wrapper.
class ActionFileMenuWrapper : public QObject
{
    Q_OBJECT

public:
    ActionFileMenuWrapper(QMenu *menu, QObject *parent);
    virtual ~ActionFileMenuWrapper();

    QMenu *getMenu();

private:
    QMenu *m_menu;
};

}

#endif

    
    
