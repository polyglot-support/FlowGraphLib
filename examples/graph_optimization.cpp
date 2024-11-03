#include <flowgraph/core/graph.hpp>
#include <flowgraph/optimization/dead_node_elimination.hpp>
#include <flowgraph/optimization/node_fusion.hpp>
#include <iostream>
#include <chrono>
#include <string>

using namespace flowgraph;

// Simple computation node
class ComputeNode : public Node<double> {
public:
    ComputeNode(const std::string& name, double value, std::chrono::milliseconds delay = std::chrono::milliseconds(0))
        : Node<double>(name), value_(value), delay_(delay) {}

protected:
    Task<double> compute_impl() override {
        if (delay_.count() > 0) {
            std::this_thread::sleep_for(delay_);
        }
        co_return value_;
    }

private:
    double value_;
    std::chrono::milliseconds delay_;
};

// Node that multiplies its input by a constant
class MultiplyNode : public Node<double> {
public:
    MultiplyNode(const std::string& name, double factor)
        : Node<double>(name), factor_(factor) {}

protected:
    Task<double> compute_impl() override {
        co_return factor_;
    }

private:
    double factor_;
};

void run_optimized_graph() {
    std::cout << "Running optimization demonstration...\n";

    // Create graph with thread pool
    auto thread_pool = std::make_shared<ThreadPool>(4);
    Graph<double> graph(nullptr, thread_pool);

    // Create a complex graph with some dead nodes and fusible chains
    auto node1 = std::make_shared<ComputeNode>("node1", 1.0, std::chrono::milliseconds(100));
    auto node2 = std::make_shared<ComputeNode>("node2", 2.0, std::chrono::milliseconds(100));
    auto node3 = std::make_shared<ComputeNode>("node3", 3.0, std::chrono::milliseconds(100));
    auto dead_node = std::make_shared<ComputeNode>("dead_node", 4.0, std::chrono::milliseconds(100));
    auto mult1 = std::make_shared<MultiplyNode>("mult1", 2.0);
    auto mult2 = std::make_shared<MultiplyNode>("mult2", 3.0);

    // Add nodes to graph
    graph.add_node(node1);
    graph.add_node(node2);
    graph.add_node(node3);
    graph.add_node(dead_node);
    graph.add_node(mult1);
    graph.add_node(mult2);

    // Add edges
    graph.add_edge(std::make_shared<Edge<double>>(node1, mult1));
    graph.add_edge(std::make_shared<Edge<double>>(mult1, mult2));
    graph.add_edge(std::make_shared<Edge<double>>(node2, node3));

    // Add callbacks
    node3->add_completion_callback([](const double& result) {
        std::cout << "Node3 result: " << result << "\n";
    });

    mult2->add_completion_callback([](const double& result) {
        std::cout << "Mult2 result: " << result << "\n";
    });

    // First run without optimization
    std::cout << "\nRunning without optimization...\n";
    auto start = std::chrono::high_resolution_clock::now();
    graph.execute().await_resume();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time without optimization: " << duration.count() << "ms\n";

    // Add optimization passes
    graph.add_optimization_pass(std::make_unique<DeadNodeElimination<double>>());
    graph.add_optimization_pass(std::make_unique<NodeFusion<double>>());

    // Run with optimization
    std::cout << "\nRunning with optimization...\n";
    start = std::chrono::high_resolution_clock::now();
    graph.execute().await_resume();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time with optimization: " << duration.count() << "ms\n";
}

int main() {
    run_optimized_graph();
    return 0;
}
