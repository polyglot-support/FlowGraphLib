#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <vector>
#include <cmath>
#include "concepts.hpp"

namespace flowgraph {

// Forward declaration with matching requires clause
template<typename T>
    requires NodeValue<T>
class FractalTreeNode;

// Struct to hold pending updates
template<typename T>
struct PendingUpdate {
    T value;
    double weight;
};

template<typename T>
    requires NodeValue<T>
class FractalTreeNode {
public:
    using value_type = T;
    
    FractalTreeNode(size_t max_depth = 8, double compression_threshold = 0.001)
        : max_depth_(max_depth), compression_threshold_(compression_threshold) {}

    // Store a value at a specific precision level
    void store(const T& value, size_t precision_level) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (precision_level > max_depth_) {
            precision_level = max_depth_;
        }

        // Add to pending updates at the specified precision level
        auto& updates = pending_updates_[precision_level];
        updates.push_back({value, 1.0});

        // Trigger merge if we have too many pending updates
        if (updates.size() >= merge_threshold_) {
            merge_level(precision_level);
        }
    }

    // Get value at a specific precision level
    std::optional<T> get(size_t precision_level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (precision_level > max_depth_) {
            precision_level = max_depth_;
        }

        // Check if we have an absolute value at this level
        auto it = absolute_values_.find(precision_level);
        if (it != absolute_values_.end()) {
            return it->second;
        }

        // If not, try to find the closest available level
        for (size_t level = precision_level; level > 0; --level) {
            it = absolute_values_.find(level - 1);
            if (it != absolute_values_.end()) {
                return expand_value(it->second, level - 1, precision_level);
            }
        }

        return std::nullopt;
    }

    // Merge all pending updates into absolute values
    void merge_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<size_t> levels;
        for (const auto& [level, _] : pending_updates_) {
            levels.push_back(level);
        }
        for (auto level : levels) {
            merge_level(level);
        }
        compress_tree();
    }

    // Get the maximum supported precision level
    size_t max_depth() const { return max_depth_; }

private:
    // Merge pending updates at a specific level
    void merge_level(size_t level) {
        auto updates_it = pending_updates_.find(level);
        if (updates_it == pending_updates_.end() || updates_it->second.empty()) {
            return;
        }

        // Initialize merged_value with the first update
        T merged_value = updates_it->second[0].value;
        double total_weight = updates_it->second[0].weight;

        // Weighted average of remaining updates
        for (size_t i = 1; i < updates_it->second.size(); ++i) {
            const auto& update = updates_it->second[i];
            // For arithmetic types, use weighted average
            if constexpr (std::is_arithmetic_v<T>) {
                double new_weight = total_weight + update.weight;
                merged_value = static_cast<T>(
                    (merged_value * total_weight + update.value * update.weight) / new_weight
                );
                total_weight = new_weight;
            } else {
                // For non-arithmetic types, just use the latest value
                merged_value = update.value;
            }
        }

        // Update absolute value
        if (auto it = absolute_values_.find(level); it != absolute_values_.end()) {
            // For arithmetic types, use exponential moving average
            if constexpr (std::is_arithmetic_v<T>) {
                it->second = static_cast<T>(it->second * 0.7 + merged_value * 0.3);
            } else {
                // For non-arithmetic types, just use the latest value
                it->second = merged_value;
            }
        } else {
            absolute_values_[level] = merged_value;
        }

        // Clear pending updates
        updates_it->second.clear();
    }

    // Compress tree by removing redundant levels
    void compress_tree() {
        std::vector<size_t> levels_to_remove;

        for (const auto& [level, value] : absolute_values_) {
            if (level > 0) {
                auto lower_level = level - 1;
                if (auto it = absolute_values_.find(lower_level); it != absolute_values_.end()) {
                    // Check if the difference between levels is below threshold
                    if (difference(value, it->second) < compression_threshold_) {
                        levels_to_remove.push_back(level);
                    }
                }
            }
        }

        for (auto level : levels_to_remove) {
            absolute_values_.erase(level);
        }
    }

    // Expand a value from one precision level to another
    T expand_value(const T& value, size_t from_level, size_t to_level) const {
        if constexpr (std::is_arithmetic_v<T>) {
            // For arithmetic types, adjust precision based on level difference
            size_t level_diff = to_level - from_level;
            double scale = std::pow(10.0, static_cast<double>(level_diff));
            return static_cast<T>(std::round(static_cast<double>(value) * scale) / scale);
        } else {
            return value;
        }
    }

    // Calculate difference between two values (type-dependent)
    double difference(const T& a, const T& b) const {
        if constexpr (std::is_arithmetic_v<T>) {
            return std::abs(static_cast<double>(a) - static_cast<double>(b));
        } else {
            return a == b ? 0.0 : 1.0;
        }
    }

    size_t max_depth_;
    double compression_threshold_;
    static constexpr size_t merge_threshold_ = 10;
    
    mutable std::mutex mutex_;
    mutable std::unordered_map<size_t, T> absolute_values_;
    mutable std::unordered_map<size_t, std::vector<PendingUpdate<T>>> pending_updates_;
};

} // namespace flowgraph
