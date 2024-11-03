#pragma once
#include "optimization_pass.hpp"
#include <unordered_set>

namespace flowgraph {

template<typename T>
class DeadNodeElimination : public OptimizationPass<T> {
public:
    void optimize(Graph<T>& graph) override {
        auto reachable = find_reachable_nodes(graph);
        remove_unreachable_nodes(graph, reachable);
    }

    std::string name() const override {
        return "Dead Node Elimination";
    }

private:
    std::unordered_set<std::shared_ptr<Node<T>>> find_reachable_nodes(const Graph<T>& graph) {
        std::unordered_set<std::shared_ptr<Node<T>>> reachable;
        for (const auto& node : graph.get_output_nodes()) {
            mark_reachable(node, reachable, graph);
        }
        return reachable;
    }

    void mark_reachable(const std::shared_ptr<Node<T>>& node,
                       std::unordered_set<std::shared_ptr<Node<T>>>& reachable,
                       const Graph<T>& graph) {
        if (reachable.find(node) != reachable.end()) {
            return;
        }
        reachable.insert(node);
        for (const auto& edge : graph.get_incoming_edges(node)) {
            mark_reachable(edge->from(), reachable, graph);
        }
    }

    void remove_unreachable_nodes(Graph<T>& graph,
                                const std::unordered_set<std::shared_ptr<Node<T>>>& reachable) {
        auto nodes = graph.get_nodes();
        for (const auto& node : nodes) {
            if (reachable.find(node) == reachable.end()) {
                graph.remove_node(node);
            }
        }
    }
};

} // namespace flowgraph
