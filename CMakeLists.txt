cmake_minimum_required(VERSION 3.16)
project(flowgraph CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library headers
add_library(flowgraph INTERFACE)
target_include_directories(flowgraph INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Examples
add_executable(basic_usage examples/basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE flowgraph)

# Enable testing
enable_testing()
add_subdirectory(tests)
