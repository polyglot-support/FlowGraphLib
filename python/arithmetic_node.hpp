#pragma once
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/compute_result.hpp"
#include "../include/flowgraph/core/error_state.hpp"
#include "../include/flowgraph/async/task.hpp"
#include <string>
#include <memory>
#include <cmath>

namespace flowgraph {
namespace python {

// Simple node that performs basic arithmetic
template<typename T>
class ArithmeticNode final : public Node<T> {
public:
    explicit ArithmeticNode(std::string name, T value)
        : Node<T>(std::move(name), 8)  // Support up to 8 precision levels
        , value_(value) {}

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        try {
            // Simulate computation with precision
            T result = value_;
            T factor = static_cast<T>(1.1);
            
            // Round to specified precision
            T scale = std::pow(10.0, static_cast<double>(precision_level));
            result = std::round(result * factor * scale) / scale;
            
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
