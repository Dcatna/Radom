cmake_minimum_required(VERSION 3.16)

project(Radom LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)

find_package(
    Qt5
    REQUIRED
    COMPONENTS
        Core
        Widgets
        Charts
)

pkg_check_modules(
    SOAPYSDR
    REQUIRED
    IMPORTED_TARGET
    SoapySDR
)

pkg_check_modules(
    FFTW3F
    REQUIRED
    IMPORTED_TARGET
    fftw3f
)

add_executable(
    Radom

    src/main.cpp

    src/config/config.cpp
    src/radar/radar.cpp
    src/simulator/simulator.cpp

    src/radio/radio_controller.cpp

    src/waveform/tone_generator.cpp

    src/dsp/spectrum_processor.cpp

    src/gui/main_window.cpp
    src/gui/main_window.h
    src/gui/spectrum_worker.cpp
    src/gui/spectrum_worker.h
    src/gui/waterfall_widget.cpp
    src/gui/waterfall_widget.h
)

target_include_directories(
    Radom
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    Radom
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Charts
        PkgConfig::SOAPYSDR
        PkgConfig::FFTW3F
)

target_compile_options(
    Radom
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)