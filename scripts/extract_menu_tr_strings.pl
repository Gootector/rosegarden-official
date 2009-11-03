#!/usr/bin/perl -w

use strict;

print qq{
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

/*
 * This file has been automatically generated and should not be compiled.
 *
 * The purpose of the file is to collect the translation strings from the
 * *.rc files which are used in creating the menus.
 * 
 * The code is prepared for  lupdate  program. There is make ts target :
 *
 *   make -f qt4-makefile ts
 *
 * which is used to extract translation strings and to update ts/*.tr files.
 */

};

my $file = $ARGV[0];
my $nextfile = $ARGV[1];
while (<>) {
    if ($ARGV[0]) {
        if ($nextfile ne $ARGV[0]) {
            $file = $nextfile;
            $nextfile = $ARGV[0];
        }
    } else {
        $file = $nextfile;
    }
    my $line = $_;

    my $name = "";
    my $text = "";
    my $shortcut = "";

    if ($line =~ /<text>(.*)<\/text>/) {
	$text = $1;
    } elsif ($line =~ /text="([^"]*)"/) {
	$text = $1;
    }
    $text =~ s/\&amp;/&/;

    if ($line =~ /name="([^"]*)"/) {
	$name = $1;
    }

    if ($text) {
        print 'QObject::tr("' . $text . '");';
        print ' /* ' . $file;
        if ($name) { print ' : ' . $name; }
        print " */\n";
    }

    # extract shortcuts
    #
    # NOTE: shortcuts do not have a comment field with them in order to avoid a
    # good amount of complication.  Instead, we write the comments in the code.
    if ($line =~ /shortcut="([^"]*)"/) {
        $shortcut = $1;
        print "\n/* TRANSLATOR: this is a keyboard shortcut for an English QWERTY keyboard.  Please adjust to suit your language! */\n";
        print 'QObject::tr("' . $shortcut . '");';
        if ($name) { print ' // ' . $name; }
        print "\n\n";
    }
}
