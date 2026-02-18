## DiodeScoutUI
DiodeScoutUI is a Qt 6 application for visualizing diode I‑V characteristics.
It communicates with an external DiodeScout measurement device via a serial
interface.

## Features
- Plotting with QtCharts
- Serial data acquisition
- Export to PNG, CSV, and Python script

## Build
- mkdir build && cd build
- cmake ..
- cmake --build .

## Structure
- src/     → C++ source code
- icons/   → SVG icons
- docs/    → Documentation and notes

## License
Copyright (C) 2026 Tilman Küpper

This project is released under the GNU General Public License version 3
(GPLv3). See the LICENSE file for details.

## Third‑Party Notices
This project uses the Qt 6 framework, including QtCore, QtWidgets, 
QtSerialPort (LGPLv3) and QtCharts (GPLv3). The corresponding Qt license
texts are included in the licenses/ directory.

This application dynamically links against the Qt 6 libraries. In accordance
with the LGPLv3, users are permitted to replace the Qt libraries with their
own compatible builds.

## Source Code
Binary releases of DiodeScoutUI are provided under the terms of the GNU
GPLv3. The corresponding source code is available at:
https://github.com/t4n/DiodeScoutUI

The Qt source code is available from the official Qt download site:
https://download.qt.io/official_releases/qt/
