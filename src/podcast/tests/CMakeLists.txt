#
# Audacity: A Digital Audio Editor
#

set(MODULE_TEST podcast_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/environment.cpp

    ${CMAKE_CURRENT_LIST_DIR}/speechdetectioncore_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tighten_tests.cpp
    )

set(MODULE_TEST_LINK
        podcast
        trackedit

        muse::ui
        muse::ui_qml

        Qt::Quick
)

set(MODULE_TEST_DATA_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(MODULE_TEST_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/..
)

include(SetupGTest)
