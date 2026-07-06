#include <iostream>
#include <vector>
#include <cstdint>
#include <complex>
#include <array>

#include "../../include/types/radar_types/iq_type.h"
#include "../../include/types/radar_types/target.h"

class Simulator {

    private:
        Target current_target;
        float phase;
    public:

        void addTarget(Target target);

        std::array<std::complex<float>, 1024> generateFrame(uint32_t sample_rate, uint32_t frequency);

};