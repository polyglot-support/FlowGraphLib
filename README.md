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
  - Dynamic precision scaling
  - Efficient compression mechanisms
  - Thread pool support for parallel execution

- **Caching System**
  - Local node-level caching
  - Graph-wide caching support
  - Customizable cache policies (LRU, LFU)
  - Thread-safe cache operations

- **Optimization Features**
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
```

## Basic Usage

Here's a simple example demonstrating basic usage of the library:

```cpp
#include <flowgraph/core/graph.hpp>

using namespace flowgraph;

// Create a custom node
class SquareNode : public Node<int> {
public:
    SquareNode(int input) : Node<int>("square"), input_(input) {}

protected:
    Task<int> compute_impl() override {
        co_return input_ * input_;
    }

private:
    int input_;
};

int main() {
    // Create graph
    Graph<int> graph;

    // Create nodes
    auto node1 = std::make_shared<SquareNode>(5);
    auto node2 = std::make_shared<SquareNode>(10);

    // Add completion callbacks
    node1->add_completion_callback([](const int& result) {
        std::cout << "Node 1 result: " << result << std::endl;
    });

    // Add nodes to graph
    graph.add_node(node1);
    graph.add_node(node2);

    // Create edge
    auto edge = std::make_shared<Edge<int>>(node1, node2);
    graph.add_edge(edge);

    // Execute graph
    graph.execute().await_resume();

    return 0;
}
```

## Advanced Features

### Thread Pool Usage

```cpp
// Create a custom thread pool
auto thread_pool = std::make_shared<ThreadPool>(4);
Graph<int> graph(nullptr, thread_pool);

// Add nodes and execute
// ... nodes will be executed in parallel when possible
```

### Caching

```cpp
// Create graph with LRU cache
Graph<int> graph(std::make_unique<LRUCachePolicy<int>>(100));

// Cache is automatically used for repeated computations
```

### Optimization

```cpp
// Add optimization passes
graph.add_optimization_pass(std::make_unique<DeadNodeElimination<int>>());
graph.add_optimization_pass(std::make_unique<NodeFusion<int>>());

// Optimizations are applied during execution
graph.execute().await_resume();
```

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
