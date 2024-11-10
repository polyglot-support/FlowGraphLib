#pragma once
#include <string>
#include <optional>
#include "forward_decl.hpp"
#include "error_state.hpp"

namespace flowgraph {

// Pure virtual interface for all graphs
class IGraph {
public:
    virtual ~IGraph() = default;
    virtual std::optional<ErrorState> get_node_error(const std::string& node_name) const = 0;
};

// Pure virtual interface for all nodes
class INode {
public:
    virtual ~INode() = default;
    virtual const std::string& name() const = 0;
    virtual void set_parent_graph(IGraph* graph) = 0;
    virtual size_t current_precision_level() const = 0;
    virtual size_t max_precision_level() const = 0;
    virtual size_t min_precision_level() const = 0;
    virtual void set_precision_range(size_t min_level, size_t max_level) = 0;
    virtual void adjust_precision(size_t target_level) = 0;
    virtual void merge_updates() = 0;
};

// Pure virtual interface for optimizations
class IOptimization {
public:
    virtual ~IOptimization() = default;
    virtual bool can_optimize() const = 0;
    virtual void optimize() = 0;
    virtual std::string name() const = 0;
};

} // namespace flowgraph
