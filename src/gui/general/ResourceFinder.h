/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_RESOURCE_FINDER_H_
#define _RG_RESOURCE_FINDER_H_

#include <QString>

namespace Rosegarden {
	
class ResourceFinder
{
public:
    ResourceFinder() { }
    virtual ~ResourceFinder() { }

    /**
     * Return the location (as a true file path, or a Qt4 ":"-prefixed
     * resource path) of the file best matching the given resource
     * filename in the given resource category.
     * 
     * Category should be a relative directory path without leading or
     * trailing slashes, for example "chords".  The fileName is the
     * remainder of the file name without any path content, for
     * example "user_chords.xml".
     *
     * Returns an empty string if no matching resource is found.
     *
     * Use this when you know that a particular resource is required
     * and just need to locate it.
     */
    QString getResourcePath(QString resourceCat, QString fileName);

    /**
     * Return a list of full file paths for files with the given file
     * extension, found in the given resource category.
     * 
     * Category should be a relative directory path without leading or
     * trailing slashes, for example "chords".  File extension should
     * be the extension without the dot, for example "xml".  Returned
     * list may mix true file paths in both installed and user
     * locations with Qt4 ":"-prefixed resource paths.
     *
     * Use this when you need to enumerate the options available for
     * use directly in the program (rather than e.g. offering the user
     * a file-open dialog).
     */
    QStringList getResourceFiles(QString resourceCat, QString fileExt);

    /**
     * Return the true file path for installed resource files in the
     * given resource category.  Category should be a relative
     * directory path without leading or trailing slashes, for example
     * "chords".  Note that resources may also exist in the Qt4
     * resource bundle; this method only returns the external
     * (installed) resource location.  Use getResourceFiles instead to
     * return an authoritative list of available resources of a given
     * type.
     *
     * Use this when you need a file path, e.g. for use in a file
     * finder dialog.
     */
    QString getResourceDir(QString resourceCat);

    /**
     * Return the true file path for the location in which resource
     * files in the given resource category should be saved.
     */
    QString getResourceSaveDir(QString resourceCat);

protected:
    QString getUserResourcePrefix();
    QStringList getSystemResourcePrefixList();
    QStringList getResourcePrefixList();
};

}

#endif

    
