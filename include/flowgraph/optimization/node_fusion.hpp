#pragma once
#include "optimization_pass.hpp"
#include "fused_node.hpp"
#include <vector>

namespace flowgraph {

template<typename T>
class NodeFusion : public OptimizationPass<T> {
public:
    void optimize(Graph<T>& graph) override {
        auto chains = find_fusion_chains(graph);
        for (const auto& chain : chains) {
            if (chain.size() > 1) {
                fuse_chain(graph, chain);
            }
        }
    }

    std::string name() const override {
        return "Node Fusion";
    }

private:
    std::vector<std::vector<std::shared_ptr<Node<T>>>> find_fusion_chains(const Graph<T>& graph) {
        std::vector<std::vector<std::shared_ptr<Node<T>>>> chains;
        std::unordered_set<std::shared_ptr<Node<T>>> visited;

        for (const auto& node : graph.get_nodes()) {
            if (visited.find(node) == visited.end()) {
                std::vector<std::shared_ptr<Node<T>>> chain;
                build_chain(node, graph, visited, chain);
                if (!chain.empty()) {
                    chains.push_back(chain);
                }
            }
        }
        return chains;
    }

    void build_chain(const std::shared_ptr<Node<T>>& node,
                    const Graph<T>& graph,
                    std::unordered_set<std::shared_ptr<Node<T>>>& visited,
                    std::vector<std::shared_ptr<Node<T>>>& chain) {
        visited.insert(node);
        chain.push_back(node);

        auto outgoing = graph.get_outgoing_edges(node);
        if (outgoing.size() == 1 && graph.get_incoming_edges(outgoing[0]->to()).size() == 1) {
            build_chain(outgoing[0]->to(), graph, visited, chain);
        }
    }

    void fuse_chain(Graph<T>& graph, const std::vector<std::shared_ptr<Node<T>>>& chain) {
        // Create a new fused node
        auto fused = std::make_shared<FusedNode<T>>(chain);
        
        // Add edges from inputs of first node to fused node
        auto first_node = chain.front();
        for (const auto& edge : graph.get_incoming_edges(first_node)) {
            graph.add_edge(std::make_shared<Edge<T>>(edge->from(), fused));
        }

        // Add edges from fused node to outputs of last node
        auto last_node = chain.back();
        for (const auto& edge : graph.get_outgoing_edges(last_node)) {
            graph.add_edge(std::make_shared<Edge<T>>(fused, edge->to()));
        }

        // Remove original nodes
        for (const auto& node : chain) {
            graph.remove_node(node);
        }
    }
};

} // namespace flowgraph
