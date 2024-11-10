#pragma once
#include <string>
#include <optional>
#include <vector>
#include "forward_decl.hpp"

namespace flowgraph {

// Error handling
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

} // namespace flowgraph
