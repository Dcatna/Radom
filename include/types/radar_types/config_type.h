#include <string>
#include <cstdint>

struct RadarConfig {
    std::string radar_mode = "None";
    uint32_t sample_rate = 1;
    uint32_t center_frequency = 1;
    uint32_t mode_bandwidth = 1;
    uint32_t duration = 1;
};