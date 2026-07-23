#include "spectrum_worker.h"

#include "dsp/spectrum_processor.h"
#include "radio/radio_controller.h"
#include "waveform/tone_generator.h"
#include "waveform/chirp_generator.h"

#include <QElapsedTimer>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <complex>
#include <exception>
#include <thread>
#include <vector>
SpectrumWorker::SpectrumWorker(
    const RadioSettings& settings,
    QObject* parent
)
    : QObject(parent),
      settings_(settings)
{
}

void SpectrumWorker::requestStop()
{
    stopRequested_.store(true);
}

void SpectrumWorker::run()
{
    const double sampleRateHz =
        settings_.sampleRateHz;

    const double centerFrequencyHz =
        settings_.centerFrequencyHz;

    const double toneFrequencyHz =
        settings_.toneFrequencyHz;
    
    int mode = settings_.mode;

    constexpr std::size_t txBufferSize = 10000;
    constexpr std::size_t fftSize = 16384;

    // Only display ±1 MHz around the center.
    constexpr double displayHalfSpanHz = 1e6;

    // Exponential FFT averaging.
    // Higher values produce a smoother display.
    constexpr double averagingFactor = 0.75;

    std::vector<std::complex<float>> txSamples;
    try
    {
        emit statusChanged("Opening LimeSDR...");
        if (mode == 0) {
            ChirpGenerator chirpGenerator(
                sampleRateHz,
                -800e3,
                +800e3,
                50e-3,
                0.10F,
                5
            );

            txSamples = chirpGenerator.generate();
        } else {
            ToneGenerator toneGenerator(
                sampleRateHz,
                toneFrequencyHz,
                settings_.txAmplitude
            );

            txSamples = toneGenerator.generate(txBufferSize);
        }


        RadioConfig radioConfig;
        radioConfig.sampleRateHz = sampleRateHz;
        radioConfig.centerFrequencyHz = centerFrequencyHz;
        radioConfig.bandwidthHz = sampleRateHz;
        radioConfig.txGainDb = settings_.txGainDb;
        radioConfig.rxGainDb = settings_.rxGainDb;
        radioConfig.txAntenna = "Band2";
        radioConfig.rxAntenna = "LNAW";

        /*
         * RadioController remains alive until this worker stops.
         * It owns one device instance and both TX/RX streams.
         */
        RadioController radio;
        radio.open();
        radio.configure(radioConfig);
        radio.start(txSamples);

        SpectrumProcessor spectrumProcessor(
            fftSize,
            sampleRateHz
        );

        std::vector<std::complex<float>> rxSamples(fftSize);

        std::vector<double> averagedMagnitude;
        bool averagingInitialized = false;

        emit statusChanged("Streaming TX and RX");

        /*
         * Prevent the worker from flooding the GUI with hundreds of
         * updates per second. The radio still reads continuously, but
         * the graph updates at approximately 20 frames per second.
         */
        QElapsedTimer displayTimer;
        displayTimer.start();

        QElapsedTimer temperatureTimer;
        temperatureTimer.start();

        while (!stopRequested_.load())
        {
            const std::size_t received =
                radio.receive(rxSamples);

            if (received != fftSize)
            {
                continue;
            }

            SpectrumFrame frame =
                spectrumProcessor.process(rxSamples);

            if (!averagingInitialized)
            {
                averagedMagnitude = frame.magnitudeDbfs;
                averagingInitialized = true;
            }
            else
            {
                for (std::size_t index = 0;
                     index < averagedMagnitude.size();
                     ++index)
                {
                    averagedMagnitude[index] =
                        averagingFactor *
                        averagedMagnitude[index] +
                        (1.0 - averagingFactor) *
                        frame.magnitudeDbfs[index];
                }
            }

            if (displayTimer.elapsed() < 50)
            {
                continue;
            }

            displayTimer.restart();

            QVector<double> frequencyKhz;
            QVector<double> magnitudeDbfs;

            /*
             * Reserve only the visible portion. Also decimate the
             * displayed points by two to reduce GUI rendering load.
             */
            frequencyKhz.reserve(
                static_cast<int>(fftSize / 2)
            );

            magnitudeDbfs.reserve(
                static_cast<int>(fftSize / 2)
            );

            constexpr std::size_t displayDecimation = 2;

            for (std::size_t index = 0;
                 index < frame.frequencyHz.size();
                 index += displayDecimation)
            {
                const double frequencyHz =
                    frame.frequencyHz[index];

                if (frequencyHz < -displayHalfSpanHz ||
                    frequencyHz > displayHalfSpanHz)
                {
                    continue;
                }

                frequencyKhz.append(
                    frequencyHz / 1e3
                );

                magnitudeDbfs.append(
                    averagedMagnitude[index]
                );
            }

            emit spectrumReady(
                std::move(frequencyKhz),
                std::move(magnitudeDbfs),
                frame.peakFrequencyHz / 1e3,
                frame.peakLevelDbfs,
                frame.noiseFloorDbfs
            );

            if (temperatureTimer.elapsed() >= 1000) {
                temperatureTimer.restart();

                const auto temp = radio.readTemperatureCelsius();

                if (temp.has_value()) {
                    emit temperatureChanged(
                        temp.value()
                    );
                }
            }
        }

        emit statusChanged("Stopping...");
        radio.stop();
    }
    catch (const std::exception& exception)
    {
        emit errorOccurred(
            QString::fromStdString(exception.what())
        );
    }

    emit finished();
}