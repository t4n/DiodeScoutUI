// ---------------------------------------------------------------------------
//  Core measurement data types
//
//  Defines the core data structures used throughout the DiodeScout UI.
//  They represent individual measurement points and complete measurement
//  series as produced by the parser, stored by the data manager, and used
//  by the UI and export functionality.
// ---------------------------------------------------------------------------

#include "coredatatypes.h"

// Adds a new measurement point.
void MeasurementSeries::addPoint(double voltage, double currentMilliAmp)
{
    points_.push_back({voltage, currentMilliAmp});
}

// Returns all measurement points.
const std::vector<MeasurementPoint> &MeasurementSeries::points() const noexcept
{
    return points_;
}

// Returns the number of measurement points.
std::size_t MeasurementSeries::size() const noexcept
{
    return points_.size();
}

// Checks whether the series is empty.
bool MeasurementSeries::empty() const noexcept
{
    return points_.empty();
}
