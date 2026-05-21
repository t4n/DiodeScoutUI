// ---------------------------------------------------------------------------
//  Custom QChartView subclass
//
//  Responsibilities:
//   - Interactive zooming (mouse wheel + keyboard)
//   - Panning / scrolling support (keyboard)
//   - Real-time coordinate tooltip display
//   - Convenience accessors for chart axes (X/Y)
// ---------------------------------------------------------------------------

#pragma once

#include <QValueAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

// ---------------------------------------------------------------------------
//  MyChartView:
//  Custom QChartView subclass
// ---------------------------------------------------------------------------
class MyChartView final : public QChartView
{
  private:
    static constexpr qreal ZoomFactor = 1.05; // zoom step for mouse wheel
    static constexpr qreal ScrollStep = 5; // pixels per key press

  public:
    // Inherit constructors.
    using QChartView::QChartView;

    // Convenience accessor for the chart's horizontal axis.
    QValueAxis *getAxisX() const;

    // Convenience accessor for the chart's vertical axis.
    QValueAxis *getAxisY() const;

  protected:
    // Checks if value is within the axis limits.
    bool inAxisRange(qreal value, const QValueAxis *axis) const;

    // Updates the tooltip with mouse position in chart coordinates.
    void mouseMoveEvent(QMouseEvent *event) override;

    // Cleans up tooltip state when the cursor leaves the widget.
    void leaveEvent(QEvent *event) override;

    // Handles mouse wheel input to zoom the chart view.
    void wheelEvent(QWheelEvent *event) override;

    // Keyboard-based scrolling and zooming.
    void keyPressEvent(QKeyEvent *event) override;
};
