#include <iostream>
#include <cassert>
#include <flowgraph/core/graph.hpp>
#include <flowgraph/cache/cache_policy.hpp>
#include <flowgraph/optimization/dead_node_elimination.hpp>
#include <flowgraph/optimization/node_fusion.hpp>

using namespace flowgraph;

// Test node that returns a constant value
class TestNode : public Node<int> {
public:
    TestNode(int value) : Node<int>("test"), value_(value) {}

protected:
    Task<int> compute_impl() override {
        compute_count_++;
        co_return value_;
    }

public:
    int compute_count_ = 0;

private:
    int value_;
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

int main() {
    test_basic_functionality();
    test_graph_creation();
    test_dead_node_elimination();
    test_node_fusion();
    test_lru_cache();
    test_lfu_cache();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
