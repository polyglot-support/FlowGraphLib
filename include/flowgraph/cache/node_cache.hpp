#pragma once
#include <optional>
#include <mutex>

namespace flowgraph {

template<typename T>
class NodeCache {
public:
    void store(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cached_value_ = value;
    }

    std::optional<T> get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cached_value_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cached_value_.reset();
    }

private:
    mutable std::mutex mutex_;
    std::optional<T> cached_value_;
};

} // namespace flowgraph
