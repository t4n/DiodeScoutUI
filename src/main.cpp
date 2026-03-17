// ---------------------------------------------------------------------------
//  Entry point of the DiodeScout application. This file initializes the Qt
//  application environment, applies the dark Fusion UI theme, loads the
//  application icon, and launches the MainWindow instance.
//
//  All UI logic and serial communication are handled inside MainWindow.
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
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

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/appicon.svg"));
    a.setStyle(QStyleFactory::create("Fusion"));
    a.setPalette(darkPalette);

    MainWindow w;
    w.show();
    w.resize(800, 600);
    return a.exec();
}
