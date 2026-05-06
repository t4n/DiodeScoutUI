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
#include <QFileDialog>
#include <QLineSeries>
#include <QMessageBox>
#include <QSplineSeries>
#include <QStatusBar>
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
        Q_ASSERT(axis);
        if(!axis)
            return false;

        return value >= axis->min() && value <= axis->max();
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
//  MainWindow:
//  Main application window for the DiodeScout UI.
// ---------------------------------------------------------------------------

// Constructs the main window and initializes UI components.
MainWindow::MainWindow(QSerialPort &diodeScoutPort) : serial_(diodeScoutPort)
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
    exportCSVAct_ = toolbar->addAction(QIcon(":/icons/exportcsv.svg"), "Export CSV");
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
    chart_->setTitle("Press the button on the DiodeScout ...");
    setChartTitleFont();

    chartView_ = new MyChartView(chart_);
    chartView_->setMouseTracking(true);
    chartView_->setRenderHint(QPainter::Antialiasing);
    chartView_->setRubberBand(QChartView::RectangleRubberBand);
    setCentralWidget(chartView_);

    // Setup data source: Use simulation if no hardware is
    // connected, otherwise initialize serial communication.
    if (!serial_.isOpen())
    {
        dataManager_.appendSimulatedSeries(1.0e-8);
        dataManager_.appendSimulatedSeries(1.0e-10);
        rebuildChart();
        statusBar()->showMessage("Simulation");
    }
    else
    {
        QString prettyName = serial_.portName();
        prettyName.replace("\\\\.\\", "");
        connect(&serial_, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived, Qt::QueuedConnection);
        statusBar()->showMessage(QString("DiodeScout at %1").arg(prettyName));
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
        QMessageBox::warning(this,
            "Piecewise-linear diode model",
            "Please ensure that:\n\n"
            "- Exactly one measurement series is loaded\n"
            "- The diode is connected with the correct polarity\n"
            "- The forward voltage is below approximately 4 V\n"
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

    const auto axesX = chart_->axes(Qt::Horizontal);
    const auto axesY = chart_->axes(Qt::Vertical);
    auto *axisX = qobject_cast<QValueAxis *>(axesX.value(0));
    auto *axisY = qobject_cast<QValueAxis *>(axesY.value(0));
    Q_ASSERT(axisX && axisY);

    if (axisX && axisY)
    {
        line->attachAxis(axisX);
        line->attachAxis(axisY);
    }

    // Show model parameters in status bar
    const char *fmt = "Forward voltage (turn-on): %.2f V, "
                      "Effective series resistance: %.2f \u03A9";

    QString msg = QString::asprintf(fmt, forwardV, seriesR);
    statusBar()->showMessage(msg);
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
        if (!dataManager_.exportCSV(fileName.toStdString()))
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
        if (!dataManager_.exportPython(fileName.toStdString()))
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
        const auto result = serialParser_.processReceivedChar(c);
        int n;

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

    const auto axesX = chart_->axes(Qt::Horizontal);
    const auto axesY = chart_->axes(Qt::Vertical);
    auto *axisX = qobject_cast<QValueAxis *>(axesX.value(0));
    auto *axisY = qobject_cast<QValueAxis *>(axesY.value(0));
    Q_ASSERT(axisX && axisY);

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
    // series remain. Does not modify the MeasurementdataManager.
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
