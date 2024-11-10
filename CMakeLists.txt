cmake_minimum_required(VERSION 3.20)
project(FlowGraphLib VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard and enable latest features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch pybind11 if building Python bindings
if(BUILD_PYTHON_BINDINGS)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

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
    add_compile_options(-pthread)
    add_link_options(
        "SHELL:-s USE_PTHREADS=1"
        "SHELL:-s PTHREAD_POOL_SIZE=4"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "SHELL:-s EXPORTED_FUNCTIONS=['_malloc','_free']"
        "SHELL:-s WASM=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME='createModule'"
        "SHELL:-s NO_EXIT_RUNTIME=1"
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-lembind"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang flags for C++20
    add_compile_options(-std=c++20)
    
    # Additional optimization and warning flags
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    if(APPLE)
        # macOS-specific flags for concepts and coroutines
        add_compile_options(-stdlib=libc++)
    endif()
else()
    # GCC flags for C++20
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
    # Add WASM library
    add_executable(flowgraph_wasm wasm/flowgraph.hpp examples/basic_usage.cpp)
    target_link_libraries(flowgraph_wasm PRIVATE flowgraph)
    
    # Configure WASM output
    set_target_properties(flowgraph_wasm PROPERTIES
        LINK_FLAGS "--shell-file ${CMAKE_SOURCE_DIR}/wasm/shell.html \
                   -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
                   -s EXPORTED_FUNCTIONS=['_malloc','_free'] \
                   -s ALLOW_MEMORY_GROWTH=1 \
                   -s MODULARIZE=1 \
                   -s EXPORT_NAME='createModule' \
                   -s NO_EXIT_RUNTIME=1 \
                   -s ASSERTIONS=1 \
                   -lembind"
    )

    # Copy shell.html to build directory
    configure_file(
        ${CMAKE_SOURCE_DIR}/wasm/shell.html
        ${CMAKE_BINARY_DIR}/shell.html
        COPYONLY
    )
endif()

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    pybind11_add_module(flowgraph_python python/flowgraph.hpp)
    set_target_properties(flowgraph_python PROPERTIES
        OUTPUT_NAME "flowgraph"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
        LINKER_LANGUAGE CXX
    )
    target_link_libraries(flowgraph_python PRIVATE flowgraph)
endif()
