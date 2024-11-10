#pragma once
#include "concepts.hpp"
#include "error_state.hpp"
#include <variant>
#include <type_traits>
#include <stdexcept>

namespace flowgraph {

template<typename T>
    requires NodeValue<T>
class ComputeResult {
public:
    using value_type = T;

    ComputeResult() : data_(T{}) {}

    explicit ComputeResult(T value) noexcept(std::is_nothrow_move_constructible_v<T>) 
        : data_(std::move(value)) {}
        
    explicit ComputeResult(ErrorState error) noexcept 
        : data_(std::move(error)) {}

    bool has_error() const noexcept {
        return std::holds_alternative<ErrorState>(data_);
    }

    const T& value() const & {
        if (has_error()) {
            throw std::runtime_error("Attempting to access value of failed computation");
        }
        return std::get<T>(data_);
    }

    T value() && {
        if (has_error()) {
            throw std::runtime_error("Attempting to access value of failed computation");
        }
        return std::get<T>(std::move(data_));
    }

    const ErrorState& error() const & {
        if (!has_error()) {
            throw std::runtime_error("Attempting to access error of successful computation");
        }
        return std::get<ErrorState>(data_);
    }

    ErrorState error() && {
        if (!has_error()) {
            throw std::runtime_error("Attempting to access error of successful computation");
        }
        return std::get<ErrorState>(std::move(data_));
    }

private:
    std::variant<T, ErrorState> data_;
};

} // namespace flowgraph
