# Fetch Microsoft Windows MIDI Services App SDK + CppWinRT for Bridge (Win32 only).
# Downloads NuGet packages at configure time and generates C++/WinRT headers.

function(unitor_setup_microsoft_midi_app_sdk target_name)
    if(NOT WIN32)
        message(FATAL_ERROR "unitor_setup_microsoft_midi_app_sdk requires Windows")
    endif()

    set(_midi_sdk_version "1.0.17-rc.4.25")
    set(_cppwinrt_version "2.0.250303.1")
    set(_deps_root "${CMAKE_BINARY_DIR}/_deps/microsoft-midi-app-sdk")
    set(_midi_nupkg "${_deps_root}/Microsoft.Windows.Devices.Midi2.${_midi_sdk_version}.nupkg")
    set(_midi_extract "${_deps_root}/midi2-sdk")
    set(_cppwinrt_nupkg "${_deps_root}/Microsoft.Windows.CppWinRT.${_cppwinrt_version}.nupkg")
    set(_cppwinrt_extract "${_deps_root}/cppwinrt")
    set(_generated "${_deps_root}/generated")
    set(_midi_winmd "${_midi_extract}/ref/native/Microsoft.Windows.Devices.Midi2.winmd")
    set(_cppwinrt_exe "${_cppwinrt_extract}/bin/cppwinrt.exe")
    set(_marker "${_generated}/.unitor-midi-projection-ready")

    file(MAKE_DIRECTORY "${_deps_root}")

    if(NOT EXISTS "${_midi_nupkg}")
        message(STATUS "Downloading Microsoft.Windows.Devices.Midi2 ${_midi_sdk_version}")
        file(DOWNLOAD
            "https://github.com/microsoft/MIDI/releases/download/rc-4/Microsoft.Windows.Devices.Midi2.${_midi_sdk_version}.nupkg"
            "${_midi_nupkg}"
            SHOW_PROGRESS
            STATUS _midi_dl_status
        )
        list(GET _midi_dl_status 0 _midi_dl_code)
        if(NOT _midi_dl_code EQUAL 0)
            message(FATAL_ERROR
                "Failed to download Microsoft.Windows.Devices.Midi2 ${_midi_sdk_version}. "
                "Bridge WMS backend requires this NuGet (Story 6.1). Status: ${_midi_dl_status}")
        endif()
    endif()

    if(NOT EXISTS "${_midi_winmd}")
        file(REMOVE_RECURSE "${_midi_extract}")
        file(MAKE_DIRECTORY "${_midi_extract}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xf "${_midi_nupkg}"
            WORKING_DIRECTORY "${_midi_extract}"
            RESULT_VARIABLE _midi_extract_rc
        )
        if(NOT _midi_extract_rc EQUAL 0 OR NOT EXISTS "${_midi_winmd}")
            message(FATAL_ERROR
                "Failed to extract Microsoft.Windows.Devices.Midi2 nupkg "
                "(expected ${_midi_winmd})")
        endif()
    endif()

    if(NOT EXISTS "${_cppwinrt_nupkg}")
        message(STATUS "Downloading Microsoft.Windows.CppWinRT ${_cppwinrt_version}")
        file(DOWNLOAD
            "https://api.nuget.org/v3-flatcontainer/microsoft.windows.cppwinrt/${_cppwinrt_version}/microsoft.windows.cppwinrt.${_cppwinrt_version}.nupkg"
            "${_cppwinrt_nupkg}"
            SHOW_PROGRESS
            STATUS _cppwinrt_dl_status
        )
        list(GET _cppwinrt_dl_status 0 _cppwinrt_dl_code)
        if(NOT _cppwinrt_dl_code EQUAL 0)
            message(FATAL_ERROR
                "Failed to download Microsoft.Windows.CppWinRT ${_cppwinrt_version}. "
                "Required to project Windows MIDI Services winmd headers. Status: ${_cppwinrt_dl_status}")
        endif()
    endif()

    if(NOT EXISTS "${_cppwinrt_exe}")
        file(REMOVE_RECURSE "${_cppwinrt_extract}")
        file(MAKE_DIRECTORY "${_cppwinrt_extract}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E tar xf "${_cppwinrt_nupkg}"
            WORKING_DIRECTORY "${_cppwinrt_extract}"
            RESULT_VARIABLE _cppwinrt_extract_rc
        )
        if(NOT _cppwinrt_extract_rc EQUAL 0 OR NOT EXISTS "${_cppwinrt_exe}")
            message(FATAL_ERROR
                "Failed to extract Microsoft.Windows.CppWinRT nupkg "
                "(expected ${_cppwinrt_exe})")
        endif()
    endif()

    if(EXISTS "${_marker}")
        file(READ "${_marker}" _marker_contents)
        string(STRIP "${_marker_contents}" _marker_contents)
        if(NOT _marker_contents STREQUAL "${_midi_sdk_version}")
            file(REMOVE "${_marker}")
            file(REMOVE_RECURSE "${_generated}")
        endif()
    endif()

    if(NOT EXISTS "${_marker}")
        file(REMOVE_RECURSE "${_generated}")
        file(MAKE_DIRECTORY "${_generated}")
        execute_process(
            COMMAND "${_cppwinrt_exe}" -input "${_midi_winmd}" -input sdk -output "${_generated}"
            RESULT_VARIABLE _proj_rc
            OUTPUT_VARIABLE _proj_out
            ERROR_VARIABLE _proj_err
        )
        if(NOT _proj_rc EQUAL 0)
            message(FATAL_ERROR
                "cppwinrt projection failed for Microsoft.Windows.Devices.Midi2.\n"
                "Ensure a Windows 10/11 SDK is installed for the MSVC toolchain.\n"
                "stdout: ${_proj_out}\nstderr: ${_proj_err}")
        endif()
        file(WRITE "${_marker}" "${_midi_sdk_version}\n")
    endif()

    target_sources(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/src/Midi/WmsMidiBackendPorts.cpp"
        "${CMAKE_SOURCE_DIR}/src/Midi/WmsMidiBackendHost.cpp"
        "${CMAKE_SOURCE_DIR}/src/Midi/WmsMidiWinSupport.cpp"
    )
    target_include_directories(${target_name} PRIVATE
        "${_generated}"
        "${_midi_extract}/build/native/include"
    )
    target_compile_definitions(${target_name} PRIVATE
        UNITOR_HAS_WMS_MIDI_BACKEND=1
    )
    target_link_libraries(${target_name} PRIVATE
        runtimeobject
        windowsapp
    )
endfunction()
