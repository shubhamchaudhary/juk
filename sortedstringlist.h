/***************************************************************************
                          sortedstringlist.h  -  description
                             -------------------
    begin                : Wed Jan 29 2003
    copyright            : (C) 2003 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SORTEDSTRINGLIST_H
#define SORTEDSTRINGLIST_H

#include <qstring.h>
#include <qstringlist.h>

class SortedStringList 
{
public: 
    SortedStringList();
    ~SortedStringList();

    bool insert(const QString &value);
    bool contains(const QString &value) const;

    /**
     * Returns a sorted list of the values.
     * Warning, this method is expensive and shouldn't be used except when 
     * necessary.
     */
    QStringList values() const;

private:
    class Node;

    /**
     * The insertion implementation.  Returns true if the item was already 
     * present in the list.
     */
    bool BSTInsert(const QString &value);
    void traverse(const Node *n, QStringList &list) const;

    Node *root;
};

#endif
