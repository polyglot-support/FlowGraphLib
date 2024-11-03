#pragma once
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include "node.hpp"
#include "edge.hpp"
#include "../async/task.hpp"

namespace flowgraph {

template<NodeValue T>
class Graph {
public:
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
        
        co_await node->compute();
        co_return;  // Added missing co_return
    }

    std::unordered_set<std::shared_ptr<Node<T>>> nodes_;
    std::unordered_set<std::shared_ptr<Edge<T>>> edges_;
};

} // namespace flowgraph
