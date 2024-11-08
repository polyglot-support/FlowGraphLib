#pragma once
#include <memory>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include "../core/node.hpp"
#include "optimization_pass.hpp"

namespace flowgraph {

template<typename T>
class CompressionOptimizationPass : public OptimizationPass<T> {
public:
    CompressionOptimizationPass(
        double memory_threshold = 0.8,    // Trigger compression at 80% memory usage
        double activity_threshold = 0.2    // Consider nodes inactive below 20% access rate
    )
        : memory_threshold_(memory_threshold)
        , activity_threshold_(activity_threshold) {}

    void optimize(Graph<T>& graph) override {
        auto nodes = graph.get_nodes();
        
        // Skip if graph is small
        if (nodes.size() < 2) return;

        // Analyze memory usage and node activity
        auto memory_stats = analyze_memory_usage(nodes);
        auto activity_stats = analyze_node_activity(nodes);

        if (memory_stats.usage_ratio > memory_threshold_) {
            // Need to compress some nodes
            compress_inactive_nodes(graph, activity_stats);
        }

        // Look for expansion opportunities in critical paths
        expand_critical_path_nodes(graph, memory_stats, activity_stats);

        // Balance precision levels across parallel paths
        balance_parallel_paths(graph);
    }

private:
    struct MemoryStats {
        size_t total_memory;
        size_t available_memory;
        double usage_ratio;
    };

    struct ActivityStats {
        std::unordered_map<std::shared_ptr<Node<T>>, double> access_rates;
        double average_access_rate;
    };

    MemoryStats analyze_memory_usage(
        const std::unordered_set<std::shared_ptr<Node<T>>>& nodes
    ) {
        MemoryStats stats{};
        
        // In practice, this would use actual memory metrics
        // For now, estimate based on precision levels and node count
        stats.total_memory = nodes.size() * 1024 * 1024; // Assume 1MB per node
        stats.available_memory = stats.total_memory;
        
        for (const auto& node : nodes) {
            size_t node_memory = estimate_node_memory(node);
            stats.available_memory -= node_memory;
        }

        stats.usage_ratio = 1.0 - (static_cast<double>(stats.available_memory) / 
                                 static_cast<double>(stats.total_memory));
        
        return stats;
    }

    ActivityStats analyze_node_activity(
        const std::unordered_set<std::shared_ptr<Node<T>>>& nodes
    ) {
        ActivityStats stats{};
        double total_access_rate = 0.0;

        for (const auto& node : nodes) {
            double access_rate = estimate_node_activity(node);
            stats.access_rates[node] = access_rate;
            total_access_rate += access_rate;
        }

        stats.average_access_rate = total_access_rate / nodes.size();
        return stats;
    }

    void compress_inactive_nodes(
        Graph<T>& graph,
        const ActivityStats& activity_stats
    ) {
        for (const auto& [node, access_rate] : activity_stats.access_rates) {
            if (access_rate < activity_threshold_ * activity_stats.average_access_rate) {
                // Node is relatively inactive, try to compress
                size_t current_level = node->current_precision_level();
                if (current_level > node->min_precision_level()) {
                    // Reduce precision level
                    node->adjust_precision(current_level - 1);
                    node->merge_updates(); // Force compression
                }
            }
        }
    }

    void expand_critical_path_nodes(
        Graph<T>& graph,
        const MemoryStats& memory_stats,
        const ActivityStats& activity_stats
    ) {
        if (memory_stats.usage_ratio >= memory_threshold_) {
            return; // No room for expansion
        }

        // Identify critical path nodes (high activity, many dependents)
        std::vector<std::shared_ptr<Node<T>>> critical_nodes;
        for (const auto& [node, access_rate] : activity_stats.access_rates) {
            if (access_rate > activity_stats.average_access_rate * 2.0 &&
                graph.get_outgoing_edges(node).size() > 1) {
                critical_nodes.push_back(node);
            }
        }

        // Sort by activity rate
        std::sort(critical_nodes.begin(), critical_nodes.end(),
            [&](const auto& a, const auto& b) {
                return activity_stats.access_rates.at(a) > 
                       activity_stats.access_rates.at(b);
            });

        // Expand most critical nodes if memory allows
        for (const auto& node : critical_nodes) {
            size_t current_level = node->current_precision_level();
            if (current_level < node->max_precision_level() &&
                would_fit_in_memory(node, current_level + 1, memory_stats)) {
                node->adjust_precision(current_level + 1);
            }
        }
    }

    void balance_parallel_paths(Graph<T>& graph) {
        auto nodes = graph.get_nodes();
        
        // Find parallel paths (nodes with same source and destination)
        for (const auto& node : nodes) {
            auto outgoing = graph.get_outgoing_edges(node);
            if (outgoing.size() < 2) continue;

            // Group parallel paths
            std::unordered_map<std::shared_ptr<Node<T>>, 
                              std::vector<std::shared_ptr<Node<T>>>> parallel_groups;
            
            for (const auto& edge : outgoing) {
                auto end_nodes = find_path_endpoints(graph, edge->to());
                for (const auto& end_node : end_nodes) {
                    parallel_groups[end_node].push_back(edge->to());
                }
            }

            // Balance precision levels within each group
            for (const auto& [end_node, path_nodes] : parallel_groups) {
                balance_group_precision(path_nodes);
            }
        }
    }

    std::unordered_set<std::shared_ptr<Node<T>>> find_path_endpoints(
        Graph<T>& graph,
        std::shared_ptr<Node<T>> start_node
    ) {
        std::unordered_set<std::shared_ptr<Node<T>>> endpoints;
        std::unordered_set<std::shared_ptr<Node<T>>> visited;
        
        find_endpoints_dfs(graph, start_node, visited, endpoints);
        return endpoints;
    }

    void find_endpoints_dfs(
        Graph<T>& graph,
        std::shared_ptr<Node<T>> node,
        std::unordered_set<std::shared_ptr<Node<T>>>& visited,
        std::unordered_set<std::shared_ptr<Node<T>>>& endpoints
    ) {
        visited.insert(node);
        
        auto outgoing = graph.get_outgoing_edges(node);
        if (outgoing.empty()) {
            endpoints.insert(node);
            return;
        }

        for (const auto& edge : outgoing) {
            if (visited.find(edge->to()) == visited.end()) {
                find_endpoints_dfs(graph, edge->to(), visited, endpoints);
            }
        }
    }

    void balance_group_precision(
        const std::vector<std::shared_ptr<Node<T>>>& nodes
    ) {
        if (nodes.empty()) return;

        // Calculate average precision level
        size_t total_precision = 0;
        size_t min_precision = nodes[0]->min_precision_level();
        size_t max_precision = nodes[0]->max_precision_level();

        for (const auto& node : nodes) {
            total_precision += node->current_precision_level();
            min_precision = std::max(min_precision, node->min_precision_level());
            max_precision = std::min(max_precision, node->max_precision_level());
        }

        size_t target_precision = std::clamp(
            total_precision / nodes.size(),
            min_precision,
            max_precision
        );

        // Adjust nodes to target precision
        for (const auto& node : nodes) {
            node->adjust_precision(target_precision);
        }
    }

    size_t estimate_node_memory(const std::shared_ptr<Node<T>>& node) {
        // In practice, this would measure actual memory usage
        // For now, estimate based on precision level
        return (1 << node->current_precision_level()) * sizeof(T);
    }

    double estimate_node_activity(const std::shared_ptr<Node<T>>& node) {
        // In practice, this would use actual access metrics
        // For now, return a random activity level
        return 0.5; // 50% activity
    }

    bool would_fit_in_memory(
        const std::shared_ptr<Node<T>>& node,
        size_t new_precision_level,
        const MemoryStats& memory_stats
    ) {
        size_t current_memory = estimate_node_memory(node);
        size_t new_memory = (1 << new_precision_level) * sizeof(T);
        size_t memory_difference = new_memory - current_memory;

        return memory_stats.available_memory >= memory_difference;
    }

    double memory_threshold_;
    double activity_threshold_;
};

} // namespace flowgraph
