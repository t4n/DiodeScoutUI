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
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStyleFactory>

// ---------------------------------------------------------------------------
//  DiodeScoutSerialConnector:
//  Utility class for detecting and opening the DiodeScout serial connection.
// ---------------------------------------------------------------------------
class DiodeScoutSerialConnector
{
  public:
    // Opens a DiodeScout serial connection via auto-detection
    // or by prompting the user to select a serial port.
    static bool FindAndOpen(QSerialPort &serial)
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
                return Open(serial, p);
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
                return Open(serial, ports.at(index));
        }

        return false;
    }

  private:
    // Configures and opens the serial port.
    static bool Open(QSerialPort &serial, const QSerialPortInfo &info)
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
//  Displays a Yes/No dialog with the application icon;
//  returns true if the user selects Yes.
// ---------------------------------------------------------------------------
static bool Question(const QString &title, const QString &msg)
{
    constexpr int PixmapSize = 48;
    const QIcon appIcon = QApplication::windowIcon();
    QMessageBox msgBox(nullptr);

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setIconPixmap(appIcon.pixmap(PixmapSize, PixmapSize));
    msgBox.setWindowTitle(title);
    msgBox.setText(msg);

    return msgBox.exec() == QMessageBox::Yes;
}

// ---------------------------------------------------------------------------
//  Application entry point.
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Use DarkPalette for consistent cross-platform styling and to
    // avoid rare macOS native-dialog crashes when launched via "open"
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);

    // Initialize the main application framework
    QApplication application(argc, argv);

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
        const bool simulation = Question("DiodeScoutUI", "No DiodeScout detected.\n"
                                                         "Start in simulation mode?");
        if (!simulation)
            return EXIT_SUCCESS;
    }

    // MainWindow enters simulation mode if the serial port is not open
    MainWindow w(diodeScoutPort);
    w.resize(800, 600);
    w.show();

    return application.exec();
}