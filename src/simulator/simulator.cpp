#include <cmath>
#include <array>
#include <complex>
#include <numbers>
#include <cstdint>

#include "simulator.h"


// void Simulator::addTarget(Target target) {
    
// }

std::array<std::complex<float>, 1024> Simulator::generateFrame(uint32_t sample_rate, uint32_t frequency) {
    //phase increment = sample_rate / frequency
    float pi_f = std::numbers::pi_v<float>;
    const float phase_increment = 2.0f * std::numbers::pi_v<float> * frequency / sample_rate;
    float current_phase = 0.0f;
    // to generate IQ samples you generate the sin(phase) and the cos(phase)
    // we will generate 1024 IQ samples
    std::array<std::complex<float>, 1024> iq_buffer;
    uint32_t sample = 0;
    while (sample < 1024) {
        // std::cout << current_phase << std::endl;
        float cos_wave = cos(current_phase);
        float sin_wave = sin(current_phase);

        std::complex<float> iq_sample = std::complex(cos_wave, sin_wave);
        iq_buffer[sample] = iq_sample;

        current_phase += phase_increment;
        if (current_phase >= 2.0f * pi_f) { // wrap phase so it doesnt grow forever
            current_phase -= 2.0f * pi_f;
        }
        // std::cout << sample << std::endl;
        // std::cout << "I: " << iq_buffer[sample].real() << std::endl;
        // std::cout << "Q: " << iq_buffer[sample].imag() << std::endl;
        sample++;
    }
    // Sampling must be done at 2 times the bandwidth of the signal
    return iq_buffer;
}