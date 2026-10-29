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
#include <QDebug>
#include <QInputDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

// Attempts to connect to the DiodeScout. If automatic connection fails,
// prompts the user to select a serial port. Returns Connected on success,
// OpenFailed if opening the selected port fails, NoConnection if no ports
// are available or the port selection dialog is cancelled.
ConnectionStatus DiodeScoutSerialConnector::findAndOpen(QSerialPort &serial)
{
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty())
        return ConnectionStatus::NoConnection;

    // 1) Try automatic device identification and connection
    for (const QSerialPortInfo &p : ports)
    {
        QString identification = p.description() + ' ' + p.manufacturer();
        identification += ' ' + p.serialNumber() + ' ' + p.systemLocation();

        if (identification.contains("DIODESCOUT", Qt::CaseInsensitive))
        {
            serial.close();
            serial.clearError();

            if (open(serial, p))
                return ConnectionStatus::Connected;
        }
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
        {
            serial.close();
            serial.clearError();

            if (open(serial, ports.at(index)))
                return ConnectionStatus::Connected;

            qWarning() << "Failed to open" << serial.portName();
            qWarning() << serial.errorString();
            return ConnectionStatus::OpenFailed;
        }
    }

    return ConnectionStatus::NoConnection;
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
