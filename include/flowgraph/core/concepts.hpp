#pragma once
#include <type_traits>
#include <concepts>

namespace flowgraph {

// Core value type requirements
template<typename T>
concept NodeValue = std::is_default_constructible_v<T> &&
                   std::is_copy_constructible_v<T> &&
                   std::is_move_constructible_v<T> &&
                   std::is_copy_assignable_v<T> &&
                   std::is_move_assignable_v<T>;

// Computation requirements
template<typename T>
concept Computable = requires(T a) {
    { a.compute() } -> std::convertible_to<bool>;
};

// Serialization requirements
template<typename T>
concept Serializable = requires(T a) {
    { a.serialize() } -> std::convertible_to<std::string>;
    { T::deserialize(std::string{}) } -> std::convertible_to<T>;
};

// Optimization requirements
template<typename T>
concept Optimizable = requires(T a) {
    { a.optimize() } -> std::same_as<void>;
    { a.can_optimize() } -> std::same_as<bool>;
};

} // namespace flowgraph
