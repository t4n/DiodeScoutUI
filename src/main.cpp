// ---------------------------------------------------------------------------
//  Entry point of the DiodeScout application. Initializes Qt, applies the
//  dark Fusion theme, loads the application icon, establishes the serial
//  connection to the DiodeScout device, and launches the main window.
//
//  All UI logic and serial communication are handled by MainWindow.
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include "serialconnector.h"
#include <QApplication>
#include <QMessageBox>
#include <QPalette>
#include <QSerialPort>
#include <QStyleFactory>
#include <cstdlib>

// ---------------------------------------------------------------------------
//  Populates a QPalette with the application's dark Fusion color scheme.
// ---------------------------------------------------------------------------
static void fillDarkPalette(QPalette &palette)
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
//  Displays a Yes/No dialog; returns true if the user selects Yes.
// ---------------------------------------------------------------------------
static bool question(const QString &title, const QString &text)
{
    // Use a custom dialog instead of QMessageBox::question() because native
    // dialogs have caused rare crashes on macOS when the app is launched
    // from Terminal via "open". Native dialogs are disabled globally now.
    constexpr int PixmapSize = 48;
    const QIcon appIcon = QApplication::windowIcon();
    QMessageBox msgBox(nullptr);

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);
    msgBox.setIconPixmap(appIcon.pixmap(PixmapSize, PixmapSize));
    msgBox.setWindowTitle(title);
    msgBox.setText(text);

    return msgBox.exec() == QMessageBox::Yes;
}

// ---------------------------------------------------------------------------
//  Application entry point.
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Use Qt dialogs for consistent cross-platform styling and to
    // avoid rare macOS native-dialog crashes when launched via "open"
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);

    // Initialize the main application framework
    QApplication application(argc, argv);

    // Apply application theme
    QStyle *fusion = QStyleFactory::create("Fusion");
    Q_ASSERT(fusion);

    QPalette darkPalette;
    fillDarkPalette(darkPalette);

    application.setStyle(fusion);
    application.setPalette(darkPalette);
    application.setWindowIcon(QIcon(":/icons/appicon.svg"));

    // Establish DiodeScout serial connection
    QSerialPort diodeScoutPort;
    if (!DiodeScoutSerialConnector::findAndOpen(diodeScoutPort))
    {
        const QString txt = "No DiodeScout detected.\nStart in simulation mode?";
        const bool simulationRequested = question("DiodeScoutUI", txt);
        if (!simulationRequested)
            return EXIT_SUCCESS;
    }

    // MainWindow enters simulation mode if the serial port is not open
    MainWindow w(diodeScoutPort);
    w.resize(800, 600);
    w.show();

    return application.exec();
}