// ---------------------------------------------------------------------------
//  State-machine parser for the DiodeScout serial data format.
//  It detects BEGIN/END blocks, parses DATA lines, and builds a
//  MeasurementSeries from the incoming character stream.
//
//  - Call processReceivedChar() for each incoming character.
//  - When SeriesCompleted is returned, the current series
//    contains a fully parsed measurement sequence.
//  - The next BEGIN block automatically starts a new series.
// ---------------------------------------------------------------------------

#pragma once

#include "coredatatypes.h"
#include <string>

// ---------------------------------------------------------------------------
//  ParseResult:
//  Indicates whether processing the latest character added a data point,
//  completed a series, or had no effect.
// ---------------------------------------------------------------------------
enum class ParseResult
{
    Nothing,
    DataPointAdded,
    SeriesCompleted
};

// ---------------------------------------------------------------------------
//  SerialParser:
//  State-machine parser for the DiodeScout serial data format.
// ---------------------------------------------------------------------------
class SerialParser
{
  public:
    // Returns the number of points collected in the current series.
    std::size_t currentSeriesSize() const noexcept;

    // Provides read-only access to the current measurement series.
    const MeasurementSeries &currentSeries() const noexcept;

    // Returns DataPointAdded when a DATA line was parsed,
    // SeriesCompleted when END was received, or Nothing otherwise.
    ParseResult processReceivedChar(char c);

  private:
    // Internal parser state.
    enum class ParserState
    {
        Idle,
        ReceivingSeries
    };

    // The series currently being received.
    MeasurementSeries currentSeries_;

    // Buffer for the line currently being received.
    std::string lineBuffer_;

    // Current state of the parser.
    ParserState state_ = ParserState::Idle;

    // Removes leading and trailing whitespace.
    static std::string trim(const std::string &s);

    // Processes a fully received line and updates the parser state.
    ParseResult handleCompletedLine(const std::string &rawLine);

    // Appends "<x> <y>" data from the received line to the current series.
    ParseResult parseDataLine(const std::string &line);
};
