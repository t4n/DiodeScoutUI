// ---------------------------------------------------------------------------
//  Main application window for the DiodeScout UI. Responsible for:
//
//  - Creating and managing the toolbar and chart view
//  - Handling serial communication with the DiodeScout device
//  - Receiving and parsing incoming measurement data
//  - Updating the chart when new data is available
//  - Providing user actions (export, reset, clear, exit)
// ---------------------------------------------------------------------------

#pragma once

#include "measurementdata.h"
#include <QMainWindow>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtSerialPort/QSerialPort>

// ---------------------------------------------------------------------------
//  MainWindow:
//  Primary application window for the DiodeScout user interface.
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // Constructs the main window and initializes UI components.
    MainWindow(QSerialPort &diodeScoutPort);

    // Destructor.
    ~MainWindow() = default;

  private slots:
    // Triggered when the user selects "Restore default view".
    void onRestoreViewClicked();

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

    // Called when serial data is available.
    void onSerialDataReceived();

  private:
    // Active serial port connection to the DiodeScout device.
    QSerialPort &serial;

    // Manages all recorded measurement data and series.
    MeasurementDataManager dataManager;

    // Chart object and chart view (central widget).
    QChart *chart;
    QChartView *chartView;

    // UI actions for menu and toolbar commands.
    QAction *restoreViewAct;
    QAction *exportCSVAct;
    QAction *exportPythonAct;
    QAction *exportPNGAct;
    QAction *removeLastAct;
    QAction *removeAllAct;
    QAction *quitAct;

    // Handles a single received byte from the serial interface.
    void handleSerialByte(char c);

    // Rounds a value up to the next 0.5 increment.
    double roundUpToHalf(double value) const;

    // Rebuilds the chart from all stored measurement series.
    void rebuildChart();

    // Resets the chart to an empty default state.
    void resetChartToEmpty();
};
