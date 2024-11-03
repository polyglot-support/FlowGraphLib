# C++20 Flowgraph Library

A high-performance C++20 library for building and executing asynchronous data flow graphs with coroutine support and advanced caching mechanisms.

## Features

- **Asynchronous Execution**: Leverages C++20 coroutines for non-blocking operations
- **Flexible Graph Structure**: Build directed acyclic graphs (DAG) with customizable nodes and edges
- **Advanced Caching**: Implements LRU and LFU caching policies with graph-level optimization
- **Optimization Passes**: Includes dead node elimination and node fusion optimizations
- **Thread Pool Support**: Efficient parallel execution of independent nodes
- **Fractal Tree Storage**: Hierarchical value storage with dynamic precision scaling
- **Serialization Support**: JSON-based graph serialization and deserialization

## Requirements

- C++20 compatible compiler
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

## Usage Example

Here's a simple example demonstrating basic usage:

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
auto thread_pool = std::make_shared<ThreadPool>(4);
Graph<int> graph(nullptr, thread_pool);
```

### Custom Cache Policies

```cpp
Graph<int> graph(std::make_unique<LRUCachePolicy<int>>(100));
```

### Optimization Passes

```cpp
graph.add_optimization_pass(std::make_unique<DeadNodeElimination<int>>());
graph.add_optimization_pass(std::make_unique<NodeFusion<int>>());
```

## Documentation

For detailed documentation of all features and APIs, please refer to the header files in the `include/flowgraph` directory.

## Contributing

Contributions are welcome! Please read our [Contributing Guidelines](CONTRIBUTING.md) for details on how to submit pull requests, report issues, and contribute to the project.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
