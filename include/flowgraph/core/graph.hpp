#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include "node.hpp"
#include "edge.hpp"
#include "../async/task.hpp"
#include "../cache/graph_cache.hpp"
#include "../cache/cache_policy.hpp"
#include "../optimization/optimization_pass.hpp"

namespace flowgraph {

template<NodeValue T>
class Graph {
public:
    explicit Graph(std::unique_ptr<CachePolicy<T>> cache_policy = nullptr)
        : cache_(std::make_unique<GraphCache<T>>(std::move(cache_policy))) {}

    void add_node(std::shared_ptr<Node<T>> node) {
        nodes_.insert(node);
    }

    void remove_node(std::shared_ptr<Node<T>> node) {
        // Remove all edges connected to this node
        auto edges_copy = edges_;
        for (const auto& edge : edges_copy) {
            if (edge->from() == node || edge->to() == node) {
                edges_.erase(edge);
            }
        }
        nodes_.erase(node);
    }

    void add_edge(std::shared_ptr<Edge<T>> edge) {
        if (!has_cycle(edge)) {
            edges_.insert(edge);
        } else {
            throw std::runtime_error("Adding edge would create a cycle");
        }
    }

    void add_optimization_pass(std::unique_ptr<OptimizationPass<T>> pass) {
        optimization_passes_.push_back(std::move(pass));
    }

    void optimize() {
        for (const auto& pass : optimization_passes_) {
            pass->optimize(*this);
        }
    }

    // Accessor methods for optimization passes
    const std::unordered_set<std::shared_ptr<Node<T>>>& get_nodes() const {
        return nodes_;
    }

    std::vector<std::shared_ptr<Edge<T>>> get_incoming_edges(const std::shared_ptr<Node<T>>& node) const {
        std::vector<std::shared_ptr<Edge<T>>> incoming;
        for (const auto& edge : edges_) {
            if (edge->to() == node) {
                incoming.push_back(edge);
            }
        }
        return incoming;
    }

    std::vector<std::shared_ptr<Edge<T>>> get_outgoing_edges(const std::shared_ptr<Node<T>>& node) const {
        std::vector<std::shared_ptr<Edge<T>>> outgoing;
        for (const auto& edge : edges_) {
            if (edge->from() == node) {
                outgoing.push_back(edge);
            }
        }
        return outgoing;
    }

    std::unordered_set<std::shared_ptr<Node<T>>> get_output_nodes() const {
        std::unordered_set<std::shared_ptr<Node<T>>> outputs;
        for (const auto& node : nodes_) {
            if (get_outgoing_edges(node).empty()) {
                outputs.insert(node);
            }
        }
        return outputs;
    }

    void set_cache_policy(std::unique_ptr<CachePolicy<T>> policy) {
        cache_ = std::make_unique<GraphCache<T>>(std::move(policy));
    }

    // Async execution of the entire graph
    Task<void> execute() {
        // Run optimization passes before execution
        optimize();

        std::unordered_set<std::shared_ptr<Node<T>>> visited;
        for (const auto& node : nodes_) {
            if (visited.find(node) == visited.end()) {
                co_await execute_node(node, visited);
            }
        }
        co_return;
    }

private:
    bool has_cycle(std::shared_ptr<Edge<T>> new_edge) {
        std::unordered_set<std::shared_ptr<Node<T>>> visited;
        return detect_cycle(new_edge->to(), visited);
    }

    bool detect_cycle(std::shared_ptr<Node<T>> node,
                     std::unordered_set<std::shared_ptr<Node<T>>>& visited) {
        if (visited.find(node) != visited.end()) {
            return true;
        }

        visited.insert(node);
        
        for (const auto& edge : edges_) {
            if (edge->from() == node) {
                if (detect_cycle(edge->to(), visited)) {
                    return true;
                }
            }
        }

        visited.erase(node);
        return false;
    }

    Task<void> execute_node(std::shared_ptr<Node<T>> node,
                          std::unordered_set<std::shared_ptr<Node<T>>>& visited) {
        visited.insert(node);
        
        // Execute dependencies first
        for (const auto& edge : edges_) {
            if (edge->to() == node && visited.find(edge->from()) == visited.end()) {
                co_await execute_node(edge->from(), visited);
            }
        }

        // Execute node and handle caching
        T result = co_await node->compute();
        
        if (cache_) {
            if (auto cached = cache_->get(result); !cached.has_value()) {
                cache_->store(result);
            }
        }
        
        co_return;
    }

    std::unordered_set<std::shared_ptr<Node<T>>> nodes_;
    std::unordered_set<std::shared_ptr<Edge<T>>> edges_;
    std::unique_ptr<GraphCache<T>> cache_;
    std::vector<std::unique_ptr<OptimizationPass<T>>> optimization_passes_;
};

} // namespace flowgraph
