#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/error_state.hpp"

namespace flowgraph {
namespace test {

// Test node that can generate different types of errors
template<typename T>
class ErrorTestNode : public Node<T> {
public:
    ErrorTestNode(std::string name, ErrorType error_type = ErrorType::None)
        : Node<T>(std::move(name))
        , error_type_(error_type) {}

    void set_error_type(ErrorType type) {
        error_type_ = type;
    }

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        switch (error_type_) {
            case ErrorType::ComputationError: {
                auto error = ErrorState::computation_error(
                    "Simulated computation error in " + this->name()
                );
                error.set_source_node(this->name());
                co_return ComputeResult<T>(std::move(error));
            }
            case ErrorType::PrecisionError: {
                auto error = ErrorState::precision_error(
                    "Simulated precision error in " + this->name()
                );
                error.set_source_node(this->name());
                co_return ComputeResult<T>(std::move(error));
            }
            case ErrorType::DependencyError: {
                auto error = ErrorState::dependency_error(
                    "Simulated dependency error in " + this->name()
                );
                error.set_source_node(this->name());
                co_return ComputeResult<T>(std::move(error));
            }
            default:
                co_return ComputeResult<T>(T{42}); // Default success value
        }
    }

private:
    ErrorType error_type_;
};

class ErrorPropagationTest : public ::testing::Test {
protected:
    void SetUp() override {
        graph_ = std::make_unique<Graph<double>>();
    }

    std::unique_ptr<Graph<double>> graph_;
};

// Test basic error generation and detection
TEST_F(ErrorPropagationTest, BasicErrorHandling) {
    auto node = std::make_shared<ErrorTestNode<double>>("error_node", ErrorType::ComputationError);
    graph_->add_node(node);

    auto result = node->compute().get();
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(result.error().type(), ErrorType::ComputationError);
    EXPECT_EQ(result.error().source_node(), "error_node");
}

// Test error propagation in a linear chain
TEST_F(ErrorPropagationTest, LinearPropagation) {
    auto node1 = std::make_shared<ErrorTestNode<double>>("node1", ErrorType::ComputationError);
    auto node2 = std::make_shared<ErrorTestNode<double>>("node2");
    auto node3 = std::make_shared<ErrorTestNode<double>>("node3");

    graph_->add_node(node1);
    graph_->add_node(node2);
    graph_->add_node(node3);

    graph_->add_edge(std::make_shared<Edge<double>>(node1, node2));
    graph_->add_edge(std::make_shared<Edge<double>>(node2, node3));

    // Execute graph and verify error propagation
    graph_->execute().get();
    
    auto result = node3->compute().get();
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(result.error().source_node(), "node1");
    
    const auto& path = result.error().propagation_path();
    EXPECT_EQ(path.size(), 2);
    EXPECT_EQ(path[0], "node2");
    EXPECT_EQ(path[1], "node3");
}

// Test error propagation in a diamond-shaped graph
TEST_F(ErrorPropagationTest, DiamondPropagation) {
    auto source = std::make_shared<ErrorTestNode<double>>("source", ErrorType::PrecisionError);
    auto branch1 = std::make_shared<ErrorTestNode<double>>("branch1");
    auto branch2 = std::make_shared<ErrorTestNode<double>>("branch2");
    auto sink = std::make_shared<ErrorTestNode<double>>("sink");

    graph_->add_node(source);
    graph_->add_node(branch1);
    graph_->add_node(branch2);
    graph_->add_node(sink);

    graph_->add_edge(std::make_shared<Edge<double>>(source, branch1));
    graph_->add_edge(std::make_shared<Edge<double>>(source, branch2));
    graph_->add_edge(std::make_shared<Edge<double>>(branch1, sink));
    graph_->add_edge(std::make_shared<Edge<double>>(branch2, sink));

    // Execute graph and verify error propagation
    graph_->execute().get();
    
    auto result = sink->compute().get();
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(result.error().type(), ErrorType::PrecisionError);
    EXPECT_EQ(result.error().source_node(), "source");
    
    const auto& path = result.error().propagation_path();
    EXPECT_GE(path.size(), 2); // Should contain at least one branch and sink
}

// Test multiple error sources
TEST_F(ErrorPropagationTest, MultipleErrorSources) {
    auto node1 = std::make_shared<ErrorTestNode<double>>("node1", ErrorType::ComputationError);
    auto node2 = std::make_shared<ErrorTestNode<double>>("node2", ErrorType::PrecisionError);
    auto node3 = std::make_shared<ErrorTestNode<double>>("node3");

    graph_->add_node(node1);
    graph_->add_node(node2);
    graph_->add_node(node3);

    graph_->add_edge(std::make_shared<Edge<double>>(node1, node3));
    graph_->add_edge(std::make_shared<Edge<double>>(node2, node3));

    // Execute graph and verify error handling
    graph_->execute().get();
    
    auto result = node3->compute().get();
    EXPECT_TRUE(result.has_error());
    
    // Should capture the first error encountered
    EXPECT_TRUE(result.error().type() == ErrorType::ComputationError ||
                result.error().type() == ErrorType::PrecisionError);
}

// Test error recovery
TEST_F(ErrorPropagationTest, ErrorRecovery) {
    auto node = std::make_shared<ErrorTestNode<double>>("recovery_node", ErrorType::ComputationError);
    graph_->add_node(node);

    // First computation should fail
    auto result1 = node->compute().get();
    EXPECT_TRUE(result1.has_error());

    // Clear error and retry
    node->set_error_type(ErrorType::None);
    auto result2 = node->compute().get();
    EXPECT_FALSE(result2.has_error());
    EXPECT_EQ(result2.value(), 42);
}

} // namespace test
} // namespace flowgraph
