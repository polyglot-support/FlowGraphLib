#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_set>
#include "cache_policy.hpp"

namespace flowgraph {

template<typename T>
class GraphCache {
private:
    std::unique_ptr<CachePolicy<T>> policy_;
    std::unordered_set<T> cache_;
    mutable std::mutex mutex_;

public:
    explicit GraphCache(std::unique_ptr<CachePolicy<T>> policy = nullptr)
        : policy_(std::move(policy)) {}

    void store(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (cache_.find(value) != cache_.end()) {
            if (policy_) {
                policy_->on_access(value);
            }
            return;
        }

        if (policy_) {
            if (!policy_->should_cache(value)) {
                T victim = policy_->select_victim();
                cache_.erase(victim);
            }
            policy_->on_insert(value);
        }

        cache_.insert(value);
    }

    std::optional<T> get(const T& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (policy_) {
                policy_->on_access(key);
            }
            return *it;
        }
        
        return std::nullopt;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }
};

} // namespace flowgraph
