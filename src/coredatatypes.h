// ---------------------------------------------------------------------------
//  Core measurement data types
//
//  Defines the core data structures used throughout the DiodeScout UI.
//  They represent individual measurement points and complete measurement
//  series as produced by the parser, stored by the data manager, and used
//  by the UI and export functionality.
// ---------------------------------------------------------------------------

#pragma once

// Portable core module, no Qt dependencies.
#include <vector>

// ---------------------------------------------------------------------------
//  MeasurementPoint:
//  A single measurement sample containing voltage (V) and current (mA).
// ---------------------------------------------------------------------------
struct MeasurementPoint
{
    double voltageVolt; // x-value
    double currentMilliAmp; // y-value

    // Constructs a MeasurementPoint from voltage and current.
    MeasurementPoint(double voltage, double current) noexcept :
        voltageVolt(voltage),
        currentMilliAmp(current)
    {
    }
};

// ---------------------------------------------------------------------------
//  MeasurementSeries:
//  A complete set of measurement points forming one I–V curve. Built by
//  the parser and later used for visualization, analysis, and export.
// ---------------------------------------------------------------------------
class MeasurementSeries
{
  public:
    // Adds a new measurement point.
    void addPoint(double voltage, double currentMilliAmp);

    // Returns all measurement points.
    const std::vector<MeasurementPoint> &points() const noexcept;

    // Returns the number of measurement points.
    std::size_t size() const noexcept;

    // Checks whether the series is empty.
    bool empty() const noexcept;

  private:
    // Measurement points.
    std::vector<MeasurementPoint> points_;
};
