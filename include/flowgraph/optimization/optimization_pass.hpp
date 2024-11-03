#pragma once
#include <memory>
#include <string>
#include "../core/concepts.hpp"

namespace flowgraph {

// Forward declaration with consistent constraints
template<NodeValue T>
class Graph;

template<NodeValue T>
class OptimizationPass {
public:
    virtual ~OptimizationPass() = default;
    virtual void optimize(Graph<T>& graph) = 0;
    virtual std::string name() const = 0;
};

} // namespace flowgraph
