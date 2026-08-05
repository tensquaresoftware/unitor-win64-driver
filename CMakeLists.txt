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
    src/Device/DeviceSession.cpp
    src/Profile/DeviceProfile.cpp
    src/Protocol/EmagicCableMapper.cpp
    src/Usb/WinUsbTransport.cpp
)

target_include_directories(Bridge
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(WIN32)
    target_sources(Bridge PRIVATE
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
