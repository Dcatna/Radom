#include <string>

#include "../config/config.h"

class Radar {

    private:

    //----- Config Variables
    Config configLoader = Config();
    RadarConfig config;
    // uint32_t sample_rate;
    // uint32_t center_frequency;
    // uint32_t mode_bandwidth;
    // uint32_t duration;
    // std::string radar_mode;

    void loadConfig(const std::string filename);

    public:
    
        void printHello();

        void initialize(const std::string filename);


        void run_simulation();
};