// ---------------------------------------------------------------------------
//  Main application window for the DiodeScout UI. Responsible for:
//
//  - Creating and managing the toolbar and chart view
//  - Handling serial communication with the DiodeScout device
//  - Receiving and parsing incoming measurement data
//  - Updating the chart when new data is available
//  - Providing user actions (export, reset, clear, exit)
// ---------------------------------------------------------------------------

#include "mainwindow.h"
#include <QDateTime>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSplineSeries>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QValueAxis>

// ---------------------------------------------------------------------------
//  MyChartView:
//  Custom QChartView with mouse-position tooltips, scrolling and zooming.
// ---------------------------------------------------------------------------
class MyChartView : public QChartView
{
  public:
    using QChartView::QChartView; // inherit constructors

  private:
    static constexpr qreal ZoomFactor = 1.05; // zoom step for mouse wheel
    static constexpr qreal ScrollStep = 5; // pixels per key press

  protected:
    // Checks if value is within the axis limits.
    bool inAxisRange(qreal value, const QValueAxis *axis) const
    {
        return axis && value >= axis->min() && value <= axis->max();
    }

    // Update tooltip with mouse position in chart coordinates.
    void mouseMoveEvent(QMouseEvent *event) override
    {
        Q_ASSERT(chart());
        bool showTip = false;
        QString text;

        if (!chart()->series().empty())
        {
            const auto axesX = chart()->axes(Qt::Horizontal);
            const auto axesY = chart()->axes(Qt::Vertical);
            const auto *axisX = qobject_cast<QValueAxis *>(axesX.value(0));
            const auto *axisY = qobject_cast<QValueAxis *>(axesY.value(0));
            Q_ASSERT(axisX && axisY);

            const QPointF value = chart()->mapToValue(event->position());
            if (inAxisRange(value.x(), axisX) && inAxisRange(value.y(), axisY))
            {
                text = QString::asprintf("%.3f V, %.3f mA", value.x(), value.y());
                showTip = true;
            }
        }

        if (showTip)
            QToolTip::showText(event->globalPosition().toPoint(), text, this);
        else
            QToolTip::hideText();

        QChartView::mouseMoveEvent(event);
    }

    // Cleans up tooltip state when the cursor leaves the widget.
    void leaveEvent(QEvent *event) override
    {
        QToolTip::hideText();
        QChartView::leaveEvent(event);
    }

    // Handles mouse wheel input to zoom the chart view.
    void wheelEvent(QWheelEvent *event) override
    {
        Q_ASSERT(chart());

        if (event->angleDelta().y() > 0)
            chart()->zoom(ZoomFactor); // zoom in
        else
            chart()->zoom(1.0 / ZoomFactor); // zoom out

        event->accept();
    }

    // Keyboard-based scrolling and zooming.
    void keyPressEvent(QKeyEvent *event) override
    {
        Q_ASSERT(chart());
        bool handled = true;

        switch (event->key())
        {
        case Qt::Key_Down:
            chart()->scroll(0, ScrollStep);
            break;
        case Qt::Key_Up:
            chart()->scroll(0, -ScrollStep);
            break;
        case Qt::Key_Right:
            chart()->scroll(-ScrollStep, 0);
            break;
        case Qt::Key_Left:
            chart()->scroll(ScrollStep, 0);
            break;
        case Qt::Key_Plus:
            chart()->zoom(ZoomFactor);
            break;
        case Qt::Key_Minus:
            chart()->zoom(1.0 / ZoomFactor);
            break;
        default:
            handled = false;
            break;
        }

        if (handled)
            event->accept();
        else
            QGraphicsView::keyPressEvent(event);
    }
};

// ---------------------------------------------------------------------------
//  MainWindow – Implementation
// ---------------------------------------------------------------------------

// Constructs the main window and initializes UI components.
MainWindow::MainWindow(QSerialPort &diodeScoutPort) : serial(diodeScoutPort)
{
    // Toolbar
    auto *toolbar = new QToolBar("Main Toolbar", this);
    toolbar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto *spacer1 = new QWidget(toolbar);
    auto *spacer2 = new QWidget(toolbar);
    spacer1->setFixedWidth(20);
    spacer2->setFixedWidth(20);

    restoreViewAct = toolbar->addAction(QIcon(":/icons/restoreview.svg"), "Restore default view");
    darkModeAct = toolbar->addAction(QIcon(":/icons/darkmode.svg"), "Dark mode");
    lightModeAct = toolbar->addAction(QIcon(":/icons/lightmode.svg"), "Light mode");
    toolbar->addWidget(spacer1);
    exportCSVAct = toolbar->addAction(QIcon(":/icons/exportcsv.svg"), "Export CSV");
    exportPythonAct = toolbar->addAction(QIcon(":/icons/exportpython.svg"), "Export Python script");
    exportPNGAct = toolbar->addAction(QIcon(":/icons/exportpng.svg"), "Export PNG");
    toolbar->addWidget(spacer2);
    removeLastAct = toolbar->addAction(QIcon(":/icons/removelast.svg"), "Remove last series");
    removeAllAct = toolbar->addAction(QIcon(":/icons/removeall.svg"), "Remove all series");
    quitAct = toolbar->addAction(QIcon(":/icons/quit.svg"), "Quit");

    connect(restoreViewAct, &QAction::triggered, this, &MainWindow::onRestoreViewClicked);
    connect(darkModeAct, &QAction::triggered, this, &MainWindow::onDarkModeClicked);
    connect(lightModeAct, &QAction::triggered, this, &MainWindow::onLightModeClicked);
    connect(exportCSVAct, &QAction::triggered, this, &MainWindow::onExportCSVClicked);
    connect(exportPythonAct, &QAction::triggered, this, &MainWindow::onExportPythonClicked);
    connect(exportPNGAct, &QAction::triggered, this, &MainWindow::onExportPNGClicked);
    connect(removeLastAct, &QAction::triggered, this, &MainWindow::onRemoveLastClicked);
    connect(removeAllAct, &QAction::triggered, this, &MainWindow::onRemoveAllClicked);
    connect(quitAct, &QAction::triggered, this, &MainWindow::onQuitClicked);

    // Chart
    chart = new QChart();
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->setTitle("Press the button on the DiodeScout ...");
    setChartTitleFont();

    chartView = new MyChartView(chart);
    chartView->setMouseTracking(true);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::RectangleRubberBand);
    setCentralWidget(chartView);

    // Setup data source: Use simulation if no hardware is
    // connected, otherwise initialize serial communication.
    if (!serial.isOpen())
    {
        statusBar()->showMessage("Simulation");
        dataManager.appendSimulatedSeries(1.0e-8);
        dataManager.appendSimulatedSeries(1.0e-10);
        rebuildChart();
    }
    else
    {
        QString prettyName = serial.portName();
        prettyName.replace("\\\\.\\", "");
        statusBar()->showMessage(QString("DiodeScout at %1").arg(prettyName));
        connect(&serial, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived);
    }
}

// Triggered when the user selects "Restore default view".
void MainWindow::onRestoreViewClicked()
{
    rebuildChart();
}

// Triggered when the user selects "Dark mode".
void MainWindow::onDarkModeClicked()
{
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    setChartTitleFont();
}

// Triggered when the user selects "Light mode".
void MainWindow::onLightModeClicked()
{
    chart->setTheme(QChart::ChartThemeBlueNcs);
    setChartTitleFont();
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
        resetChartToEmpty();
    else
        rebuildChart();
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

// Reads all available serial data and processes it byte-by-byte.
void MainWindow::onSerialDataReceived()
{
    while (serial.bytesAvailable() > 0)
    {
        const QByteArray data = serial.readAll();
        for (auto c : data)
            handleSerialByte(c);
    }
}

// Handles a single received byte from the serial interface.
void MainWindow::handleSerialByte(char c)
{
    const auto result = dataManager.processReceivedChar(c);
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

// Rounds a value up to the next 0.5 increment.
double MainWindow::roundUpToHalf(double value) const
{
    return std::ceil(value * 2.0) / 2.0;
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
        auto *line = new QSplineSeries();
        for (const auto &p : seriesData.points())
            line->append(p.voltageVolt, p.currentMilliAmp);
        chart->addSeries(line);
    }

    chart->setTitle(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    chart->createDefaultAxes();
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    const auto axesX = chart->axes(Qt::Horizontal);
    const auto axesY = chart->axes(Qt::Vertical);
    auto *axisX = qobject_cast<QValueAxis *>(axesX.value(0));
    auto *axisY = qobject_cast<QValueAxis *>(axesY.value(0));

    double maxVoltage, maxCurrent;
    dataManager.getMaxVoltageAndCurrent(maxVoltage, maxCurrent);

    if (axisX)
    {
        axisX->setTitleText("Volt (V)");
        axisX->setTickType(QValueAxis::TicksDynamic);
        axisX->setRange(0, roundUpToHalf(maxVoltage));
        axisX->setTickInterval(0.5);
        axisX->setMinorTickCount(4);
    }

    if (axisY)
    {
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
    // series remain. Does not touch the MeasurementDataManager.
    chart->removeAllSeries();
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setTitle("Press the button on the DiodeScout ...");

    const auto axes = chart->axes();
    for (QAbstractAxis *axis : axes)
        axis->setVisible(false);
}

// Slightly increases the title font size.
void MainWindow::setChartTitleFont()
{
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(12);
    titleFont.setBold(true);

    chart->setTitleFont(titleFont);
}
