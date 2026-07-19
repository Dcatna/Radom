#pragma once

#include <complex>
#include <cstddef>
#include <vector>

class ToneGenerator
{
public:
    ToneGenerator(
        double sampleRateHz,
        double toneFrequencyHz,
        float amplitude
    );

    std::vector<std::complex<float>> generate(
        std::size_t sampleCount
    );

    void resetPhase();

private:
    double sampleRateHz_;
    double toneFrequencyHz_;
    float amplitude_;
    double phaseRadians_;
};