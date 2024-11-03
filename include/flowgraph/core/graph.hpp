#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include "node.hpp"
#include "edge.hpp"
#include "../async/task.hpp"
#include "../cache/graph_cache.hpp"
#include "../cache/cache_policy.hpp"

namespace flowgraph {

template<NodeValue T>
class Graph {
public:
    explicit Graph(std::unique_ptr<CachePolicy<T>> cache_policy = nullptr)
        : cache_(std::make_unique<GraphCache<T>>(std::move(cache_policy))) {}

    void add_node(std::shared_ptr<Node<T>> node) {
        nodes_.insert(node);
    }

    void add_edge(std::shared_ptr<Edge<T>> edge) {
        if (!has_cycle(edge)) {
            edges_.insert(edge);
        } else {
            throw std::runtime_error("Adding edge would create a cycle");
        }
    }

    void set_cache_policy(std::unique_ptr<CachePolicy<T>> policy) {
        cache_ = std::make_unique<GraphCache<T>>(std::move(policy));
    }

    // Async execution of the entire graph
    Task<void> execute() {
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
};

} // namespace flowgraph
