#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include "concepts.hpp"
#include "../cache/node_cache.hpp"
#include "../async/task.hpp"

namespace flowgraph {

template<NodeValue T>
class Node {
public:
    using value_type = T;
    using callback_type = std::function<void(const T&)>;

    Node(std::string name) : name_(std::move(name)) {}

    // Coroutine-based computation
    [[nodiscard]] Task<T> compute() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check cache first
        if (auto cached = cache_.get(); cached.has_value()) {
            co_return cached.value();
        }

        // Perform computation
        T result = co_await compute_impl();
        
        // Cache result
        cache_.store(result);
        
        // Notify callbacks
        for (const auto& callback : completion_callbacks_) {
            callback(result);
        }

        co_return result;
    }

    void add_completion_callback(callback_type callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        completion_callbacks_.push_back(std::move(callback));
    }

    const std::string& name() const { return name_; }

protected:
    virtual Task<T> compute_impl() = 0;

private:
    std::string name_;
    std::mutex mutex_;
    NodeCache<T> cache_;
    std::vector<callback_type> completion_callbacks_;
};

} // namespace flowgraph
