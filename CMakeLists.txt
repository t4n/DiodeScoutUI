# CMakeLists.txt for DiodeScoutUI
cmake_minimum_required(VERSION 3.19)
project(DiodeScoutUI LANGUAGES CXX)

# Qt packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    SerialPort
    Charts
)

qt_standard_project_setup()

# Executable target
qt_add_executable(DiodeScoutUI
    WIN32
    MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/measurementdata.cpp
    src/measurementdata.h
)

target_compile_features(DiodeScoutUI PRIVATE cxx_std_17)

# Application icon (macOS bundle)
if(APPLE)
    target_sources(DiodeScoutUI PRIVATE icons/appicon.icns)
    set_source_files_properties(icons/appicon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(DiodeScoutUI PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "appicon.icns")
endif()

# Qt UI resources (embedded icons)
qt_add_resources(DiodeScoutUI "app_resources"
    FILES
        icons/appicon.svg
        icons/exportcsv.svg
        icons/exportpng.svg
        icons/exportpython.svg
        icons/quit.svg
        icons/removeall.svg
        icons/removelast.svg
        icons/restoreview.svg
)

# Link Qt libraries
target_link_libraries(DiodeScoutUI PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::SerialPort
    Qt::Charts
)
