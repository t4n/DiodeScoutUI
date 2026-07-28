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
        auto result = handleCompletedLine(lineBuffer_);
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
    line = trim(line);

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
            state_ = ParserState::ReceivingSeries;
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
            state_ = ParserState::ReceivingSeries;
        }
        break;
    }

    return result;
}

// Extracts an XY data point and appends it to currentSeries_.
ParseResult SerialParser::extractXYData(std::string_view data)
{
    const char *first = data.begin();
    const char *last = data.end();

    // Parse voltage (V)
    double x = 0;
    auto [sep, ec1] = std::from_chars(first, last, x);

    if (ec1 != std::errc{} || sep == last || *sep != ' ')
        return ParseResult::ParseError;
    if (x < VoltageRangeMin || x > VoltageRangeMax)
        return ParseResult::ParseError;

    // Parse current (mA)
    double y = 0;
    auto [ptr, ec2] = std::from_chars(sep + 1, last, y);

    if (ec2 != std::errc{} || ptr != last)
        return ParseResult::ParseError;
    if (y < CurrentRangeMin || y > CurrentRangeMax)
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
