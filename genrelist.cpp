/***************************************************************************
                          genrelist.cpp  -  description
                             -------------------
    begin                : Sun Mar 3 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <kdebug.h>

#include "genrelist.h"
#include "genrelistreader.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreList::GenreList(bool createIndex) : QValueList<Genre>()
{
    hasIndex = createIndex;
}

GenreList::GenreList(const QString &file, bool createIndex) : QValueList<Genre>()
{
    hasIndex = createIndex;
    load(file);
}

GenreList::~GenreList()
{

}

void GenreList::load(const QString &file)
{
    GenreListReader *handler = new GenreListReader(this);
    QFile input(file);
    QXmlInputSource source(input);
    QXmlSimpleReader reader;
    reader.setContentHandler(handler);
    reader.parse(source);

    if(hasIndex)
        initializeIndex();
}

QString GenreList::name(int ID3v1)
{
    if(hasIndex && ID3v1 >= 0 && ID3v1 < int(index.size()))
        return(index[ID3v1]);
    else
        return(QString::null);
}

int GenreList::findIndex(const QString &item)
{
    // cache the previous search -- since there are a lot of "two in a row"
    // searchs this should optimize things a little (this is also why this
    // method isn't a "const" method)

    static QString lastItem;
    static int lastIndex;

    // a cache hit

    if(!lastItem.isEmpty() && lastItem == item)
        return(lastIndex);

    int i = 0;
    for(GenreList::Iterator it = begin(); it != end(); ++it) {
        if(item == (*it)) {
            lastItem = item;
            lastIndex = i;
            return(i);
        }
        i++;
    }
    return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void GenreList::initializeIndex()
{
    index.clear();
    index.resize(size());
    for(GenreList::Iterator it = begin(); it != end(); ++it)
        if((*it).getID3v1() >= 0 && (*it).getID3v1() < int(index.size()))
            index[(*it).getID3v1()] = QString(*it);
}
