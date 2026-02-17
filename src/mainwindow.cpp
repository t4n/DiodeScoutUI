// ---------------------------------------------------------------------------
//  Main application window for the DiodeScout UI, responsible for:
//
//  - Creating and managing the toolbar and chart view
//  - Handling serial communication with the DiodeScout device
//  - Receiving and parsing measurement data
//  - Updating the chart when new data arrives
//  - Providing user actions (export, reset, clear, exit)
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include <QDateTime>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QValueAxis>

// Rounds a value up to the next 0.5 step.
inline double roundUpToHalf(double value)
{
    return std::ceil(value * 2.0) / 2.0;
}

// Constructs the main window and initializes UI components.
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DontShowOnScreen, true);

    // Toolbar
    auto *toolbar = new QToolBar("Main Toolbar", this);
    toolbar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto *spacer1 = new QWidget(toolbar);
    auto *spacer2 = new QWidget(toolbar);
    spacer1->setFixedWidth(20);
    spacer2->setFixedWidth(20);

    restoreViewAct = toolbar->addAction(QIcon(":/icons/restoreview.svg"), "Restore default view");
    toolbar->addWidget(spacer1);
    exportCSVAct = toolbar->addAction(QIcon(":/icons/exportcsv.svg"), "Export CSV");
    exportPythonAct = toolbar->addAction(QIcon(":/icons/exportpython.svg"), "Export Python script");
    exportPNGAct = toolbar->addAction(QIcon(":/icons/exportpng.svg"), "Export PNG");
    toolbar->addWidget(spacer2);
    removeLastAct = toolbar->addAction(QIcon(":/icons/removelast.svg"), "Remove last series");
    removeAllAct = toolbar->addAction(QIcon(":/icons/removeall.svg"), "Remove all series");
    quitAct = toolbar->addAction(QIcon(":/icons/quit.svg"), "Quit");

    connect(restoreViewAct, &QAction::triggered, this, &MainWindow::onRestoreViewClicked);
    connect(exportCSVAct, &QAction::triggered, this, &MainWindow::onExportCSVClicked);
    connect(exportPythonAct, &QAction::triggered, this, &MainWindow::onExportPythonClicked);
    connect(exportPNGAct, &QAction::triggered, this, &MainWindow::onExportPNGClicked);
    connect(removeLastAct, &QAction::triggered, this, &MainWindow::onRemoveLastClicked);
    connect(removeAllAct, &QAction::triggered, this, &MainWindow::onRemoveAllClicked);
    connect(quitAct, &QAction::triggered, this, &MainWindow::onQuitClicked);

    // Chart setup
    chart = new QChart();
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->setTitleFont(titleFont);
    chart->setTitle("Press the button on the DiodeScout ...");

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::RectangleRubberBand);
    setCentralWidget(chartView);

    // Connect to DiodeScout
    if (!findAndOpenDiodeScout())
    {
        QMessageBox::warning(this, "DiodeScoutUI", "No DiodeScout device detected.\nPlease check the connection.");
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        return;
    }

    // Connection successful, now allow the window to be shown
    setAttribute(Qt::WA_DontShowOnScreen, false);
}

// Triggered when the user selects "Restore default view".
void MainWindow::onRestoreViewClicked()
{
    chart->zoomReset();
}

// Triggered when the user selects "Export CSV".
void MainWindow::onExportCSVClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export CSV",
        QDir::homePath() + "/dscout.csv",
        "CSV file (*.csv)",
        nullptr,
        QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        if (!dataManager.exportCSV(fileName.toStdString()))
            QMessageBox::warning(this, "Error", "CSV export failed.");
    }
}

// Triggered when the user selects "Export Python script".
void MainWindow::onExportPythonClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Python script",
        QDir::homePath() + "/dscout.py",
        "Python script (*.py)",
        nullptr,
        QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        if (!dataManager.exportPython(fileName.toStdString()))
            QMessageBox::warning(this, "Error", "Python export failed.");
    }
}

// Triggered when the user selects "Export PNG".
void MainWindow::onExportPNGClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export PNG",
        QDir::homePath() + "/dscout.png",
        "PNG file (*.png)",
        nullptr,
        QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        QPixmap pixmap(chartView->size());
        pixmap.fill(Qt::white);

        QPainter painter(&pixmap);
        chartView->render(&painter);
        painter.end();

        if (!pixmap.save(fileName, "PNG"))
            QMessageBox::warning(this, "Error", "PNG export failed.");
    }
}

// Triggered when the user selects "Remove last series".
void MainWindow::onRemoveLastClicked()
{
    dataManager.removeLastSeries();
    if (dataManager.seriesCount() == 0)
    {
        resetChartToEmpty();
    }
    else
    {
        rebuildChart();
    }
}

// Triggered when the user selects "Remove all series".
void MainWindow::onRemoveAllClicked()
{
    dataManager.removeAllSeries();
    resetChartToEmpty();
}

// Triggered when the user selects "Quit".
void MainWindow::onQuitClicked()
{
    qApp->quit();
}

// Search for the DiodeScout device and open the serial port.
bool MainWindow::findAndOpenDiodeScout()
{
    // 1) Try to automatically detect the DiodeScout device
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        QString hw = info.description() + " " + info.manufacturer();
        hw += " " + info.serialNumber() + " " + info.systemLocation();

        if (hw.contains("DIODESCOUT", Qt::CaseInsensitive))
        {
            return openSerialPort(info);
        }
    }

    // 2) DiodeScout not found, ask the user to select a serial port
    QStringList portNames;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    for (const auto &p : ports)
        portNames << p.systemLocation() + "   (" + p.description() + ")";

    bool ok = false;
    QString choice = QInputDialog::getItem(this,
        "DiodeScoutUI",
        "No DiodeScout device detected.\nPlease select the correct serial port:",
        portNames,
        0,
        false,
        &ok);

    if (!ok || choice.isEmpty())
        return false;

    int index = portNames.indexOf(choice);
    if (index < 0)
        return false;

    return openSerialPort(ports[index]);
}

// Opens the given serial port and initializes the connection.
bool MainWindow::openSerialPort(const QSerialPortInfo &info)
{
    serial = new QSerialPort(info, this);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite))
        return false;

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived);

    QString prettyName = info.systemLocation();
    prettyName.replace("\\\\.\\", "");
    statusBar()->showMessage(QString("DiodeScout at %1").arg(prettyName));
    return true;
}

// Reads all available serial data and processes it byte-by-byte.
void MainWindow::onSerialDataReceived()
{
    const QByteArray data = serial->readAll();
    for (auto c : data)
        handleSerialByte(c);
}

// Handles a single received byte from the serial interface.
void MainWindow::handleSerialByte(char c)
{
    auto result = dataManager.processReceivedChar(c);
    if (result == ParseResult::SeriesCompleted)
    {
        statusBar()->showMessage("Ready");
        rebuildChart();
    }
    else if (c == '\n')
    {
        int n = dataManager.tempSeriesSize();
        statusBar()->showMessage(QString("Receiving data ") + QString(n, '.'));
    }
}

// Rebuilds the chart from all stored measurement series.
void MainWindow::rebuildChart()
{
    if (dataManager.seriesCount() == 0)
    {
        resetChartToEmpty();
        return;
    }

    chart->removeAllSeries();
    const auto &all = dataManager.allSeries();
    for (const auto &seriesData : all)
    {
        auto *line = new QLineSeries();
        for (const auto &p : seriesData.points())
            line->append(p.voltageVolt, p.currentMilliAmp);

        chart->addSeries(line);
    }

    chart->setTitle(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    chart->createDefaultAxes();
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto axesX = chart->axes(Qt::Horizontal);
    if (!axesX.isEmpty())
    {
        auto *axisX = qobject_cast<QValueAxis *>(axesX.first());
        if (axisX)
        {
            axisX->setTitleText("Volt (V)");
            axisX->setTickType(QValueAxis::TicksDynamic);
            axisX->setRange(0, roundUpToHalf(dataManager.getMaxVoltage()));
            axisX->setTickInterval(0.5);
            axisX->setMinorTickCount(4);
        }
    }

    auto axesY = chart->axes(Qt::Vertical);
    if (!axesY.isEmpty())
    {
        auto *axisY = qobject_cast<QValueAxis *>(axesY.first());
        if (axisY)
        {
            axisY->setLabelFormat("%.2f");
            axisY->setTitleText("\nMilliampere (mA)");
            axisY->setTickType(QValueAxis::TicksDynamic);
            axisY->setRange(0, roundUpToHalf(dataManager.getMaxCurrent()));
            axisY->setTickInterval(0.5);
            axisY->setMinorTickCount(4);
        }
    }
}

// Resets the chart to an empty default state.
//
// Clears all visual content from the chart and restores the initial
// empty-state appearance. Used when no measurement series remain.
// Does not touch the MeasurementDataManager.
void MainWindow::resetChartToEmpty()
{
    chart->removeAllSeries();
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setTitle("Press the button on the DiodeScout ...");

    for (auto *axis : chart->axes(Qt::Horizontal))
        chart->removeAxis(axis);
    for (auto *axis : chart->axes(Qt::Vertical))
        chart->removeAxis(axis);
}
