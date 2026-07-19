#include "radio_controller.h"

#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Formats.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <algorithm>
#include <optional>

RadioController::RadioController() = default;

RadioController::~RadioController()
{
    stop();

    /*
     * Your current LimeSuiteNG build has previously segfaulted during
     * stream teardown. For now, do not call closeStream() or unmake()
     * here until we verify that cleanup works safely in C++.
     */
}

void RadioController::open()
{
    if (device_ != nullptr)
    {
        return;
    }

    SoapySDR::Kwargs arguments;
    arguments["driver"] = "limesuiteng";

    device_ = SoapySDR::Device::make(arguments);

    if (device_ == nullptr)
    {
        throw std::runtime_error(
            "Unable to open LimeSDR with driver=limesuiteng."
        );
    }

    std::cout << "Opened LimeSDR successfully.\n";
}

void RadioController::configure(const RadioConfig& config)
{

    if (device_ == nullptr)
    {
        throw std::runtime_error(
            "Radio must be opened before configuration."
        );
    }

    if (running_)
    {
        throw std::runtime_error(
            "Cannot configure the radio while streaming."
        );
    }

    if (rxStream_ != nullptr || txStream_ != nullptr)
    {
        throw std::runtime_error(
            "Radio streams are already configured."
        );
    }

    config_ = config;

    device_->setSampleRate(
        SOAPY_SDR_RX,
        config_.channel,
        config_.sampleRateHz
    );

    device_->setFrequency(
        SOAPY_SDR_RX,
        config_.channel,
        config_.centerFrequencyHz
    );

    device_->setBandwidth(
        SOAPY_SDR_RX,
        config_.channel,
        config_.bandwidthHz
    );

    device_->setAntenna(
        SOAPY_SDR_RX,
        config_.channel,
        config_.rxAntenna
    );

    device_->setGain(
        SOAPY_SDR_RX,
        config_.channel,
        config_.rxGainDb
    );

    device_->setSampleRate(
        SOAPY_SDR_TX,
        config_.channel,
        config_.sampleRateHz
    );

    device_->setFrequency(
        SOAPY_SDR_TX,
        config_.channel,
        config_.centerFrequencyHz
    );

    device_->setBandwidth(
        SOAPY_SDR_TX,
        config_.channel,
        config_.bandwidthHz
    );

    device_->setAntenna(
        SOAPY_SDR_TX,
        config_.channel,
        config_.txAntenna
    );

    device_->setGain(
        SOAPY_SDR_TX,
        config_.channel,
        config_.txGainDb
    );

    rxStream_ = device_->setupStream(
        SOAPY_SDR_RX,
        SOAPY_SDR_CF32,
        {config_.channel}
    );

    txStream_ = device_->setupStream(
        SOAPY_SDR_TX,
        SOAPY_SDR_CF32,
        {config_.channel}
    );

    if (rxStream_ == nullptr || txStream_ == nullptr)
    {
        throw std::runtime_error(
            "Unable to create LimeSDR RX/TX streams."
        );
    }

    std::cout
        << "RX rate: "
        << device_->getSampleRate(
            SOAPY_SDR_RX,
            config_.channel
        ) / 1e6
        << " MS/s\n";

    std::cout
        << "TX rate: "
        << device_->getSampleRate(
            SOAPY_SDR_TX,
            config_.channel
        ) / 1e6
        << " MS/s\n";

    std::cout
        << "Center frequency: "
        << config_.centerFrequencyHz / 1e6
        << " MHz\n";
}

void RadioController::start(
    const std::vector<std::complex<float>>& txSamples
)
{
    if (device_ == nullptr ||
        rxStream_ == nullptr ||
        txStream_ == nullptr)
    {
        throw std::runtime_error(
            "Radio must be opened and configured before start()."
        );
    }

    if (txSamples.empty())
    {
        throw std::invalid_argument(
            "TX sample buffer cannot be empty."
        );
    }

    if (running_)
    {
        return;
    }

    txSamples_ = txSamples;
    txFailed_ = false;
    txError_.clear();

    const int rxResult = device_->activateStream(rxStream_);

    if (rxResult < 0)
    {
        throw std::runtime_error(
            std::string("RX activateStream failed: ") +
            SoapySDR::errToStr(rxResult)
        );
    }

    const int txResult = device_->activateStream(txStream_);

    if (txResult < 0)
    {
        throw std::runtime_error(
            std::string("TX activateStream failed: ") +
            SoapySDR::errToStr(txResult)
        );
    }

    running_ = true;

    txThread_ = std::thread(
        &RadioController::transmitLoop,
        this
    );

    std::this_thread::sleep_for(
        std::chrono::milliseconds(500)
    );
}

std::size_t RadioController::receive(
    std::vector<std::complex<float>>& output
)
{
    if (!running_)
    {
        throw std::runtime_error(
            "Radio is not currently streaming."
        );
    }

    if (output.empty())
    {
        throw std::invalid_argument(
            "RX output buffer cannot be empty."
        );
    }

    std::size_t received = 0;

    while (received < output.size() && running_)
    {
        void* buffers[] = {
            output.data() + received
        };

        int flags = 0;
        long long timeNs = 0;

        const int result = device_->readStream(
            rxStream_,
            buffers,
            output.size() - received,
            flags,
            timeNs,
            500000
        );

        if (result == SOAPY_SDR_TIMEOUT)
        {
            continue;
        }

        if (result < 0)
        {
            throw std::runtime_error(
                std::string("RX readStream failed: ") +
                SoapySDR::errToStr(result)
            );
        }

        received += static_cast<std::size_t>(result);
    }

    if (txFailed_)
    {
        throw std::runtime_error(
            "TX worker failed: " + txError_
        );
    }

    return received;
}

void RadioController::transmitLoop()
{
    while (running_)
    {
        const void* buffers[] = {
            txSamples_.data()
        };

        int flags = 0;
        long long timeNs = 0;

        const int result = device_->writeStream(
            txStream_,
            buffers,
            txSamples_.size(),
            flags,
            timeNs,
            100000
        );

        if (result == SOAPY_SDR_TIMEOUT)
        {
            continue;
        }

        if (result < 0)
        {
            txError_ = SoapySDR::errToStr(result);
            txFailed_ = true;
            running_ = false;
            return;
        }
    }
}

void RadioController::stop()
{
    const bool wasRunning =
        running_.exchange(false);

    if (txThread_.joinable())
    {
        txThread_.join();
    }

    if (!wasRunning || device_ == nullptr)
    {
        return;
    }

    SoapySDR::Stream* sharedStream = nullptr;

    if (rxStream_ != nullptr)
    {
        sharedStream = rxStream_;
    }
    else if (txStream_ != nullptr)
    {
        sharedStream = txStream_;
    }

    if (sharedStream != nullptr)
    {
        std::cerr
            << "Deactivating shared RX/TX stream...\n";

        const int deactivateResult =
            device_->deactivateStream(sharedStream);

        if (deactivateResult < 0)
        {
            std::cerr
                << "deactivateStream warning: "
                << SoapySDR::errToStr(
                       deactivateResult
                   )
                << '\n';
        }

        std::cerr
            << "Closing shared RX/TX stream...\n";

        device_->closeStream(sharedStream);
    }

    rxStream_ = nullptr;
    txStream_ = nullptr;

    txSamples_.clear();
    txFailed_.store(false);
    txError_.clear();

    std::cerr
        << "Shared stream stopped successfully.\n";
}

bool RadioController::isRunning() const
{
    return running_;
}

std::optional<double> RadioController::readTemperatureCelsius() const {
    if (device_ == nullptr) {
        return std::nullopt;
    }
    
    auto sensors = device_->listSensors();

    const auto sensorIt = std::find(
        sensors.begin(),
        sensors.end(),
        "lms7_temp"
    );

    if (sensorIt == sensors.end()) {
        return std::nullopt;
    }

    try {
        const std::string val = device_->readSensor("lms7_temp");
        return std::stod(val);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}