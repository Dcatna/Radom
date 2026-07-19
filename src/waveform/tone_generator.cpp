#include "tone_generator.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

ToneGenerator::ToneGenerator(
    double sampleRateHz,
    double toneFrequencyHz,
    float amplitude
)
    : sampleRateHz_(sampleRateHz),
      toneFrequencyHz_(toneFrequencyHz),
      amplitude_(amplitude),
      phaseRadians_(0.0)
{
    if (sampleRateHz_ <= 0.0)
    {
        throw std::invalid_argument(
            "Sample rate must be greater than zero."
        );
    }

    if (std::abs(toneFrequencyHz_) >= sampleRateHz_ / 2.0)
    {
        throw std::invalid_argument(
            "Tone frequency must be inside the Nyquist range."
        );
    }

    if (amplitude_ < 0.0F || amplitude_ > 1.0F)
    {
        throw std::invalid_argument(
            "Amplitude must be between 0 and 1."
        );
    }
}

std::vector<std::complex<float>> ToneGenerator::generate(
    std::size_t sampleCount
)
{
    std::vector<std::complex<float>> samples(sampleCount);

    const double phaseIncrement =
        2.0 *
        std::numbers::pi *
        toneFrequencyHz_ /
        sampleRateHz_;

    for (std::size_t index = 0; index < sampleCount; ++index)
    {
        samples[index] = amplitude_ *
            std::complex<float>(
                static_cast<float>(std::cos(phaseRadians_)),
                static_cast<float>(std::sin(phaseRadians_))
            );

        phaseRadians_ += phaseIncrement;

        if (phaseRadians_ >= 2.0 * std::numbers::pi)
        {
            phaseRadians_ -= 2.0 * std::numbers::pi;
        }
        else if (phaseRadians_ < 0.0)
        {
            phaseRadians_ += 2.0 * std::numbers::pi;
        }
    }

    return samples;
}

void ToneGenerator::resetPhase()
{
    phaseRadians_ = 0.0;
}