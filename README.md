# C++20 Flowgraph Library

A modern C++20 flowgraph library for asynchronous data flow operations with coroutines support. The library provides a flexible and efficient framework for building and executing directed acyclic graphs (DAG) with features like caching, optimization, and thread pool support.

## Features

- **Modern C++20 Design**
  - Utilizes C++20 coroutines for asynchronous execution
  - Concepts for compile-time interface validation
  - Thread-safe operations

- **Core Functionality**
  - Directed Acyclic Graph (DAG) structure
  - Asynchronous node execution
  - Flexible node and edge types
  - Callback system for execution events

- **Advanced Features**
  - Fractal Tree Node structure for efficient value storage
  - Dynamic precision scaling with automatic optimization
  - Efficient compression mechanisms
  - Thread pool support for parallel execution
  - Comprehensive error handling and propagation
  - Memory-aware precision management

- **Precision Management**
  - Expansive/compressive fractal tree structure
  - Dynamic precision level adjustment
  - Automatic precision optimization
  - Memory-aware compression
  - Precision-aware node fusion
  - Parallel path precision balancing

- **Error Handling**
  - Comprehensive error type system
  - Error propagation tracking
  - Source node identification
  - Error path tracing
  - Recovery mechanisms
  - Error-aware optimization

- **Caching System**
  - Fractal tree-based caching
  - Precision-aware cache policies
  - Local node-level caching
  - Graph-wide caching support
  - Customizable cache policies (LRU, LFU)
  - Thread-safe cache operations

- **Optimization Features**
  - Precision-aware node fusion
  - Memory-aware compression
  - Dead node elimination
  - Node fusion optimization
  - Lazy evaluation support
  - Custom optimization passes

- **Serialization Support**
  - JSON serialization for graphs
  - Custom serialization formats
  - Complete graph state preservation

## Requirements

- C++20 compliant compiler
- CMake 3.16 or higher
- nlohmann/json library (automatically fetched by CMake)
- Google Test and Google Benchmark (automatically fetched for testing)

## Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/flowgraph.git
cd flowgraph

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

using namespace flowgraph;

// Create a custom node with precision support
class SquareNode : public Node<double> {
public:
    SquareNode(double input) 
        : Node<double>("square", 8) // Support up to 8 precision levels
        , input_(input) {}

protected:
    Task<ComputeResult<double>> compute_impl(size_t precision_level) override {
        try {
            // Compute with specified precision
            double result = std::round(input_ * input_ * std::pow(10, precision_level)) 
                          / std::pow(10, precision_level);
            co_return ComputeResult<double>(result);
        } catch (const std::exception& e) {
            auto error = ErrorState::computation_error(e.what());
            error.set_source_node(this->name());
            co_return ComputeResult<double>(std::move(error));
        }
    }

private:
    double input_;
};

int main() {
    // Create graph
    Graph<double> graph;

    // Create nodes with precision support
    auto node1 = std::make_shared<SquareNode>(5.123456789);
    auto node2 = std::make_shared<SquareNode>(10.987654321);

    // Set precision ranges
    node1->set_precision_range(2, 6); // Precision between 2 and 6 decimal places
    node2->set_precision_range(2, 6);

    // Add completion callbacks with error handling
    node1->add_completion_callback([](const ComputeResult<double>& result) {
        if (result.has_error()) {
            std::cerr << "Error in node 1: " << result.error().message() << std::endl;
        } else {
            std::cout << "Node 1 result: " << result.value() << std::endl;
        }
    });

    // Add nodes to graph
    graph.add_node(node1);
    graph.add_node(node2);

    // Create edge
    auto edge = std::make_shared<Edge<double>>(node1, node2);
    graph.add_edge(edge);

    // Add optimization passes
    graph.add_optimization_pass(std::make_unique<PrecisionOptimizationPass<double>>());
    graph.add_optimization_pass(std::make_unique<CompressionOptimizationPass<double>>());

    // Execute graph
    graph.execute().await_resume();

    return 0;
}
```

## Advanced Features

### Precision Management

```cpp
// Create node with precision support
auto node = std::make_shared<ComputeNode<double>>("precise_node", 8);

// Set precision range
node->set_precision_range(2, 6);

// Dynamic precision adjustment
node->adjust_precision(4);

// Force compression
node->merge_updates();
```

### Error Handling

```cpp
// Add error-aware callback
node->add_completion_callback([](const ComputeResult<double>& result) {
    if (result.has_error()) {
        const auto& error = result.error();
        std::cout << "Error type: " << static_cast<int>(error.type()) << std::endl;
        std::cout << "Source node: " << error.source_node().value() << std::endl;
        std::cout << "Error path: ";
        for (const auto& node : error.propagation_path()) {
            std::cout << node << " -> ";
        }
        std::cout << std::endl;
    }
});
```

### Memory-Aware Optimization

```cpp
// Add memory-aware optimization passes
graph.add_optimization_pass(std::make_unique<CompressionOptimizationPass<double>>(
    0.8,  // Compress when memory usage exceeds 80%
    0.2   // Consider nodes inactive below 20% access rate
));

// Add precision-aware node fusion
graph.add_optimization_pass(std::make_unique<PrecisionAwareNodeFusion<double>>(
    0.1,  // Precision compatibility threshold
    2     // Minimum operations for fusion
));
```

### Fractal Cache Policy

```cpp
// Create graph with fractal tree cache
Graph<double> graph(std::make_unique<FractalCachePolicy<double>>(
    1000,   // Max entries per precision level
    0.001   // Compression threshold
));

// Cache automatically manages precision levels
```

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
