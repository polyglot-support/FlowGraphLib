#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/fractal_tree_node.hpp"
#include "../include/flowgraph/optimization/precision_optimization.hpp"
#include "../include/flowgraph/optimization/compression_optimization.hpp"

namespace flowgraph {
namespace test {

// Test node that supports precision levels
template<typename T>
class TestNode : public Node<T> {
public:
    TestNode(std::string name, T value) 
        : Node<T>(std::move(name))
        , value_(std::move(value)) {}

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        // Simulate computation at different precision levels
        auto result = adjust_value_precision(value_, precision_level);
        co_return ComputeResult<T>(result);
    }

private:
    T adjust_value_precision(const T& value, size_t precision_level) {
        // For numeric types, simulate precision by truncating decimal places
        if constexpr (std::is_floating_point_v<T>) {
            double scale = std::pow(10.0, precision_level);
            return std::round(value * scale) / scale;
        }
        return value;
    }

    T value_;
};

class PrecisionManagementTest : public ::testing::Test {
protected:
    void SetUp() override {
        graph_ = std::make_unique<Graph<double>>();
    }

    std::unique_ptr<Graph<double>> graph_;
};

// Test precision level management
TEST_F(PrecisionManagementTest, BasicPrecisionControl) {
    auto node = std::make_shared<TestNode<double>>("test", 3.14159265359);
    
    // Test precision range setting
    ASSERT_NO_THROW(node->set_precision_range(2, 8));
    EXPECT_EQ(node->min_precision_level(), 2);
    EXPECT_EQ(node->max_precision_level(), 8);
    
    // Test precision adjustment
    node->adjust_precision(4);
    EXPECT_EQ(node->current_precision_level(), 4);

    // Verify computation respects precision level
    auto result = node->compute(3).get();
    EXPECT_FALSE(result.has_error());
    EXPECT_NEAR(result.value(), 3.142, 0.001);
}

// Test precision propagation through graph
TEST_F(PrecisionManagementTest, PrecisionPropagation) {
    auto node1 = std::make_shared<TestNode<double>>("node1", 3.14159);
    auto node2 = std::make_shared<TestNode<double>>("node2", 2.71828);
    
    graph_->add_node(node1);
    graph_->add_node(node2);
    graph_->add_edge(std::make_shared<Edge<double>>(node1, node2));

    auto optimizer = std::make_unique<PrecisionOptimizationPass<double>>();
    graph_->add_optimization_pass(std::move(optimizer));
    
    // Optimize and verify precision propagation
    graph_->optimize();
    
    // Verify nodes have compatible precision levels
    EXPECT_LE(std::abs(static_cast<int>(node1->current_precision_level()) - 
                      static_cast<int>(node2->current_precision_level())), 1);
}

// Test memory-aware compression
TEST_F(PrecisionManagementTest, CompressionOptimization) {
    // Create a chain of nodes with high precision
    std::vector<std::shared_ptr<TestNode<double>>> nodes;
    for (int i = 0; i < 10; ++i) {
        auto node = std::make_shared<TestNode<double>>(
            "node" + std::to_string(i), 
            3.14159265359
        );
        node->set_precision_range(0, 8);
        node->adjust_precision(8); // Start with max precision
        nodes.push_back(node);
        graph_->add_node(node);
        
        if (i > 0) {
            graph_->add_edge(std::make_shared<Edge<double>>(nodes[i-1], nodes[i]));
        }
    }

    auto optimizer = std::make_unique<CompressionOptimizationPass<double>>(0.5, 0.2);
    graph_->add_optimization_pass(std::move(optimizer));
    
    // Optimize and verify compression
    graph_->optimize();
    
    // Verify some nodes were compressed (should have lower precision)
    bool found_compressed = false;
    for (const auto& node : nodes) {
        if (node->current_precision_level() < 8) {
            found_compressed = true;
            break;
        }
    }
    EXPECT_TRUE(found_compressed);
}

// Benchmark fractal tree performance
TEST_F(PrecisionManagementTest, FractalTreePerformance) {
    const size_t NUM_OPERATIONS = 1000;
    auto node = std::make_shared<TestNode<double>>("benchmark", 3.14159265359);
    node->set_precision_range(0, 8);

    // Measure time for operations at different precision levels
    std::vector<double> times;
    for (size_t precision = 0; precision <= 8; ++precision) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < NUM_OPERATIONS; ++i) {
            node->adjust_precision(precision);
            auto result = node->compute(precision).get();
            EXPECT_FALSE(result.has_error());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        times.push_back(duration.count() / static_cast<double>(NUM_OPERATIONS));
        
        // Log performance metrics
        std::cout << "Precision level " << precision 
                  << ": " << times.back() << " microseconds per operation" << std::endl;
    }

    // Verify performance scaling is reasonable (should not grow exponentially)
    for (size_t i = 1; i < times.size(); ++i) {
        EXPECT_LE(times[i] / times[i-1], 2.0); // Should not double with each precision level
    }
}

} // namespace test
} // namespace flowgraph
