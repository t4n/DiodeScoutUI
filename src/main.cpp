// ---------------------------------------------------------------------------
//  Entry point of the DiodeScout application. This file initializes the Qt
//  application environment, applies the dark Fusion UI theme, loads the
//  application icon, opens the serial connection to the DiodeScout device,
//  and launches the MainWindow instance.
//
//  All UI logic and serial communication are handled inside MainWindow.
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QPalette>
#include <QSerialPortInfo>
#include <QStyleFactory>

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Handles detecting and opening a DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Locates the DiodeScout device and opens the connection.
    static bool FindAndOpen(QSerialPort &serial)
    {
        // 1) Try automatic detection
        const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &p : ports)
        {
            QString hw = p.description() + ' ' + p.manufacturer();
            hw += ' ' + p.serialNumber() + ' ' + p.systemLocation();

            if (hw.contains("DIODESCOUT", Qt::CaseInsensitive))
                return Open(serial, p);
        }

        // 2) Ask user to select port
        QStringList portNames;
        for (const QSerialPortInfo &p : ports)
        {
            QString prettyName = p.systemLocation();
            prettyName.replace("\\\\.\\", "");
            portNames << prettyName + "   (" + p.description() + ")";
        }

        bool ok = false;
        QString choice = QInputDialog::getItem(nullptr,
            "DiodeScoutUI",
            "No DiodeScout device detected.\nPlease select the correct serial port:",
            portNames,
            0,
            false,
            &ok);

        if (ok)
        {
            int index = portNames.indexOf(choice);
            if (index >= 0)
                return Open(serial, ports.at(index));
        }

        return false;
    }

  private:
    // Configures and opens the serial port.
    static bool Open(QSerialPort &serial, const QSerialPortInfo &info)
    {
        serial.setPortName(info.portName());
        serial.setBaudRate(QSerialPort::Baud9600);
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);
        return serial.open(QIODevice::ReadWrite);
    }
};

// ---------------------------------------------------------------------------
//  Main entry point.
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    // Background colors
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53)); // Window background
    darkPalette.setColor(QPalette::WindowText, Qt::white); // Window text
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 30)); // Input fields / text areas
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45)); // Alternating rows (tables)
    darkPalette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53)); // Tooltip background
    darkPalette.setColor(QPalette::ToolTipText, Qt::white); // Tooltip text

    // Text colors
    darkPalette.setColor(QPalette::Text, Qt::white); // Default text
    darkPalette.setColor(QPalette::BrightText, Qt::red); // Warnings / emphasis
    darkPalette.setColor(QPalette::HighlightedText, Qt::white); // Text on blue highlight

    // Buttons
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60)); // Button background slightly lighter
    darkPalette.setColor(QPalette::ButtonText, Qt::white); // Button text

    // Links / selection
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218)); // Links / clickable elements
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Selection / highlight

    // Make it look good
    application.setWindowIcon(QIcon(":/icons/appicon.svg"));
    application.setStyle(QStyleFactory::create("Fusion"));
    application.setPalette(darkPalette);

    // Establish DiodeScout serial connection
    QSerialPort diodeScoutPort;
    if (!DiodeScoutSerialConnector::FindAndOpen(diodeScoutPort))
    {
        auto result = QMessageBox::question(nullptr,
            "DiodeScoutUI",
            "No DiodeScout device detected.\nDo you want to start in simulation mode?",
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::No)
            return 0;
    }

    // Show MainWindow
    MainWindow w(diodeScoutPort);
    w.resize(800, 600);
    w.show();
    return application.exec();
}
