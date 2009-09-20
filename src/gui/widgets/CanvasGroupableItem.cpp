/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <Q3Canvas>
#include <Q3CanvasEllipse>
#include <Q3CanvasItem>
#include <Q3CanvasItemList>
#include <Q3CanvasLine>
#include <Q3CanvasPixmapArray>
#include <Q3CanvasPolygonalItem>
#include <Q3CanvasRectangle>
#include <Q3CanvasSprite>
#include <Q3CanvasText>
#include <Q3Canvas>
#include "misc/Debug.h"

#include "CanvasGroupableItem.h"

CanvasGroupableItem::CanvasGroupableItem(Q3CanvasItem *i,
        QCanvasItemGroup *g,
        bool withRelativeCoords)
        : m_group(g),
        m_item(i)
{
    //     RG_DEBUG << "CanvasGroupableItem() - this : " << this
    //                          << " - group : " << g
    //                          << " - item : " << i << endl;

    if (withRelativeCoords)
        group()->addItemWithRelativeCoords(item());
    else
        group()->addItem(item());
}

CanvasGroupableItem::~CanvasGroupableItem()
{
    //     RG_DEBUG << "~CanvasGroupableItem() - this : " << this
    //                          << " - group : " << group()
    //                          << " - item : " << item() << endl;

    // remove from the item group if we're still attached to one
    if (group())
        group()->removeItem(item());
}

void
CanvasGroupableItem::relativeMoveBy(double dx, double dy)
{
    m_item->moveBy(dx + m_group->x(),
                   dy + m_group->y());
}

void
CanvasGroupableItem::detach()
{
    m_group = 0;
}

//////////////////////////////////////////////////////////////////////

QCanvasItemGroup::QCanvasItemGroup(Q3Canvas *c)
        : Q3CanvasItem(c)
{
    //     RG_DEBUG << "QCanvasItemGroup() - this : " << this << endl;
}

QCanvasItemGroup::~QCanvasItemGroup()
{
    //     RG_DEBUG << "~QCanvasItemGroup() - this : " << this << endl;

    // Tell all our items that we're being destroyed
    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i) {
        CanvasGroupableItem *t = dynamic_cast<CanvasGroupableItem*>(*i);
        if (t)
            t->detach();
    }
}

void
QCanvasItemGroup::moveBy(double dx, double dy)
{
    Q3CanvasItem::moveBy(dx, dy); // move ourselves

    Q3CanvasItemList::Iterator i; // move group items
    for (i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->moveBy(dx, dy);
}

void
QCanvasItemGroup::advance(int stage)
{
    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->advance(stage);
}

bool
QCanvasItemGroup::collidesWith(const Q3CanvasItem *item) const
{
    Q3CanvasItemList::ConstIterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        if ((*i)->collidesWith(item))
            return true;

    return false;
}

void
QCanvasItemGroup::draw(QPainter&)
{
    // There isn't anything to do - all the items will be drawn
    // seperately by the canvas anyway. However the function has to be
    // implemented because it's an abstract virtual in Q3CanvasItem.

    //     Q3CanvasItemList::Iterator i;
    //     for(i = m_items.begin(); i != m_items.end(); ++i)
    //         (*i)->draw(p);
}

void
QCanvasItemGroup::setVisible(bool yes)
{
    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setSelected(bool yes)
{
    Q3CanvasItem::setSelected(yes);

    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setVisible(yes);
}

void
QCanvasItemGroup::setEnabled(bool yes)
{
    Q3CanvasItem::setEnabled(yes);

    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        (*i)->setEnabled(yes);
}

void
QCanvasItemGroup::setActive(bool yes)
{
    Q3CanvasItem::setActive(yes);

    Q3CanvasItemList::Iterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
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

    Q3CanvasItemList::ConstIterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRect());

    return r;
}

QRect
QCanvasItemGroup::boundingRectAdvanced() const
{
    QRect r;

    Q3CanvasItemList::ConstIterator i;
    for (i = m_items.begin(); i != m_items.end(); ++i)
        r.unite((*i)->boundingRectAdvanced());

    return r;
}

bool
QCanvasItemGroup::collidesWith(const Q3CanvasSprite *s,
                               const Q3CanvasPolygonalItem *p,
                               const Q3CanvasRectangle *r,
                               const Q3CanvasEllipse *e,
                               const Q3CanvasText *t) const
{
    if (s)
        return collidesWith(s);
    else if (p)
        return collidesWith(p);
    else if (r)
        return collidesWith(r);
    else if (e)
        return collidesWith(e);
    else if (t)
        return collidesWith(t);

    return false;

}

void
QCanvasItemGroup::addItem(Q3CanvasItem *i)
{
    m_items.append(i);
}

void
QCanvasItemGroup::addItemWithRelativeCoords(Q3CanvasItem *i)
{
    i->moveBy(x(), y());
    addItem(i);
}

void
QCanvasItemGroup::removeItem(Q3CanvasItem *i)
{
    //     RG_DEBUG << "QCanvasItemGroup::removeItem() - this : "
    //                          << this << " - item : " << i << endl;
    m_items.remove(i);
}

//////////////////////////////////////////////////////////////////////


QCanvasLineGroupable::QCanvasLineGroupable(Q3Canvas *c,
        QCanvasItemGroup *g)
        : Q3CanvasLine(c),
        CanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasRectangleGroupable::QCanvasRectangleGroupable(Q3Canvas *c,
        QCanvasItemGroup *g)
        : Q3CanvasRectangle(c),
        CanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasTextGroupable::QCanvasTextGroupable(const QString& label,
        Q3Canvas *c,
        QCanvasItemGroup *g)
        : Q3CanvasText(label, c),
        CanvasGroupableItem(this, g)
{}

QCanvasTextGroupable::QCanvasTextGroupable(Q3Canvas *c,
        QCanvasItemGroup *g)
        : Q3CanvasText(c),
        CanvasGroupableItem(this, g)
{}

//////////////////////////////////////////////////////////////////////

QCanvasSpriteGroupable::QCanvasSpriteGroupable(Q3CanvasPixmapArray *pa,
        Q3Canvas *c,
        QCanvasItemGroup *g)
        : Q3CanvasSprite(pa, c),
        CanvasGroupableItem(this, g)
{}
