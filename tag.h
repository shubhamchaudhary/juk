/***************************************************************************
                          tag.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#ifndef TAG_H
#define TAG_H

#include <qfileinfo.h>

namespace TagLib { class File; }

class CacheDataStream;

/*!
 * This should really be called "metadata" and may at some point be titled as
 * such.  Right now it's mostly a Qt wrapper around TagLib.
 */

class Tag
{
    friend class FileHandle;
public:
    Tag(const QString &fileName);

    void save();

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString genre() const { return m_genre; }
    int track() const { return m_track; }
    int year() const { return m_year; }
    QString comment() const { return m_comment; }

    void setTitle(const QString &value) { m_title = value; }
    void setArtist(const QString &value) { m_artist = value; }
    void setAlbum(const QString &value) { m_album = value; }
    void setGenre(const QString &value) { m_genre = value; }
    void setTrack(int value) { m_track = value; }
    void setYear(int value) { m_year = value; }
    void setComment(const QString &value) { m_comment = value; }

    int seconds() const { return m_seconds; }
    int bitrate() const { return m_bitrate; }

    bool isValid() const { return m_isValid; }

    /**
     * As a convenience, since producing a length string from a number of second
     * isn't a one liner, provide the lenght in string form.
     */
    QString lengthString() const { return m_lengthString; }
    CacheDataStream &read(CacheDataStream &s);

    // TODO -- REMOVE THESE METHODS ONCE THE CACHE IS FILEHANDLE BASED
    const QFileInfo &fileInfo() const { return m_info; }
    const QString &fileName() const { return m_fileName; }

private:
    /**
     * Create an empty tag.  Used in FileHandle for cache restoration.
     */
    Tag(const QString &fileName, bool);
    void setup(TagLib::File *file);

    // TODO -- remove m_info and prefer the one in the FileHandle

    QFileInfo m_info;
    QString m_fileName;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_genre;
    QString m_comment;
    int m_track;
    int m_year;
    int m_seconds;
    int m_bitrate;
    QDateTime m_modificationTime;
    QString m_lengthString;
    bool m_isValid;
};

QDataStream &operator<<(QDataStream &s, const Tag &t);
CacheDataStream &operator>>(CacheDataStream &s, Tag &t);

#endif
