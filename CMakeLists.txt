cmake_minimum_required(VERSION 3.20)

project(unitor-win64-driver
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Out-of-source builds only. Configure into builds/, never repo-root build/.
# Example: cmake -S . -B builds/debug && cmake --build builds/debug

add_executable(Bridge
    src/App/Main.cpp
    src/App/MapperSmoke.cpp
    src/App/FramerSmoke.cpp
    src/App/MidiSessionCli.cpp
    src/Device/DeviceSession.cpp
    src/Device/DeviceSessionDeviceHost.cpp
    src/Device/DeviceSessionManager.cpp
    src/Device/DeviceHostCounters.cpp
    src/Device/DeviceSessionSupport.cpp
    src/Midi/VirtualMidiBackend.cpp
    src/Profile/DeviceProfile.cpp
    src/Protocol/EmagicCableMapper.cpp
    src/Protocol/EmagicMapperSmokeSupport.cpp
    src/Protocol/EmagicMapperRealtimeSmoke.cpp
    src/Protocol/MidiMessageFramer.cpp
    src/Usb/WinUsbTransport.cpp
    src/Usb/WinUsbTransportInit.cpp
    src/Usb/WinUsbBulkProbe.cpp
)

target_include_directories(Bridge
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(WIN32)
    target_sources(Bridge PRIVATE
        src/Midi/VirtualMidiWinSupport.cpp
        src/Usb/WinUsbOpenDetail.cpp
        src/Usb/WinUsbOpenSupport.cpp
    )
    target_link_libraries(Bridge
        PRIVATE
            setupapi
            winusb
            ole32
    )
endif()

# --- BridgeTests: Profile + Protocol only (no WinUSB / VirtualMIDI) ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        f7cfc885ba68c317d20cbbfa6b8aa8ecbcbde19f # v3.8.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(BridgeTests
    tests/DeviceProfileTests.cpp
    tests/EmagicCableMapperTests.cpp
    tests/MidiMessageFramerTests.cpp
    src/Profile/DeviceProfile.cpp
    src/Protocol/EmagicCableMapper.cpp
    src/Protocol/EmagicMapperSmokeSupport.cpp
    src/Protocol/EmagicMapperRealtimeSmoke.cpp
    src/Protocol/MidiMessageFramer.cpp
)

target_include_directories(BridgeTests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(BridgeTests
    PRIVATE
        Catch2::Catch2WithMain
)

enable_testing()
# TARGET_FILE resolves per-config output dirs (e.g. builds/ci/Release on VS).
add_test(NAME BridgeTests COMMAND $<TARGET_FILE:BridgeTests>)
