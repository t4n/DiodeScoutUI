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

#include "datamanager.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

// Returns the number of stored measurement series.
std::size_t MeasurementDataManager::seriesCount() const noexcept
{
    return series_.size();
}

// Returns a read-only reference to all stored measurement series.
const std::vector<MeasurementSeries> &MeasurementDataManager::allSeries() const noexcept
{
    return series_;
}

// Removes all stored measurement series.
void MeasurementDataManager::removeAllSeries()
{
    series_.clear();
}

// Removes the most recently added measurement series.
void MeasurementDataManager::removeLastSeries()
{
    if (!series_.empty())
        series_.pop_back();
}

// Adds a completed measurement series to the collection.
void MeasurementDataManager::appendSeries(MeasurementSeries series)
{
    series_.push_back(std::move(series));
}

// Appends simulated diode I–V characteristics to the collection.
void MeasurementDataManager::appendSimulatedSeries()
{
    static constexpr std::array voltage_1 = {0.000000, 0.193000, 0.290000, 0.389000, 0.489000, 0.552000, 0.603000,
        0.630000, 0.650000, 0.659000, 0.671000, 0.682000, 0.687000, 0.696000, 0.701000, 0.707000, 0.711000, 0.716000,
        0.720000, 0.724000, 0.727000, 0.730000, 0.734000, 0.734000, 0.739000, 0.739000, 0.744000, 0.744000, 0.745000,
        0.748000, 0.749000, 0.753000, 0.753000, 0.754000, 0.758000, 0.758000, 0.758000, 0.761000, 0.763000, 0.763000,
        0.763000, 0.767000, 0.767000, 0.767000, 0.768000, 0.772000, 0.772000, 0.772000, 0.772000, 0.772000};

    static constexpr std::array current_1 = {0.000000, 0.000000, 0.000000, 0.002000, 0.009000, 0.045000, 0.151000,
        0.294000, 0.468000, 0.595000, 0.777000, 0.969000, 1.104000, 1.301000, 1.506000, 1.693000, 1.863000, 2.027000,
        2.214000, 2.403000, 2.636000, 2.818000, 2.984000, 3.190000, 3.373000, 3.540000, 3.748000, 3.935000, 4.068000,
        4.293000, 4.471000, 4.642000, 4.883000, 5.054000, 5.235000, 5.423000, 5.611000, 5.792000, 5.988000, 6.179000,
        6.365000, 6.581000, 6.740000, 6.937000, 7.137000, 7.328000, 7.505000, 7.711000, 7.768000, 7.768000};

    static constexpr std::array voltage_2 = {0.000000, 0.195000, 0.293000, 0.391000, 0.481000, 0.588000, 0.678000,
        0.772000, 0.875000, 0.959000, 1.073000, 1.158000, 1.261000, 1.355000, 1.453000, 1.536000, 1.583000, 1.614000,
        1.640000, 1.656000, 1.670000, 1.684000, 1.691000, 1.701000, 1.712000, 1.717000, 1.726000, 1.734000, 1.740000,
        1.746000, 1.754000, 1.757000, 1.764000, 1.769000, 1.774000, 1.779000, 1.786000, 1.789000, 1.796000, 1.802000,
        1.807000, 1.811000, 1.816000, 1.820000, 1.825000, 1.830000, 1.834000, 1.839000, 1.840000, 1.840000};

    static constexpr std::array current_2 = {0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.008000, 0.042000, 0.127000, 0.236000,
        0.408000, 0.546000, 0.715000, 0.895000, 1.037000, 1.215000, 1.413000, 1.599000, 1.726000, 1.960000, 2.084000,
        2.267000, 2.495000, 2.626000, 2.799000, 3.022000, 3.203000, 3.371000, 3.572000, 3.718000, 3.902000, 4.108000,
        4.297000, 4.447000, 4.655000, 4.819000, 5.009000, 5.198000, 5.379000, 5.576000, 5.715000, 5.724000};

    // Helper to add a series
    auto addSeries = [this](const auto &v, const auto &i)
    {
        if (v.size() != i.size())
            return; // malformed built-in simulation data

        MeasurementSeries s;
        for (size_t idx = 0; idx < v.size(); ++idx)
            s.addPoint(v[idx], i[idx]);
        series_.push_back(std::move(s));
    };

    // Append both series
    addSeries(voltage_1, current_1);
    addSeries(voltage_2, current_2);
}

// Retrieves the maximum voltage and current across all series.
void MeasurementDataManager::getMaxVoltageAndCurrent(double &maxV, double &maxI) const noexcept
{
    maxV = 0.0;
    maxI = 0.0;

    for (const auto &series : series_)
    {
        for (const auto &p : series.points())
        {
            maxV = std::max(maxV, p.voltageVolt);
            maxI = std::max(maxI, p.currentMilliAmp);
        }
    }
}

// Exports all stored measurement series to a CSV file.
// Returns true on success.
bool MeasurementDataManager::exportCSV(const std::string &filePath, CSVSettings csv) const
{
    std::ofstream out(filePath);
    if (!out.is_open())
        return false;

    // Format numeric values according to the CSV settings
    const auto formatNumber = [&csv](double value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        std::string s = oss.str();
        if (csv.decimalSeparator != '.')
            std::replace(s.begin(), s.end(), '.', csv.decimalSeparator);
        return s;
    };

    for (std::size_t i = 0; i < series_.size(); ++i)
    {
        const auto &s = series_[i];
        out << "Series " << (i + 1) << "\n";
        out << "Volt (V)" << csv.fieldSeparator << "Milliampere (mA)\n";

        for (const auto &p : s.points())
            out << formatNumber(p.voltageVolt) << csv.fieldSeparator << formatNumber(p.currentMilliAmp) << "\n";

        out << "\n";
    }

    return out.flush().good();
}

// Exports all stored measurement series to a Python script.
// Returns true on success.
bool MeasurementDataManager::exportPython(const std::string &filePath) const
{
    std::ofstream out(filePath);
    if (!out.is_open())
        return false;

    // Header
    out << std::fixed << std::setprecision(6);
    out << "#!/usr/bin/env python3\n";
    out << "import matplotlib.pyplot as plt\n\n";
    out << "series = []\n\n";

    for (std::size_t i = 0; i < series_.size(); ++i)
    {
        const auto &s = series_[i];
        const std::size_t idx = i + 1;
        out << "# Series " << idx << "\n";

        // Voltage list
        out << "voltage_" << idx << " = [";
        for (std::size_t j = 0; j < s.points().size(); ++j)
        {
            const auto &p = s.points()[j];
            out << p.voltageVolt;
            if (j + 1 < s.points().size())
                out << ", ";
        }
        out << "]\n";

        // Current list
        out << "current_" << idx << " = [";
        for (std::size_t j = 0; j < s.points().size(); ++j)
        {
            const auto &p = s.points()[j];
            out << p.currentMilliAmp;
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
    out << "plt.minorticks_on()\n";
    out << "# plt.savefig('plot.png', dpi=300)\n";
    out << "plt.show()\n";

    return out.flush().good();
}

// Computes piecewise-linear diode parameters (Vf, Rs).
// Returns true on success.
bool MeasurementDataManager::computePWL(double &forwardV, double &seriesR) const
{
    // Exactly one measurement series is required
    if (series_.size() != 1 || series_[0].empty())
        return false;

    // Ignore measurement points below 0.5 * maxI (non-conducting diode)
    const double noiseFloor = 0.1; // mA
    const double maxI = series_[0].points().back().currentMilliAmp;
    const double threshold = std::max(0.5 * maxI, noiseFloor);

    // Linear least-squares fit: V = Rs * I + Vf where
    // Rs = Effective series resistance, Vf = Forward voltage (turn-on)
    int n = 0;
    double sumI = 0.0;
    double sumV = 0.0;
    double sumIV = 0.0;
    double sumII = 0.0;

    for (const auto &p : series_[0].points())
    {
        if (p.currentMilliAmp < threshold)
            continue;

        double I = p.currentMilliAmp * 1e-3; // convert mA to A
        sumI += I;
        sumV += p.voltageVolt;
        sumIV += I * p.voltageVolt;
        sumII += I * I;
        ++n;
    }

    // Require at least 2 points
    const double denom = (n * sumII - sumI * sumI);
    if (n < 2 || denom < 1e-15)
        return false;

    seriesR = (n * sumIV - sumI * sumV) / denom;
    forwardV = (sumV - seriesR * sumI) / n;

    // Sanity checks
    if (!std::isfinite(seriesR) || !std::isfinite(forwardV))
        return false;
    if (seriesR <= 0.0 || forwardV <= 0.0)
        return false;

    return true;
}
