#include "spectrum_processor.h"

#include <fftw3.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>
#include <vector>

struct SpectrumProcessor::Impl
{
    std::size_t fftSize;
    double sampleRateHz;

    fftwf_complex* input{nullptr};
    fftwf_complex* output{nullptr};
    fftwf_plan plan{nullptr};

    std::vector<double> window;
    double windowSum{0.0};

    Impl(
        std::size_t requestedFftSize,
        double requestedSampleRate
    )
        : fftSize(requestedFftSize),
          sampleRateHz(requestedSampleRate),
          window(requestedFftSize)
    {
        if (fftSize == 0)
        {
            throw std::invalid_argument(
                "FFT size must be greater than zero."
            );
        }

        if (sampleRateHz <= 0.0)
        {
            throw std::invalid_argument(
                "Sample rate must be greater than zero."
            );
        }

        input = fftwf_alloc_complex(
            static_cast<int>(fftSize)
        );

        output = fftwf_alloc_complex(
            static_cast<int>(fftSize)
        );

        if (input == nullptr || output == nullptr)
        {
            throw std::runtime_error(
                "Unable to allocate FFTW buffers."
            );
        }

        plan = fftwf_plan_dft_1d(
            static_cast<int>(fftSize),
            input,
            output,
            FFTW_FORWARD,
            FFTW_ESTIMATE
        );

        if (plan == nullptr)
        {
            throw std::runtime_error(
                "Unable to create FFTW plan."
            );
        }

        for (std::size_t index = 0; index < fftSize; ++index)
        {
            const double phase =
                2.0 *
                std::numbers::pi *
                static_cast<double>(index) /
                static_cast<double>(fftSize - 1);

            window[index] =
                0.42 -
                0.5 * std::cos(phase) +
                0.08 * std::cos(2.0 * phase);

            windowSum += window[index];
        }
    }

    ~Impl()
    {
        if (plan != nullptr)
        {
            fftwf_destroy_plan(plan);
        }

        if (input != nullptr)
        {
            fftwf_free(input);
        }

        if (output != nullptr)
        {
            fftwf_free(output);
        }
    }
};

SpectrumProcessor::SpectrumProcessor(
    std::size_t fftSize,
    double sampleRateHz
)
    : impl_(new Impl(fftSize, sampleRateHz))
{
}

SpectrumProcessor::~SpectrumProcessor()
{
    delete impl_;
}

SpectrumFrame SpectrumProcessor::process(
    const std::vector<std::complex<float>>& samples
)
{
    if (samples.size() != impl_->fftSize)
    {
        throw std::invalid_argument(
            "Input sample count must match FFT size."
        );
    }

    std::complex<double> mean{0.0, 0.0};

    for (const auto& sample : samples)
    {
        mean += sample;
    }

    mean /= static_cast<double>(samples.size());

    for (std::size_t index = 0;
         index < impl_->fftSize;
         ++index)
    {
        const std::complex<double> centered =
            static_cast<std::complex<double>>(samples[index]) -
            mean;

        impl_->input[index][0] =
            static_cast<float>(
                centered.real() * impl_->window[index]
            );

        impl_->input[index][1] =
            static_cast<float>(
                centered.imag() * impl_->window[index]
            );
    }

    fftwf_execute(impl_->plan);

    SpectrumFrame frame;
    frame.frequencyHz.resize(impl_->fftSize);
    frame.magnitudeDbfs.resize(impl_->fftSize);

    frame.peakLevelDbfs = -300.0;

    std::vector<double> noiseValues;
    noiseValues.reserve(impl_->fftSize);

    for (std::size_t shiftedIndex = 0;
         shiftedIndex < impl_->fftSize;
         ++shiftedIndex)
    {
        const std::size_t rawIndex =
            (shiftedIndex + impl_->fftSize / 2) %
            impl_->fftSize;

        const long signedBin =
            static_cast<long>(shiftedIndex) -
            static_cast<long>(impl_->fftSize / 2);

        const double frequencyHz =
            static_cast<double>(signedBin) *
            impl_->sampleRateHz /
            static_cast<double>(impl_->fftSize);

        const double real =
            impl_->output[rawIndex][0];

        const double imaginary =
            impl_->output[rawIndex][1];

        const double magnitude =
            std::hypot(real, imaginary) /
            impl_->windowSum;

        const double magnitudeDbfs =
            20.0 *
            std::log10(
                std::max(magnitude, 1e-15)
            );

        frame.frequencyHz[shiftedIndex] = frequencyHz;
        frame.magnitudeDbfs[shiftedIndex] = magnitudeDbfs;

        if (std::abs(frequencyHz) > 20e3 &&
            magnitudeDbfs > frame.peakLevelDbfs)
        {
            frame.peakFrequencyHz = frequencyHz;
            frame.peakLevelDbfs = magnitudeDbfs;
        }

        if (std::abs(frequencyHz) > 30e3)
        {
            noiseValues.push_back(magnitudeDbfs);
        }
    }

    if (!noiseValues.empty())
    {
        const auto middle =
            noiseValues.begin() +
            static_cast<std::ptrdiff_t>(
                noiseValues.size() / 2
            );

        std::nth_element(
            noiseValues.begin(),
            middle,
            noiseValues.end()
        );

        frame.noiseFloorDbfs = *middle;
    }

    return frame;
}