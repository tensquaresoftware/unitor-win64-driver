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
)

target_include_directories(Bridge
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
