cmake_minimum_required(VERSION 3.16)
project(flowgraph CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Library headers
add_library(flowgraph INTERFACE)
target_include_directories(flowgraph INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(flowgraph INTERFACE nlohmann_json::nlohmann_json)

# Examples
add_executable(basic_usage examples/basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE flowgraph)

# Enable testing
enable_testing()
add_subdirectory(tests)
