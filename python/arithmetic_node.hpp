#pragma once

#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/compute_result.hpp"
#include "../include/flowgraph/core/error_state.hpp"
#include "../include/flowgraph/async/task.hpp"
#include "../include/flowgraph/core/concepts.hpp"
#include "../include/flowgraph/core/edge.hpp"
#include <string>
#include <memory>

namespace flowgraph {
namespace python {

template<typename T>
    requires NodeValue<T> && std::is_arithmetic_v<T>
class ArithmeticNode : public Node<T> {
public:
    using value_type = T;
    using Base = Node<T>;

    explicit ArithmeticNode(std::string name, value_type value)
        : Base(std::move(name), 8),  // Support up to 8 precision levels
          value_(value) {}

protected:
    Task<ComputeResult<value_type>> compute_impl(size_t) override {
        try {
            co_return ComputeResult<value_type>{value_};
        } catch (const std::exception& e) {
            auto error = ErrorState::computation_error(e.what());
            error.set_source_node(Base::name());
            co_return ComputeResult<value_type>{std::move(error)};
        }
    }

private:
    value_type value_;
};

} // namespace python
} // namespace flowgraph
