/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
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

#include "playlistinterface.h"

////////////////////////////////////////////////////////////////////////////////
// Watched implementation
////////////////////////////////////////////////////////////////////////////////

void Watched::update()
{
    for(QValueList<PlaylistObserver *>::ConstIterator it = m_observers.begin();
	it != m_observers.end();
	++it)
    {
        (*it)->update();
    }    
}

void Watched::addObserver(PlaylistObserver *observer)
{
    m_observers.append(observer);
}

void Watched::removeObserver(PlaylistObserver *observer)
{
    m_observers.remove(observer);
}

Watched::~Watched()
{
    QValueList<PlaylistObserver *> l = m_observers;
    for(QValueList<PlaylistObserver *>::Iterator it = l.begin(); it != l.end(); ++it)
        (*it)->clearWatched();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistObserver implementation
////////////////////////////////////////////////////////////////////////////////

PlaylistObserver::~PlaylistObserver()
{
    if(m_playlist)
        m_playlist->removeObserver(this);
}

PlaylistObserver::PlaylistObserver(PlaylistInterface *playlist) :
    m_playlist(playlist)
{
    playlist->addObserver(this);
}

const PlaylistInterface *PlaylistObserver::playlist() const
{
    return m_playlist;
}
