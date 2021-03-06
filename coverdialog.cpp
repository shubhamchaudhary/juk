/**
 * Copyright (C) 2005 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "coverdialog.h"

#include <k3listview.h>
#include <k3iconview.h>
#include <k3iconviewsearchline.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kmenu.h>
#include <klocale.h>

#include <QTimer>

#include "covericonview.h"
#include "covermanager.h"
#include "collectionlist.h"

using CoverUtility::CoverIconViewItem;

class AllArtistsListViewItem : public K3ListViewItem
{
public:
    AllArtistsListViewItem(Q3ListView *parent) :
        K3ListViewItem(parent, i18n("&lt;All Artists&gt;"))
    {
    }

    int compare(Q3ListViewItem *, int, bool) const
    {
        return -1; // Always be at the top.
    }
};

class CaseInsensitiveItem : public K3ListViewItem
{
public:
    CaseInsensitiveItem(Q3ListView *parent, const QString &text) :
        K3ListViewItem(parent, text)
    {
    }

    int compare(Q3ListViewItem *item, int column, bool ascending) const
    {
        Q_UNUSED(ascending);
        return text(column).toLower().localeAwareCompare(item->text(column).toLower());
    }
};

CoverDialog::CoverDialog(QWidget *parent) :
    QWidget(parent, Qt::WType_Dialog)
{
    setupUi(this);

    setObjectName( QLatin1String("juk_cover_dialog" ));

    m_covers->setResizeMode(Q3IconView::Adjust);
    m_covers->setGridX(140);
    m_covers->setGridY(150);

    m_searchLine->setIconView(m_covers);
}

CoverDialog::~CoverDialog()
{
}

void CoverDialog::show()
{
    m_artists->clear();
    m_covers->clear();

    QStringList artists = CollectionList::instance()->uniqueSet(CollectionList::Artists);

    m_artists->setSorting(-1);
    new AllArtistsListViewItem(m_artists);
    for(QStringList::ConstIterator it = artists.constBegin(); it != artists.constEnd(); ++it)
        new CaseInsensitiveItem(m_artists, *it);

    m_artists->setSorting(0);

    QTimer::singleShot(0, this, SLOT(loadCovers()));
    QWidget::show();
}

// Here we try to keep the GUI from freezing for too long while we load the
// covers.
void CoverDialog::loadCovers()
{
    CoverDataMapIterator it, end;
    int i = 0;

    it  = CoverManager::begin();
    end = CoverManager::end();

    for(; it != end; ++it) {
        (void) new CoverIconViewItem(it.key(), m_covers);

        // TODO: Threading!
        if(++i == 10) {
            i = 0;
            kapp->processEvents();
        }
    }
}

// TODO: Add a way to show cover art for tracks with no artist.
void CoverDialog::slotArtistClicked(Q3ListViewItem *item)
{
    m_covers->clear();
    if (!item) {
        return;
    }
    if(dynamic_cast<AllArtistsListViewItem *>(item)) {
        // All artists.
        loadCovers();
    }
    else {
        QString artist = item->text(0).toLower();

        CoverDataMapIterator it, end;

        it  = CoverManager::begin();
        end = CoverManager::end();

        for(; it != end; ++it) {
            if(it.value()->artist == artist)
                (void) new CoverIconViewItem(it.key(), m_covers);
        }
    }
}

void CoverDialog::slotContextRequested(Q3IconViewItem *item, const QPoint &pt)
{
    static KMenu *menu = 0;

    if(!item)
        return;

    if(!menu) {
        menu = new KMenu(this);
        menu->addAction(i18n("Remove Cover"), this, SLOT(removeSelectedCover()));
    }

    menu->popup(pt);
}

void CoverDialog::removeSelectedCover()
{
    CoverIconViewItem *coverItem = m_covers->currentItem();

    if(!coverItem || !coverItem->isSelected()) {
        kWarning() << "No item selected for removeSelectedCover.\n";
        return;
    }

    if(!CoverManager::removeCover(coverItem->id()))
        kError() << "Unable to remove selected cover: " << coverItem->id() << endl;
    else
        delete coverItem;
}

#include "coverdialog.moc"

// vim: set et sw=4 tw=0 sta:
