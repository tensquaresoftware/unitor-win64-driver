cmake_minimum_required(VERSION 3.20)

# MIDI Path harness (AD-11) — WinMM client against Bridge Virtual Ports.
# Windows-only: QueryPerformanceCounter + WinMM.

add_executable(MidiPathHarness
    Main.cpp
    QpcClock.cpp
    WinMmMidiIo.cpp
    MidiPathRunner.cpp
    MidiPathStats.cpp
    PortNameNormalize.cpp
)

target_compile_features(MidiPathHarness PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(MidiPathHarness PRIVATE /W4 /permissive-)
endif()

if(WIN32)
    target_link_libraries(MidiPathHarness PRIVATE winmm)
else()
    message(FATAL_ERROR "MidiPathHarness requires Windows (WinMM + QPC).")
endif()
