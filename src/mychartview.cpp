// ---------------------------------------------------------------------------
//  Custom QChartView subclass
//
//  Responsibilities:
//  - Interactive zooming (mouse wheel + keyboard)
//  - Panning / scrolling support (keyboard)
//  - Real-time coordinate tooltip display
//  - Convenience accessors for chart axes (X/Y)
// ---------------------------------------------------------------------------

#include "mychartview.h"
#include <QToolTip>

// Convenience accessor for the chart's horizontal axis.
QValueAxis *MyChartView::getAxisX() const
{
    Q_ASSERT(chart());
    const auto axesX = chart()->axes(Qt::Horizontal);

    if (axesX.empty())
        return nullptr;

    return qobject_cast<QValueAxis *>(axesX.first());
}

// Convenience accessor for the chart's vertical axis.
QValueAxis *MyChartView::getAxisY() const
{
    Q_ASSERT(chart());
    const auto axesY = chart()->axes(Qt::Vertical);

    if (axesY.empty())
        return nullptr;

    return qobject_cast<QValueAxis *>(axesY.first());
}

// Checks if value is within the axis limits.
bool MyChartView::inAxisRange(qreal value, const QValueAxis *axis) const
{
    if (!axis)
        return false;

    return value >= axis->min() && value <= axis->max();
}

// Updates the tooltip with mouse position in chart coordinates.
void MyChartView::mouseMoveEvent(QMouseEvent *event)
{
    Q_ASSERT(chart());
    bool showTip = false;
    QString text;

    if (!chart()->series().empty())
    {
        const auto *axisX = getAxisX();
        const auto *axisY = getAxisY();
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
void MyChartView::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    QChartView::leaveEvent(event);
}

// Handles mouse wheel input to zoom the chart view.
void MyChartView::wheelEvent(QWheelEvent *event)
{
    Q_ASSERT(chart());
    const int delta = event->angleDelta().y();

    if (delta > 0)
        chart()->zoom(ZoomFactor); // zoom in
    else if (delta < 0)
        chart()->zoom(1.0 / ZoomFactor); // zoom out

    event->accept();
}

// Keyboard-based scrolling and zooming.
void MyChartView::keyPressEvent(QKeyEvent *event)
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
        QChartView::keyPressEvent(event);
}
