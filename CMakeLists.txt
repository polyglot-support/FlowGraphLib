cmake_minimum_required(VERSION 3.20)
project(FlowGraphLib VERSION 0.1.0)

# Set C++20 standard and enable latest features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler-specific flags for C++20 features
if(MSVC)
    # Visual Studio 2019 version 16.11 or later required for C++20
    add_compile_options(/std:c++latest /Zc:__cplusplus /EHsc /permissive-)
    
    # Enable concepts and coroutines
    add_compile_definitions(_HAS_CXX20=1)
    add_compile_options(/await:strict)
    
    # Additional optimization and warning flags
    add_compile_options(/W4)
elseif(EMSCRIPTEN)
    # WASM-specific settings
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    add_compile_options(
        -std=c++20
        -s USE_PTHREADS=1
        -s PTHREAD_POOL_SIZE=4
        -s ALLOW_MEMORY_GROWTH=1
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -s EXPORTED_FUNCTIONS=['_malloc','_free']
    )
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1")
else()
    # GCC/Clang flags for C++20
    add_compile_options(-std=c++20 -fconcepts -fcoroutines)
    
    # Additional optimization and warning flags
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add include directories
include_directories(${CMAKE_SOURCE_DIR})

# Add core library target
add_library(flowgraph INTERFACE)
target_include_directories(flowgraph INTERFACE ${CMAKE_SOURCE_DIR})

# Enable testing unless building for WASM
if(NOT EMSCRIPTEN)
    enable_testing()
    add_subdirectory(tests)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# WASM-specific targets
if(EMSCRIPTEN)
    # Add WASM example
    add_executable(flowgraph_wasm examples/basic_usage.cpp)
    target_link_libraries(flowgraph_wasm PRIVATE flowgraph)
    
    # Configure WASM output
    set_target_properties(flowgraph_wasm PROPERTIES
        LINK_FLAGS "-s WASM=1 \
                   -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
                   -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
                   -s ALLOW_MEMORY_GROWTH=1 \
                   --no-entry \
                   --shell-file ${CMAKE_SOURCE_DIR}/wasm/shell.html"
    )
endif()
