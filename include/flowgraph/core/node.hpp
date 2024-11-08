#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include "concepts.hpp"
#include "fractal_tree_node.hpp"
#include "error_state.hpp"
#include "../async/task.hpp"

namespace flowgraph {

template<typename T>
class Node {
    static_assert(NodeValue<T>, "Type T must satisfy NodeValue concept");

public:
    using value_type = T;
    using callback_type = std::function<void(const ComputeResult<T>&)>;

    Node(std::string name, size_t max_precision_depth = 8, double compression_threshold = 0.001)
        : name_(std::move(name))
        , value_storage_(max_precision_depth, compression_threshold)
        , current_precision_level_(0)
        , min_precision_level_(0)
        , max_precision_level_(max_precision_depth) {}

    // Coroutine-based computation with precision level
    [[nodiscard]] Task<ComputeResult<T>> compute(size_t precision_level = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            // Validate precision level
            if (precision_level > max_precision_level_) {
                auto error = ErrorState::precision_error(
                    "Requested precision level exceeds maximum supported level"
                );
                error.set_source_node(name_);
                co_return ComputeResult<T>(std::move(error));
            }

            current_precision_level_ = precision_level;

            // Check fractal tree storage first
            if (auto cached = value_storage_.get(precision_level); cached.has_value()) {
                co_return ComputeResult<T>(cached.value());
            }

            // Perform computation at requested precision level
            auto result = co_await compute_impl(precision_level);
            
            if (result.has_error()) {
                // Add this node to error propagation path
                auto error = result.error();
                error.add_propagation_path(name_);
                co_return ComputeResult<T>(std::move(error));
            }

            // Store result in fractal tree
            value_storage_.store(result.value(), precision_level);
            
            // Notify callbacks
            for (const auto& callback : completion_callbacks_) {
                callback(result);
            }

            // Periodically merge updates in the fractal tree
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

    void add_completion_callback(callback_type callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        completion_callbacks_.push_back(std::move(callback));
    }

    const std::string& name() const { 
        return name_; 
    }

    // Precision level management
    size_t current_precision_level() const { 
        return current_precision_level_; 
    }

    size_t max_precision_level() const { 
        return max_precision_level_; 
    }

    size_t min_precision_level() const {
        return min_precision_level_;
    }

    void set_precision_range(size_t min_level, size_t max_level) {
        if (max_level > value_storage_.max_depth()) {
            throw std::invalid_argument("Maximum precision level exceeds storage capacity");
        }
        if (min_level > max_level) {
            throw std::invalid_argument("Minimum precision level cannot exceed maximum level");
        }
        min_precision_level_ = min_level;
        max_precision_level_ = max_level;
    }

    // Dynamic precision scaling
    void adjust_precision(size_t target_level) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (target_level >= min_precision_level_ && target_level <= max_precision_level_) {
            current_precision_level_ = target_level;
        }
    }

    // Force merge of pending updates
    void merge_updates() {
        std::lock_guard<std::mutex> lock(mutex_);
        value_storage_.merge_all();
    }

protected:
    // Override this to implement actual computation logic
    virtual Task<ComputeResult<T>> compute_impl(size_t precision_level) = 0;

private:
    bool should_merge_updates() {
        static const size_t MERGE_INTERVAL = 10;
        return (++computation_count_ % MERGE_INTERVAL) == 0;
    }

    std::string name_;
    std::mutex mutex_;
    FractalTreeNode<T> value_storage_;
    std::vector<callback_type> completion_callbacks_;
    size_t current_precision_level_;
    size_t min_precision_level_;
    size_t max_precision_level_;
    size_t computation_count_ = 0;
};

} // namespace flowgraph
