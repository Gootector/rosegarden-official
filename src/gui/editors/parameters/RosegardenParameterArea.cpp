/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    This file Copyright 2006 Martin Shepherd <mcs@astro.caltech.edu>.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenParameterArea.h"

#include "RosegardenParameterBox.h"
#include <iostream>
#include <set>

#include <QTabWidget>
#include <QFont>
#include <QFrame>
#include <QPoint>
//#include <Q3ScrollView>
#include <QScrollArea>
#include <QString>
#include <QLayout>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QGroupBox>
//#include <QPushButton>

#include "misc/Debug.h"

namespace Rosegarden
{

RosegardenParameterArea::RosegardenParameterArea(
	QWidget *parent,
	const char *name
	)	//, WFlags f)
	: QStackedWidget(parent),//, name),//, f),
        m_style(RosegardenParameterArea::CLASSIC_STYLE),
//         m_scrollView(new Q3ScrollView(this, 0, Qt::WStaticContents)),
		m_scrollView( new QScrollArea(this) ),
// 		m_classic(new QWidget(m_scrollView->viewport())),
		m_classic(0),
		m_classicLayout( new QVBoxLayout(m_classic) ),
        m_tabBox(new QTabWidget(this)),
        m_active(0),
        m_spacing(0)
{
	setObjectName( name );
	
	
	m_classic = new QWidget( m_scrollView );
	m_classic->setLayout(m_classicLayout);
	
	//### how to make this self-adjusting (to the size of child widgets)??
	// without this, the size tends to become 0
	m_classic->setMinimumSize( 400,800 );
	//m_classic->setMaximumSize( 400,400 );
	
//	m_scrollView->setWidget(m_classic);
	m_scrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
//	m_scrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	
	// set a dark background
// 	m_scrollView->setBackgroundRole(QPalette::Dark);
	
	//
	QSizePolicy poli;
// 	m_scrollView->setResizePolicy(QScrollArea::AutoOneFit);
	poli.setVerticalPolicy( QSizePolicy::MinimumExpanding );		//@@@ was AutoOneFit
	poli.setHorizontalPolicy( QSizePolicy::MinimumExpanding );
	m_scrollView->setSizePolicy( poli );
	// */
	
	/*
    m_scrollView->addChild(m_classic);
	m_scrollView->setLayout( m_classicLayout );
    m_scrollView->setHScrollBarMode(Q3ScrollView::AlwaysOff);
	m_scrollView->setVScrollBarMode(Q3ScrollView::AlwaysOn);
    m_scrollView->setResizePolicy(Q3ScrollView::AutoOneFit);
	*/
	
	// add 2 wigets as stacked widgets
    // Install the classic-style VBox widget in the widget-stack.
	addWidget(m_scrollView);//, CLASSIC_STYLE);	//&&& 

    // Install the widget that implements the tab-style to the widget-stack.
	addWidget(m_tabBox); //, TAB_BOX_STYLE);
	
	setCurrentWidget( m_scrollView );
	
	//m_scrollView->setVisible( true );

}

void RosegardenParameterArea::addRosegardenParameterBox(
    RosegardenParameterBox *b)
{
    RG_DEBUG << "RosegardenParameterArea::addRosegardenParameterBox" << endl;

	
    // Check that the box hasn't been added before.

    for (unsigned int i = 0; i < m_parameterBoxes.size(); i++) {
        if (m_parameterBoxes[i] == b)
            return ;
    }

    // Append the parameter box to the list to be displayed.
     m_parameterBoxes.push_back(b);
 
    /*
    //### Painter not active on create?? ... come me back to setting minimum width.
    m_scrollView->setMinimumWidth(std::max(m_scrollView->minimumWidth(),
                                           b->sizeHint().width()) + 8);
    */

    // Create a titled group box for the parameter box, parented by the
    // classic layout widget, so that it can be used to provide a title
    // and outline, in classic mode. Add this container to an array that
    // parallels the above array of parameter boxes.

    QGroupBox *box = new QGroupBox(b->getLongLabel(), m_classic);
	//box->setMinimumSize( 40,40 );
	m_classicLayout->addWidget(box);
	
	box->setLayout( new QVBoxLayout(box) );
    box->layout()->setMargin( 4 ); // about half the default value
    QFont f;
    f.setBold( true );
    box->setFont( f );
	
    m_groupBoxes.push_back(box);

	// add the ParameterBox to the Layout
    box->layout()->addWidget(b);
	
	
	
    if (m_spacing)
        delete m_spacing;
    m_spacing = new QFrame(m_classic);
    m_classicLayout->addWidget(m_spacing);
    m_classicLayout->setStretchFactor(m_spacing, 100);
	
	

    // Add the parameter box to the current container of the displayed
    // widgets, unless the current container has been set up yet.

//    if (m_active)
//        moveWidget(0, m_active, b);

    // Queue a redisplay of the parameter area, to incorporate the new box.

// 	update();
}

void RosegardenParameterArea::setArrangement(Arrangement style)
{
    RG_DEBUG << "RosegardenParameterArea::setArrangement(" << style << ")" << endl;

	///CJNFTEMP
	m_scrollView->setWidget(m_classic);
	
    // Lookup the container of the specified style.

    //QWidget *container;
    //switch (style) {
    //case CLASSIC_STYLE:
        //container = m_classic;
        //break;
    //case TAB_BOX_STYLE:
        //container = m_tabBox;
        //break;
    //default:
        //std::cerr << "setArrangement() was passed an unknown arrangement style."
        //<< std::endl;
        //return ;
    //}

    //// Does the current container of the parameter-box widgets differ
    //// from the one that is associated with the currently configured
    //// style?

    //if (container != m_active) {

        //// Move the parameter boxes from the old container to the new one.

        //std::vector<RosegardenParameterBox *> sorted;
        //std::set<RosegardenParameterBox *> unsorted;

        //for (unsigned int i = 0; i < m_parameterBoxes.size(); i++) {
            //unsorted.insert(m_parameterBoxes[i]);
        //}

        //QString previous = "";

        //while (!unsorted.empty()) {
            //std::set<RosegardenParameterBox *>::iterator i = unsorted.begin();
            //bool have = false;
            //while (i != unsorted.end()) {
                //if ((*i)->getPreviousBox(style) == previous) {
                    //sorted.push_back(*i);
                    //previous = (*i)->getShortLabel();
                    //unsorted.erase(i);
                    //have = true;
                    //break;
                //}
                //++i;
            //}
            //if (!have) {
                //while (!unsorted.empty()) {
                    //sorted.push_back(*unsorted.begin());
                    //unsorted.erase(unsorted.begin());
                //}
                //break;
            //}
        //}

        //for (std::vector<RosegardenParameterBox *>::iterator i = sorted.begin();
                //i != sorted.end(); ++i) {
            //moveWidget(m_active, container, *i);
            //(*i)->showAdditionalControls(style == TAB_BOX_STYLE);
        //}

        //// Switch the widget stack to displaying the new container.

        //switch (style) {
        //case CLASSIC_STYLE:
            //setCurrentWidget(m_scrollView);
            //break;
        //case TAB_BOX_STYLE:
            //setCurrentWidget(m_tabBox);
            //break;
        //}
    //}

    //// Record the identity of the active container, and the associated
    //// arrangement style.

    //m_active = container;
    //m_style = style;

}

void RosegardenParameterArea::moveWidget(QWidget *old_container,
        QWidget *new_container,
        RosegardenParameterBox *box)
{
    RG_DEBUG << "RosegardenParameterArea::moveWidget" << endl;

    // Remove any state that is associated with the parameter boxes,
    // from the active container.

    if (old_container == m_classic) {
        ;
    } else if (old_container == m_tabBox) {
        m_tabBox->removePage(box);
    }

    // Reparent the parameter box, and perform any container-specific
    // configuration.

    if (new_container == m_classic) {
        int index = 0;
        while (index < m_parameterBoxes.size()) {
            if (box == m_parameterBoxes[index])
                break;
            ++index;
        }
        if (index < m_parameterBoxes.size()) {
            box->reparent(m_groupBoxes[index], 0, QPoint(0, 0), FALSE);
        }
    } else if (new_container == m_tabBox) {
        box->reparent(new_container, 0, QPoint(0, 0), FALSE);
        m_tabBox->insertTab(box, box->getShortLabel());
    }
}

}
#include "RosegardenParameterArea.moc"
