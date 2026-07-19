#pragma once

#include <complex>
#include <cstddef>
#include <vector>

struct SpectrumFrame
{
    std::vector<double> frequencyHz;
    std::vector<double> magnitudeDbfs;

    double peakFrequencyHz{0.0};
    double peakLevelDbfs{0.0};
    double noiseFloorDbfs{0.0};
};

class SpectrumProcessor
{
public:
    SpectrumProcessor(
        std::size_t fftSize,
        double sampleRateHz
    );

    ~SpectrumProcessor();

    SpectrumProcessor(const SpectrumProcessor&) = delete;
    SpectrumProcessor& operator=(const SpectrumProcessor&) = delete;

    SpectrumFrame process(
        const std::vector<std::complex<float>>& samples
    );

private:
    struct Impl;
    Impl* impl_;
};