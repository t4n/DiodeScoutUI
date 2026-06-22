// ---------------------------------------------------------------------------
//  Core measurement data types
//
//  Defines the core data structures used throughout the DiodeScout UI.
//  They represent individual measurement points and complete measurement
//  series as produced by the parser, stored by the data manager, and used
//  by the UI and export features.
// ---------------------------------------------------------------------------

// Portable core module, no Qt dependencies.
#include "coredatatypes.h"

// Constructs an empty measurement series.
MeasurementSeries::MeasurementSeries()
{
    points_.reserve(InitialPointCapacity);
}

// Adds a new measurement point.
void MeasurementSeries::addPoint(double voltage, double currentMilliAmp)
{
    points_.emplace_back(voltage, currentMilliAmp);
}

// Returns a read-only reference to the measurement points.
const std::vector<MeasurementPoint> &MeasurementSeries::points() const noexcept
{
    return points_;
}

// Returns the number of measurement points.
std::size_t MeasurementSeries::size() const noexcept
{
    return points_.size();
}

// Returns true if the series is empty.
bool MeasurementSeries::empty() const noexcept
{
    return points_.empty();
}
