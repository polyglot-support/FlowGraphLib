#pragma once
#include <string>
#include "graph.hpp"

namespace flowgraph {

// Base class for optimization passes
template<typename T>
class OptimizationPassBase {
public:
    virtual ~OptimizationPassBase() = default;
    virtual std::string name() const = 0;
    virtual void optimize(Graph<T>& graph) = 0;
};

} // namespace flowgraph
