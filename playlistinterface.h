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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include "filehandle.h"


class PlaylistObserver;

class Watched
{
public:
    void addObserver(PlaylistObserver *observer);
    void removeObserver(PlaylistObserver *observer);
    virtual void currentChanged();
    virtual void dataChanged();

protected:
    virtual ~Watched();

private:
    QValueList<PlaylistObserver *> m_observers;
};

/**
 * This is a simple interface that should be used by things that implement a
 * playlist-like API.
 */

class PlaylistInterface : public Watched
{
public:
    virtual QString name() const = 0;
    virtual FileHandle currentFile() const = 0;
    virtual int time() const = 0;
    virtual int count() const = 0;

    virtual void playNext() = 0;
    virtual void playPrevious() = 0;
    virtual void stop() = 0;

    virtual bool playing() const = 0;
};

class PlaylistObserver
{
public:
    virtual ~PlaylistObserver();
    virtual void updateCurrent() = 0;
    virtual void updateData() = 0;

    void clearWatched() { m_playlist = 0; }

protected:
    PlaylistObserver(PlaylistInterface *playlist);
    const PlaylistInterface *playlist() const;

private:
    PlaylistInterface *m_playlist;
};

#endif
