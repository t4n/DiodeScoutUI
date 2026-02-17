// ---------------------------------------------------------------------------
//  Data model for capturing, storing, and parsing measurement series.
//  This file contains the logic for:
//
//  - Managing individual measurement points and complete measurement series
//  - Receiving and parsing serial input line-by-line
//  - Building temporary series during acquisition
//  - Storing completed series for later visualization (e.g., QtCharts)
// ---------------------------------------------------------------------------

#include "measurementdata.h"
#include <QLocale> // exportToCsv
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------------
//  MeasurementSeries – Implementation
// ---------------------------------------------------------------------------

// Adds a new measurement point.
void MeasurementSeries::addPoint(double voltage, double currentMilliAmp)
{
    points_.push_back({voltage, currentMilliAmp});
}

// Returns all measurement points.
const std::vector<MeasurementPoint> &MeasurementSeries::points() const noexcept
{
    return points_;
}

// Returns the number of measurement points.
std::size_t MeasurementSeries::size() const noexcept
{
    return points_.size();
}

// Checks whether the series is empty.
bool MeasurementSeries::empty() const noexcept
{
    return points_.empty();
}

// ---------------------------------------------------------------------------
//  MeasurementDataManager – Implementation
// ---------------------------------------------------------------------------

// Returns the number of stored measurement series.
std::size_t MeasurementDataManager::seriesCount() const noexcept
{
    return series_.size();
}

// Returns a specific measurement series by index.
const MeasurementSeries &MeasurementDataManager::series(std::size_t index) const
{
    return series_.at(index);
}

// Returns all measurement series.
const std::vector<MeasurementSeries> &MeasurementDataManager::allSeries() const noexcept
{
    return series_;
}

// Removes all stored measurement series.
void MeasurementDataManager::removeAllSeries()
{
    series_.clear();
}

// Removes the last measurement series.
void MeasurementDataManager::removeLastSeries()
{
    if (!series_.empty())
        series_.pop_back();
}

// Returns the number of points in the temporary series.
std::size_t MeasurementDataManager::tempSeriesSize() const noexcept
{
    return tempSeries_.size();
}

// Returns the maximum voltage across all series.
double MeasurementDataManager::getMaxVoltage() const noexcept
{
    double maxV = 0.0;

    for (const auto &series : series_)
        for (const auto &p : series.points())
            if (p.voltageVolt > maxV)
                maxV = p.voltageVolt;

    for (const auto &p : tempSeries_.points())
        if (p.voltageVolt > maxV)
            maxV = p.voltageVolt;

    return maxV;
}

// Returns the maximum current across all series.
double MeasurementDataManager::getMaxCurrent() const noexcept
{
    double maxI = 0.0;

    for (const auto &series : series_)
        for (const auto &p : series.points())
            if (p.currentMilliAmp > maxI)
                maxI = p.currentMilliAmp;

    for (const auto &p : tempSeries_.points())
        if (p.currentMilliAmp > maxI)
            maxI = p.currentMilliAmp;

    return maxI;
}

// Parser: processes a single received character.
ParseResult MeasurementDataManager::processReceivedChar(char c)
{
    if (c == '\n')
    {
        auto result = handleCompletedLine(currentLine_);
        currentLine_.clear();
        return result;
    }
    else if (c != '\r')
    {
        currentLine_.push_back(c);
    }

    return ParseResult::Nothing;
}

// Exports all stored measurement series to a CSV file.
// Returns true on success, false on failure.
bool MeasurementDataManager::exportCSV(const std::string &filePath) const
{
    std::ofstream out(filePath);
    if (!out.is_open())
        return false;

    // Use Qt's locale (system locale)
    QLocale loc;

    for (std::size_t i = 0; i < series_.size(); ++i)
    {
        const auto &s = series_[i];

        out << "Series " << (i + 1) << "\n";
        out << "Voltage (V);Current (mA)\n";

        for (const auto &p : s.points())
        {
            QString v = loc.toString(p.voltageVolt, 'f', 6);
            QString c = loc.toString(p.currentMilliAmp, 'f', 6);

            out << v.toStdString() << ";" << c.toStdString() << "\n";
        }

        out << "\n";
    }

    return true;
}

// Exports all stored measurement series to a Python script.
// Returns true on success, false on failure.
bool MeasurementDataManager::exportPython(const std::string &filePath) const
{
    std::ofstream out(filePath);
    if (!out.is_open())
        return false;

    // Force C locale (Python expects dot!)
    QLocale loc(QLocale::C);

    // Header
    out << "#!/usr/bin/env python3\n";
    out << "import matplotlib.pyplot as plt\n\n";
    out << "series = []\n\n";

    for (std::size_t i = 0; i < series_.size(); ++i)
    {
        const auto &s = series_[i];
        int idx = i + 1;

        out << "# Series " << idx << "\n";
        out << "voltage_" << idx << " = [";
        for (size_t j = 0; j < s.points().size(); ++j)
        {
            const auto &p = s.points()[j];
            out << loc.toString(p.voltageVolt, 'f', 6).toStdString();
            if (j + 1 < s.points().size())
                out << ", ";
        }
        out << "]\n";

        out << "current_" << idx << " = [";
        for (size_t j = 0; j < s.points().size(); ++j)
        {
            const auto &p = s.points()[j];
            out << loc.toString(p.currentMilliAmp, 'f', 6).toStdString();
            if (j + 1 < s.points().size())
                out << ", ";
        }
        out << "]\n";

        out << "series.append((voltage_" << idx << ", current_" << idx << "))\n\n";
    }

    // Plot section
    out << "for i, (v, c) in enumerate(series):\n";
    out << "    plt.plot(v, c, label=f'Series {i+1}')\n\n";
    out << "plt.xlabel('Volt (V)')\n";
    out << "plt.ylabel('Milliampere (mA)')\n";
    out << "plt.legend()\n";
    out << "plt.grid(True)\n";
    out << "plt.show()\n";

    return true;
}

// Removes leading and trailing whitespace.
std::string MeasurementDataManager::trim(const std::string &s)
{
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;

    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;

    return s.substr(start, end - start);
}

// Processes a fully received line.
ParseResult MeasurementDataManager::handleCompletedLine(const std::string &rawLine)
{
    std::string line = trim(rawLine);
    if (line.empty())
        return ParseResult::Nothing;

    // Start of a new measurement series
    if (line == "*")
    {
        tempSeries_ = MeasurementSeries{};
        state_ = ParserState::ReceivingSeries;
        return ParseResult::Nothing;
    }

    // Ignore lines like "* AVCC = ..."
    if (line[0] == '*' && line != "*")
        return ParseResult::Nothing;

    // End of a measurement series
    if (line == "#")
    {
        if (state_ == ParserState::ReceivingSeries && !tempSeries_.empty())
        {
            series_.push_back(tempSeries_);
            tempSeries_ = MeasurementSeries{};
            state_ = ParserState::Idle;
            return ParseResult::SeriesCompleted;
        }
        return ParseResult::Nothing;
    }

    // Normal data line
    if (state_ == ParserState::ReceivingSeries)
        parseDataLine(line);

    return ParseResult::Nothing;
}

// Parses a data line in the format "<x> <y>".
void MeasurementDataManager::parseDataLine(const std::string &line)
{
    std::istringstream iss(line);
    double x = 0.0;
    double y = 0.0;

    if (!(iss >> x >> y))
        return;

    tempSeries_.addPoint(x, y);
}
