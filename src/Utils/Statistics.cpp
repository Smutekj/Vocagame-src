#include "Statistics.h"

#include <algorithm>
#include <numeric>
#include <cmath>

Statistics::Statistics()
    : m_hist(101)
{
}

void Statistics::addNumber(double num)
{
    data.push_back(num);
    if (data.size() >= averaging_interval)
    {
        data.pop_front();
    }
    avg = std::accumulate(data.begin(), data.end(), 0.) / data.size();
    m_hist.addNumber(num);

    max = std::max(max, num);
}
double Statistics::getAverage()
{
    return avg;
}
void Statistics::reset()
{
    max = std::numeric_limits<double>::min();
}
double Statistics::getVariance()
{
    auto avg2 = std::accumulate(data.begin(), data.end(), 0., [](double sum_2, double datum){ return sum_2 + datum*datum;}) / data.size();
    return avg2 - avg * avg;
}

double Statistics::getMax()
{
    return max;
}

Histogram::Histogram(std::size_t n_bins, double min_val, double max_val)
    : min_value(min_val), max_value(max_val), m_bin_counts(n_bins)
{
}

void Histogram::addNumber(double num)
{
    auto index = getIndex(num);
    if (index != -1)
    {
        m_bin_counts.at(index)++;
    }
}

void Histogram::clear()
{
    std::fill(m_bin_counts.begin(), m_bin_counts.end(), 0);
}

std::vector<double> Histogram::getNormalizedHist() const
{

    auto total_count = std::accumulate(m_bin_counts.begin(), m_bin_counts.end(), 0);

    std::vector<double> values(n_bins);
    for (std::size_t i = 0; i < n_bins; ++i)
    {
        values.at(i) = (double)m_bin_counts.at(i) / (double)total_count;
    }
    return values;
}

Histogram::BinIndexType Histogram::getIndex(double value) const
{
    double dr = (max_value - min_value) / n_bins;
    int index = std::floor((value - min_value) / dr);
    if (index >= 0 && index < m_bin_counts.size())
    {
        return index;
    }
    return -1;
}