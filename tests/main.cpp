#include <iostream>
#include <cassert>
#include <flowgraph/core/graph.hpp>
#include <flowgraph/cache/cache_policy.hpp>

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
    test_lru_cache();
    test_lfu_cache();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
