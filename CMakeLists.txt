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

# Installation rules
include(GNUInstallDirs)

install(TARGETS DiodeScoutUI
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Deployment script (Qt deployment)
qt_generate_deploy_app_script(
    TARGET DiodeScoutUI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
