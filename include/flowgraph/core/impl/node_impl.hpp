#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include "../base.hpp"
#include "../concepts.hpp"
#include "../error_state.hpp"
#include "../fractal_tree_node.hpp"
#include "../../async/task.hpp"
#include "../compute_result.hpp"

namespace flowgraph {

template<typename T>
    requires NodeValue<T>
bool Node<T>::should_merge_updates() {
    static const size_t MERGE_INTERVAL = 10;
    return (++computation_count_ % MERGE_INTERVAL) == 0;
}

template<typename T>
    requires NodeValue<T>
Node<T>::Node(std::string name, size_t max_precision_depth, double compression_threshold)
    : name_(std::move(name))
    , value_storage_(max_precision_depth, compression_threshold)
    , current_precision_level_(0)
    , min_precision_level_(0)
    , max_precision_level_(max_precision_depth)
    , computation_count_(0)
    , parent_graph_(nullptr) {}

template<typename T>
    requires NodeValue<T>
const std::string& Node<T>::name() const { 
    return name_; 
}

template<typename T>
    requires NodeValue<T>
void Node<T>::set_parent_graph(IGraph* graph) {
    parent_graph_ = graph;
}

template<typename T>
    requires NodeValue<T>
size_t Node<T>::current_precision_level() const { 
    return current_precision_level_; 
}

template<typename T>
    requires NodeValue<T>
size_t Node<T>::max_precision_level() const { 
    return max_precision_level_; 
}

template<typename T>
    requires NodeValue<T>
size_t Node<T>::min_precision_level() const {
    return min_precision_level_;
}

template<typename T>
    requires NodeValue<T>
void Node<T>::set_precision_range(size_t min_level, size_t max_level) {
    if (max_level > value_storage_.max_depth()) {
        throw std::invalid_argument("Maximum precision level exceeds storage capacity");
    }
    if (min_level > max_level) {
        throw std::invalid_argument("Minimum precision level cannot exceed maximum level");
    }
    min_precision_level_ = min_level;
    max_precision_level_ = max_level;
}

template<typename T>
    requires NodeValue<T>
void Node<T>::adjust_precision(size_t target_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (target_level >= min_precision_level_ && target_level <= max_precision_level_) {
        current_precision_level_ = target_level;
    }
}

template<typename T>
    requires NodeValue<T>
void Node<T>::merge_updates() {
    std::lock_guard<std::mutex> lock(mutex_);
    value_storage_.merge_all();
}

template<typename T>
    requires NodeValue<T>
Task<ComputeResult<T>> Node<T>::compute(size_t precision_level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        if (parent_graph_) {
            if (auto error = parent_graph_->get_node_error(name_)) {
                co_return ComputeResult<T>(std::move(*error));
            }
        }

        if (precision_level > max_precision_level_) {
            auto error = ErrorState::precision_error(
                "Requested precision level exceeds maximum supported level"
            );
            error.set_source_node(name_);
            co_return ComputeResult<T>(std::move(error));
        }

        current_precision_level_ = precision_level;

        if (auto cached = value_storage_.get(precision_level); cached.has_value()) {
            co_return ComputeResult<T>(cached.value());
        }

        auto result = co_await compute_impl(precision_level);
        
        if (result.has_error()) {
            auto error = result.error();
            if (!error.source_node() || error.source_node().value() != name_) {
                error.add_propagation_path(name_);
            } else {
                error.set_source_node(name_);
            }
            co_return ComputeResult<T>(std::move(error));
        }

        value_storage_.store(result.value(), precision_level);
        
        for (const auto& callback : completion_callbacks_) {
            callback(result);
        }

        if (should_merge_updates()) {
            value_storage_.merge_all();
        }

        co_return result;
    }
    catch (const std::exception& e) {
        auto error = ErrorState::computation_error(e.what());
        error.set_source_node(name_);
        co_return ComputeResult<T>(std::move(error));
    }
}

template<typename T>
    requires NodeValue<T>
void Node<T>::add_completion_callback(callback_type callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    completion_callbacks_.push_back(std::move(callback));
}

} // namespace flowgraph
