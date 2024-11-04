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

add_executable(matrix_operations examples/matrix_operations.cpp)
target_link_libraries(matrix_operations PRIVATE flowgraph)

add_executable(image_pipeline examples/image_pipeline.cpp)
target_link_libraries(image_pipeline PRIVATE flowgraph)

add_executable(graph_optimization examples/graph_optimization.cpp)
target_link_libraries(graph_optimization PRIVATE flowgraph)

add_executable(neural_network examples/neural_network.cpp)
target_link_libraries(neural_network PRIVATE flowgraph)

add_executable(signal_processing examples/signal_processing.cpp)
target_link_libraries(signal_processing PRIVATE flowgraph)

# Enable testing
enable_testing()
add_subdirectory(tests)
