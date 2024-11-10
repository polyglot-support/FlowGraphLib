#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <mutex>
#include "concepts.hpp"
#include "core.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "../async/task.hpp"
#include "../async/thread_pool.hpp"
#include "../async/future_helpers.hpp"
#include "../cache/graph_cache.hpp"
#include "../cache/cache_policy.hpp"
#include "../optimization/optimization_pass.hpp"

namespace flowgraph {

template<typename T>
    requires NodeValue<T>
class Graph final : public GraphBase {
public:
    using value_type = T;
    using compute_result_type = ComputeResult<T>;
    using task_type = Task<compute_result_type>;
    using node_type = Node<T>;
    using edge_type = Edge<T>;

    explicit Graph(std::unique_ptr<CachePolicy<T>> cache_policy = nullptr,
                  std::shared_ptr<ThreadPool> thread_pool = nullptr)
        : cache_(std::make_unique<GraphCache<T>>(std::move(cache_policy))),
          thread_pool_(thread_pool ? thread_pool : std::make_shared<ThreadPool>()) {}

    void add_optimization_pass(std::unique_ptr<OptimizationPass<T>> pass) {
        optimization_passes_.push_back(std::move(pass));
    }

    void optimize() {
        for (const auto& pass : optimization_passes_) {
            pass->optimize(*this);
        }
    }

    void add_node(std::shared_ptr<node_type> node) {
        nodes_.insert(node);
        node->set_parent_graph(this);
        node->add_completion_callback([this](const compute_result_type& result) {
            if (result.has_error()) {
                std::lock_guard<std::mutex> lock(error_mutex_);
                node_errors_[result.error().source_node().value_or("")] = result.error();
            }
        });
    }

    void remove_node(std::shared_ptr<node_type> node) {
        // Remove all edges connected to this node
        auto edges_copy = edges_;
        for (const auto& edge : edges_copy) {
            if (edge->from() == node || edge->to() == node) {
                edges_.erase(edge);
            }
        }
        node->set_parent_graph(nullptr);
        nodes_.erase(node);
        {
            std::lock_guard<std::mutex> lock(error_mutex_);
            node_errors_.erase(node->name());
        }
    }

    void add_edge(std::shared_ptr<edge_type> edge) {
        if (!has_cycle(edge)) {
            edges_.insert(edge);
        } else {
            throw std::runtime_error("Adding edge would create a cycle");
        }
    }

    void set_thread_pool(std::shared_ptr<ThreadPool> thread_pool) {
        thread_pool_ = thread_pool;
    }

    std::shared_ptr<ThreadPool> get_thread_pool() const {
        return thread_pool_;
    }

    // Accessor methods
    const std::unordered_set<std::shared_ptr<node_type>>& get_nodes() const {
        return nodes_;
    }

    std::vector<std::shared_ptr<edge_type>> get_incoming_edges(const std::shared_ptr<node_type>& node) const {
        std::vector<std::shared_ptr<edge_type>> incoming;
        for (const auto& edge : edges_) {
            if (edge->to() == node) {
                incoming.push_back(edge);
            }
        }
        return incoming;
    }

    std::vector<std::shared_ptr<edge_type>> get_outgoing_edges(const std::shared_ptr<node_type>& node) const {
        std::vector<std::shared_ptr<edge_type>> outgoing;
        for (const auto& edge : edges_) {
            if (edge->from() == node) {
                outgoing.push_back(edge);
            }
        }
        return outgoing;
    }

    std::unordered_set<std::shared_ptr<node_type>> get_output_nodes() const {
        std::unordered_set<std::shared_ptr<node_type>> outputs;
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

    Task<void> execute() {
        // Clear previous errors
        {
            std::lock_guard<std::mutex> lock(error_mutex_);
            node_errors_.clear();
        }

        std::vector<std::pair<std::shared_ptr<node_type>, task_type>> tasks;
        std::unordered_set<std::shared_ptr<node_type>> visited;
        
        // Schedule independent nodes for parallel execution
        for (const auto& node : nodes_) {
            if (get_incoming_edges(node).empty()) {
                tasks.emplace_back(node, execute_node_async(node, visited));
            }
        }

        // Wait for all tasks to complete
        for (auto& [node, task] : tasks) {
            auto result = co_await task;
            if (result.has_error()) {
                std::lock_guard<std::mutex> lock(error_mutex_);
                auto error = result.error();
                if (!error.source_node()) {
                    error.set_source_node(node->name());
                }
                node_errors_[error.source_node().value()] = std::move(error);
            }
        }

        // Propagate errors through the graph
        bool changed;
        do {
            changed = false;
            for (const auto& node : nodes_) {
                auto incoming = get_incoming_edges(node);
                for (const auto& edge : incoming) {
                    auto from_node = std::dynamic_pointer_cast<node_type>(edge->from());
                    if (from_node) {
                        auto error = get_node_error(from_node->name());
                        if (error) {
                            std::lock_guard<std::mutex> lock(error_mutex_);
                            auto it = node_errors_.find(node->name());
                            if (it == node_errors_.end()) {
                                auto propagated_error = *error;
                                propagated_error.add_propagation_path(node->name());
                                node_errors_[node->name()] = std::move(propagated_error);
                                changed = true;
                            }
                        }
                    }
                }
            }
        } while (changed);

        co_return;
    }

    std::optional<ErrorState> get_node_error(const std::string& node_name) const override {
        std::lock_guard<std::mutex> lock(error_mutex_);
        auto it = node_errors_.find(node_name);
        if (it != node_errors_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::shared_ptr<node_type> find_node_by_name(const std::string& name) const {
        for (const auto& node : nodes_) {
            if (node->name() == name) {
                return node;
            }
        }
        return nullptr;
    }

    bool has_cycle(std::shared_ptr<edge_type> new_edge) {
        std::unordered_set<std::shared_ptr<node_type>> visited;
        auto to_node = std::dynamic_pointer_cast<node_type>(new_edge->to());
        return detect_cycle(to_node, visited);
    }

    bool detect_cycle(std::shared_ptr<node_type> node,
                     std::unordered_set<std::shared_ptr<node_type>>& visited) {
        if (visited.find(node) != visited.end()) {
            return true;
        }

        visited.insert(node);
        
        for (const auto& edge : edges_) {
            if (edge->from() == node) {
                auto to_node = std::dynamic_pointer_cast<node_type>(edge->to());
                if (detect_cycle(to_node, visited)) {
                    return true;
                }
            }
        }

        visited.erase(node);
        return false;
    }

    Task<compute_result_type> execute_node_async(
        std::shared_ptr<node_type> node,
        std::unordered_set<std::shared_ptr<node_type>>& visited
    ) {
        visited.insert(node);

        // Execute dependencies first
        std::vector<task_type> dep_tasks;
        for (const auto& edge : edges_) {
            if (edge->to() == node && visited.find(std::dynamic_pointer_cast<node_type>(edge->from())) == visited.end()) {
                dep_tasks.push_back(execute_node_async(std::dynamic_pointer_cast<node_type>(edge->from()), visited));
            }
        }

        // Wait for dependencies and check for errors
        for (auto& task : dep_tasks) {
            auto result = co_await task;
            if (result.has_error()) {
                // Store and propagate the error
                auto error = result.error();
                error.add_propagation_path(node->name());
                
                std::lock_guard<std::mutex> lock(error_mutex_);
                if (!error.source_node()) {
                    error.set_source_node(node->name());
                }
                node_errors_[error.source_node().value()] = error;
                node_errors_[node->name()] = error;  // Store error for this node too
                
                co_return compute_result_type(std::move(error));
            }
        }

        // Check for existing errors that might affect this node
        {
            std::lock_guard<std::mutex> lock(error_mutex_);
            if (!node_errors_.empty()) {
                // Take the first error and propagate it
                auto error = node_errors_.begin()->second;
                error.add_propagation_path(node->name());
                node_errors_[node->name()] = error;  // Store error for this node
                co_return compute_result_type(std::move(error));
            }
        }

        // Execute node computation
        auto result = co_await node->compute();
        
        // Handle errors from computation
        if (result.has_error()) {
            std::lock_guard<std::mutex> lock(error_mutex_);
            auto error = result.error();
            if (!error.source_node()) {
                error.set_source_node(node->name());
            }
            node_errors_[error.source_node().value()] = error;
            node_errors_[node->name()] = error;  // Store error for this node
        }
        // Handle caching for successful computations
        else if (cache_) {
            auto value = result.value();
            if (auto cached = cache_->get(value); !cached.has_value()) {
                cache_->store(value);
            }
        }
        
        co_return result;
    }

    std::unordered_set<std::shared_ptr<node_type>> nodes_;
    std::unordered_set<std::shared_ptr<edge_type>> edges_;
    std::unique_ptr<GraphCache<T>> cache_;
    std::shared_ptr<ThreadPool> thread_pool_;
    mutable std::mutex error_mutex_;
    std::unordered_map<std::string, ErrorState> node_errors_;
    std::vector<std::unique_ptr<OptimizationPass<T>>> optimization_passes_;
};

} // namespace flowgraph
