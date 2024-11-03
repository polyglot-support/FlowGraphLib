#pragma once
#include <cstddef>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

namespace flowgraph {

// Base cache policy interface
template<typename T>
struct CachePolicy {
    virtual ~CachePolicy() = default;
    virtual bool should_cache(const T& value) const = 0;
    virtual void on_access(const T& value) = 0;
    virtual void on_insert(const T& value) = 0;
    virtual T select_victim() = 0;
    virtual std::size_t max_size() const = 0;
};

// LRU (Least Recently Used) cache policy
template<typename T>
class LRUCachePolicy : public CachePolicy<T> {
public:
    explicit LRUCachePolicy(std::size_t capacity) : capacity_(capacity) {}

    bool should_cache(const T&) const override {
        return access_list_.size() < capacity_;
    }

    void on_access(const T& value) override {
        auto it = item_map_.find(value);
        if (it != item_map_.end()) {
            access_list_.erase(it->second);
            access_list_.push_front(value);
            it->second = access_list_.begin();
        }
    }

    void on_insert(const T& value) override {
        access_list_.push_front(value);
        item_map_[value] = access_list_.begin();
    }

    T select_victim() override {
        if (access_list_.empty()) {
            throw std::runtime_error("Cache is empty");
        }
        T victim = access_list_.back();
        access_list_.pop_back();
        item_map_.erase(victim);
        return victim;
    }

    std::size_t max_size() const override { return capacity_; }

private:
    std::size_t capacity_;
    std::list<T> access_list_;
    std::unordered_map<T, typename std::list<T>::iterator> item_map_;
};

// LFU (Least Frequently Used) cache policy
template<typename T>
class LFUCachePolicy : public CachePolicy<T> {
public:
    explicit LFUCachePolicy(std::size_t capacity) : capacity_(capacity) {}

    bool should_cache(const T&) const override {
        return freq_map_.size() < capacity_;
    }

    void on_access(const T& value) override {
        if (auto it = freq_map_.find(value); it != freq_map_.end()) {
            ++it->second;
        }
    }

    void on_insert(const T& value) override {
        freq_map_[value] = 1;
    }

    T select_victim() override {
        if (freq_map_.empty()) {
            throw std::runtime_error("Cache is empty");
        }
        auto min_it = std::min_element(
            freq_map_.begin(), freq_map_.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        T victim = min_it->first;
        freq_map_.erase(min_it);
        return victim;
    }

    std::size_t max_size() const override { return capacity_; }

private:
    std::size_t capacity_;
    std::unordered_map<T, std::size_t> freq_map_;
};

} // namespace flowgraph
