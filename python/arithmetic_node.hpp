#pragma once
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/compute_result.hpp"
#include "../include/flowgraph/core/error_state.hpp"
#include "../include/flowgraph/async/task.hpp"
#include <string>
#include <memory>

namespace flowgraph {
namespace python {

// Simple node that performs basic arithmetic
template<typename T>
class ArithmeticNode : public Node<T> {
public:
    ArithmeticNode(std::string name, T value)
        : Node<T>(std::move(name))
        , value_(value) {}

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        try {
            // Simulate computation with precision
            T result = value_;
            for (size_t i = 0; i < precision_level; ++i) {
                result *= static_cast<T>(1.1);  // Simple operation affected by precision
            }
            co_return ComputeResult<T>(result);
        } catch (const std::exception& e) {
            auto error = ErrorState::computation_error(e.what());
            error.set_source_node(this->name());
            co_return ComputeResult<T>(std::move(error));
        }
    }

private:
    T value_;
};

} // namespace python
} // namespace flowgraph
