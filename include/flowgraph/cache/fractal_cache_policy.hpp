#pragma once
#include <unordered_map>
#include <list>
#include <optional>
#include "cache_policy.hpp"
#include "../core/fractal_tree_node.hpp"

namespace flowgraph {

template<typename T>
class FractalCachePolicy : public CachePolicy<T> {
public:
    FractalCachePolicy(
        size_t max_entries_per_level = 1000,
        double compression_threshold = 0.001
    )
        : max_entries_per_level_(max_entries_per_level)
        , compression_threshold_(compression_threshold) {}

    bool should_cache(const T& value) override {
        // Always cache at appropriate precision level
        return true;
    }

    void on_access(const T& value) override {
        // Update access history for the value's precision level
        auto precision_level = determine_precision_level(value);
        update_access_history(precision_level);
    }

    void on_insert(const T& value) override {
        auto precision_level = determine_precision_level(value);
        
        // Check if we need to compress entries at this level
        if (entries_at_level_[precision_level] >= max_entries_per_level_) {
            compress_level(precision_level);
        }

        entries_at_level_[precision_level]++;
    }

    T select_victim() override {
        // Find least recently used precision level with most entries
        size_t target_level = find_compression_candidate();
        
        // Get least recently used value at that level
        if (auto victim = get_lru_value(target_level)) {
            entries_at_level_[target_level]--;
            return *victim;
        }

        // If no suitable victim found, return default value
        return T{};
    }

    size_t max_size() const override {
        return max_entries_per_level_ * MAX_PRECISION_LEVELS;
    }

private:
    static constexpr size_t MAX_PRECISION_LEVELS = 32;

    size_t determine_precision_level(const T& value) {
        // In practice, this would analyze the value's precision
        // For now, use a simple heuristic based on value complexity
        return estimate_value_precision(value);
    }

    void update_access_history(size_t precision_level) {
        // Move precision level to front of recently used list
        auto it = std::find(level_access_history_.begin(), 
                           level_access_history_.end(), 
                           precision_level);
        
        if (it != level_access_history_.end()) {
            level_access_history_.erase(it);
        }
        
        level_access_history_.push_front(precision_level);
    }

    void compress_level(size_t level) {
        if (level == 0) return; // Can't compress base level

        // Merge entries into lower precision level
        size_t target_level = level - 1;
        
        // In practice, this would perform actual value compression
        entries_at_level_[level] = entries_at_level_[level] / 2;
        entries_at_level_[target_level] += entries_at_level_[level] / 2;
    }

    size_t find_compression_candidate() {
        // Start with least recently used level
        for (auto level : level_access_history_) {
            if (entries_at_level_[level] > max_entries_per_level_ / 2) {
                return level;
            }
        }
        
        // Default to highest used level
        return find_highest_used_level();
    }

    std::optional<T> get_lru_value(size_t level) {
        // In practice, this would return actual LRU value at level
        // For now, return empty to indicate no victim found
        return std::nullopt;
    }

    size_t find_highest_used_level() {
        for (size_t level = MAX_PRECISION_LEVELS - 1; level > 0; --level) {
            if (entries_at_level_[level] > 0) {
                return level;
            }
        }
        return 0;
    }

    size_t estimate_value_precision(const T& value) {
        // In practice, this would analyze the value's structure
        // For now, return a default precision level
        return 4; // Mid-range precision
    }

    size_t max_entries_per_level_;
    double compression_threshold_;
    std::unordered_map<size_t, size_t> entries_at_level_;
    std::list<size_t> level_access_history_;
};

} // namespace flowgraph
