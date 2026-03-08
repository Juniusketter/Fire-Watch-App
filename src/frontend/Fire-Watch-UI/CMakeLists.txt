cmake_minimum_required(VERSION 3.16)

project(Fire-Watch-UI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This grabs the standard widgets and the SQL database tools
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(Fire-Watch-UI
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
)

# This links those tools to your specific app
target_link_libraries(Fire-Watch-UI PRIVATE Qt6::Widgets Qt6::Sql)

include(GNUInstallDirs)
install(TARGETS Fire-Watch-UI
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Fire-Watch-UI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
