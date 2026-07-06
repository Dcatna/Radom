cmake_minimum_required(VERSION 3.25)

project(Radom)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Radom
    src/config/config.cpp
    src/main.cpp
    src/radar/radar.cpp
    src/simulator/simulator.cpp
)

target_include_directories(Radom PRIVATE include)