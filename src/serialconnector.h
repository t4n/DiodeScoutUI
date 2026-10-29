// ---------------------------------------------------------------------------
//  Serial connection management for detecting, configuring, and opening
//  a DiodeScout serial connection.
//
//  - Automatically detects the DiodeScout device
//  - Prompts the user to select a serial port if auto-detection fails
//  - Configures serial communication parameters
//  - Opens the serial port for read/write access
// ---------------------------------------------------------------------------

#pragma once

#include <QSerialPort>
#include <QSerialPortInfo>

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Serial connection management for detecting, configuring, and opening
//  a DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Opens a DiodeScout serial connection via auto-detection
    // or by prompting the user to select a serial port.
    static bool findAndOpen(QSerialPort &serial);

    // Configures and opens the serial port.
    static bool open(QSerialPort &serial, const QSerialPortInfo &info);
};
