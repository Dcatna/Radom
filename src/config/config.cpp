#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"

std::string Config::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

RadarConfig Config::readConfig(const std::string filename) {
    RadarConfig radarConfig;
    std::ifstream file(filename);

    if (!file.is_open()){
        std::cerr << "Error: Could no open file " << filename << std::endl;
        return radarConfig;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, delimiterPos));
        std::string value = trim(line.substr(delimiterPos + 1));
        if (key == "sample_rate") {
            radarConfig.sample_rate = std::stoul(value);
        }
        else if (key == "center_frequency") {
            radarConfig.center_frequency = std::stoul(value);
        }
        else if (key == "mode_bandwidth") {
            radarConfig.mode_bandwidth = std::stoul(value);
        }
        else if (key == "duration") {
            radarConfig.duration = std::stoul(value);
        }
        else if (key == "radar_mode") {
            radarConfig.radar_mode = value;
        }
        
    }

    return radarConfig;
}

