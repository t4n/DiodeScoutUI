// ---------------------------------------------------------------------------
//  State-machine parser for the DiodeScout serial data format.
//  It detects BEGIN/END blocks, parses DATA lines, and builds a
//  MeasurementSeries from the incoming character stream.
//
//  - Call processReceivedChar() for each incoming character.
//  - When SeriesCompleted is returned, the current series
//    contains a fully parsed measurement sequence.
// ---------------------------------------------------------------------------

#include "serialparser.h"
#include <algorithm>
#include <locale>

// Configure iss_ to use the classic C locale so floating-point
// parsing is independent of the user's system locale.
SerialParser::SerialParser()
{
    xyStream_.imbue(std::locale::classic());
}

// Returns the number of points collected in the current series.
std::size_t SerialParser::currentSeriesSize() const noexcept
{
    return currentSeries_.size();
}

// Provides read-only access to the current measurement series.
const MeasurementSeries &SerialParser::currentSeries() const noexcept
{
    return currentSeries_;
}

// Returns DataPointAdded when a DATA line is parsed,
// SeriesCompleted when END is received, ParseError on
// invalid or malformed input data, or Nothing otherwise.
ParseResult SerialParser::processReceivedChar(char c)
{
    if (c == '\n')
    {
        auto result = handleCompletedLine(lineBuffer_);
        lineBuffer_.clear();
        return result;
    }

    if (c == '\r')
    {
        // Ignore \r
        return ParseResult::Nothing;
    }

    if (lineBuffer_.size() >= MaxLineLength)
    {
        // Prevent unbounded buffer growth on malformed input
        lineBuffer_.clear();
        return ParseResult::ParseError;
    }

    lineBuffer_.push_back(c);
    return ParseResult::Nothing;
}

// Returns a copy of s with leading and trailing whitespace removed.
std::string SerialParser::trim(const std::string &s)
{
    const auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return {};

    const auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}

// Processes a fully received line and updates the parser state.
ParseResult SerialParser::handleCompletedLine(const std::string &rawLine)
{
    auto result = ParseResult::Nothing; // default return value
    std::string line = trim(rawLine);

    switch (state_)
    {
    case ParserState::Idle:
        if (line == "BEGIN")
        {
            currentSeries_ = MeasurementSeries{};
            state_ = ParserState::ReceivingSeries;
        }
        break;

    case ParserState::ReceivingSeries:
        if (line == "END")
        {
            if (!currentSeries_.empty())
                result = ParseResult::SeriesCompleted;
            state_ = ParserState::Idle;
        }
        else if (line.rfind("DATA ", 0) == 0)
        {
            std::string payload = line.substr(5); // skip "DATA "
            std::replace(payload.begin(), payload.end(), ',', '.');
            result = extractXYData(payload);
            state_ = ParserState::ReceivingSeries;
        }
        else if (line == "BEGIN")
        {
            // Resync, discard incomplete series and start fresh
            currentSeries_ = MeasurementSeries{};
            state_ = ParserState::ReceivingSeries;
        }
        break;
    }

    return result;
}

// Extracts an XY data point and appends it to currentSeries_.
ParseResult SerialParser::extractXYData(const std::string &data)
{
    double x = 0.0;
    double y = 0.0;

    xyStream_.clear();
    xyStream_.str(data);
    xyStream_ >> x >> y;

    if (xyStream_.fail())
        return ParseResult::ParseError;
    if (x < VoltageRangeMin || x > VoltageRangeMax)
        return ParseResult::ParseError;
    if (y < CurrentRangeMin || y > CurrentRangeMax)
        return ParseResult::ParseError;
    if (currentSeriesSize() >= MaxPointsCount)
        return ParseResult::ParseError;

    currentSeries_.addPoint(x, y);
    return ParseResult::DataPointAdded;
}
