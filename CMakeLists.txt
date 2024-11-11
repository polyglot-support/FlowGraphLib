cmake_minimum_required(VERSION 3.20)
project(FlowGraphLib VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard and enable latest features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler-specific flags for C++20 features
if(MSVC)
    add_compile_options(/std:c++latest /Zc:__cplusplus /EHsc /permissive- /W4)
    add_compile_definitions(_HAS_CXX20=1)
    add_compile_options(/await:strict)
elseif(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    add_compile_options(-pthread)
    add_link_options(
        "SHELL:-s USE_PTHREADS=1"
        "SHELL:-s PTHREAD_POOL_SIZE=4"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s WASM=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME='createModule'"
        "SHELL:-s NO_EXIT_RUNTIME=1"
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-lembind"
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-std=c++20 -Wall -Wextra -Wpedantic)
    if(APPLE)
        add_compile_options(-stdlib=libc++)
    endif()
else()
    add_compile_options(-std=c++20 -fconcepts -fcoroutines -Wall -Wextra -Wpedantic)
endif()

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

# WASM target
if(EMSCRIPTEN)
    add_executable(flowgraph_wasm wasm/flowgraph.hpp examples/basic_usage.cpp)
    target_link_libraries(flowgraph_wasm PRIVATE flowgraph)
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
    configure_file(
        ${CMAKE_SOURCE_DIR}/wasm/shell.html
        ${CMAKE_BINARY_DIR}/shell.html
        COPYONLY
    )
endif()

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)

    pybind11_add_module(flowgraph_python 
        python/flowgraph_module.cpp
        include/flowgraph/optimization/compression_optimization.hpp
        include/flowgraph/optimization/precision_optimization.hpp
    )
    target_include_directories(flowgraph_python PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${pybind11_INCLUDE_DIRS}
    )
    target_link_libraries(flowgraph_python PRIVATE flowgraph)
    set_target_properties(flowgraph_python PROPERTIES
        OUTPUT_NAME "flowgraph"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python"
    )
endif()
