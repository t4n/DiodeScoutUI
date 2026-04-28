// ---------------------------------------------------------------------------
//  Measurement data model and parser
//
//  - Defines data structures for measurement points and series
//  - Manages temporary and finalized measurement series
//  - Provides a parser for incoming serial data
//  - Includes export utilities (CSV, Python)
// ---------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
//  ParseResult:
//  Return value of the parser indicating whether a measurement series
//  has been completed.
// ---------------------------------------------------------------------------
enum class ParseResult
{
    Nothing,
    SeriesCompleted
};

// ---------------------------------------------------------------------------
//  MeasurementPoint:
//  Represents a single measurement point containing voltage (V) and
//  current (mA).
// ---------------------------------------------------------------------------
struct MeasurementPoint
{
    double voltageVolt = 0.0; // x-value
    double currentMilliAmp = 0.0; // y-value
};

// ---------------------------------------------------------------------------
//  MeasurementSeries:
//  Represents a complete measurement series of multiple measurement
//  points. Built by the parser and later used for visualization.
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

// ---------------------------------------------------------------------------
//  MeasurementDataManager, central data management:
//  - Stores all completed measurement series
//  - Maintains a temporary series while receiving data
//  - Implements a parser for serial input
//  - Provides helper utilities (max values, data export)
// ---------------------------------------------------------------------------
class MeasurementDataManager
{
  public:
    MeasurementDataManager() = default;

    // Returns the number of stored measurement series.
    std::size_t seriesCount() const noexcept;

    // Returns all completed measurement series.
    const std::vector<MeasurementSeries> &allSeries() const noexcept;

    // Removes all stored measurement series.
    void removeAllSeries();

    // Removes the last measurement series.
    void removeLastSeries();

    // Appends a simulated diode characteristic curve.
    void appendSimulatedSeries(double scaleCurrent);

    // Returns the number of points in the temporary series.
    std::size_t tempSeriesSize() const noexcept;

    // Returns the maximum voltage and maximum current across all series.
    void getMaxVoltageAndCurrent(double &maxV, double &maxI) const noexcept;

    // Processes a single incoming character from the serial stream.
    ParseResult processReceivedChar(char c);

    // Exports all stored measurement series to a CSV file.
    // Returns true on success.
    bool exportCSV(const std::string &filePath) const;

    // Exports all stored measurement series to a Python script.
    // Returns true on success.
    bool exportPython(const std::string &filePath) const;

  private:
    // Current state of the serial data stream parser.
    enum class ParserState
    {
        Idle,
        ReceivingSeries
    };

    // Current state of the internal parser.
    ParserState state_ = ParserState::Idle;

    // Accumulates characters from the serial port.
    std::string currentLine_;

    // Buffer for the series currently being received.
    MeasurementSeries tempSeries_;

    // Collection of all successfully completed series.
    std::vector<MeasurementSeries> series_;

    // Removes leading and trailing whitespace.
    static std::string trim(const std::string &s);

    // Processes a fully received line, updates the parser state.
    ParseResult handleCompletedLine(const std::string &rawLine);

    // Parses a data line in the format "<x> <y>".
    void parseDataLine(const std::string &line);
};
