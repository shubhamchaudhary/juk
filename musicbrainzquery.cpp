/***************************************************************************
    begin                : Tue Aug 3 2004
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

#include "musicbrainzquery.h"

#if HAVE_MUSICBRAINZ

#include "trackpickerdialog.h"
#include "tag.h"
#include "collectionlist.h"

#include <kmainwindow.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfileinfo.h>

MusicBrainzLookup::MusicBrainzLookup(const FileHandle &file) :
    KTRMLookup(file.absFilePath()),
    m_file(file)
{
    message(i18n("Querying MusicBrainz server..."));
}

void MusicBrainzLookup::recognized()
{
    KTRMLookup::recognized();
    confirmation();
}

void MusicBrainzLookup::unrecognized()
{
    KTRMLookup::unrecognized();
    message(i18n("No matches found."));
}

void MusicBrainzLookup::collision()
{
    KTRMLookup::collision();
    confirmation();
}

void MusicBrainzLookup::error()
{
    KTRMLookup::error();
    message(i18n("Error connecting to MusicBrainz server."));
}

void MusicBrainzLookup::message(const QString &s) const
{
    KMainWindow *w = static_cast<KMainWindow *>(kapp->mainWidget());
    w->statusBar()->message(QString("%1 (%2)").arg(s).arg(m_file.fileInfo().fileName()), 3000);
}

void MusicBrainzLookup::confirmation()
{
    // Here we do a bit of queuing to make sure that we don't pop up multiple
    // instances of the confirmation dialog at once.

    static QValueList< QPair<FileHandle, KTRMResultList> > queue;

    if(results().isEmpty())
        return;

    if(!queue.isEmpty()) {
        queue.append(qMakePair(m_file, results()));
        return;
    }

    queue.append(qMakePair(m_file, results()));

    while(!queue.isEmpty()) {
        FileHandle file = queue.first().first;
        KTRMResultList results = queue.first().second;
        TrackPickerDialog dialog(file.fileInfo().fileName(), results);

        if(dialog.exec() == QDialog::Accepted && !dialog.result().isEmpty()) {

            KTRMResult result = dialog.result();

            if(!result.title().isEmpty())
                file.tag()->setTitle(result.title());
            if(!result.artist().isEmpty())
                file.tag()->setArtist(result.artist());
            if(!result.album().isEmpty())
                file.tag()->setAlbum(result.album());
            if(result.track() != 0)
                file.tag()->setTrack(result.track());
            if(result.year() != 0)
                file.tag()->setYear(result.year());

            file.tag()->save();

            CollectionList::instance()->slotRefreshItem(file.absFilePath());
        }

        queue.pop_front();
    }
}

#endif
