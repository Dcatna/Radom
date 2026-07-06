#include <string>

#include "../../include/types/radar_types/config_type.h"



class Config {

    private:

        std::string trim(const std::string& str);

    public:

        RadarConfig readConfig(const std::string filename);

};