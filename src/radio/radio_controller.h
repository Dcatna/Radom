#pragma once

#include <SoapySDR/Device.hpp>

#include <atomic>
#include <complex>
#include <cstddef>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <optional>

struct RadioConfig
{
    double sampleRateHz{5e6};
    double centerFrequencyHz{915e6};
    double bandwidthHz{5e6};

    double txGainDb{10.0};
    double rxGainDb{30.0};

    std::string txAntenna{"Band2"};
    std::string rxAntenna{"LNAW"};

    std::size_t channel{0};
};

class RadioController
{
public:
    RadioController();
    ~RadioController();

    RadioController(const RadioController&) = delete;
    RadioController& operator=(const RadioController&) = delete;

    void open();
    void configure(const RadioConfig& config);

    void start(
        const std::vector<std::complex<float>>& txSamples
    );

    std::size_t receive(
        std::vector<std::complex<float>>& output
    );

    void stop();

    bool isRunning() const;

    std::optional<double> readTemperatureCelsius() const;

private:
    void transmitLoop();

    SoapySDR::Device* device_{nullptr};
    SoapySDR::Stream* rxStream_{nullptr};
    SoapySDR::Stream* txStream_{nullptr};

    RadioConfig config_;

    std::vector<std::complex<float>> txSamples_;

    std::thread txThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> txFailed_{false};

    std::string txError_;
    
};