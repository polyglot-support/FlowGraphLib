#pragma once
#include <concepts>
#include <type_traits>

namespace flowgraph {

// Concept for types that can be used as node values
template<typename T>
concept NodeValue = std::movable<T> && std::copyable<T>;

// Concept for callback functions
template<typename F, typename T>
concept NodeCallback = std::invocable<F, T>;

// Concept for cache policy
template<typename T>
concept CachePolicy = requires(T policy) {
    { policy.should_cache() } -> std::same_as<bool>;
    { policy.evict() } -> std::same_as<void>;
};

} // namespace flowgraph
