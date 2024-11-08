#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "../core/node.hpp"
#include "../core/fractal_tree_node.hpp"
#include "optimization_pass.hpp"
#include "fused_node.hpp"

namespace flowgraph {

template<typename T>
class PrecisionAwareNodeFusion : public OptimizationPass<T> {
public:
    PrecisionAwareNodeFusion(
        double precision_compatibility_threshold = 0.1,
        size_t min_ops_for_fusion = 2
    ) 
        : precision_compatibility_threshold_(precision_compatibility_threshold)
        , min_ops_for_fusion_(min_ops_for_fusion) {}

    void optimize(Graph<T>& graph) override {
        auto nodes = graph.get_nodes();
        std::unordered_map<std::shared_ptr<Node<T>>, std::unordered_set<std::shared_ptr<Node<T>>>> fusion_candidates;

        // Identify fusion candidates
        for (const auto& node : nodes) {
            auto outgoing = graph.get_outgoing_edges(node);
            
            for (const auto& edge : outgoing) {
                auto target = edge->to();
                
                if (can_fuse_nodes(node, target)) {
                    fusion_candidates[node].insert(target);
                }
            }
        }

        // Perform fusion for compatible groups
        for (const auto& [source, targets] : fusion_candidates) {
            if (targets.size() >= min_ops_for_fusion_) {
                fuse_node_group(graph, source, targets);
            }
        }
    }

private:
    bool can_fuse_nodes(
        const std::shared_ptr<Node<T>>& source,
        const std::shared_ptr<Node<T>>& target
    ) {
        // Check precision level compatibility
        auto source_precision = source->current_precision_level();
        auto target_precision = target->current_precision_level();
        
        // Nodes should have similar precision requirements
        if (std::abs(static_cast<int>(source_precision) - 
                    static_cast<int>(target_precision)) > 1) {
            return false;
        }

        // Check precision ranges overlap
        if (source->max_precision_level() < target->min_precision_level() ||
            target->max_precision_level() < source->min_precision_level()) {
            return false;
        }

        // Check if the nodes have compatible error rates
        if (!have_compatible_error_rates(source, target)) {
            return false;
        }

        return true;
    }

    bool have_compatible_error_rates(
        const std::shared_ptr<Node<T>>& node1,
        const std::shared_ptr<Node<T>>& node2
    ) {
        // In practice, you would analyze historical error rates
        // For now, assume compatibility based on precision levels
        auto precision_diff = std::abs(
            static_cast<int>(node1->current_precision_level()) -
            static_cast<int>(node2->current_precision_level())
        );
        
        return precision_diff <= 1;
    }

    void fuse_node_group(
        Graph<T>& graph,
        const std::shared_ptr<Node<T>>& source,
        const std::unordered_set<std::shared_ptr<Node<T>>>& targets
    ) {
        // Calculate optimal precision level for fused node
        size_t optimal_precision = calculate_optimal_precision(source, targets);

        // Create fused node with optimal precision
        auto fused = create_precision_aware_fused_node(source, targets, optimal_precision);

        // Update graph structure
        graph.add_node(fused);

        // Redirect incoming edges to source
        auto incoming = graph.get_incoming_edges(source);
        for (const auto& edge : incoming) {
            graph.add_edge(std::make_shared<Edge<T>>(edge->from(), fused));
        }

        // Redirect outgoing edges from targets
        for (const auto& target : targets) {
            auto outgoing = graph.get_outgoing_edges(target);
            for (const auto& edge : outgoing) {
                if (targets.find(edge->to()) == targets.end()) {
                    graph.add_edge(std::make_shared<Edge<T>>(fused, edge->to()));
                }
            }
        }

        // Remove original nodes
        graph.remove_node(source);
        for (const auto& target : targets) {
            graph.remove_node(target);
        }
    }

    size_t calculate_optimal_precision(
        const std::shared_ptr<Node<T>>& source,
        const std::unordered_set<std::shared_ptr<Node<T>>>& targets
    ) {
        // Start with source precision
        size_t optimal = source->current_precision_level();
        size_t min_precision = source->min_precision_level();
        size_t max_precision = source->max_precision_level();

        // Consider target nodes' precision requirements
        for (const auto& target : targets) {
            min_precision = std::max(min_precision, target->min_precision_level());
            max_precision = std::min(max_precision, target->max_precision_level());
            optimal = std::max(optimal, target->current_precision_level());
        }

        // Ensure optimal precision is within valid range
        return std::clamp(optimal, min_precision, max_precision);
    }

    std::shared_ptr<Node<T>> create_precision_aware_fused_node(
        const std::shared_ptr<Node<T>>& source,
        const std::unordered_set<std::shared_ptr<Node<T>>>& targets,
        size_t optimal_precision
    ) {
        // Create a new fused node with optimal precision settings
        auto fused = std::make_shared<FusedNode<T>>(
            source->name() + "_fused",
            optimal_precision,
            precision_compatibility_threshold_
        );

        // Set precision range
        size_t min_precision = source->min_precision_level();
        size_t max_precision = source->max_precision_level();
        
        for (const auto& target : targets) {
            min_precision = std::max(min_precision, target->min_precision_level());
            max_precision = std::min(max_precision, target->max_precision_level());
        }

        fused->set_precision_range(min_precision, max_precision);
        
        return fused;
    }

    double precision_compatibility_threshold_;
    size_t min_ops_for_fusion_;
};

} // namespace flowgraph
