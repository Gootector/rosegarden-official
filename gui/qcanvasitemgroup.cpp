/***************************************************************************
                          qcanvasitemgroup.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qcanvasitemgroup.h"

QCanvasItemGroup::QCanvasItemGroup(QCanvas *c)
    : QCanvasItem(c)
{}

QCanvasItemGroup::~QCanvasItemGroup()
{}

void
QCanvasItemGroup::moveBy(double dx, double dy)
{
    QCanvasItem::moveBy(dx, dy); // move ourselves

    QCanvasItemList::Iterator i; // move group items
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->moveBy(dx, dy);
}

void
QCanvasItemGroup::advance(int stage)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->advance(stage);
}

bool
QCanvasItemGroup::collidesWith(const QCanvasItem *item) const
{
    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        if ((*i)->collidesWith(item)) return true;

    return false;
}

void
QCanvasItemGroup::draw(QPainter &p)
{
//     // don't think this is necessary - disabling it for now
//     QCanvasItemList::Iterator i;
//     for(i = m_items.begin(); i != m_items.end(); ++i)
//         (*i)->draw(p);
}

void
QCanvasItemGroup::setVisible(bool yes)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setSelected(bool yes)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setEnabled(bool yes)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setEnabled(yes);
}

void
QCanvasItemGroup::setActive(bool yes)
{
    QCanvasItemList::Iterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setActive(yes);
}

int
QCanvasItemGroup::rtti() const
{
    return 10002;
}

QRect
QCanvasItemGroup::boundingRect() const
{
    QRect r;

    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRect());

    return r;
}

QRect
QCanvasItemGroup::boundingRectAdvanced() const
{
    QRect r;

    QCanvasItemList::ConstIterator i;
    for(i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRectAdvanced());

    return r;
}

bool
QCanvasItemGroup::collidesWith(const QCanvasSprite *s,
                               const QCanvasPolygonalItem *p,
                               const QCanvasRectangle *r,
                               const QCanvasEllipse *e,
                               const QCanvasText *t) const
{
    if (s) return collidesWith(s);
    else if(p) return collidesWith(p);
    else if(r) return collidesWith(r);
    else if(e) return collidesWith(e);
    else if(t) return collidesWith(t);

    return false;

}

void
QCanvasItemGroup::addItem(QCanvasItem *i)
{
    m_items.append(i);
}

void
QCanvasItemGroup::removeItem(QCanvasItem *i)
{
    m_items.remove(i);
}
