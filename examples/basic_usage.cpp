#include <iostream>
#include <flowgraph/core/graph.hpp>

using namespace flowgraph;

// Example node that squares its input
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

    node2->add_completion_callback([](const int& result) {
        std::cout << "Node 2 result: " << result << std::endl;
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
