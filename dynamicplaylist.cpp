/***************************************************************************
                          dynamicplaylist.h
                             -------------------
    begin                : Mon May 5 2003
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

#include "dynamicplaylist.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DynamicPlaylist::DynamicPlaylist(const PlaylistList &playlists, QWidget *parent, const QString &name) :
    Playlist(parent, name),
    m_playlists(playlists),
    m_dirty(true)
{
    for(PlaylistList::ConstIterator it = m_playlists.begin(); it != m_playlists.end(); ++it) {
        if(*it) {
            connect(*it, SIGNAL(signalDataChanged()), this, SLOT(slotSetDirty()));
            connect(*it, SIGNAL(signalNumberOfItemsChanged()), this, SLOT(slotSetDirty()));
        }
        else
            m_playlists.remove(*it);
    }
    connect(CollectionList::instance(), SIGNAL(collectionChanged()), this, SLOT(slotSetDirty()));
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItemList DynamicPlaylist::items() const
{
    PlaylistItemList l;

    for(PlaylistList::ConstIterator it = m_playlists.begin(); it != m_playlists.end(); ++it)
        l += (*it)->items();

    return l;
}

void DynamicPlaylist::showEvent(QShowEvent *e)
{
    if(m_dirty) {
        PlaylistItemList newItems = items();
        if(m_items != newItems) {
            m_items = newItems;
            clear();
            createItems(m_items);
        }
        m_dirty = false;
    }

    Playlist::showEvent(e);
}

#include "dynamicplaylist.moc"
