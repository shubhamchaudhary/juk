/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kapplication.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kcombobox.h>

#include <q3hbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qeventloop.h>
//Added by qt3to4:
#include <QPixmap>

#include "googlefetcherdialog.h"
#include "tag.h"

GoogleFetcherDialog::GoogleFetcherDialog(const QString &name,
                                         const GoogleImageList &imageList,
                                         const FileHandle &file,
                                         QWidget *parent) :
    KDialogBase(parent, name.latin1(), true, QString::null,
                Ok | Cancel | User1 , NoDefault, true),
    m_pixmap(QPixmap()),
    m_imageList(imageList),
    m_takeIt(false),
    m_newSearch(false),
    m_file(file)
{
    disableResize();

    Q3HBox *mainBox = new Q3HBox(this);
    m_iconWidget = new KIconView(mainBox);
    m_iconWidget->setResizeMode(Q3IconView::Adjust);
    m_iconWidget->setSpacing(10);
    m_iconWidget->setFixedSize(500,550);
    m_iconWidget->arrangeItemsInGrid();
    m_iconWidget->setItemsMovable(FALSE);
    connect(m_iconWidget, SIGNAL(executed(Q3IconViewItem *)),
	    this, SLOT(slotOk()));

    Q3HBox *imgSize = new Q3HBox(actionButton(User1)->parentWidget());
    QLabel *label = new QLabel(imgSize);
    label->setText(i18n("Image size:"));

    KComboBox *combo = new KComboBox(imgSize);
    combo->insertItem(i18n("All Sizes"));
    combo->insertItem(i18n("Very Small"));
    combo->insertItem(i18n("Small"));
    combo->insertItem(i18n("Medium"));
    combo->insertItem(i18n("Large"));
    combo->insertItem(i18n("Very Large"));
    combo->setCurrentItem(0);
    connect(combo, SIGNAL(activated(int)), this, SLOT(imgSizeChanged(int)));

    imgSize->adjustSize();
    setMainWidget(mainBox);
    setButtonText(User1, i18n("New Search"));
}

GoogleFetcherDialog::~GoogleFetcherDialog()
{

}

void GoogleFetcherDialog::setLayout()
{
    setCaption(QString("%1 - %2 (%3)")
              .arg(m_file.tag()->artist())
              .arg(m_file.tag()->album())
              .arg(m_imageList.size()));

    m_iconWidget->clear();
    for(uint i = 0; i < m_imageList.size(); i++)
        new CoverIconViewItem(m_iconWidget, m_imageList[i]);

    adjustSize();
}

void GoogleFetcherDialog::setImageList(const GoogleImageList &imageList)
{
    m_imageList=imageList;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void GoogleFetcherDialog::refreshScreen(GoogleImageList &imageList)
{
    setImageList(imageList);
    setLayout();
}

int GoogleFetcherDialog::exec()
{
    setLayout();
    return KDialogBase::exec();
}

void GoogleFetcherDialog::slotOk()
{
    uint selectedIndex = m_iconWidget->index(m_iconWidget->currentItem());
    m_pixmap = pixmapFromURL(m_imageList[selectedIndex].imageURL());

    if(m_pixmap.isNull()) {
        KMessageBox::sorry(this,
                           i18n("The cover you have selected is unavailable. Please select another."),
                           i18n("Cover Unavailable"));
        QPixmap blankPix;
        blankPix.resize(80, 80);
        blankPix.fill();
        m_iconWidget->currentItem()->setPixmap(blankPix, true, true);
        return;
    }

    m_takeIt = true;
    m_newSearch = false;
    hide();
}

void GoogleFetcherDialog::slotCancel()
{
    m_takeIt = true;
    m_newSearch = false;
    m_pixmap = QPixmap();
    hide();
}

void GoogleFetcherDialog::slotUser1()
{
    m_takeIt = false;
    m_newSearch = true;
    m_pixmap = QPixmap();
    hide();
}

void GoogleFetcherDialog::imgSizeChanged(int index)
{
    GoogleFetcher::ImageSize imageSize = GoogleFetcher::All;
    switch (index) {
        case 1:
            imageSize = GoogleFetcher::Icon;
            break;
        case 2:
            imageSize = GoogleFetcher::Small;
            break;
        case 3:
            imageSize = GoogleFetcher::Medium;
            break;
        case 4:
            imageSize=GoogleFetcher::Large;
            break;
        case 5:
            imageSize=GoogleFetcher::XLarge;
            break;
        default:
            break;
    }
    emit sizeChanged(imageSize);
}

QPixmap GoogleFetcherDialog::fetchedImage(uint index) const
{
    return (index > m_imageList.count()) ? QPixmap() : pixmapFromURL(m_imageList[index].imageURL());
}

QPixmap GoogleFetcherDialog::pixmapFromURL(const KUrl &url) const
{
    QString file;

    if(KIO::NetAccess::download(url, file, 0)) {
        QPixmap pixmap = QPixmap(file);
        KIO::NetAccess::removeTempFile(file);
        return pixmap;
    }
    KIO::NetAccess::removeTempFile(file);
    return QPixmap();
}

////////////////////////////////////////////////////////////////////////////////
// CoverIconViewItem
////////////////////////////////////////////////////////////////////////////////

CoverIconViewItem::CoverIconViewItem(Q3IconView *parent, const GoogleImage &image) :
    QObject(parent), KIconViewItem(parent, parent->lastItem(), image.size()), m_job(0)
{
    // Set up the iconViewItem

    QPixmap mainMap;
    mainMap.resize(80, 80);
    mainMap.fill();
    setPixmap(mainMap, true, true);

    // Start downloading the image.

    m_job = KIO::get(image.thumbURL(), false, false);
    connect(m_job, SIGNAL(result(KIO::Job *)), this, SLOT(imageResult(KIO::Job *)));
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(imageData(KIO::Job *, const QByteArray &)));
}

CoverIconViewItem::~CoverIconViewItem()
{
    if(m_job) {
        m_job->kill();

        // Drain results issued by KIO before being deleted,
        // and before deleting the job.
        kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);

        delete m_job;
    }
}

void CoverIconViewItem::imageData(KIO::Job *, const QByteArray &data)
{
    int currentSize = m_buffer.size();
    m_buffer.resize(currentSize + data.size(), Q3GArray::SpeedOptim);
    memcpy(&(m_buffer.data()[currentSize]), data.data(), data.size());
}

void CoverIconViewItem::imageResult(KIO::Job *job)
{
    if(job->error())
        return;

    QPixmap iconImage(m_buffer);
    iconImage = QImage(iconImage.convertToImage()).smoothScale(80, 80, QImage::ScaleMin);
    setPixmap(iconImage, true, true);
}

#include "googlefetcherdialog.moc"
