/***************************************************************************
    begin                : Tue Nov 9 2004
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

#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <QWidget>
#include <QLabel>
#include <QPointer>
#include <QList>

#include "filehandle.h"
#include "playlistinterface.h"

class QTimer;
class QPoint;

class NowPlayingItem;
class PlaylistCollection;
class Playlist;

/**
 * This is the widget that holds all of the other items and handles updating them
 * when the playing item changes.
 */

class NowPlaying : public QWidget
{
    Q_OBJECT

public:
    NowPlaying(QWidget *parent, PlaylistCollection *collection);
    void addItem(NowPlayingItem *item);
    PlaylistCollection *collection() const;

private slots:
    void slotUpdate();

private:
    struct Observer : public PlaylistObserver
    {
        Observer(NowPlaying *parent, PlaylistInterface *playlist) :
            PlaylistObserver(playlist),
            m_parent(parent) {}
        virtual void updateCurrent() {}
        virtual void updateData() { m_parent->slotUpdate(); }
        NowPlaying *m_parent;
    };
    friend struct Observer;

    Observer m_observer;
    PlaylistCollection *m_collection;
    QList<NowPlayingItem *> m_items;
};

/**
 * Abstract base for the other NowPlaying items.
 */

class NowPlayingItem
{
public:
    virtual ~NowPlayingItem() {}
    virtual void update(const FileHandle &file) = 0;
    NowPlaying *parent() const { return m_parent; }
protected:
    NowPlayingItem(NowPlaying *parent) : m_parent(parent) { parent->addItem(this); }
private:
    NowPlaying *m_parent;
};

/**
 * Displays the cover of the currently playing file if available, or hides
 * itself if not.
 */

class CoverItem : public QLabel, public NowPlayingItem
{
public:
    CoverItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);
    virtual void mouseReleaseEvent(QMouseEvent *event);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dropEvent(QDropEvent *e);

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);

private:
    FileHandle m_file;
    bool m_dragging;
    QPoint m_dragStart;
};

/**
 * Show the text information on the current track and provides links to the
 * album and artist of the currently playing item.
 */

class TrackItem : public QWidget, public NowPlayingItem
{
    Q_OBJECT

public:
    TrackItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);

private slots:
    void slotOpenLink(const QString &link);
    void slotUpdate();

private:
    FileHandle m_file;
    QLabel *m_label;
};

/**
 * Shows up to 10 items of history and links to those items.
 */

class HistoryItem : public QLabel, public NowPlayingItem
{
    Q_OBJECT

public:
    HistoryItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);
    virtual void openLink(const QString &link);

private slots:
    void slotAddPlaying();

private:
    struct Item
    {
        Item() {}
        Item(const QString &a, const FileHandle &f, Playlist *p)
            : anchor(a), file(f), playlist(p) {}

        QString anchor;
        FileHandle file;
        QPointer<Playlist> playlist;
    };

    QList<Item> m_history;
    QLabel *m_label;
    QTimer *m_timer;
    FileHandle m_file;
};

#endif

// vim: set et sw=4 tw=0 sta:
