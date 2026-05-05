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
#include <QLocale> // exportCSV, exportPython
#include <algorithm>
#include <cmath>
#include <fstream>

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
void MeasurementDataManager::appendSeries(const MeasurementSeries &series)
{
    series_.push_back(series);
}

// Generates and appends a simulated diode I–V characteristic.
void MeasurementDataManager::appendSimulatedSeries(double scaleCurrent)
{
    constexpr double vMax = 5.00; // Max DiodeScout voltage
    constexpr double iMax = 0.01; // Max DiodeScout current
    constexpr double emission = 1.950; // Emission coefficient
    constexpr double vThermal = 0.026; // Thermal voltage, room temp

    constexpr double voltageStep = 0.01;
    constexpr double slope = iMax / vMax;

    MeasurementSeries simul;
    double voltage = 0.0;
    double current = 0.0;

    // Keep below DiodeScout load line (5 V, 500 Ohm)
    while (current < iMax - slope * voltage)
    {
        // First iteration includes origin (0 V, 0 mA)
        simul.addPoint(voltage, current * 1000.0);

        voltage += voltageStep;
        current = scaleCurrent * (std::exp(voltage / emission / vThermal) - 1.0);
    }

    series_.push_back(std::move(simul));
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
        out << "Volt (V);Milliampere (mA)\n";

        for (const auto &p : s.points())
        {
            QString v = loc.toString(p.voltageVolt, 'f', 6);
            QString c = loc.toString(p.currentMilliAmp, 'f', 6);

            out << v.toStdString() << ";" << c.toStdString() << "\n";
        }

        out << "\n";
    }

    return out.good();
}

// Exports all stored measurement series to a Python script.
// Returns true on success.
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
    out << "plt.minorticks_on()\n";
    out << "# plt.savefig('plot.png', dpi=300)\n";
    out << "plt.show()\n";

    return out.good();
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

    // Linear least-squares fit, V = Rs * I + Vf
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
