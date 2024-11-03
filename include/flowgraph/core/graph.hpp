#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "node.hpp"
#include "edge.hpp"
#include "../async/task.hpp"
#include "../async/thread_pool.hpp"
#include "../async/future_helpers.hpp"
#include "../cache/graph_cache.hpp"
#include "../cache/cache_policy.hpp"
#include "../optimization/optimization_pass.hpp"
#include "../serialization/serialization.hpp"

namespace flowgraph {

template<NodeValue T>
class Graph {
public:
    explicit Graph(std::unique_ptr<CachePolicy<T>> cache_policy = nullptr,
                  std::shared_ptr<ThreadPool> thread_pool = nullptr)
        : cache_(std::make_unique<GraphCache<T>>(std::move(cache_policy))),
          thread_pool_(thread_pool ? thread_pool : std::make_shared<ThreadPool>()) {}

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

    void set_thread_pool(std::shared_ptr<ThreadPool> thread_pool) {
        thread_pool_ = thread_pool;
    }

    std::shared_ptr<ThreadPool> get_thread_pool() const {
        return thread_pool_;
    }

    // Serialization support
    nlohmann::json to_json() const {
        return serialize_graph(*this);
    }

    void from_json(const nlohmann::json& j, 
                  std::function<std::shared_ptr<Node<T>>(const std::string&)> node_factory) {
        // Clear existing state
        nodes_.clear();
        edges_.clear();

        // Deserialize nodes
        if (j.contains("nodes")) {
            for (const auto& node_json : j["nodes"]) {
                std::string name = node_json["name"];
                auto node = node_factory(name);
                deserialize_node(*node, node_json);
                add_node(node);
            }
        }

        // Deserialize edges
        if (j.contains("edges")) {
            for (const auto& edge_json : j["edges"]) {
                std::string from_name = edge_json["from"];
                std::string to_name = edge_json["to"];
                
                auto from_node = find_node_by_name(from_name);
                auto to_node = find_node_by_name(to_name);
                
                if (from_node && to_node) {
                    add_edge(std::make_shared<Edge<T>>(from_node, to_node));
                }
            }
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

        // Create a vector to store all node execution futures
        std::vector<std::future<T>> futures;
        std::unordered_set<std::shared_ptr<Node<T>>> visited;
        
        // Schedule independent nodes for parallel execution
        for (const auto& node : nodes_) {
            if (get_incoming_edges(node).empty()) {
                futures.push_back(execute_node_async(node, visited));
            }
        }

        // Wait for all futures to complete using our helper
        for (auto& future : futures) {
            co_await make_task_from_future(std::move(future));
        }

        co_return;
    }

private:
    std::shared_ptr<Node<T>> find_node_by_name(const std::string& name) const {
        for (const auto& node : nodes_) {
            if (node->name() == name) {
                return node;
            }
        }
        return nullptr;
    }

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

    std::future<T> execute_node_async(std::shared_ptr<Node<T>> node,
                                    std::unordered_set<std::shared_ptr<Node<T>>>& visited) {
        return thread_pool_->enqueue([this, node, &visited]() -> T {
            visited.insert(node);
            
            // Execute dependencies first
            std::vector<std::future<T>> dep_futures;
            for (const auto& edge : edges_) {
                if (edge->to() == node && visited.find(edge->from()) == visited.end()) {
                    dep_futures.push_back(execute_node_async(edge->from(), visited));
                }
            }

            // Wait for dependencies to complete
            for (auto& future : dep_futures) {
                future.wait();
            }

            // Execute node and handle caching
            auto task = node->compute();
            T result = task.await_resume();
            
            if (cache_) {
                if (auto cached = cache_->get(result); !cached.has_value()) {
                    cache_->store(result);
                }
            }
            
            return result;
        });
    }

    std::unordered_set<std::shared_ptr<Node<T>>> nodes_;
    std::unordered_set<std::shared_ptr<Edge<T>>> edges_;
    std::unique_ptr<GraphCache<T>> cache_;
    std::vector<std::unique_ptr<OptimizationPass<T>>> optimization_passes_;
    std::shared_ptr<ThreadPool> thread_pool_;
};

} // namespace flowgraph
