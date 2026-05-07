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

#include "serialparser.h"
#include <QDebug>
#include <algorithm>
#include <cctype>
#include <sstream>

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
// SeriesCompleted when END is received, or Nothing otherwise.
ParseResult SerialParser::processReceivedChar(char c)
{
    if (c == '\n')
    {
        auto result = handleCompletedLine(lineBuffer_);
        lineBuffer_.clear();
        return result;
    }

    if (c != '\r')
    {
        lineBuffer_.push_back(c);
    }

    // Safety guard: prevent unbounded buffer growth on malformed input
    if (lineBuffer_.size() >= MaxLineLength)
    {
        qWarning() << "Invalid data:" << QString::fromStdString(lineBuffer_);
        lineBuffer_.clear();
    }

    return ParseResult::Nothing;
}

// Removes leading and trailing whitespace.
std::string SerialParser::trim(const std::string &s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
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
        else if (line.find("DATA ") == 0)
        {
            std::string payload = line.substr(5); // skip "DATA "
            std::replace(payload.begin(), payload.end(), ',', '.');
            result = extractXYData(payload);
        }
        else if (line == "BEGIN")
        {
            qWarning() << "Resync: discard incomplete series and start fresh";
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
    auto fail = [&data](const QString &msg)
    {
        qWarning() << msg << QString::fromStdString(data);
        return ParseResult::Nothing;
    };

    double x = 0.0;
    double y = 0.0;

    std::istringstream iss(data);
    iss >> x >> y;

    if (iss.fail())
        return fail("Invalid DATA: ");
    if (x < VoltageRangeMin || x > VoltageRangeMax)
        return fail("Invalid voltage: ");
    if (y < CurrentRangeMin || y > CurrentRangeMax)
        return fail("Invalid current: ");
    if (currentSeriesSize() >= MaxPointsCount)
        return fail("MaxPointsCount overrun: ");

    currentSeries_.addPoint(x, y);
    return ParseResult::DataPointAdded;
}
