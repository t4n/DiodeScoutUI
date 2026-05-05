// ---------------------------------------------------------------------------
//  Central data management, stores and manages all acquired measurement
//  series and provides utilities for exporting, analyzing, and generating
//  measurement data.
//
//  - Maintains a collection of measurement series
//  - Exports data to CSV or Python format
//  - Generates simulated diode characteristics
//  - Computes piecewise-linear diode parameters
// ---------------------------------------------------------------------------

#pragma once

#include "coredatatypes.h"
#include <cstddef>
#include <string>

// ---------------------------------------------------------------------------
//  MeasurementDataManager:
//  Stores and manages all acquired measurement series.
// ---------------------------------------------------------------------------
class MeasurementDataManager
{
  public:
    // Returns the number of stored measurement series.
    std::size_t seriesCount() const noexcept;

    // Returns a read-only reference to all stored measurement series.
    const std::vector<MeasurementSeries> &allSeries() const noexcept;

    // Removes all stored measurement series.
    void removeAllSeries();

    // Removes the most recently added measurement series.
    void removeLastSeries();

    // Adds a completed measurement series to the collection.
    void appendSeries(const MeasurementSeries &series);

    // Generates and appends a simulated diode I–V characteristic.
    void appendSimulatedSeries(double scaleCurrent);

    // Retrieves the maximum voltage and current across all series.
    void getMaxVoltageAndCurrent(double &maxV, double &maxI) const noexcept;

    // Exports all stored measurement series to a CSV file.
    // Returns true on success.
    bool exportCSV(const std::string &filePath) const;

    // Exports all stored measurement series to a Python script.
    // Returns true on success.
    bool exportPython(const std::string &filePath) const;

    // Computes piecewise-linear diode parameters (Vf, Rs).
    // Returns true on success.
    bool computePWL(double &forwardV, double &seriesR) const;

  private:
    // Collection of all acquired measurement series.
    std::vector<MeasurementSeries> series_;
};
