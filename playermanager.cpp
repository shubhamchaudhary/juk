/***************************************************************************
    begin                : Sat Feb 14 2004
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

#include <kdebug.h>
#include <klocale.h>

#include <qslider.h>
#include <qtimer.h>

#include "artsplayer.h"
#include "gstreamerplayer.h"
#include "playermanager.h"
#include "playlistinterface.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "actioncollection.h"
#include "collectionlist.h"
#include "tag.h"

#include "config.h"

using namespace ActionCollection;

PlayerManager *PlayerManager::m_instance = 0;

enum PlayerManagerStatus { StatusStopped = -1, StatusPaused = 1, StatusPlaying = 2 };

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

enum SoundSystem { ArtsBackend = 0, GStreamerBackend = 1 };

static Player *createPlayer(int system = ArtsBackend)
{

    Player *p = 0;

#if HAVE_ARTS && HAVE_GSTREAMER
    switch(system) {
    case ArtsBackend:
        p = new ArtsPlayer;
        break;
    case GStreamerBackend:
        p = new GStreamerPlayer;
        break;
    default:
        p = new ArtsPlayer;
        break;
    }
#else
    Q_UNUSED(system)
#if HAVE_ARTS
    p = new ArtsPlayer;
#endif

#if HAVE_GSTREAMER
    p = new GStreamerPlayer;
#else
#warning No Player Backend Available
#endif
#endif

    return p;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlayerManager::PlayerManager() :
    Player(),
    m_sliderAction(0),
    m_playlistInterface(0),
    m_statusLabel(0),
    m_player(0),
    m_timer(0),
    m_noSeek(false),
    m_muted(false)
{
// This class is the first thing constructed during program startup, and
// therefore has no access to the widgets needed by the setup() method.
// Since the setup() method will be called indirectly by the player() method
// later, just disable it here. -- mpyne
//    setup();
}

PlayerManager::~PlayerManager()
{
    delete m_player;
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlayerManager *PlayerManager::instance() // static
{
    if(!m_instance)
        m_instance = new PlayerManager;
    return m_instance;
}

bool PlayerManager::playing() const
{
    if(!player())
        return false;

    return player()->playing();
}

bool PlayerManager::paused() const
{
    if(!player())
        return false;

    return player()->paused();
}

float PlayerManager::volume() const
{
    if(!player())
        return 0;

    return player()->volume();
}

int PlayerManager::status() const
{
    if(!player())
        return StatusStopped;

    if(player()->paused())
        return StatusPaused;

    if(player()->playing())
        return StatusPlaying;
    
    return 0;
}

int PlayerManager::totalTime() const
{
    if(!player())
        return 0;

    return player()->totalTime();
}

int PlayerManager::currentTime() const
{
    if(!player())
        return 0;

    return player()->currentTime();
}

int PlayerManager::position() const
{
    if(!player())
        return 0;

    return player()->position();
}

QStringList PlayerManager::trackProperties()
{
    return FileHandle::properties();
}

QString PlayerManager::trackProperty(const QString &property) const
{
    if(!playing())
        return QString::null;

    return m_file.property(property);
}

FileHandle PlayerManager::playingFile() const
{
    return m_file;
}

QString PlayerManager::playingString() const
{
    if(!playing())
        return QString::null;

    return m_file.tag()->artist() + " - " + m_file.tag()->title();
}

void PlayerManager::setPlaylistInterface(PlaylistInterface *interface)
{
    m_playlistInterface = interface;
}

void PlayerManager::setStatusLabel(StatusLabel *label)
{
    m_statusLabel = label;
}

KSelectAction *PlayerManager::playerSelectAction(QObject *parent) // static
{
    KSelectAction *action = 0;
    action = new KSelectAction(i18n("&Output To"), 0, parent, "outputSelect");
    QStringList l;

#if defined(HAVE_ARTS) && defined(HAVE_GSTREAMER)
    l << i18n("aRts") << i18n("GStreamer");
    action->setItems(l);
#else
    Q_UNUSED(parent)
#endif
    return action;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::play(const FileHandle &file)
{
    if(!player() || !m_playlistInterface)
        return;

    if(file.isNull()) {
        if(player()->paused())
            player()->play();
        else if(player()->playing()) {
            if(m_sliderAction->trackPositionSlider())
                m_sliderAction->trackPositionSlider()->setValue(0);
            player()->seekPosition(0);
        }
        else {
	    m_playlistInterface->playNext();
            m_file = m_playlistInterface->currentFile();

            if(!m_file.isNull())
                player()->play(m_file);
        }
    }
    else {
        m_file = file;
        player()->play(file);
    }

    // Make sure that the player() actually starts before doing anything.

    if(!player()->playing()) {
        stop();
        return;
    }

    action("pause")->setEnabled(true);
    action("stop")->setEnabled(true);
    action("forward")->setEnabled(true);
    action("back")->setEnabled(true);

    if(m_sliderAction->trackPositionSlider())
        m_sliderAction->trackPositionSlider()->setEnabled(true);

    m_timer->start(m_pollInterval);

    emit signalPlay();
}

void PlayerManager::play(const QString &file)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file);
    if(item) {
        Playlist::setPlaying(item);
        play(item->file());
    }
}

void PlayerManager::play()
{
    play(FileHandle::null());
}

void PlayerManager::pause()
{
    if(!player())
        return;

    if(player()->paused()) {
        play();
        return;
    }

    m_timer->stop();
    action("pause")->setEnabled(false);

    player()->pause();

    emit signalPause();
}

void PlayerManager::stop()
{
    if(!player() || !m_playlistInterface)
        return;

    m_timer->stop();

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);

    if(m_sliderAction->trackPositionSlider()) {
        m_sliderAction->trackPositionSlider()->setValue(0);
        m_sliderAction->trackPositionSlider()->setEnabled(false);
    }

    player()->stop();
    m_playlistInterface->stop();

    m_file = FileHandle::null();

    emit signalStop();
}

void PlayerManager::setVolume(float volume)
{
    if(!player())
        return;

    player()->setVolume(volume);
}

void PlayerManager::seek(int seekTime)
{
    if(!player())
        return;

    player()->seek(seekTime);
}

void PlayerManager::seekPosition(int position)
{
    if(!player())
        return;

    if(!player()->playing() || m_noSeek)
        return;

    slotUpdateTime(position);
    player()->seekPosition(position);

    if(m_sliderAction->trackPositionSlider())
        m_sliderAction->trackPositionSlider()->setValue(position);
}

void PlayerManager::seekForward()
{
    seekPosition(kMin(SliderAction::maxPosition, position() + 10));
}

void PlayerManager::seekBack()
{
    seekPosition(kMax(SliderAction::minPosition, position() - 10));
}

void PlayerManager::playPause()
{
    playing() ? action("pause")->activate() : action("play")->activate();
}

void PlayerManager::forward()
{
    m_playlistInterface->playNext();
    FileHandle file = m_playlistInterface->currentFile();

    if(!file.isNull())
        play(file);
    else
        stop();
}

void PlayerManager::back()
{
    m_playlistInterface->playPrevious();
    FileHandle file = m_playlistInterface->currentFile();

    if(!file.isNull())
        play(file);
    else
        stop();
}

void PlayerManager::volumeUp()
{
    if(!player() || !m_sliderAction || !m_sliderAction->volumeSlider())
        return;

    int volume = m_sliderAction->volumeSlider()->volume() +
        m_sliderAction->volumeSlider()->maxValue() / 25; // 4% up

    slotSetVolume(volume);
    m_sliderAction->volumeSlider()->setVolume(volume);
}

void PlayerManager::volumeDown()
{
    if(!player() || !m_sliderAction || !m_sliderAction->volumeSlider())
        return;

    int volume = m_sliderAction->volumeSlider()->value() -
        m_sliderAction->volumeSlider()->maxValue() / 25; // 4% down

    slotSetVolume(volume);
    m_sliderAction->volumeSlider()->setVolume(volume);
}

void PlayerManager::mute()
{
    if(!player() || !m_sliderAction || !m_sliderAction->volumeSlider())
        return;

    slotSetVolume(m_muted ? m_sliderAction->volumeSlider()->value() : 0);
    m_muted = !m_muted;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotPollPlay()
{
    if(!player() || !m_playlistInterface)
        return;

    m_noSeek = true;

    if(!player()->playing()) {
        m_timer->stop();

        m_playlistInterface->playNext();
        FileHandle nextFile = m_playlistInterface->currentFile();
        if(!nextFile.isNull())
            play(nextFile);
        else
            stop();
    }
    else if(!m_sliderAction->dragging()) {
        if(m_sliderAction->trackPositionSlider())
            m_sliderAction->trackPositionSlider()->setValue(player()->position());

        if(m_statusLabel) {
            m_statusLabel->setItemTotalTime(player()->totalTime());
            m_statusLabel->setItemCurrentTime(player()->currentTime());
        }
    }

    // This call is done because when the user adds the slider to the toolbar
    // while playback is occuring the volume slider generally defaults to 0,
    // and doesn't get updated to the correct volume.  It might be better to
    // have the SliderAction class fill in the correct volume, but I'm trying
    // to avoid having it depend on PlayerManager since it may not be
    // constructed in time during startup. -mpyne

    if(!m_sliderAction->volumeDragging() && m_sliderAction->volumeSlider())
        m_sliderAction->volumeSlider()->setVolume((int) (m_sliderAction->volumeSlider()->maxValue() * volume()));

    // Ok, this is weird stuff, but it works pretty well.  Ordinarily we don't
    // need to check up on our playing time very often, but in the span of the
    // last interval, we want to check a lot -- to figure out that we've hit the
    // end of the song as soon as possible.

    if(player()->playing() &&
       player()->totalTime() > 0 &&
       float(player()->totalTime() - player()->currentTime()) < m_pollInterval * 2)
    {
        m_timer->changeInterval(50);
    }

    m_noSeek = false;
}

void PlayerManager::slotSetOutput(int system)
{
    stop();
    delete m_player;
    m_player = createPlayer(system);
    setup();
}

void PlayerManager::slotSetVolume(int volume)
{
    if(m_sliderAction->volumeSlider())
        setVolume(float(volume) / float(m_sliderAction->volumeSlider()->maxValue()));
    else
        setVolume(float(volume) / 100.0);
}

void PlayerManager::slotUpdateTime(int position)
{
    if(!m_statusLabel)
        return;

    float positionFraction = float(position) / SliderAction::maxPosition;
    float totalTime = float(player()->totalTime());
    int seekTime = int(positionFraction * totalTime + 0.5); // "+0.5" for rounding

    m_statusLabel->setItemCurrentTime(seekTime);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

Player *PlayerManager::player() const
{
    if(!m_player)
        instance()->setup();

    return m_player;
}

void PlayerManager::setup()
{
    // All of the actions required by this class should be listed here.

    if(!action("pause") ||
       !action("stop") ||
       !action("back") ||
       !action("forward") ||
       !action("trackPositionAction"))
    {
        kdWarning(65432) << k_funcinfo << "Could not find all of the required actions." << endl;
        return;
    }

    // initialize action states

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);

    // setup sliders

    m_sliderAction = action<SliderAction>("trackPositionAction");

    connect(m_sliderAction, SIGNAL(signalPositionChanged(int)),
            this, SLOT(seekPosition(int)));
    connect(m_sliderAction->trackPositionSlider(), SIGNAL(valueChanged(int)),
            this, SLOT(slotUpdateTime(int)));
    connect(m_sliderAction, SIGNAL(signalVolumeChanged(int)),
            this, SLOT(slotSetVolume(int)));

    // Call this method manually to avoid warnings.
    
    KAction *outputAction = actions()->action("outputSelect");

    if(outputAction) {
        int mediaSystem = static_cast<KSelectAction *>(outputAction)->currentItem();
        m_player = createPlayer(mediaSystem);
        connect(outputAction, SIGNAL(activated(int)), this, SLOT(slotSetOutput(int)));
    }
    else
        m_player = createPlayer();

    float volume;
    
    if(m_sliderAction->volumeSlider()) {
        volume = 
            float(m_sliderAction->volumeSlider()->volume()) /
            float(m_sliderAction->volumeSlider()->maxValue());
    }
    else
        volume = 1; // Assume user wants full volume

    m_player->setVolume(volume);

    m_timer = new QTimer(this, "play timer");
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotPollPlay()));
}

#include "playermanager.moc"

// vim: set et ts=4 sw=4:
