// ---------------------------------------------------------------------------
//  State-machine parser for the DiodeScout serial data format.
//  It detects BEGIN/END blocks, parses DATA lines, and builds a
//  MeasurementSeries from the incoming character stream.
//
//  - Call processReceivedChar() for each incoming character.
//  - When SeriesCompleted is returned, the current series
//    contains a fully parsed measurement sequence.
// ---------------------------------------------------------------------------

// Portable core module, no Qt dependencies.
#include "serialparser.h"
#include <charconv>
#include <cmath>

// Returns a read-only reference to the current measurement series.
// The series is parser-owned and may change as parsing continues.
const MeasurementSeries &SerialParser::currentSeries() const noexcept
{
    return currentSeries_;
}

// Returns DataPointAdded when a DATA line is parsed, SeriesCompleted when
// END is received, ParseError on invalid input, or Nothing otherwise.
ParseResult SerialParser::processReceivedChar(char c)
{
    if (c == '\n')
    {
        std::string_view trimmedLine = trim(lineBuffer_);
        auto result = handleCompletedLine(trimmedLine);
        lineBuffer_.clear();
        return result;
    }

    if (c == '\r')
    {
        // CRLF normalization
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

// Processes a fully received line and updates the parser state.
ParseResult SerialParser::handleCompletedLine(std::string_view line)
{
    auto result = ParseResult::Nothing; // default return value

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
        if (line.starts_with("DATA "))
        {
            result = extractXYData(line.substr(5)); // skip "DATA "
            if (result != ParseResult::DataPointAdded)
                state_ = ParserState::Idle;
        }
        else if (line == "END")
        {
            if (!currentSeries_.empty())
                result = ParseResult::SeriesCompleted;
            state_ = ParserState::Idle;
        }
        else if (line == "BEGIN")
        {
            // Resync, discard incomplete series and start fresh
            currentSeries_ = MeasurementSeries{};
        }
        break;
    }

    return result;
}

// Extracts an XY data point and appends it to currentSeries_.
ParseResult SerialParser::extractXYData(std::string_view data)
{
    const char *begin = data.data();
    const char *end = begin + data.size();

    // Parse voltage (V)
    double x = 0.0;
    auto retX = std::from_chars(begin, end, x);

    if (retX.ec != std::errc() || retX.ptr == end || *retX.ptr != ' ')
        return ParseResult::ParseError;
    if (std::isnan(x) || x < VoltageRangeMin || x > VoltageRangeMax)
        return ParseResult::ParseError;

    // Parse current (mA)
    double y = 0.0;
    auto retY = std::from_chars(retX.ptr + 1, end, y); // skip delimiter

    if (retY.ec != std::errc() || retY.ptr != end)
        return ParseResult::ParseError;
    if (std::isnan(y) || y < CurrentRangeMin || y > CurrentRangeMax)
        return ParseResult::ParseError;

    // Series exceeds expected size
    if (currentSeries_.size() >= MaxPointsCount)
        return ParseResult::ParseError;

    currentSeries_.addPoint(x, y);
    return ParseResult::DataPointAdded;
}

// Returns a view of s without leading/trailing whitespace.
std::string_view SerialParser::trim(std::string_view s)
{
    const auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string_view::npos)
        return {};

    const auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}
