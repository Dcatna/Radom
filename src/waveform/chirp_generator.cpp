#include "chirp_generator.h"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstdint>

ChirpGenerator::ChirpGenerator(
    double sampleRateHz,
    double startFrequencyHz,
    double stopFrequencyHz,
    double durationSeconds,
    float amplitude,
    std::uint32_t pulseRepititionInterval
)
    : sampleRateHz_(sampleRateHz),
      startFrequencyHz_(startFrequencyHz),
      stopFrequencyHz_(stopFrequencyHz),
      durationSeconds_(durationSeconds),
      amplitude_(amplitude),
      pulseRepititionInterval_(pulseRepititionInterval)
{
    if (sampleRateHz_ <= 0.0)
    {
        throw std::invalid_argument(
            "Sample rate must be greater than zero."
        );
    }

    if (durationSeconds_ <= 0.0)
    {
        throw std::invalid_argument(
            "Chirp duration must be greater than zero."
        );
    }

    if (amplitude_ < 0.0F || amplitude_ > 1.0F)
    {
        throw std::invalid_argument(
            "Amplitude must be between 0 and 1."
        );
    }

    const double nyquistHz = sampleRateHz_ / 2.0;

    if (
        std::abs(startFrequencyHz_) >= nyquistHz ||
        std::abs(stopFrequencyHz_) >= nyquistHz
    )
    {
        throw std::invalid_argument(
            "Chirp endpoints must be inside "
            "the Nyquist frequency range."
        );
    }
}

std::size_t ChirpGenerator::sampleCount() const
{
    return static_cast<std::size_t>(
        std::llround(
            sampleRateHz_ * durationSeconds_
        )
    );
}

std::vector<std::complex<float>>
ChirpGenerator::generate() const
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(pulseRepititionInterval_));
    const std::size_t count = sampleCount();

    if (count < 2)
    {
        throw std::runtime_error(
            "Chirp must contain at least two samples."
        );
    }

    // how many samples are in one chirp
    // N = F_s * T where N is number of samples, F_s is the sample rate in samples/second, and T is the duration in seconds
    // So if F = 5e6 and T is .005s then N = 250,000 samples (IQ) for one 50ms chirp
    std::vector<std::complex<float>> samples(count);

    // Chirp bandwidth -> B = f_stop - f_start
    // so for a chirp from -800 kHz to +800 kHz
    // B = 800,000 - (-800,000) = 1,600,000 Hz (1.6 MHz)
    // with a 915 MHz LO -> 915 MHz - 0.8 MHz = 914.2 MHz and ends at 915 MHz + 0.8 MHz = 915.8 MHz
    const double bandwidthHz =
        stopFrequencyHz_ - startFrequencyHz_;

    // Chirp slope -> k = B / T
    // For B = 1.6 MHz and T = 50ms (.05s) -> k = 1,600,000 / .05 = 32,000,000 Hz/s
    // This means the isntantaneous frequncy increase by 32 MHz per second
    const double chirpSlopeHzPerSecond =
        bandwidthHz / durationSeconds_;

    for (std::size_t index = 0; index < count; ++index)
    {
        // convert the sample index into time -> t_n = n / F_s, where n is the sample index and F_s is the sample rate in samples/second
        const double timeSeconds =
            static_cast<double>(index) /
            sampleRateHz_;

        // this calculates the phase
        // implementing -> ϕ(t)=2π(f_0​t + 1/2/​kt^2)
        // where f_0 is startFrequecny Hz, k is the chirp slope and t is time in seconds
        const double phaseRadians =
            2.0 *
            std::numbers::pi *
            (
                startFrequencyHz_ * timeSeconds +
                0.5 *
                chirpSlopeHzPerSecond *
                timeSeconds *
                timeSeconds
            );

        //conver the phase into I/Q smaples
        samples[index] =
            amplitude_ *
            std::complex<float>(
                static_cast<float>(
                    std::cos(phaseRadians)
                ),
                static_cast<float>(
                    std::sin(phaseRadians)
                )
            );
    }

    return samples;
}