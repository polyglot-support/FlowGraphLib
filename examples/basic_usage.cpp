#include <iostream>
#include <memory>
#include <string>
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/edge.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

namespace flowgraph {
namespace example {

// Simple node that performs basic arithmetic
template<typename T>
class ArithmeticNode : public Node<T> {
public:
    ArithmeticNode(std::string name, T value)
        : Node<T>(std::move(name))
        , value_(value) {}

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        // Simulate computation with precision
        T result = value_;
        for (size_t i = 0; i < precision_level; ++i) {
            result *= 1.1;  // Simple operation affected by precision
        }
        co_return ComputeResult<T>(result);
    }

private:
    T value_;
};

// Example function that creates and runs a simple graph
void run_example() {
    // Create a simple graph
    Graph<double> graph;

    // Create nodes
    auto node1 = std::make_shared<ArithmeticNode<double>>("input1", 10.0);
    auto node2 = std::make_shared<ArithmeticNode<double>>("input2", 20.0);
    auto node3 = std::make_shared<ArithmeticNode<double>>("output", 0.0);

    // Add nodes to graph
    graph.add_node(node1);
    graph.add_node(node2);
    graph.add_node(node3);

    // Connect nodes
    graph.add_edge(std::make_shared<Edge<double>>(node1, node3));
    graph.add_edge(std::make_shared<Edge<double>>(node2, node3));

    // Set precision levels
    node1->set_precision_range(0, 8);
    node2->set_precision_range(0, 8);
    node3->set_precision_range(0, 8);

    node1->adjust_precision(4);
    node2->adjust_precision(4);
    node3->adjust_precision(4);

    // Execute graph
    graph.execute().get();

    // Get results
    auto result1 = node1->compute().get();
    auto result2 = node2->compute().get();
    auto result3 = node3->compute().get();

    // Print results
    std::cout << "Node 1 result: " << result1.value() << std::endl;
    std::cout << "Node 2 result: " << result2.value() << std::endl;
    std::cout << "Node 3 result: " << result3.value() << std::endl;
}

} // namespace example
} // namespace flowgraph

// Main function that can be called from both native and WASM environments
extern "C" {
    int main() {
        flowgraph::example::run_example();
        return 0;
    }
}

#ifdef __EMSCRIPTEN__
// WASM bindings
EMSCRIPTEN_BINDINGS(flowgraph) {
    emscripten::function("runExample", &flowgraph::example::run_example);
}
#endif
