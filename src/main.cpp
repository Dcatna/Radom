#include <iostream>

#include "radar/radar.h"

int main(int argc, char* argv[])
{

    if (argc < 2) {
        std::cerr << "Usage: ./Radom <config-file>\n";
        return 1;
    }

    std::cout << "Radom starting...\n";

    const std::string configPath = argv[1];

    Radar radar;

    radar.initialize(configPath);

    radar.printHello();
    // radar.loadConfig(configPath);

    radar.run_simulation();

    return 0;
}