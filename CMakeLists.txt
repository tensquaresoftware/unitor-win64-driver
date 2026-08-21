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

# Packaging (Public Installer) reads this next to the build tree — same SSOT as BridgeVersion*.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/bridge-version.txt" "${PROJECT_VERSION}\n")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single version source for Bridge.exe (--version, console banner, VERSIONINFO, manifest).
set(UNITOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${UNITOR_GENERATED_DIR}/App")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/App/BridgeVersion.h.in"
    "${UNITOR_GENERATED_DIR}/App/BridgeVersion.h"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/App/BridgeVersion.rc.in"
    "${UNITOR_GENERATED_DIR}/App/BridgeVersion.rc"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/App/Bridge.manifest.in"
    "${UNITOR_GENERATED_DIR}/App/Bridge.manifest"
    @ONLY
)

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
    src/App/Mt4PresenceWait.cpp
    src/App/MapperSmoke.cpp
    src/App/FramerSmoke.cpp
    src/App/FramerMtcSmoke.cpp
    src/App/FramerSysexSmoke.cpp
    src/App/MidiSessionCli.cpp
    src/App/MidiSessionMultiHost.cpp
    src/App/MidiSessionMultiHostHotPlug.cpp
    src/App/MidiSessionMultiHostMatch.cpp
    src/App/MidiSessionDiagnostics.cpp
    src/App/WmsBackendSmoke.cpp
    src/Device/DeviceSession.cpp
    src/Device/DeviceSessionDeviceHost.cpp
    src/Device/DeviceSessionHostOutbound.cpp
    src/Device/DeviceSessionMatrixDump.cpp
    src/Device/DeviceSessionBulkInDeliver.cpp
    src/Device/DeviceSessionManager.cpp
    src/Device/UnitIdentityRegistry.cpp
    src/Device/UnitIdentityRegistryIo.cpp
    src/Device/DeviceHostCounters.cpp
    src/Device/DeviceSessionSupport.cpp
    src/Device/HostOutboundQueue.cpp
    src/Midi/VirtualMidiBackend.cpp
    src/Midi/VirtualMidiBackendPorts.cpp
    src/Midi/MidiBackendSelect.cpp
    src/Midi/MidiBackendFactory.cpp
    src/Midi/WmsMidiBackend.cpp
    src/Midi/Midi1StreamAssembler.cpp
    src/Midi/SoftEchoGate.cpp
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
        "${UNITOR_GENERATED_DIR}"
)

if(WIN32)
    target_sources(Bridge PRIVATE
        src/Midi/VirtualMidiWinSupport.cpp
        src/Usb/WinUsbOpenDetail.cpp
        src/Usb/WinUsbOpenSupport.cpp
        src/Usb/WinUsbOpenIdentity.cpp
        src/Usb/WinUsbEnumerate.cpp
        "${UNITOR_GENERATED_DIR}/App/BridgeVersion.rc"
        "${UNITOR_GENERATED_DIR}/App/Bridge.manifest"
    )
    target_link_libraries(Bridge
        PRIVATE
            setupapi
            winusb
            ole32
            taskschd
            comsuppw
    )
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchMicrosoftMidiAppSdk.cmake")
    unitor_setup_microsoft_midi_app_sdk(Bridge)
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
    tests/unit/Midi1StreamAssemblerTests.cpp
    tests/unit/TeVirtualMidiLimitsTests.cpp
    tests/unit/AutoStartContractTests.cpp
    tests/unit/HotPlugContractTests.cpp
    tests/unit/MultiClientContractTests.cpp
    tests/unit/UnitIdentityRegistryTests.cpp
    tests/unit/MidiPathStatsTests.cpp
    tests/unit/SoftEchoGateTests.cpp
    tests/unit/MidiBackendSelectTests.cpp
    tools/midi-path-harness/MidiPathStats.cpp
    src/Midi/SoftEchoGate.cpp
    src/Midi/MidiBackendSelect.cpp
    src/Midi/Midi1StreamAssembler.cpp
    src/App/AutoStartRegistration.cpp
    src/App/AutoStartTaskScheduler.cpp
    src/App/AutoStartRunKey.cpp
    src/App/AutoStartWinUtil.cpp
    src/Device/HostOutboundQueue.cpp
    src/Device/DeviceSessionSupport.cpp
    src/Device/DeviceSessionManager.cpp
    src/Device/UnitIdentityRegistry.cpp
    src/Device/UnitIdentityRegistryIo.cpp
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
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/midi-path-harness"
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

# AD-11 MIDI Path harness (Windows WinMM + QPC). Separate from BridgeTests.
if(WIN32)
    add_subdirectory(tools/midi-path-harness)
endif()
