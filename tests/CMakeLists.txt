cmake_minimum_required(VERSION 3.14)

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# Download and configure Google Benchmark
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.7.1
)

# Make both available
FetchContent_MakeAvailable(googletest benchmark)

# Enable testing
enable_testing()

# Create test executable
add_executable(flowgraph_tests
    main.cpp
    precision_management_test.cpp
    error_propagation_test.cpp
)

# Create benchmark executable
add_executable(flowgraph_benchmarks
    fractal_tree_benchmark.cpp
)

# Link test executable
target_link_libraries(flowgraph_tests
    PRIVATE
    gtest
    gtest_main
    flowgraph
)

# Link benchmark executable
target_link_libraries(flowgraph_benchmarks
    PRIVATE
    benchmark
    benchmark_main
    flowgraph
)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(flowgraph_tests)

# Set include directories for both executables
target_include_directories(flowgraph_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(flowgraph_benchmarks
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Set C++ standard for both executables
set_target_properties(flowgraph_tests flowgraph_benchmarks
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
