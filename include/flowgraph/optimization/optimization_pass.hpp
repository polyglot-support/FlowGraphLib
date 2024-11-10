#pragma once
#include <memory>
#include <string>
#include "../core/concepts.hpp"
#include "../core/optimization_base.hpp"

namespace flowgraph {

template<typename T>
class OptimizationPass : public OptimizationPassBase<T> {
public:
    virtual ~OptimizationPass() = default;
    virtual std::string name() const override = 0;
    virtual void optimize(Graph<T>& graph) override = 0;
};

} // namespace flowgraph
