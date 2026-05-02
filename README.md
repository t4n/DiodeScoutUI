# DiodeScoutUI

DiodeScoutUI is a Qt 6 desktop application for visualizing diode I-V
characteristics measured with the DiodeScout device.

## Features

* Plotting with Qt Charts
* Simulation mode for testing without physical hardware
* Serial data acquisition
* Export to PNG, CSV, and Python script
* Computation of piecewise-linear diode model

## Build

1. Install Qt 6 with QtSerialPort, QtCharts, and Qt Creator.
2. Open CMakeLists.txt in Qt Creator.
3. Select a Qt kit and build the project.

## Structure

* src/ → C++ source code
* icons/ → SVG icons
* docs/ → Documentation and notes

## License

Copyright (C) 2026 Tilman Küpper

DiodeScoutUI is licensed under the GNU General Public License
version 3 (GPLv3). See the LICENSE file for the full license text.

## Third-Party Software

This project uses the Qt 6 framework, including QtCore, QtWidgets,
QtSerialPort and QtCharts. Qt modules are licensed under the GNU
LGPLv3, GPLv3, or commercial licenses, depending on the module and
distribution, see:

https://doc.qt.io/qt-6/licensing.html

Copies of applicable third-party license texts are included in the
licenses/ directory.

## Binary Releases

Binary releases of DiodeScoutUI are distributed under GPLv3.
Complete corresponding source code is available at:

https://github.com/t4n/DiodeScoutUI

Distributed binaries may include third-party components such as Qt
libraries, MinGW-w64 runtime libraries, and GCC runtime libraries.
See licenses/ for details.

This application dynamically links against Qt libraries. Users
may replace those libraries with compatible modified versions in
accordance with the LGPLv3.

