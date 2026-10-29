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
//  ConnectionStatus:
//  Indicates if the DiodeScout serial connection could be established.
// ---------------------------------------------------------------------------
enum class ConnectionStatus
{
    Connected,
    OpenFailed,
    NoConnection // No ports available or manual selection cancelled
};

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Serial connection management for detecting, configuring, and opening
//  a DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Attempts to connect to the DiodeScout. If automatic connection fails,
    // prompts the user to select a serial port. Returns Connected on success,
    // OpenFailed if opening the selected port fails, NoConnection if no ports
    // are available or the port selection dialog is cancelled.
    static ConnectionStatus findAndOpen(QSerialPort &serial);

  private:
    // Configures and opens the serial port.
    static bool open(QSerialPort &serial, const QSerialPortInfo &info);
};
