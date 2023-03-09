cmake_minimum_required(VERSION 3.5)

project(KBTinfo VERSION 1.0.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools SerialPort Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools SerialPort Charts)

set(TS_FILES KBTinfo_en_001.ts)

set(HELPERS SettingsNames.h SerialPort.h SerialPort.cpp)

set(DLG_OPTIONS OptionsDialog.h OptionsDialog.cpp OptionsDialog.ui)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
        ${DLG_OPTIONS}
        ${HELPERS}
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    if(WIN32)
        set(WIN32_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/windows/winres.rc")
        qt_add_executable(KBTinfo
            MANUAL_FINALIZATION
            resources.qrc
            ${PROJECT_SOURCES}
            ${WIN32_RESOURCES}
        )
    else()
        qt_add_executable(KBTinfo
            MANUAL_FINALIZATION
            resources.qrc
            ${PROJECT_SOURCES}
        )
    endif()
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET KBTinfo APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(KBTinfo SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(KBTinfo
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(KBTinfo PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Charts
)

set_target_properties(KBTinfo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS KBTinfo
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(KBTinfo)
endif()
