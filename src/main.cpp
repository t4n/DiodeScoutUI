// ---------------------------------------------------------------------------
//  Entry point of the DiodeScout application. Initializes Qt, applies the
//  dark Fusion theme, loads the application icon, establishes the serial
//  connection to the DiodeScout device, and launches the main window.
//
//  All UI logic and serial communication are handled by MainWindow.
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include <QApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QPalette>
#include <QSerialPortInfo>
#include <QStyleFactory>
#include <clocale>

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Utility class for detecting and opening a DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Opens a DiodeScout serial connection via auto-detection
    // or by prompting the user to select a serial port.
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
            QString prettyName = p.systemLocation().remove("\\\\.\\");
            portNames << prettyName + "   (" + p.description() + ")";
        }

        bool ok = false;
        const QString choice = QInputDialog::getItem(nullptr, "DiodeScoutUI",
            "No DiodeScout device detected.\nPlease select the correct serial port:", portNames, 0, false, &ok);

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
//  Populates a QPalette with the application's dark Fusion color scheme.
// ---------------------------------------------------------------------------
static void FillDarkPalette(QPalette &palette)
{
    // Background colors
    palette.setColor(QPalette::Window, QColor(53, 53, 53)); // Window background
    palette.setColor(QPalette::WindowText, Qt::white); // Window text
    palette.setColor(QPalette::Base, QColor(30, 30, 30)); // Input fields / text areas
    palette.setColor(QPalette::AlternateBase, QColor(45, 45, 45)); // Alternating rows (tables)
    palette.setColor(QPalette::ToolTipBase, QColor(53, 53, 53)); // Tooltip background
    palette.setColor(QPalette::ToolTipText, Qt::white); // Tooltip text

    // Text colors
    palette.setColor(QPalette::Text, Qt::white); // Default text
    palette.setColor(QPalette::BrightText, Qt::red); // Warnings / emphasis
    palette.setColor(QPalette::HighlightedText, Qt::white); // Text on blue highlight

    // Buttons
    palette.setColor(QPalette::Button, QColor(60, 60, 60)); // Button background slightly lighter
    palette.setColor(QPalette::ButtonText, Qt::white); // Button text

    // Links / selection
    palette.setColor(QPalette::Link, QColor(42, 130, 218)); // Links / clickable elements
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Selection / highlight
}

// ---------------------------------------------------------------------------
//  Application entry point.
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Initialize the main application framework
    QApplication application(argc, argv);

    // Force locale-independent decimal separator ('.'),
    // required by MeasurementDataManager and SerialParser,
    // see Qt docs: https://doc.qt.io/qt-6/qcoreapplication.html
    std::setlocale(LC_NUMERIC, "C");
    Q_ASSERT(std::string(".") == std::localeconv()->decimal_point);

    // Apply application theme
    QStyle *fusion = QStyleFactory::create("Fusion");
    Q_ASSERT(fusion);

    QPalette darkPalette;
    FillDarkPalette(darkPalette);

    application.setStyle(fusion);
    application.setPalette(darkPalette);
    application.setWindowIcon(QIcon(":/icons/appicon.svg"));

    // Establish DiodeScout serial connection
    QSerialPort diodeScoutPort;
    if (!DiodeScoutSerialConnector::FindAndOpen(diodeScoutPort))
    {
        auto result = QMessageBox::question(nullptr, "DiodeScoutUI",
            "No DiodeScout device detected.\nDo you want to start in simulation mode?",
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::No)
            return EXIT_SUCCESS;
    }

    // MainWindow enters simulation mode if the serial port is not open
    MainWindow w(diodeScoutPort);
    w.resize(800, 600);
    w.show();
    return application.exec();
}
