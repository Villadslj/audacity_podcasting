#
# Audacity: A Digital Audio Editor
#

declare_module(podcast)

set(MODULE_QRC podcast.qrc)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/podcastmodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/podcastmodule.h
    ${CMAKE_CURRENT_LIST_DIR}/podcasterrors.h
    ${CMAKE_CURRENT_LIST_DIR}/podcasttypes.h
    ${CMAKE_CURRENT_LIST_DIR}/ipodcastconfiguration.h
    ${CMAKE_CURRENT_LIST_DIR}/ipodcasteditservice.h
    ${CMAKE_CURRENT_LIST_DIR}/ispeechdetector.h

    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastactionscontroller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastactionscontroller.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastconfiguration.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastconfiguration.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcasteditservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcasteditservice.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastuiactions.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/podcastuiactions.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/speechdetectioncore.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/speechdetectioncore.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/speechdetector.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/speechdetector.h

    ${CMAKE_CURRENT_LIST_DIR}/view/podcastsettingsmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/view/podcastsettingsmodel.h
    )

set(MODULE_LINK
        trackedit
        au3wrap
        au3-label-track
        au3-math
        au3-project
        au3-track
        au3-wave-track
        muse::draw
        Qt::Quick
)

set(MODULE_USE_UNITY OFF)

setup_module()

if (AU_BUILD_PODCAST_TESTS)
    add_subdirectory(tests)
endif()
