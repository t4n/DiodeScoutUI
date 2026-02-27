// ---------------------------------------------------------------------------
//  Data model for capturing, storing, and parsing measurement series.
//  This file contains the logic for:
//
//  - Managing individual measurement points and complete measurement series
//  - Receiving and parsing serial input line-by-line
//  - Building temporary series during acquisition
//  - Storing completed series for later visualization (e.g., QtCharts)
// ---------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  ParseResult:
//  Return value of the parser. Indicates whether a complete measurement
//  series has been received or if nothing special happened.
// ---------------------------------------------------------------------------
enum class ParseResult
{
    Nothing,
    SeriesCompleted
};

// ---------------------------------------------------------------------------
//  MeasurementPoint:
//  Represents a single measurement point consisting of voltage (Volt)
//  and current (Milliampere). Used as an element of a measurement series.
// ---------------------------------------------------------------------------
struct MeasurementPoint
{
    double voltageVolt = 0.0; // x-value
    double currentMilliAmp = 0.0; // y-value
};

// ---------------------------------------------------------------------------
//  MeasurementSeries:
//  Represents a complete measurement series consisting of multiple
//  measurement points. Built by the parser and later visualized by the UI.
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
    std::vector<MeasurementPoint> points_;
};

// ---------------------------------------------------------------------------
//  MeasurementDataManager:
//  Central data management:
//  - Stores all completed measurement series
//  - Holds a temporary series while receiving data
//  - Provides parser functionality for the serial interface
//  - Provides helper functions for the UI (e.g., max values)
// ---------------------------------------------------------------------------
class MeasurementDataManager
{
  public:
    MeasurementDataManager() = default;

    // Returns the number of stored measurement series.
    std::size_t seriesCount() const noexcept;

    // Returns all measurement series.
    const std::vector<MeasurementSeries> &allSeries() const noexcept;

    // Removes all stored measurement series.
    void removeAllSeries();

    // Removes the last measurement series.
    void removeLastSeries();

    // Returns the number of points in the temporary series.
    std::size_t tempSeriesSize() const noexcept;

    // Returns the maximum voltage across all series (including temporary).
    double getMaxVoltage() const noexcept;

    // Returns the maximum current across all series (including temporary).
    double getMaxCurrent() const noexcept;

    // Parser: processes a single received character.
    ParseResult processReceivedChar(char c);

    // Exports all stored measurement series to a CSV file.
    // Returns true on success, false on failure.
    bool exportCSV(const std::string &filePath) const;

    // Exports all stored measurement series to a Python script.
    // Returns true on success, false on failure.
    bool exportPython(const std::string &filePath) const;

  private:
    enum class ParserState
    {
        Idle,
        ReceivingSeries
    };

    ParserState state_ = ParserState::Idle;
    std::string currentLine_;
    MeasurementSeries tempSeries_;
    std::vector<MeasurementSeries> series_;

    // Removes leading and trailing whitespace.
    static std::string trim(const std::string &s);

    // Processes a fully received line.
    ParseResult handleCompletedLine(const std::string &rawLine);

    // Parses a data line in the format "<x> <y>".
    void parseDataLine(const std::string &line);
};
