cmake_minimum_required(VERSION 3.19)
project(Fire-Watch-UI LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

qt_standard_project_setup()

set(BACKEND_SOURCES
    ../../backend/User.cpp
    ../../backend/Admin.cpp
    ../../backend/Inv.cpp
    ../../backend/ThirdPAdmin.cpp
    ../../backend/ThirdPInv.cpp
)

include_directories(../../backend)

qt_add_executable(Fire-Watch-UI
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    dashboard.cpp
    dashboard.h
    dashboard.ui
    extdialog.cpp
    extdialog.h
    assigndialog.h
    reportdialog.h
    ${BACKEND_SOURCES}
)

target_link_libraries(Fire-Watch-UI
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
)

include(GNUInstallDirs)

install(TARGETS Fire-Watch-UI
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Fire-Watch-UI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
