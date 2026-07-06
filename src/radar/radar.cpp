#include <iostream>
#include <string>
//#include <vector>
#include <complex>
#include <array>

#include "radar.h"
#include "../simulator/simulator.h"

void Radar::printHello() {
    std::cout << "HELLLOOOO" << std::endl;
}

void Radar::loadConfig(const std::string filename){
    RadarConfig radarConfig = configLoader.readConfig(filename);

    config = radarConfig;

    //return radarConfig;
}

void Radar::initialize(const std::string filename) {
    loadConfig(filename);

}

void Radar::run_simulation() {
    Simulator simulator = Simulator();

    std::cout << "Sample Rate: " << config.sample_rate << std::endl;
    std::cout << "Center Frequency: " << config.center_frequency << std::endl;
    std::cout << "Mode Bandwidth: " << config.mode_bandwidth << std::endl;
    std::cout << "Duration: " << config.duration << std::endl;
    std::cout << "Radar Mode: " << config.radar_mode << std::endl;

    std::array<std::complex<float>, 1024> frame = simulator.generateFrame(config.sample_rate, config.center_frequency); 

}