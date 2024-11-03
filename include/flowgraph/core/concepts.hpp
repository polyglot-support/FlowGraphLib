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

// Concept for cache policy interface validation
template<typename T, typename Policy>
concept CachePolicyInterface = requires(Policy& policy, const T& value) {
    { policy.should_cache(value) } -> std::same_as<bool>;
    { policy.on_access(value) } -> std::same_as<void>;
    { policy.on_insert(value) } -> std::same_as<void>;
    { policy.select_victim() } -> std::same_as<T>;
    { policy.max_size() } -> std::same_as<std::size_t>;
};

} // namespace flowgraph
