/***************************************************************************
                          tageditor.cpp  -  description
                             -------------------
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#include "tageditor.h"
#include "collectionlist.h"
#include "playlistitem.h"
#include "tag.h"

#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <keditcl.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qdir.h>
#include <qvalidator.h>
#include <qtooltip.h>
#include <qeventloop.h>

#include <id3v1genres.h>

class FileNameValidator : public QValidator
{
public:
    FileNameValidator(QObject *parent, const char *name = 0) :
	QValidator(parent, name) {}

    virtual void fixup(QString &s) const
    {
	s.remove('/');
    }

    virtual State validate(QString &s, int &) const
    {
	if(s.find('/' != -1))
	   return Invalid;
	return Acceptable;
    }
};

class FileBoxToolTip : public QToolTip
{
public:
    FileBoxToolTip(TagEditor *editor, QWidget *widget) :
	QToolTip(widget), m_editor(editor) {}
protected:
    virtual void maybeTip(const QPoint &)
    {
	tip(parentWidget()->rect(), m_editor->items().first()->file().absFilePath());
    }
private:
    TagEditor *m_editor;
};

class FixedHLayout : public QHBoxLayout
{
public:
    FixedHLayout(QWidget *parent, int margin = 0, int spacing = -1, const char *name = 0) :
	QHBoxLayout(parent, margin, spacing, name),
	m_width(-1) {}
    FixedHLayout(QLayout *parentLayout, int spacing = -1, const char *name = 0) :
	QHBoxLayout(parentLayout, spacing, name),
	m_width(-1) {}
    void setWidth(int w = -1)
    {
	m_width = w == -1 ? QHBoxLayout::minimumSize().width() : w;
    }
    virtual QSize minimumSize() const
    {
	QSize s = QHBoxLayout::minimumSize();
	s.setWidth(m_width);
	return s;
    }
private:
    int m_width;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TagEditor::TagEditor(QWidget *parent, const char *name) :
    QWidget(parent, name),
    m_currentPlaylist(0)
{
    setupLayout();
    readConfig();
    m_dataChanged = false;
}

TagEditor::~TagEditor()
{
    saveConfig();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotSetItems(const PlaylistItemList &list)
{
    saveChangesPrompt();

    if(m_currentPlaylist) {
	disconnect(m_currentPlaylist, SIGNAL(signalAboutToRemove(PlaylistItem *)),
		   this, SLOT(slotItemRemoved(PlaylistItem *)));
    }

    m_currentPlaylist = list.isEmpty() ? 0 : static_cast<Playlist *>(list.first()->listView());

    if(m_currentPlaylist) {
	connect(m_currentPlaylist, SIGNAL(signalAboutToRemove(PlaylistItem *)),
		this, SLOT(slotItemRemoved(PlaylistItem *)));
    }

    m_items = list;

    if(isVisible())
	slotRefresh();
}

void TagEditor::slotRefresh()
{
    // This method takes the list of currently selected m_items and tries to 
    // figure out how to show that in the tag editor.  The current strategy --
    // the most common case -- is to just process the first item.  Then we
    // check after that to see if there are other m_items and adjust accordingly.

    if(m_items.isEmpty() || !m_items.first()->file().tag()) {
	slotClear();
	setEnabled(false);
	return;
    }
    
    setEnabled(true);

    PlaylistItem *item = m_items.first();

    Tag *tag = item->file().tag();
	
    m_artistNameBox->setEditText(tag->artist());
    m_trackNameBox->setText(tag->title());
    m_albumNameBox->setEditText(tag->album());

    m_fileNameBox->setText(item->file().fileInfo().fileName());
    new FileBoxToolTip(this, m_fileNameBox);
    m_bitrateBox->setText(QString::number(tag->bitrate()));
    m_lengthBox->setText(tag->lengthString());

    if(m_genreList.findIndex(tag->genre()) >= 0)
	m_genreBox->setCurrentItem(m_genreList.findIndex(tag->genre()) + 1);
    else {
	m_genreBox->setCurrentItem(0);
	m_genreBox->setEditText(tag->genre());
    }

    m_trackSpin->setValue(tag->track());
    m_yearSpin->setValue(tag->year());
    
    m_commentBox->setText(tag->comment());
    
    // Start at the second item, since we've already processed the first.
    
    PlaylistItemList::Iterator it = m_items.begin();
    ++it;

    // If there is more than one item in the m_items that we're dealing with...
    
    if(it != m_items.end()) {

	QValueListIterator<QWidget *> hideIt = m_hideList.begin();
	for(; hideIt != m_hideList.end(); ++hideIt)
	    (*hideIt)->hide();

	BoxMap::Iterator boxIt = m_enableBoxes.begin();
	for(; boxIt != m_enableBoxes.end(); boxIt++) {
	    (*boxIt)->setChecked(true);
	    (*boxIt)->show();
	}
	
	// Yep, this is ugly.  Loop through all of the files checking to see
	// if their fields are the same.  If so, by default, enable their 
	// checkbox.
	
	// Also, if there are more than 50 m_items, don't scan all of them.
	
	if(m_items.count() > 50) {
	    m_enableBoxes[m_artistNameBox]->setChecked(false);
	    m_enableBoxes[m_trackNameBox]->setChecked(false);
	    m_enableBoxes[m_albumNameBox]->setChecked(false);
	    m_enableBoxes[m_genreBox]->setChecked(false);
	    m_enableBoxes[m_trackSpin]->setChecked(false);
	    m_enableBoxes[m_yearSpin]->setChecked(false);
	    m_enableBoxes[m_commentBox]->setChecked(false);
	}
	else {
	    for(; it != m_items.end(); ++it) {
		tag = (*it)->file().tag();

		if(tag) {

		    if(m_artistNameBox->currentText() != tag->artist() &&
		       m_enableBoxes.contains(m_artistNameBox))
		    {
			m_artistNameBox->lineEdit()->clear();
			m_enableBoxes[m_artistNameBox]->setChecked(false);
		    }
		    if(m_trackNameBox->text() != tag->title() &&
		       m_enableBoxes.contains(m_trackNameBox))
		    {
			m_trackNameBox->clear();
			m_enableBoxes[m_trackNameBox]->setChecked(false);
		    }
		    if(m_albumNameBox->currentText() != tag->album() &&
		       m_enableBoxes.contains(m_albumNameBox))
		    {
			m_albumNameBox->lineEdit()->clear();
			m_enableBoxes[m_albumNameBox]->setChecked(false);
		    }
		    if(m_genreBox->currentText() != tag->genre() &&
		       m_enableBoxes.contains(m_genreBox))
		    {
			m_genreBox->lineEdit()->clear();
			m_enableBoxes[m_genreBox]->setChecked(false);
		    }		
		    if(m_trackSpin->value() != tag->track() &&
		       m_enableBoxes.contains(m_trackSpin))
		    {
			m_trackSpin->setValue(0);
			m_enableBoxes[m_trackSpin]->setChecked(false);
		    }		
		    if(m_yearSpin->value() != tag->year() &&
		       m_enableBoxes.contains(m_yearSpin))
		    {
			m_yearSpin->setValue(0);
			m_enableBoxes[m_yearSpin]->setChecked(false);
		    }
		    if(m_commentBox->text() != tag->comment() &&
		       m_enableBoxes.contains(m_commentBox))
		    {
			m_commentBox->clear();
			m_enableBoxes[m_commentBox]->setChecked(false);
		    }
		}
	    }
	}
    }
    else {
	// Clean up in the case that we are only handling one item.

	QValueListIterator<QWidget *> showIt = m_hideList.begin();
	for(; showIt != m_hideList.end(); ++showIt)
	    (*showIt)->show();

	BoxMap::iterator boxIt = m_enableBoxes.begin();
	for(; boxIt != m_enableBoxes.end(); boxIt++) {
	    (*boxIt)->setChecked(true);
	    (*boxIt)->hide();
	}
    }
    m_dataChanged = false;
}

void TagEditor::slotClear()
{
    m_artistNameBox->lineEdit()->clear();
    m_trackNameBox->clear();
    m_albumNameBox->lineEdit()->clear();
    m_genreBox->setCurrentItem(0);
    m_fileNameBox->clear();
    m_trackSpin->setValue(0);
    m_yearSpin->setValue(0);
    m_lengthBox->clear();
    m_bitrateBox->clear();
    m_commentBox->clear();    
}

void TagEditor::slotUpdateCollection()
{
    CollectionList *list = CollectionList::instance();

    if(!list)
	return;
    
    QStringList artistList = list->uniqueSet(CollectionList::Artists);
    m_artistNameBox->listBox()->clear();
    m_artistNameBox->listBox()->insertStringList(artistList);
    m_artistNameBox->completionObject()->setItems(artistList);

    QStringList albumList = list->uniqueSet(CollectionList::Albums);
    m_albumNameBox->listBox()->clear();
    m_albumNameBox->listBox()->insertStringList(albumList);
    m_albumNameBox->completionObject()->setItems(albumList);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TagEditor::readConfig()
{
    KConfig *config = KGlobal::config();
    { // combo box completion modes
	KConfigGroupSaver saver(config, "TagEditor");
	if(m_artistNameBox && m_albumNameBox) {
	    readCompletionMode(config, m_artistNameBox, "ArtistNameBoxMode");
	    readCompletionMode(config, m_albumNameBox, "AlbumNameBoxMode");
	    readCompletionMode(config, m_genreBox, "GenreBoxMode");
        }
    }

    TagLib::StringList genres = TagLib::ID3v1::genreList();

    for(TagLib::StringList::ConstIterator it = genres.begin(); it != genres.end(); ++it)
	m_genreList.append(TStringToQString((*it)));
    m_genreList.sort();

    m_genreBox->clear();
    m_genreBox->insertItem(QString::null);
    m_genreBox->insertStringList(m_genreList);
    m_genreBox->completionObject()->setItems(m_genreList);
}

void TagEditor::readCompletionMode(KConfig *config, KComboBox *box, const QString &key)
{
    KGlobalSettings::Completion mode =
	KGlobalSettings::Completion(config->readNumEntry(key, KGlobalSettings::CompletionAuto));

    box->setCompletionMode(mode);
}

void TagEditor::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // combo box completion modes
        KConfigGroupSaver saver(config, "TagEditor");
        if(m_artistNameBox && m_albumNameBox) {
	    config->writeEntry("ArtistNameBoxMode", m_artistNameBox->completionMode());
	    config->writeEntry("AlbumNameBoxMode", m_albumNameBox->completionMode());
	    config->writeEntry("GenreBoxMode", m_genreBox->completionMode());
        }
    }

}

void TagEditor::setupLayout()
{
    static const int horizontalSpacing = 12;
    static const int verticalSpacing = 2;

    QHBoxLayout *layout = new QHBoxLayout(this, 6, horizontalSpacing);

    //////////////////////////////////////////////////////////////////////////////
    // define two columns of the bottem layout
    //////////////////////////////////////////////////////////////////////////////
    QVBoxLayout *leftColumnLayout = new QVBoxLayout(layout, verticalSpacing);
    QVBoxLayout *rightColumnLayout = new QVBoxLayout(layout, verticalSpacing);

    layout->setStretchFactor(leftColumnLayout, 2);
    layout->setStretchFactor(rightColumnLayout, 3);

    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the left column -- all of the field names are class wide
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
    	
	m_artistNameBox = new KComboBox(true, this, "artistNameBox");
	m_artistNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("&Artist name:"), m_artistNameBox, leftColumnLayout, "personal");

        m_trackNameBox = new KLineEdit(this, "trackNameBox");
	addItem(i18n("&Track name:"), m_trackNameBox, leftColumnLayout, "player_play");

	m_albumNameBox = new KComboBox(true, this, "albumNameBox");
	m_albumNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Album &name:"), m_albumNameBox, leftColumnLayout, "cdrom_unmount");

        m_genreBox = new KComboBox(true, this, "genreBox");
	addItem(i18n("&Genre:"), m_genreBox, leftColumnLayout, "knotify");

        // this fills the space at the bottem of the left column
        leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
						  QSizePolicy::Expanding));
    }
    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the right column
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
	
	QHBoxLayout *fileNameLayout = new QHBoxLayout(rightColumnLayout,
						      horizontalSpacing);

	m_fileNameBox = new KLineEdit(this, "fileNameBox");
	m_fileNameBox->setValidator(new FileNameValidator(m_fileNameBox));	

	QLabel *fileNameIcon = new QLabel(this);
	fileNameIcon->setPixmap(SmallIcon("sound"));
	QWidget *fileNameLabel = addHidden(new QLabel(m_fileNameBox, i18n("&File name:"), this));

	fileNameLayout->addWidget(addHidden(fileNameIcon));
	fileNameLayout->addWidget(fileNameLabel);
	fileNameLayout->setStretchFactor(fileNameIcon, 0);
	fileNameLayout->setStretchFactor(fileNameLabel, 0);
	fileNameLayout->insertStretch(-1, 1);
	rightColumnLayout->addWidget(addHidden(m_fileNameBox));

        { // lay out the track row
            FixedHLayout *trackRowLayout = new FixedHLayout(rightColumnLayout,
							    horizontalSpacing);

	    m_trackSpin = new KIntSpinBox(0, 255, 1, 0, 10, this, "trackSpin");
	    addItem(i18n("T&rack:"), m_trackSpin, trackRowLayout);
	    m_trackSpin->installEventFilter(this);

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    m_yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "yearSpin");
	    addItem(i18n("&Year:"), m_yearSpin, trackRowLayout);
	    m_yearSpin->installEventFilter(this);

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    trackRowLayout->addWidget(addHidden(new QLabel(i18n("Length:"), this)));
	    m_lengthBox = new KLineEdit(this, "lengthBox");
	    // addItem(i18n("Length:"), m_lengthBox, trackRowLayout);
	    m_lengthBox->setMaximumWidth(50);
	    m_lengthBox->setAlignment(Qt::AlignRight);
	    m_lengthBox->setReadOnly(true);
	    trackRowLayout->addWidget(addHidden(m_lengthBox));

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    trackRowLayout->addWidget(addHidden(new QLabel(i18n("Bitrate:"), this)));
	    m_bitrateBox = new KLineEdit(this, "bitrateBox");
	    // addItem(i18n("Bitrate:"), m_bitrateBox, trackRowLayout);
	    m_bitrateBox->setMaximumWidth(50);
	    m_bitrateBox->setAlignment(Qt::AlignRight);
	    m_bitrateBox->setReadOnly(true);
	    trackRowLayout->addWidget(addHidden(m_bitrateBox));

	    trackRowLayout->setWidth();
        }

        m_commentBox = new KEdit(this, "commentBox");
	m_commentBox->setTextFormat(Qt::PlainText);
	addItem(i18n("&Comment:"), m_commentBox, rightColumnLayout, "edit");
    	fileNameLabel->setMinimumHeight(m_trackSpin->height());

    }

    connect(m_artistNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_trackNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_albumNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_genreBox, SIGNAL(activated(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_genreBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_fileNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_yearSpin, SIGNAL(valueChanged(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_trackSpin, SIGNAL(valueChanged(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_commentBox, SIGNAL(textChanged()),
	    this, SLOT(slotDataChanged()));
}

void TagEditor::save(const PlaylistItemList &list)
{
    if(!list.isEmpty() && m_dataChanged) {
	
	KApplication::setOverrideCursor(Qt::waitCursor);

	// To keep track of the files that don't cooperate...

	QStringList errorFiles;
	
	for(PlaylistItemList::ConstIterator it = list.begin(); it != list.end(); ++it) {
	    PlaylistItem *item = *it;
	    
	    QFileInfo newFile(item->file().fileInfo().dirPath() + QDir::separator() +
			      m_fileNameBox->text());
	    QFileInfo directory(item->file().fileInfo().dirPath());
	    
	    // If (the new file is writable or the new file doesn't exist and
	    // it's directory is writable) and the old file is writable...  
	    // If not we'll append it to errorFiles to tell the user which
	    // files we couldn't write to.
	    
	    if(item &&
	       item->file().tag() &&
	       (newFile.isWritable() || (!newFile.exists() && directory.isWritable())) &&
	       item->file().fileInfo().isWritable())
	    {
		
		// If the file name in the box doesn't match the current file
		// name...
		
		if(list.count() == 1 &&
		   item->file().fileInfo().fileName() != newFile.fileName())
		{
		    
		    // Rename the file if it doesn't exist or the user says
		    // that it's ok.
		    
		    if(!newFile.exists() ||
		       KMessageBox::warningYesNo(
			   this, 
			   i18n("This file already exists.\nDo you want to replace it?"),
			   i18n("File Exists")) == KMessageBox::Yes)
		    {
			QDir currentDir;
			currentDir.rename(item->file().absFilePath(), newFile.filePath());
			item->file().setFile(newFile.filePath());
		    }
		}
		
		// A bit more ugliness.  If there are multiple files that are
		// being modified, they each have a "enabled" checkbox that
		// says if that field is to be respected for the multiple 
		// files.  We have to check to see if that is enabled before
		// each field that we write.
		
		if(m_enableBoxes[m_artistNameBox]->isOn())
		    item->file().tag()->setArtist(m_artistNameBox->currentText());
		if(m_enableBoxes[m_trackNameBox]->isOn())
		    item->file().tag()->setTitle(m_trackNameBox->text());
		if(m_enableBoxes[m_albumNameBox]->isOn())
		    item->file().tag()->setAlbum(m_albumNameBox->currentText());
		if(m_enableBoxes[m_trackSpin]->isOn())
		    item->file().tag()->setTrack(m_trackSpin->value());
		if(m_enableBoxes[m_yearSpin]->isOn())
		    item->file().tag()->setYear(m_yearSpin->value());
		if(m_enableBoxes[m_commentBox]->isOn())
		    item->file().tag()->setComment(m_commentBox->text());
		
		if(m_enableBoxes[m_genreBox]->isOn())
		    item->file().tag()->setGenre(m_genreBox->currentText());
		
		item->file().tag()->save();
		
		item->refresh();
	    }
	    else if(item)
		errorFiles.append(item->file().absFilePath());

	    kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);
	}
	
	if(!errorFiles.isEmpty())
	    KMessageBox::detailedSorry(this,
				       i18n("Could not save to specified file(s)."), 
				       i18n("Could Not Write to:\n") + errorFiles.join("\n"));
	m_dataChanged = false;

	KApplication::restoreOverrideCursor();
    }
}

void TagEditor::saveChangesPrompt()
{
    if(!isVisible() || !m_dataChanged || m_items.isEmpty())
	return;

    QStringList files;

    for(PlaylistItemList::Iterator it = m_items.begin(); it != m_items.end(); it++)
	files.append((*it)->file().absFilePath());

    if(KMessageBox::questionYesNoList(this,
				      i18n("Do you want to save your changes to:\n"), 
				      files, 
				      i18n("Save Changes"),
				      KStdGuiItem::yes(),
				      KStdGuiItem::no(),
				      "tagEditor_showSaveChangesBox") == KMessageBox::Yes)
    {
	save(m_items);
    }
}

void TagEditor::addItem(const QString &text, QWidget *item, QBoxLayout *layout, const QString &iconName)
{
    if(!item || !layout)
	return;

    QLabel *label = new QLabel(item, text, this);
    QLabel *iconLabel = new QLabel(item, 0, this);
    
    if(!iconName.isNull())
	iconLabel->setPixmap(SmallIcon(iconName));
    
    QCheckBox *enableBox = new QCheckBox(i18n("Enable"), this);
    enableBox->setChecked(true);

    label->setMinimumHeight(enableBox->height());

    if(layout->direction() == QBoxLayout::LeftToRight) {
    	layout->addWidget(iconLabel);
	layout->addWidget(label);
	layout->addWidget(item);
	layout->addWidget(enableBox);
    }
    else {
	QHBoxLayout *l = new QHBoxLayout(layout);
	
	l->addWidget(iconLabel);
	l->addWidget(label);
	l->setStretchFactor(label, 1);

	l->insertStretch(-1, 1);

	l->addWidget(enableBox);
	l->setStretchFactor(enableBox, 0);

	layout->addWidget(item);
    }

    enableBox->hide();

    connect(enableBox, SIGNAL(toggled(bool)), item, SLOT(setEnabled(bool)));
    m_enableBoxes.insert(item, enableBox);
}

void TagEditor::showEvent(QShowEvent *e)
{
    slotRefresh();
    QWidget::showEvent(e);
}

bool TagEditor::eventFilter(QObject *watched, QEvent *e)
{
    if(watched->inherits("QSpinBox") && e->type() == QEvent::KeyRelease)
	slotDataChanged();

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotDataChanged(bool c)
{
    m_dataChanged = c;
}

void TagEditor::slotItemRemoved(PlaylistItem *item)
{
    m_items.remove(item);
    if(m_items.isEmpty())
	slotRefresh();
}

#include "tageditor.moc"
