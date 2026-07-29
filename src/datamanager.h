// ---------------------------------------------------------------------------
//  Central data management, stores and manages all acquired measurement
//  series and provides utilities for exporting, analyzing, and generating
//  simulated measurement data.
//
//  - Maintains a collection of measurement series
//  - Exports data to CSV or Python format
//  - Generates simulated diode characteristics
//  - Computes piecewise-linear diode parameters
// ---------------------------------------------------------------------------

#pragma once

// Portable core module, no Qt dependencies.
#include "coredatatypes.h"
#include <cstddef>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  CSVSettings:
//  Defines formatting rules for CSV export.
// ---------------------------------------------------------------------------
struct CSVSettings
{
    char decimalSeparator;
    char fieldSeparator;

    // Constructs CSVSettings, sets decimal and field separator.
    CSVSettings(char decimalSep, char fieldSep) noexcept :
        decimalSeparator(decimalSep),
        fieldSeparator(fieldSep)
    {
    }
};

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

    // Appends simulated measurement series to the collection.
    void appendSimulatedSeries();

    // Retrieves the maximum voltage (V) across all series.
    double maxVoltage() const noexcept;

    // Retrieves the maximum current (mA) across all series.
    double maxCurrent() const noexcept;

    // Exports all measurement series to CSV; true if successful.
    bool exportCSV(const std::string &filePath, const CSVSettings &csv) const;

    // Exports all measurement series to Python; true if successful.
    bool exportPython(const std::string &filePath) const;

    // Computes the piecewise-linear diode model; true if successful.
    bool computePWL(double &forwardV, double &seriesR) const;

  private:
    // Collection of all acquired measurement series.
    std::vector<MeasurementSeries> series_;

    // Converts a double to a string using the configured decimal separator.
    std::string formatDouble(double d, char decimalSeparator) const;
};
