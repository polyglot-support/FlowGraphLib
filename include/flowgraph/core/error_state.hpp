#pragma once
#include <string>
#include <optional>
#include <memory>
#include <vector>

namespace flowgraph {

// Enum for different types of errors that can occur in nodes
enum class ErrorType {
    None,
    ComputationError,     // Error during node computation
    PrecisionError,       // Error related to precision levels
    DependencyError,      // Error in dependent nodes
    ResourceError,        // Resource allocation/availability errors
    TimeoutError,         // Computation timeout
    ValidationError       // Invalid data or state
};

// Class to represent an error state with context
class ErrorState {
public:
    ErrorState() : type_(ErrorType::None) {}
    
    ErrorState(ErrorType type, std::string message)
        : type_(type), message_(std::move(message)) {}

    // Error type accessors
    ErrorType type() const { return type_; }
    bool has_error() const { return type_ != ErrorType::None; }
    const std::string& message() const { return message_; }

    // Error source tracking
    void set_source_node(const std::string& node_name) {
        source_node_ = node_name;
    }

    const std::optional<std::string>& source_node() const {
        return source_node_;
    }

    // Error propagation tracking
    void add_propagation_path(const std::string& node_name) {
        propagation_path_.push_back(node_name);
    }

    const std::vector<std::string>& propagation_path() const {
        return propagation_path_;
    }

    // Create specific error states
    static ErrorState computation_error(const std::string& message) {
        return ErrorState(ErrorType::ComputationError, message);
    }

    static ErrorState precision_error(const std::string& message) {
        return ErrorState(ErrorType::PrecisionError, message);
    }

    static ErrorState dependency_error(const std::string& message) {
        return ErrorState(ErrorType::DependencyError, message);
    }

    static ErrorState resource_error(const std::string& message) {
        return ErrorState(ErrorType::ResourceError, message);
    }

    static ErrorState timeout_error(const std::string& message) {
        return ErrorState(ErrorType::TimeoutError, message);
    }

    static ErrorState validation_error(const std::string& message) {
        return ErrorState(ErrorType::ValidationError, message);
    }

private:
    ErrorType type_;
    std::string message_;
    std::optional<std::string> source_node_;
    std::vector<std::string> propagation_path_;
};

// Class to hold computation result with error state
template<typename T>
class ComputeResult {
public:
    ComputeResult() : error_state_(ErrorType::None, "") {}
    
    ComputeResult(T value) : value_(std::move(value)), error_state_(ErrorType::None, "") {}
    
    ComputeResult(ErrorState error) : error_state_(std::move(error)) {}

    bool has_error() const { return error_state_.has_error(); }
    const ErrorState& error() const { return error_state_; }
    
    const T& value() const { 
        if (has_error()) {
            throw std::runtime_error("Attempting to access value of failed computation");
        }
        return value_.value();
    }

    bool has_value() const { return value_.has_value(); }

private:
    std::optional<T> value_;
    ErrorState error_state_;
};

} // namespace flowgraph
