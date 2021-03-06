project(juk)
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )
find_package( KDE4 REQUIRED )
include( KDE4Defaults )
include( MacroLibrary )
include( CheckIncludeFileCXX )

find_package( Taglib 1.6 )

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

add_subdirectory( doc )
add_subdirectory( tests )
add_subdirectory( pics )

macro_optional_find_package(TunePimp)
macro_log_feature(TUNEPIMP_FOUND "TunePimp" "A library for developing MusicBrainz enabled tagging applications" "http://www.musicbrainz.org/products/tunepimp" FALSE "" "Provides MusicBrainz tagging in Juk.")
if(TUNEPIMP_FOUND)
	set(HAVE_TUNEPIMP 1)
	if(TUNEPIMP_FOUND_VERSION_4)
		set(HAVE_TUNEPIMP 4)
        endif(TUNEPIMP_FOUND_VERSION_4)	
        if(TUNEPIMP_FOUND_VERSION_5)
                set(HAVE_TUNEPIMP 5)
        endif(TUNEPIMP_FOUND_VERSION_5) 	
else(TUNEPIMP_FOUND)
	set(HAVE_TUNEPIMP 0)
endif(TUNEPIMP_FOUND)	

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############

include_directories( ${TAGLIB_INCLUDES} )
add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS -DQT_STL)

# Look for Ogg Opus support in taglib (not released yet)
macro_push_required_vars()
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${TAGLIB_INCLUDES})
check_include_file_cxx(opusfile.h TAGLIB_HAS_OPUSFILE)
macro_pop_required_vars()

configure_file (config-juk.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-juk.h )

set(tunepimp_SRCS)
if(TUNEPIMP_FOUND)
	set(tunepimp_SRCS trackpickerdialog.cpp)
	include_directories( ${TUNEPIMP_INCLUDE_DIR} )
endif(TUNEPIMP_FOUND)	

set(juk_SRCS  ${tunepimp_SRCS}
   advancedsearchdialog.cpp
   slider.cpp
   svghandler.cpp
   volumepopupbutton.cpp
   actioncollection.cpp
   cache.cpp
   categoryreaderinterface.cpp
   collectionlist.cpp
   coverdialog.cpp
   covericonview.cpp
   coverinfo.cpp
   covermanager.cpp
   coverproxy.cpp
   dbuscollectionproxy.cpp
   deletedialog.cpp
   directorylist.cpp
   dynamicplaylist.cpp
   exampleoptions.cpp
   folderplaylist.cpp
   filehandle.cpp
   filerenamer.cpp
   filerenameroptions.cpp
   filerenamerconfigdlg.cpp
   webimagefetcher.cpp
   historyplaylist.cpp
   juk.cpp
   k3bexporter.cpp
   keydialog.cpp
   lyricswidget.cpp
   main.cpp
   mediafiles.cpp
   mpris2/mediaplayer2.cpp
   mpris2/mediaplayer2player.cpp
   mpris2/mpris2.cpp
   musicbrainzquery.cpp
   nowplaying.cpp
   playermanager.cpp
   playlist.cpp
   playlistbox.cpp
   playlistcollection.cpp
   playlistexporter.cpp
   playlistinterface.cpp
   playlistitem.cpp
   playlistsearch.cpp
   playlistsplitter.cpp
   scrobbler.cpp
   scrobbleconfigdlg.cpp
   searchplaylist.cpp
   searchwidget.cpp
   slideraction.cpp
   sortedstringlist.cpp
   splashscreen.cpp
   statuslabel.cpp
   stringshare.cpp
   systemtray.cpp
   sync/syncEngine.cpp
   sync/syncFront.cpp
   tag.cpp
   tageditor.cpp
   tagguesser.cpp
   tagguesserconfigdlg.cpp
   tagrenameroptions.cpp
   tagtransactionmanager.cpp
   tracksequenceiterator.cpp
   tracksequencemanager.cpp
   treeviewitemplaylist.cpp
   upcomingplaylist.cpp
   ktrm.cpp
   viewmode.cpp )

qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.collection.xml
    dbuscollectionproxy.h DBusCollectionProxy )
qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.player.xml playermanager.h PlayerManager)
qt4_add_dbus_adaptor( juk_SRCS org.kde.juk.search.xml searchwidget.h SearchWidget)


kde4_add_ui_files(juk_SRCS
	filerenamerbase.ui
	filerenameroptionsbase.ui
	directorylistbase.ui
	trackpickerdialogbase.ui
	tagguesserconfigdlgwidget.ui
	exampleoptionsbase.ui
	coverdialogbase.ui
	deletedialogbase.ui
	tageditor.ui
)

kde4_add_app_icon(juk_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-juk.png")
kde4_add_executable(juk ${juk_SRCS})

if(NOT MSVC AND NOT ( WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" ) )
    set( LIBMATH m )
endif(NOT MSVC AND NOT ( WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" ) )

target_link_libraries(juk ${LIBMATH} ${KDE4_KHTML_LIBS} ${TAGLIB_LIBRARIES} ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_PHONON_LIBS})
target_link_libraries(juk ${KDE4_SOLID_LIBS})
if(TUNEPIMP_FOUND)
	target_link_libraries(juk ${TUNEPIMP_LIBRARIES})
endif(TUNEPIMP_FOUND)


install(TARGETS juk  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS juk.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES juk.notifyrc jukui.rc jukui-rtl.rc  DESTINATION  ${DATA_INSTALL_DIR}/juk )
install( FILES jukservicemenu.desktop  DESTINATION
${SERVICES_INSTALL_DIR}/ServiceMenus )
install( FILES org.kde.juk.collection.xml org.kde.juk.player.xml org.kde.juk.search.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR}   )
