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

// Generates and appends simulated diode I–V characteristics.
void MeasurementDataManager::appendSimulatedSeries()
{
    static constexpr std::array voltage_1 = {0.000000, 0.100000, 0.199992, 0.299891, 0.398626, 0.486816, 0.543542,
        0.574650, 0.594020, 0.607673, 0.618098, 0.626483, 0.633477, 0.639465, 0.644694, 0.649331, 0.653495, 0.657271,
        0.660724, 0.663905, 0.666852, 0.669599, 0.672169, 0.674583, 0.676861, 0.679014, 0.681058, 0.683001, 0.684854,
        0.686625, 0.688320, 0.689945, 0.691506, 0.693008, 0.694456, 0.695852, 0.697200, 0.698503, 0.699766, 0.700989,
        0.702176, 0.703327, 0.704447, 0.705535, 0.706594, 0.707625, 0.708630, 0.709610, 0.710566, 0.711500, 0.712412};

    static constexpr std::array current_1 = {0.000000, 0.000001, 0.000017, 0.000218, 0.002748, 0.026367, 0.112915,
        0.250702, 0.411963, 0.584655, 0.763811, 0.947032, 1.133043, 1.321060, 1.510619, 1.701340, 1.893027, 2.085480,
        2.278576, 2.472198, 2.666284, 2.860813, 3.055657, 3.250852, 3.446304, 3.641954, 3.837913, 4.033993, 4.230309,
        4.426762, 4.623370, 4.820136, 5.016952, 5.214015, 5.411131, 5.608334, 5.805578, 6.002940, 6.200429, 6.397984,
        6.595630, 6.793303, 6.991101, 7.188976, 7.386809, 7.584704, 7.782756, 7.980788, 8.178849, 8.376958, 8.575172};

    static constexpr std::array voltage_2 = {0.000000, 0.100000, 0.200000, 0.299999, 0.399986, 0.499816, 0.597734,
        0.680892, 0.730980, 0.758874, 0.776760, 0.789612, 0.799549, 0.807609, 0.814372, 0.820188, 0.825285, 0.829817,
        0.833895, 0.837601, 0.840995, 0.844126, 0.847030, 0.849738, 0.852275, 0.854660, 0.856911, 0.859041, 0.861063,
        0.862988, 0.864823, 0.866578, 0.868258, 0.869870, 0.871419, 0.872909, 0.874346, 0.875732, 0.877071, 0.878366,
        0.879620, 0.880835, 0.882015, 0.883160, 0.884273, 0.885355, 0.886408, 0.887434, 0.888434, 0.889408, 0.890360};

    static constexpr std::array current_2 = {0.000000, 0.000000, 0.000000, 0.000002, 0.000028, 0.000368, 0.004531,
        0.038216, 0.138042, 0.282252, 0.446484, 0.620771, 0.800900, 0.984784, 1.171249, 1.359621, 1.549430, 1.740349,
        1.932213, 2.124786, 2.318027, 2.511748, 2.705957, 2.900531, 3.095468, 3.290674, 3.486193, 3.681937, 3.877861,
        4.074014, 4.270319, 4.466877, 4.663504, 4.860234, 5.057150, 5.254173, 5.451294, 5.648496, 5.845852, 6.043198,
        6.240717, 6.438257, 6.635948, 6.833715, 7.031450, 7.229332, 7.427231, 7.625176, 7.823144, 8.021138, 8.219277};

    // Helper to add a series
    auto addSeries = [&](const auto &v, const auto &i)
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
