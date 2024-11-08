#pragma once
#include <unordered_map>
#include <queue>
#include "../core/node.hpp"
#include "optimization_pass.hpp"

namespace flowgraph {

template<typename T>
class PrecisionOptimizationPass : public OptimizationPass<T> {
public:
    PrecisionOptimizationPass(double error_threshold = 0.001)
        : error_threshold_(error_threshold) {}

    void optimize(Graph<T>& graph) override {
        // Build node dependency map and identify output nodes
        auto nodes = graph.get_nodes();
        auto output_nodes = graph.get_output_nodes();

        // Initialize precision requirements map
        std::unordered_map<std::shared_ptr<Node<T>>, size_t> precision_requirements;
        
        // Start with output nodes and work backwards
        std::queue<std::shared_ptr<Node<T>>> node_queue;
        for (const auto& output_node : output_nodes) {
            node_queue.push(output_node);
            // Output nodes maintain their current precision level
            precision_requirements[output_node] = output_node->current_precision_level();
        }

        // Propagate precision requirements backwards through the graph
        while (!node_queue.empty()) {
            auto current_node = node_queue.front();
            node_queue.pop();

            size_t current_precision = precision_requirements[current_node];
            
            // Get incoming edges (dependencies)
            auto incoming_edges = graph.get_incoming_edges(current_node);
            for (const auto& edge : incoming_edges) {
                auto dependency = edge->from();
                
                // Calculate required precision for dependency
                size_t required_precision = calculate_required_precision(
                    current_precision,
                    dependency,
                    current_node
                );

                // Update precision requirement if higher precision is needed
                auto it = precision_requirements.find(dependency);
                if (it == precision_requirements.end() || required_precision > it->second) {
                    precision_requirements[dependency] = required_precision;
                    node_queue.push(dependency);
                }
            }
        }

        // Apply optimized precision levels
        for (const auto& [node, precision] : precision_requirements) {
            try {
                // Ensure precision is within node's supported range
                size_t min_level = node->min_precision_level();
                size_t max_level = node->max_precision_level();
                size_t optimized_precision = std::clamp(precision, min_level, max_level);
                
                // Update node's precision level
                node->adjust_precision(optimized_precision);
            }
            catch (const std::exception& e) {
                // Log error but continue with other nodes
                // In practice, you might want to use a proper logging system
                std::cerr << "Error optimizing precision for node " << node->name() 
                         << ": " << e.what() << std::endl;
            }
        }
    }

private:
    size_t calculate_required_precision(
        size_t target_precision,
        const std::shared_ptr<Node<T>>& dependency,
        const std::shared_ptr<Node<T>>& dependent
    ) {
        // Base case: maintain same precision
        size_t required_precision = target_precision;

        // Adjust based on error propagation history
        auto recent_errors = analyze_error_history(dependency, dependent);
        if (recent_errors > error_threshold_) {
            // Increase precision if errors are high
            required_precision = std::min(
                required_precision + 1,
                dependency->max_precision_level()
            );
        }
        else if (recent_errors < error_threshold_ / 2) {
            // Decrease precision if errors are low
            required_precision = std::max(
                required_precision > 0 ? required_precision - 1 : 0,
                dependency->min_precision_level()
            );
        }

        return required_precision;
    }

    double analyze_error_history(
        const std::shared_ptr<Node<T>>& dependency,
        const std::shared_ptr<Node<T>>& dependent
    ) {
        // In a real implementation, this would analyze actual error history
        // For now, return a default value
        return error_threshold_ / 2;
    }

    double error_threshold_;
};

} // namespace flowgraph
