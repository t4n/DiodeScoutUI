// ---------------------------------------------------------------------------
//  Serial connection management for detecting, configuring, and opening
//  a DiodeScout serial connection.
//
//  - Automatically detects the DiodeScout device
//  - Prompts the user to select a serial port if auto-detection fails
//  - Configures serial communication parameters
//  - Opens the serial port for read/write access
// ---------------------------------------------------------------------------

#include "serialconnector.h"
#include <QInputDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

// Opens a DiodeScout serial connection via auto-detection
// or by prompting the user to select a serial port.
bool DiodeScoutSerialConnector::findAndOpen(QSerialPort &serial)
{
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty())
        return false;

    // 1) Try automatic device identification
    for (const QSerialPortInfo &p : ports)
    {
        QString identification = p.description() + ' ' + p.manufacturer();
        identification += ' ' + p.serialNumber() + ' ' + p.systemLocation();

        if (identification.contains("DIODESCOUT", Qt::CaseInsensitive))
            return open(serial, p);
    }

    // 2) Let the user choose a serial port
    QStringList portNames;
    for (const QSerialPortInfo &p : ports)
    {
        QString prettyName = p.systemLocation();
        prettyName.remove("\\\\.\\");
        portNames << prettyName + "   (" + p.description() + ")";
    }

    bool ok = false;
    const QString choice = QInputDialog::getItem(nullptr, "DiodeScoutUI",
        "Select the serial port DiodeScout is connected to:", portNames, 0, false, &ok);

    if (ok && !choice.isEmpty())
    {
        int index = portNames.indexOf(choice);
        if (index >= 0)
            return open(serial, ports.at(index));
    }

    return false;
}

// Configures and opens the serial port.
bool DiodeScoutSerialConnector::open(QSerialPort &serial, const QSerialPortInfo &info)
{
    serial.setPortName(info.portName());

    // Serial parameters are defined by the DiodeScout firmware;
    // do not modify unless the device protocol changes.
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    return serial.open(QIODevice::ReadWrite);
}
