# C++20 Flowgraph Library

A modern C++20 flowgraph library for asynchronous data flow operations with coroutines support. The library provides a flexible and efficient framework for building and executing directed acyclic graphs (DAG) with features like caching, optimization, and thread pool support.

## Features

- **Modern C++20 Design**
  - Utilizes C++20 coroutines for asynchronous execution ([async/task.hpp](include/flowgraph/async/task.hpp))
  - Concepts for compile-time interface validation ([core/concepts.hpp](include/flowgraph/core/concepts.hpp))
  - Thread-safe operations ([async/thread_pool.hpp](include/flowgraph/async/thread_pool.hpp))
  - Future/Promise integration ([async/future_helpers.hpp](include/flowgraph/async/future_helpers.hpp))

- **Core Functionality**
  - Directed Acyclic Graph (DAG) structure ([core/graph.hpp](include/flowgraph/core/graph.hpp))
  - Asynchronous node execution ([core/node.hpp](include/flowgraph/core/node.hpp))
  - Flexible node and edge types ([core/edge.hpp](include/flowgraph/core/edge.hpp))
  - Callback system for execution events ([core/node.hpp](include/flowgraph/core/node.hpp))
  - Interface-based design ([core/interfaces.hpp](include/flowgraph/core/interfaces.hpp))
  - Base class templates ([core/base.hpp](include/flowgraph/core/base.hpp))

- **Advanced Features**
  - Fractal Tree Node structure for efficient value storage ([core/fractal_tree_node.hpp](include/flowgraph/core/fractal_tree_node.hpp))
  - Dynamic precision scaling with automatic optimization ([optimization/precision_optimization.hpp](include/flowgraph/optimization/precision_optimization.hpp))
  - Efficient compression mechanisms ([optimization/compression_optimization.hpp](include/flowgraph/optimization/compression_optimization.hpp))
  - Thread pool support for parallel execution ([async/thread_pool.hpp](include/flowgraph/async/thread_pool.hpp))
  - Comprehensive error handling and propagation ([core/error_state.hpp](include/flowgraph/core/error_state.hpp))
  - Precompiled headers support ([core/pch.hpp](include/flowgraph/core/pch.hpp))

- **Error Handling**
  - Comprehensive error type system ([core/error_state.hpp](include/flowgraph/core/error_state.hpp))
  - Error propagation tracking ([core/compute_result.hpp](include/flowgraph/core/compute_result.hpp))
  - Source node identification ([core/error_state.hpp](include/flowgraph/core/error_state.hpp))
  - Error path tracing ([core/graph.hpp](include/flowgraph/core/graph.hpp))
  - Recovery mechanisms ([core/node.hpp](include/flowgraph/core/node.hpp))
  - Error state templates ([core/templates.hpp](include/flowgraph/core/templates.hpp))

- **Caching System**
  - Fractal tree-based caching ([cache/fractal_cache_policy.hpp](include/flowgraph/cache/fractal_cache_policy.hpp))
  - Precision-aware cache policies ([cache/cache_policy.hpp](include/flowgraph/cache/cache_policy.hpp))
  - Local node-level caching ([cache/node_cache.hpp](include/flowgraph/cache/node_cache.hpp))
  - Graph-wide caching support ([cache/graph_cache.hpp](include/flowgraph/cache/graph_cache.hpp))

- **Optimization Features**
  - Node fusion optimization ([optimization/node_fusion.hpp](include/flowgraph/optimization/node_fusion.hpp))
  - Dead node elimination ([optimization/dead_node_elimination.hpp](include/flowgraph/optimization/dead_node_elimination.hpp))
  - Custom optimization passes ([optimization/optimization_pass.hpp](include/flowgraph/optimization/optimization_pass.hpp))
  - Fused node implementation ([optimization/fused_node.hpp](include/flowgraph/optimization/fused_node.hpp))

- **Implementation Details**
  - Node implementation templates ([core/impl/node_impl.hpp](include/flowgraph/core/impl/node_impl.hpp))
  - Compute result implementation ([core/impl/compute_result_impl.hpp](include/flowgraph/core/impl/compute_result_impl.hpp))
  - Forward declarations ([core/forward_decl.hpp](include/flowgraph/core/forward_decl.hpp))

- **WebAssembly Support**
  - WASM build configuration ([wasm/shell.html](wasm/shell.html))
  - Browser integration
  - Thread pool support in browser

## Requirements

- C++20 compliant compiler
- CMake 3.20 or higher
- Google Test (automatically fetched for testing)

## Installation

```bash
# Clone the repository
git clone https://github.com/polyglot-support/FlowGraphLib.git
cd FlowGraphLib

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run tests
ctest
```

## Basic Usage

Here's a simple example demonstrating basic usage of the library:

```cpp
#include <flowgraph/core/graph.hpp>
#include <flowgraph/core/node.hpp>
#include <flowgraph/core/edge.hpp>

// For a complete working example, see examples/basic_usage.cpp
```

For more examples, check out:
- [Basic Usage](examples/basic_usage.cpp)
- [Graph Optimization](examples/graph_optimization.cpp)
- [Image Pipeline](examples/image_pipeline.cpp)
- [Matrix Operations](examples/matrix_operations.cpp)
- [Neural Network](examples/neural_network.cpp)
- [Signal Processing](examples/signal_processing.cpp)

## Testing

The library includes comprehensive tests:
- [Error Propagation Tests](tests/error_propagation_test.cpp)
- [Precision Management Tests](tests/precision_management_test.cpp)
- [Fractal Tree Tests](tests/fractal_tree_test.cpp)
- [Performance Benchmarks](tests/fractal_tree_benchmark.cpp)

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
