/*
 * tagguesser.h - Copyright (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef TAGGUESSER_H
#define TAGGUESSER_H

#include <QRegExp>
//Added by qt3to4:
#include <Q3ValueList>

class FileNameScheme
{
    public:
        typedef Q3ValueList<FileNameScheme> List;

        FileNameScheme() { }
        FileNameScheme(const QString &s);

        bool matches(const QString &s) const;

        QString title() const;
        QString artist() const;
        QString album() const;
        QString track() const;
        QString comment() const;

    private:
        QString composeRegExp(const QString &s) const;

        mutable QRegExp m_regExp;
        int m_titleField;
        int m_artistField;
        int m_albumField;
        int m_trackField;
        int m_commentField;
};

class TagGuesser
{
    public:

        enum Type { FileName = 0, MusicBrainz = 1 };

        static QStringList schemeStrings();
        static void setSchemeStrings(const QStringList &schemes);

        TagGuesser();
        TagGuesser(const QString &absFileName);

        void guess(const QString &absFileName);

        QString title() const { return m_title; }
        QString artist() const { return m_artist; }
        QString album() const { return m_album; }
        QString track() const { return m_track; }
        QString comment() const { return m_comment; }

    private:
        void loadSchemes();
        QString capitalizeWords(const QString &s);

        FileNameScheme::List m_schemes;
        QString m_title;
        QString m_artist;
        QString m_album;
        QString m_track;
        QString m_comment;
};

#endif // TAGGUESSER_H

// vim: set et sw=4 tw=0 sta:
