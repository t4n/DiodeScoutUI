// ---------------------------------------------------------------------------
//  Main application window for the DiodeScout UI.
//
//  Responsibilities:
//  - Creating and managing the toolbar, chart, and overall UI layout
//  - Handling serial communication with the DiodeScout device
//  - Forwarding incoming serial data to the SerialParser
//  - Updating the chart when new measurement series become available
//  - Providing user actions (export, reset, clear, exit)
// ---------------------------------------------------------------------------

#pragma once

#include "datamanager.h"
#include "serialparser.h"
#include <QMainWindow>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtSerialPort/QSerialPort>

// ---------------------------------------------------------------------------
//  MainWindow:
//  Main application window for the DiodeScout UI.
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // Constructs the main window and initializes UI components.
    MainWindow(QSerialPort &diodeScoutPort);

  private slots:
    // Triggered when the user selects "Restore default view".
    void onRestoreViewClicked();

    // Triggered when the user selects "Light mode".
    void onLightModeClicked();

    // Triggered when the user selects "Dark mode".
    void onDarkModeClicked();

    // Triggered when the user selects "Compute piecewise-linear diode model".
    void onComputePWL();

    // Triggered when the user selects "Export CSV".
    void onExportCSVClicked();

    // Triggered when the user selects "Export Python script".
    void onExportPythonClicked();

    // Triggered when the user selects "Export PNG".
    void onExportPNGClicked();

    // Triggered when the user selects "Remove last series".
    void onRemoveLastClicked();

    // Triggered when the user selects "Remove all series".
    void onRemoveAllClicked();

    // Triggered when the user selects "Quit".
    void onQuitClicked();

    // Reads all available serial data and forwards it to the SerialParser.
    void onSerialDataReceived();

  private:
    // Active serial port connection to the DiodeScout device.
    QSerialPort &serial_;

    // Stores measurement series and provides analysis/export utilities.
    MeasurementDataManager dataManager_;

    // Parses incoming serial data forwarded from the serial port handler.
    SerialParser serialParser_;

    // Chart object and chart view (central widget).
    QChart *chart_;
    QChartView *chartView_;

    // UI actions for menu and toolbar commands.
    QAction *restoreViewAct_;
    QAction *lightModeAct_;
    QAction *darkModeAct_;
    QAction *computePWLAct_;
    QAction *exportCSVAct_;
    QAction *exportPythonAct_;
    QAction *exportPNGAct_;
    QAction *removeLastAct_;
    QAction *removeAllAct_;
    QAction *quitAct_;

    // Rounds a value up to the next 0.5 increment.
    double roundUpToHalf(double value) const;

    // Rebuilds the chart from all stored measurement series.
    void rebuildChart();

    // Resets the chart to an empty default state.
    void resetChartToEmpty();

    // Slightly increases the title font size.
    void setChartTitleFont();
};
