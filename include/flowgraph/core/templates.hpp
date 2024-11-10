#pragma once
#include "concepts.hpp"

namespace flowgraph {

// Forward declarations of template classes
template<typename T>
class Node;

template<typename T>
class Graph;

template<typename T>
class Edge;

template<typename T>
class OptimizationPass;

template<typename T>
class ComputeResult;

template<typename T>
class Task;

// Template constraints
template<typename T>
concept GraphType = requires(T t) {
    { t.execute() } -> std::same_as<Task<void>>;
    { t.get_node_error("") } -> std::same_as<std::optional<ErrorState>>;
};

template<typename T>
concept NodeType = requires(T t) {
    { t.name() } -> std::same_as<const std::string&>;
    { t.compute() } -> std::same_as<Task<ComputeResult<typename T::value_type>>>;
};

template<typename T>
concept EdgeType = requires(T t) {
    { t.from() } -> std::same_as<std::shared_ptr<NodeBase>>;
    { t.to() } -> std::same_as<std::shared_ptr<NodeBase>>;
};

} // namespace flowgraph
