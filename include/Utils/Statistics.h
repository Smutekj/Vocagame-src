#pragma once

#include <deque>
#include <vector>
#include <limits>

class Histogram
{
    using BinCountType = unsigned long long;
    using BinIndexType = std::size_t;

public:
    Histogram(std::size_t n_bins, double min_val = 0., double max_val = 1.);

    void addNumber(double num);

    std::vector<double> getNormalizedHist() const;
    
    void clear();

    private:
    BinIndexType  getIndex(double value) const;


    private:
    std::vector<BinCountType> m_bin_counts;
    std::size_t n_bins = 69;
    double min_value = 0.f;
    double max_value = 1.f;
};

struct Statistics
{

    explicit Statistics();

    void addNumber(double num);


    double getAverage();
    double getVariance();
    double getMax();

    void reset();

    std::deque<double> data;
    double avg = 0.;
    double max = std::numeric_limits<double>::min();
    int averaging_interval = 60;

    Histogram m_hist;
};