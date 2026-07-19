#pragma once

struct RadioSettings
{
    double sampleRateHz{5e6};
    double centerFrequencyHz{915e6};
    double toneFrequencyHz{400e3};

    double txGainDb{10.0};
    double rxGainDb{30.0};

    float txAmplitude{0.10F};
};