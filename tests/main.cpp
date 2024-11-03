#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <flowgraph/core/graph.hpp>
#include <flowgraph/cache/cache_policy.hpp>
#include <flowgraph/optimization/dead_node_elimination.hpp>
#include <flowgraph/optimization/node_fusion.hpp>
#include <flowgraph/serialization/serialization.hpp>
#include <flowgraph/async/thread_pool.hpp>
#include <nlohmann/json.hpp>

using namespace flowgraph;
using namespace std::chrono_literals;

// Test node that returns a constant value
class TestNode : public Node<int> {
public:
    TestNode(int value, std::chrono::milliseconds delay = 0ms) 
        : Node<int>("test"), value_(value), delay_(delay) {}

protected:
    Task<int> compute_impl() override {
        compute_count_++;
        if (delay_.count() > 0) {
            std::this_thread::sleep_for(delay_);
        }
        co_return value_;
    }

public:
    int compute_count_ = 0;

private:
    int value_;
    std::chrono::milliseconds delay_;
};

void test_basic_functionality() {
    auto node = std::make_shared<TestNode>(42);
    auto result = node->compute().await_resume();
    assert(result == 42);
    std::cout << "Basic functionality test passed!" << std::endl;
}

void test_graph_creation() {
    Graph<int> graph;
    auto node1 = std::make_shared<TestNode>(1);
    auto node2 = std::make_shared<TestNode>(2);
    graph.add_node(node1);
    graph.add_node(node2);
    auto edge = std::make_shared<Edge<int>>(node1, node2);
    graph.add_edge(edge);
    graph.execute().await_resume();
    std::cout << "Graph creation test passed!" << std::endl;
}

void test_dead_node_elimination() {
    Graph<int> graph;
    
    // Create nodes
    auto node1 = std::make_shared<TestNode>(1);
    auto node2 = std::make_shared<TestNode>(2);
    auto dead_node = std::make_shared<TestNode>(3);
    
    // Add nodes and edges
    graph.add_node(node1);
    graph.add_node(node2);
    graph.add_node(dead_node);
    graph.add_edge(std::make_shared<Edge<int>>(node1, node2));
    
    // Add dead node elimination pass
    graph.add_optimization_pass(std::make_unique<DeadNodeElimination<int>>());
    
    // Execute graph with optimization
    graph.execute().await_resume();
    
    // Check that dead_node was eliminated
    bool node_exists = false;
    for (const auto& node : graph.get_nodes()) {
        if (node == dead_node) {
            node_exists = true;
            break;
        }
    }
    assert(!node_exists);
    std::cout << "Dead node elimination test passed!" << std::endl;
}

void test_node_fusion() {
    Graph<int> graph;
    
    // Create a chain of nodes
    auto node1 = std::make_shared<TestNode>(1);
    auto node2 = std::make_shared<TestNode>(2);
    auto node3 = std::make_shared<TestNode>(3);
    
    // Add nodes and edges to form a chain
    graph.add_node(node1);
    graph.add_node(node2);
    graph.add_node(node3);
    graph.add_edge(std::make_shared<Edge<int>>(node1, node2));
    graph.add_edge(std::make_shared<Edge<int>>(node2, node3));
    
    // Add node fusion pass
    graph.add_optimization_pass(std::make_unique<NodeFusion<int>>());
    
    // Get initial node count
    size_t initial_node_count = graph.get_nodes().size();
    
    // Execute graph with optimization
    graph.execute().await_resume();
    
    // Check that nodes were fused (node count should be less)
    assert(graph.get_nodes().size() < initial_node_count);
    std::cout << "Node fusion test passed!" << std::endl;
}

void test_lru_cache() {
    Graph<int> graph(std::make_unique<LRUCachePolicy<int>>(2));
    auto node = std::make_shared<TestNode>(42);
    graph.add_node(node);

    // First execution should compute
    graph.execute().await_resume();
    assert(node->compute_count_ == 1);

    // Second execution should use cache
    graph.execute().await_resume();
    assert(node->compute_count_ == 1);  // Count shouldn't increase

    std::cout << "LRU cache test passed!" << std::endl;
}

void test_lfu_cache() {
    Graph<int> graph(std::make_unique<LFUCachePolicy<int>>(2));
    auto node = std::make_shared<TestNode>(42);
    graph.add_node(node);

    // First execution should compute
    graph.execute().await_resume();
    assert(node->compute_count_ == 1);

    // Second execution should use cache
    graph.execute().await_resume();
    assert(node->compute_count_ == 1);  // Count shouldn't increase

    std::cout << "LFU cache test passed!" << std::endl;
}

void test_serialization() {
    // Create original graph
    Graph<int> original_graph;
    auto node1 = std::make_shared<TestNode>(1);
    auto node2 = std::make_shared<TestNode>(2);
    original_graph.add_node(node1);
    original_graph.add_node(node2);
    original_graph.add_edge(std::make_shared<Edge<int>>(node1, node2));

    // Serialize graph
    nlohmann::json json = original_graph.to_json();

    // Create new graph and deserialize
    Graph<int> deserialized_graph;
    deserialized_graph.from_json(json, [](const std::string& name) {
        return std::make_shared<TestNode>(1); // Simple factory for test nodes
    });

    // Verify graph structure
    assert(deserialized_graph.get_nodes().size() == original_graph.get_nodes().size());
    
    // Execute both graphs
    original_graph.execute().await_resume();
    deserialized_graph.execute().await_resume();

    std::cout << "Serialization test passed!" << std::endl;
}

void test_thread_pool() {
    // Create a custom thread pool with 4 threads
    auto thread_pool = std::make_shared<ThreadPool>(4);
    Graph<int> graph(nullptr, thread_pool);
    
    // Create nodes with artificial delays
    auto node1 = std::make_shared<TestNode>(1, 100ms);
    auto node2 = std::make_shared<TestNode>(2, 100ms);
    auto node3 = std::make_shared<TestNode>(3, 100ms);
    auto node4 = std::make_shared<TestNode>(4, 100ms);
    
    // Add independent nodes to graph
    graph.add_node(node1);
    graph.add_node(node2);
    graph.add_node(node3);
    graph.add_node(node4);
    
    // Measure execution time
    auto start = std::chrono::high_resolution_clock::now();
    graph.execute().await_resume();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // With parallel execution, total time should be approximately 100ms
    // Allow some overhead, but it should be significantly less than 400ms (sequential execution)
    assert(duration.count() < 200);  // Allow 200ms max
    
    // Verify all nodes were executed
    assert(node1->compute_count_ == 1);
    assert(node2->compute_count_ == 1);
    assert(node3->compute_count_ == 1);
    assert(node4->compute_count_ == 1);

    std::cout << "Thread pool test passed!" << std::endl;
}

int main() {
    test_basic_functionality();
    test_graph_creation();
    test_dead_node_elimination();
    test_node_fusion();
    test_lru_cache();
    test_lfu_cache();
    test_serialization();
    test_thread_pool();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
