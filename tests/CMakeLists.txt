# Add Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Reset compiler flags for Google Test to avoid propagating our specific flags
get_target_property(gtest_cxx_flags gtest CXX_FLAGS)
if(gtest_cxx_flags)
    string(REPLACE "-fconcepts" "" gtest_cxx_flags "${gtest_cxx_flags}")
    string(REPLACE "-fcoroutines" "" gtest_cxx_flags "${gtest_cxx_flags}")
    set_target_properties(gtest PROPERTIES CXX_FLAGS "${gtest_cxx_flags}")
endif()

# Main test executable
add_executable(flowgraph_tests
    main.cpp
    error_propagation_test.cpp
    precision_management_test.cpp
)

target_link_libraries(flowgraph_tests
    PRIVATE
    flowgraph_core
    GTest::gtest
    GTest::gtest_main
)

# Only build benchmarks if explicitly requested
option(BUILD_BENCHMARKS "Build benchmark tests" OFF)

if(BUILD_BENCHMARKS)
    # Add Google Benchmark
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF)
    FetchContent_MakeAvailable(benchmark)

    # Benchmark executable
    add_executable(flowgraph_benchmarks
        fractal_tree_benchmark.cpp
    )

    target_link_libraries(flowgraph_benchmarks
        PRIVATE
        flowgraph_core
        benchmark::benchmark
        benchmark::benchmark_main
    )
endif()

# Register tests
include(GoogleTest)
gtest_discover_tests(flowgraph_tests)
