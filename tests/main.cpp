#include <iostream>
#include <cassert>
#include <flowgraph/core/graph.hpp>

using namespace flowgraph;

// Test node that returns a constant value
class TestNode : public Node<int> {
public:
    TestNode(int value) : Node<int>("test"), value_(value) {}

protected:
    Task<int> compute_impl() override {
        co_return value_;
    }

private:
    int value_;
};

int main() {
    // Test basic node creation and computation
    auto node = std::make_shared<TestNode>(42);
    auto result = node->compute().await_resume();
    assert(result == 42);

    // Test graph creation and edge addition
    Graph<int> graph;
    auto node1 = std::make_shared<TestNode>(1);
    auto node2 = std::make_shared<TestNode>(2);
    graph.add_node(node1);
    graph.add_node(node2);
    auto edge = std::make_shared<Edge<int>>(node1, node2);
    graph.add_edge(edge);

    // Test graph execution
    graph.execute().await_resume();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
