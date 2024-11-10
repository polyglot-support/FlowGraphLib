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
    GIT_TAG v1.8.3
)

# Make dependencies available
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(googletest benchmark)

# Add test executable
add_executable(flowgraph_tests
    main.cpp
    error_propagation_test.cpp
    precision_management_test.cpp
)

# Add benchmark executable
add_executable(flowgraph_benchmarks
    fractal_tree_benchmark.cpp
)

# Link test executable against Google Test and our library
target_link_libraries(flowgraph_tests
    PRIVATE
    gtest
    gtest_main
    flowgraph
)

# Link benchmark executable against Google Benchmark and our library
target_link_libraries(flowgraph_benchmarks
    PRIVATE
    benchmark
    benchmark_main
    flowgraph
)

# Inherit C++20 settings from parent
target_compile_features(flowgraph_tests PRIVATE cxx_std_20)
target_compile_features(flowgraph_benchmarks PRIVATE cxx_std_20)

# Add compiler-specific flags
if(MSVC)
    target_compile_options(flowgraph_tests PRIVATE 
        /std:c++latest 
        /Zc:__cplusplus 
        /EHsc 
        /permissive- 
        /await:strict
        /W4  # Removed /WX to allow warnings during development
    )
    target_compile_options(flowgraph_benchmarks PRIVATE 
        /std:c++latest 
        /Zc:__cplusplus 
        /EHsc 
        /permissive- 
        /await:strict
        /W4  # Removed /WX to allow warnings during development
    )
    target_compile_definitions(flowgraph_tests PRIVATE 
        _HAS_CXX20=1
    )
    target_compile_definitions(flowgraph_benchmarks PRIVATE 
        _HAS_CXX20=1
    )
else()
    target_compile_options(flowgraph_tests PRIVATE 
        -std=c++20 
        -fconcepts
        -fcoroutines
        -Wall 
        -Wextra 
        -Wpedantic
    )
    target_compile_options(flowgraph_benchmarks PRIVATE 
        -std=c++20 
        -fconcepts
        -fcoroutines
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(flowgraph_tests)
