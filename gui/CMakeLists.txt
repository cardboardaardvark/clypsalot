cmake_minimum_required(VERSION 3.5)

project(clypsalot-gui VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(
    PROJECT_SOURCES

    catalog.hxx catalog.cxx
    createobjectdialog.hxx createobjectdialog.cxx createobjectdialog.ui
    data.hxx data.cxx
    logwindow.hxx logwindow.cxx logwindow.ui
    main.hxx main.cxx
    mainwindow.hxx mainwindow.cxx mainwindow.ui
    object.hxx object.cxx
    objectproperties.hxx objectproperties.cxx
    util.hxx util.cxx
    workarea.hxx workarea.cxx
)

qt_add_executable(
    clypsalot-gui

    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_compile_definitions(clypsalot-gui PRIVATE QT_NO_KEYWORDS CLYPSALOT_GUI_VERSION="${PROJECT_VERSION}")

target_link_libraries(
    clypsalot-gui PRIVATE

    Qt${QT_VERSION_MAJOR}::Widgets
    clypsalot
    clypsalot-test
)

set_target_properties(
    clypsalot-gui PROPERTIES

    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(
    TARGETS clypsalot-gui

    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_finalize_executable(clypsalot-gui)
