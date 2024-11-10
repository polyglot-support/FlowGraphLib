#pragma once

// Standard library includes
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <mutex>
#include <concepts>
#include <coroutine>

// Forward declarations
namespace flowgraph {

// Core concepts
template<typename T>
concept NodeValue = requires(T a) {
    { a } -> std::convertible_to<T>;
};

// Forward declarations of template classes
template<typename T>
    requires NodeValue<T>
class Node;

template<typename T>
    requires NodeValue<T>
class Graph;

template<typename T>
    requires NodeValue<T>
class Edge;

template<typename T>
    requires NodeValue<T>
class OptimizationPass;

template<typename T>
class ComputeResult;

template<typename T>
class Task;

// Base interfaces
class GraphBase {
public:
    virtual ~GraphBase() = default;
    virtual std::optional<ErrorState> get_node_error(const std::string& node_name) const = 0;
};

class NodeBase {
public:
    virtual ~NodeBase() = default;
    virtual const std::string& name() const = 0;
    virtual void set_parent_graph(GraphBase* graph) = 0;
    virtual size_t current_precision_level() const = 0;
    virtual size_t max_precision_level() const = 0;
    virtual size_t min_precision_level() const = 0;
    virtual void set_precision_range(size_t min_level, size_t max_level) = 0;
    virtual void adjust_precision(size_t target_level) = 0;
    virtual void merge_updates() = 0;
};

template<typename T>
    requires NodeValue<T>
class OptimizationPassBase {
public:
    virtual ~OptimizationPassBase() = default;
    virtual void optimize(Graph<T>& graph) = 0;
    virtual std::string name() const = 0;
};

} // namespace flowgraph
