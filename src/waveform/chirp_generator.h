#pragma once

#include <complex>
#include <cstddef>
#include <vector>
#include <cstdint>

class ChirpGenerator
{
public:
    ChirpGenerator(
        double sampleRateHz,
        double startFrequencyHz,
        double stopFrequencyHz,
        double durationSeconds,
        float amplitude,
        std::uint32_t pulseRepititionInterval_
    );

    std::vector<std::complex<float>> generate() const;

    std::size_t sampleCount() const;

private:
    double sampleRateHz_;
    double startFrequencyHz_;
    double stopFrequencyHz_;
    double durationSeconds_;
    float amplitude_;
    std::uint32_t pulseRepititionInterval_;
};