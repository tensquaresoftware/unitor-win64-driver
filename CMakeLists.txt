cmake_minimum_required(VERSION 3.20)

# Reject the CMake Tools / IDE default binary dir (${source}/build).
# Official out-of-source trees are under builds/ (see CMakePresets.json).
get_filename_component(_unitor_binary_dir_name "${CMAKE_BINARY_DIR}" NAME)
if(_unitor_binary_dir_name STREQUAL "build")
    message(FATAL_ERROR
        "Refuse repo-root build/. Configure into builds/<name> "
        "(e.g. cmake --preset debug, or cmake -S . -B builds/debug). "
        "Cursor/VS Code: use CMake preset 'debug' (.vscode/settings.json).")
endif()

project(unitor-win64-driver
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Out-of-source builds only. Configure into builds/ (e.g. builds/debug, builds/ci).
# Example: cmake --preset debug && cmake --build --preset debug
# Or: cmake -S . -B builds/debug && cmake --build builds/debug

add_executable(Bridge
    src/App/Main.cpp
    src/App/AutoStartRegistration.cpp
    src/App/AutoStartTaskScheduler.cpp
    src/App/AutoStartRunKey.cpp
    src/App/AutoStartWinUtil.cpp
    src/App/Mt4WinUsbPresence.cpp
    src/App/MapperSmoke.cpp
    src/App/FramerSmoke.cpp
    src/App/FramerMtcSmoke.cpp
    src/App/FramerSysexSmoke.cpp
    src/App/MidiSessionCli.cpp
    src/Device/DeviceSession.cpp
    src/Device/DeviceSessionDeviceHost.cpp
    src/Device/DeviceSessionHostOutbound.cpp
    src/Device/DeviceSessionMatrixDump.cpp
    src/Device/DeviceSessionBulkInDeliver.cpp
    src/Device/DeviceSessionManager.cpp
    src/Device/DeviceHostCounters.cpp
    src/Device/DeviceSessionSupport.cpp
    src/Device/HostOutboundQueue.cpp
    src/Midi/VirtualMidiBackend.cpp
    src/Profile/DeviceProfile.cpp
    src/Protocol/EmagicCableMapper.cpp
    src/Protocol/EmagicMapperSmokeSupport.cpp
    src/Protocol/EmagicMapperRealtimeSmoke.cpp
    src/Protocol/EmagicMapperMtcSmoke.cpp
    src/Protocol/EmagicMapperSysexSmoke.cpp
    src/Protocol/MidiMessageFramer.cpp
    src/Usb/WinUsbTransport.cpp
    src/Usb/WinUsbTransportInit.cpp
    src/Usb/WinUsbEmagicHostMidi.cpp
    src/Usb/WinUsbBulkInAsync.cpp
    src/Usb/WinUsbBulkInCompletion.cpp
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
        src/App/Bridge.manifest
    )
    target_link_libraries(Bridge
        PRIVATE
            setupapi
            winusb
            ole32
            taskschd
            comsuppw
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
    tests/unit/DeviceProfileTests.cpp
    tests/unit/DeviceInquiryHelpersTests.cpp
    tests/unit/EmagicCableMapperTests.cpp
    tests/unit/MidiMessageFramerTests.cpp
    tests/unit/MidiMessageFramerSysexTests.cpp
    tests/unit/HostOutboundQueueTests.cpp
    tests/unit/TeVirtualMidiLimitsTests.cpp
    tests/unit/AutoStartContractTests.cpp
    src/App/AutoStartRegistration.cpp
    src/App/AutoStartTaskScheduler.cpp
    src/App/AutoStartRunKey.cpp
    src/App/AutoStartWinUtil.cpp
    src/Device/HostOutboundQueue.cpp
    src/Device/DeviceSessionSupport.cpp
    src/Profile/DeviceProfile.cpp
    src/Protocol/EmagicCableMapper.cpp
    src/Protocol/EmagicMapperSmokeSupport.cpp
    src/Protocol/EmagicMapperRealtimeSmoke.cpp
    src/Protocol/EmagicMapperMtcSmoke.cpp
    src/Protocol/EmagicMapperSysexSmoke.cpp
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

if(WIN32)
    target_link_libraries(BridgeTests
        PRIVATE
            ole32
            taskschd
            comsuppw
    )
endif()

enable_testing()
# TARGET_FILE resolves per-config output dirs (e.g. builds/ci/Release on VS).
add_test(NAME BridgeTests COMMAND $<TARGET_FILE:BridgeTests>)
