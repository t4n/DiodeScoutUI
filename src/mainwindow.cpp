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

#include "mainwindow.h"
#include "mychartview.h"
#include <QFileDialog>
#include <QLineSeries>
#include <QMessageBox>
#include <QSplineSeries>
#include <QStatusBar>
#include <QToolBar>

// Main window constructor.
MainWindow::MainWindow(QSerialPort &diodeScoutPort) : serial_(diodeScoutPort)
{
    // Initialize the main window UI, including toolbar and actions.
    setupUI();

    // Setup data source: Use simulation if no hardware is
    // connected, otherwise initialize serial communication.
    if (!serial_.isOpen())
    {
        dataManager_.appendSimulatedSeries();
        statusBar()->showMessage("Simulation");
        rebuildChart();
    }
    else
    {
        QString prettyName = serial_.portName().remove("\\\\.\\");
        connect(&serial_, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived, Qt::QueuedConnection);
        statusBar()->showMessage(QString("DiodeScout at %1").arg(prettyName));
        chart_->setTitle("Press the button on the DiodeScout ...");
    }
}

// Triggered when the user selects "Restore default view".
void MainWindow::onRestoreViewClicked()
{
    rebuildChart();
    statusBar()->showMessage("Ready");
}

// Triggered when the user selects "Light mode".
void MainWindow::onLightModeClicked()
{
    chart_->setTheme(QChart::ChartThemeBlueNcs);
    setChartTitleFont();
}

// Triggered when the user selects "Dark mode".
void MainWindow::onDarkModeClicked()
{
    chart_->setTheme(QChart::ChartThemeBlueCerulean);
    setChartTitleFont();
}

// Triggered when the user selects "Compute piecewise-linear diode model".
void MainWindow::onComputePWL()
{
    double forwardV, seriesR;
    if (!dataManager_.computePWL(forwardV, seriesR))
    {
        QMessageBox::warning(this, "Piecewise-linear diode model",
            "Please ensure that:\n"
            "- Exactly one measurement series is loaded\n"
            "- The diode is connected with the correct polarity\n"
            "- A measurable forward current is present");
        return;
    }

    // Ensure that only the diode I–V curve is visible
    Q_ASSERT(!chart_->series().empty());
    while (chart_->series().size() > 1)
        chart_->removeSeries(chart_->series().at(1));

    // Plot piecewise-linear approximation of diode I–V curve
    double maxV, maxI;
    dataManager_.getMaxVoltageAndCurrent(maxV, maxI);
    maxI /= 1000.0; // convert mA to A

    auto *line = new QLineSeries(chart_);
    line->append(0.0, 0.0);
    line->append(forwardV, 0.0);
    line->append(forwardV + seriesR * maxI, maxI * 1000.0);
    chart_->addSeries(line);

    QPen pen = line->pen();
    pen.setColor(Qt::red);
    line->setPen(pen);

    auto *axisX = chartView_->getAxisX();
    auto *axisY = chartView_->getAxisY();
    if (axisX && axisY)
    {
        line->attachAxis(axisX);
        line->attachAxis(axisY);
    }

    // Show model parameters in status bar
    const char *fmt = "Forward voltage (turn-on): %.2f V, Effective series resistance: %.2f \u03A9";
    QString msg = QString::asprintf(fmt, forwardV, seriesR);
    statusBar()->showMessage(msg);
}

// Triggered when the user selects "Export CSV".
void MainWindow::onExportCSVClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export CSV", QDir::homePath() + "/dscout.csv",
        "CSV file (*.csv)", nullptr, QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        // Locale-based CSV formatting
        const bool germanStyle = (QLocale().decimalPoint() == QString(","));
        const char decimalSeparator = germanStyle ? ',' : '.';
        const char fieldSeparator = germanStyle ? ';' : ',';

        CSVSettings csv(decimalSeparator, fieldSeparator);
        if (!dataManager_.exportCSV(fileName.toStdString(), csv))
            QMessageBox::warning(this, "Error", "CSV export failed.");
    }
}

// Triggered when the user selects "Export Python script".
void MainWindow::onExportPythonClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Python script", QDir::homePath() + "/dscout.py",
        "Python script (*.py)", nullptr, QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        if (!dataManager_.exportPython(fileName.toStdString()))
            QMessageBox::warning(this, "Error", "Python export failed.");
    }
}

// Triggered when the user selects "Export PNG".
void MainWindow::onExportPNGClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export PNG", QDir::homePath() + "/dscout.png",
        "PNG file (*.png)", nullptr, QFileDialog::DontUseNativeDialog);

    if (!fileName.isEmpty())
    {
        QPixmap pixmap(chartView_->size());
        pixmap.fill(Qt::white);

        QPainter painter(&pixmap);
        chartView_->render(&painter);
        painter.end();

        if (!pixmap.save(fileName, "PNG"))
            QMessageBox::warning(this, "Error", "PNG export failed.");
    }
}

// Triggered when the user selects "Remove last series".
void MainWindow::onRemoveLastClicked()
{
    dataManager_.removeLastSeries();
    statusBar()->showMessage("Ready");
    rebuildChart();
}

// Triggered when the user selects "Remove all series".
void MainWindow::onRemoveAllClicked()
{
    dataManager_.removeAllSeries();
    statusBar()->showMessage("Ready");
    rebuildChart();
}

// Triggered when the user selects "Quit".
void MainWindow::onQuitClicked()
{
    qApp->quit();
}

// Reads all available serial data and forwards it to the SerialParser.
void MainWindow::onSerialDataReceived()
{
    const QByteArray data = serial_.readAll();

    for (char c : data)
    {
        int n;
        const auto result = serialParser_.processReceivedChar(c);

        switch (result)
        {
        case ParseResult::SeriesCompleted:
            dataManager_.appendSeries(serialParser_.currentSeries());
            statusBar()->showMessage("Ready");
            rebuildChart();
            break;

        case ParseResult::DataPointAdded:
            // Update progress indicator in status bar
            n = serialParser_.currentSeriesSize();
            statusBar()->showMessage(QString("Receiving data ") + QString(n, '.'));
            break;

        case ParseResult::ParseError:
            qWarning() << "ParseError: " << data;
            break;

        case ParseResult::Nothing:
            break;
        }
    }
}

// Rounds a value up to the next 0.5 increment.
double MainWindow::roundUpToHalf(double value) const
{
    return std::ceil(value * 2.0) / 2.0;
}

// Rebuilds the chart from all stored measurement series.
void MainWindow::rebuildChart()
{
    if (dataManager_.seriesCount() == 0)
    {
        resetChartToEmpty();
        return;
    }

    chart_->removeAllSeries();

    const auto &all = dataManager_.allSeries();
    for (const auto &seriesData : all)
    {
        auto *line = new QSplineSeries(chart_);
        for (const auto &p : seriesData.points())
            line->append(p.voltageVolt, p.currentMilliAmp);
        chart_->addSeries(line);
    }

    chart_->setTitle(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    chart_->createDefaultAxes();
    chart_->legend()->hide();
    chart_->setAnimationOptions(QChart::SeriesAnimations);

    double maxVoltage, maxCurrent;
    dataManager_.getMaxVoltageAndCurrent(maxVoltage, maxCurrent);

    auto *axisX = chartView_->getAxisX();
    auto *axisY = chartView_->getAxisY();
    if (axisX && axisY)
    {
        axisX->setTitleText("Volt (V)");
        axisX->setTickType(QValueAxis::TicksDynamic);
        axisX->setRange(0, roundUpToHalf(maxVoltage));
        axisX->setTickInterval(0.5);
        axisX->setMinorTickCount(4);

        axisY->setLabelFormat("%.2f");
        axisY->setTitleText("\nMilliampere (mA)");
        axisY->setTickType(QValueAxis::TicksDynamic);
        axisY->setRange(0, roundUpToHalf(maxCurrent));
        axisY->setTickInterval(1.0);
        axisY->setMinorTickCount(4);
    }
}

// Resets the chart to an empty default state.
void MainWindow::resetChartToEmpty()
{
    // Clears all visual content from the chart and restores the
    // initial empty-state appearance. Used when no measurement
    // series remain. Does not modify the MeasurementDataManager.
    chart_->removeAllSeries();
    chart_->setAnimationOptions(QChart::NoAnimation);
    chart_->setTitle("Press the button on the DiodeScout ...");

    const auto axes = chart_->axes();
    for (QAbstractAxis *axis : axes)
        axis->setVisible(false);
}

// Slightly increases the title font size.
void MainWindow::setChartTitleFont()
{
    QFont titleFont = chart_->titleFont();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    chart_->setTitleFont(titleFont);
}

// Initializes the main window UI, including toolbar and actions.
void MainWindow::setupUI()
{
    // Toolbar
    auto *toolbar = new QToolBar("Main Toolbar", this);
    toolbar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto *spacer1 = new QWidget(toolbar);
    auto *spacer2 = new QWidget(toolbar);
    spacer1->setFixedWidth(20);
    spacer2->setFixedWidth(20);

    restoreViewAct_ = toolbar->addAction(QIcon(":/icons/restoreview.svg"), "Restore default view");
    lightModeAct_ = toolbar->addAction(QIcon(":/icons/lightmode.svg"), "Light mode");
    darkModeAct_ = toolbar->addAction(QIcon(":/icons/darkmode.svg"), "Dark mode");
    computePWLAct_ = toolbar->addAction(QIcon(":/icons/computepwl.svg"), "Compute piecewise-linear diode model");
    toolbar->addWidget(spacer1);
    exportCSVAct_ = toolbar->addAction(QIcon(":/icons/exportcsv.svg"), "Export CSV (Excel)");
    exportPythonAct_ = toolbar->addAction(QIcon(":/icons/exportpython.svg"), "Export Python script");
    exportPNGAct_ = toolbar->addAction(QIcon(":/icons/exportpng.svg"), "Export PNG");
    toolbar->addWidget(spacer2);
    removeLastAct_ = toolbar->addAction(QIcon(":/icons/removelast.svg"), "Remove last series");
    removeAllAct_ = toolbar->addAction(QIcon(":/icons/removeall.svg"), "Remove all series");
    quitAct_ = toolbar->addAction(QIcon(":/icons/quit.svg"), "Quit");

    connect(restoreViewAct_, &QAction::triggered, this, &MainWindow::onRestoreViewClicked);
    connect(lightModeAct_, &QAction::triggered, this, &MainWindow::onLightModeClicked);
    connect(darkModeAct_, &QAction::triggered, this, &MainWindow::onDarkModeClicked);
    connect(computePWLAct_, &QAction::triggered, this, &MainWindow::onComputePWL);
    connect(exportCSVAct_, &QAction::triggered, this, &MainWindow::onExportCSVClicked);
    connect(exportPythonAct_, &QAction::triggered, this, &MainWindow::onExportPythonClicked);
    connect(exportPNGAct_, &QAction::triggered, this, &MainWindow::onExportPNGClicked);
    connect(removeLastAct_, &QAction::triggered, this, &MainWindow::onRemoveLastClicked);
    connect(removeAllAct_, &QAction::triggered, this, &MainWindow::onRemoveAllClicked);
    connect(quitAct_, &QAction::triggered, this, &MainWindow::onQuitClicked);

    // Chart: chartView_ takes ownership of chart_
    chart_ = new QChart();
    chart_->setTheme(QChart::ChartThemeBlueCerulean);
    setChartTitleFont();

    chartView_ = new MyChartView(chart_);
    chartView_->setMouseTracking(true);
    chartView_->setRenderHint(QPainter::Antialiasing);
    chartView_->setRubberBand(QChartView::RectangleRubberBand);
    setCentralWidget(chartView_);
}
