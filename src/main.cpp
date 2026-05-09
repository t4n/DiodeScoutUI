// ---------------------------------------------------------------------------
//  Entry point of the DiodeScout application. Initializes the Qt environment,
//  applies the dark Fusion theme, loads the application icon, establishes the
//  serial connection to the DiodeScout device, and launches the main window.
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
#include <clocale>
#include <locale>

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Utility class for detecting and opening a DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Detect the DiodeScout device and open the serial connection.
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
        const QString choice = QInputDialog::getItem(nullptr,
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

        // Serial parameters are defined by the DiodeScout firmware.
        // Do not modify unless the device protocol changes.
        serial.setBaudRate(QSerialPort::Baud9600);
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);
        return serial.open(QIODevice::ReadWrite);
    }
};

// ---------------------------------------------------------------------------
//  Application entry point.
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Initialize the main application framework
    QApplication application(argc, argv);

    // Qt may use system locale on Unix/Linux, conflicting with POSIX numeric
    // parsing (decimal comma vs dot). Force C-locale for numeric operations.
    std::locale::global(std::locale::classic());
    std::setlocale(LC_NUMERIC, "C");

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
    QStyle *fusion = QStyleFactory::create("Fusion");
    Q_ASSERT(fusion);

    application.setStyle(fusion);
    application.setPalette(darkPalette);
    application.setWindowIcon(QIcon(":/icons/appicon.svg"));

    // Establish DiodeScout serial connection
    QSerialPort diodeScoutPort;
    if (!DiodeScoutSerialConnector::FindAndOpen(diodeScoutPort))
    {
        auto result = QMessageBox::question(nullptr,
            "DiodeScoutUI",
            "No DiodeScout device detected.\nDo you want to start in simulation mode?",
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::No)
            return EXIT_SUCCESS;
    }

    // Show MainWindow
    MainWindow w(diodeScoutPort);
    w.resize(800, 600);
    w.show();
    return application.exec();
}
