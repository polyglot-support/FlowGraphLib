#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <chrono>
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/fractal_tree_node.hpp"
#include "../include/flowgraph/optimization/compression_optimization.hpp"

namespace flowgraph {
namespace benchmark {

// Benchmark node that performs realistic computations
template<typename T>
class BenchmarkNode : public Node<T> {
public:
    BenchmarkNode(std::string name, size_t computation_size)
        : Node<T>(std::move(name))
        , computation_size_(computation_size) {}

protected:
    Task<ComputeResult<T>> compute_impl(size_t precision_level) override {
        // Simulate computation with varying precision
        T result = perform_computation(precision_level);
        co_return ComputeResult<T>(result);
    }

private:
    T perform_computation(size_t precision_level) {
        // Simulate complex computation that scales with precision
        T result = 0;
        size_t iterations = computation_size_ * (1 << precision_level);
        
        for (size_t i = 0; i < iterations; ++i) {
            result += std::sin(static_cast<double>(i) / iterations) * std::cos(static_cast<double>(i) / iterations);
        }
        
        return result;
    }

    size_t computation_size_;
};

// Benchmark fixture
class FractalTreeBenchmark : public ::testing::Test {
protected:
    void SetUp() override {
        graph_ = std::make_unique<Graph<double>>();
    }

    std::unique_ptr<Graph<double>> graph_;
    std::mt19937 rng_{std::random_device{}()};
};

// Benchmark single node operations at different precision levels
static void BM_SingleNodePrecision(benchmark::State& state) {
    const size_t precision_level = state.range(0);
    auto node = std::make_shared<BenchmarkNode<double>>("benchmark_node", 1000);
    node->set_precision_range(0, 8);
    node->adjust_precision(precision_level);

    for (auto _ : state) {
        auto result = node->compute(precision_level).get();
        benchmark::DoNotOptimize(result);
    }

    state.SetComplexityN(state.range(0));
}

// Benchmark graph operations with varying numbers of nodes
static void BM_GraphOperations(benchmark::State& state) {
    const size_t num_nodes = state.range(0);
    Graph<double> graph;
    
    // Create a chain of nodes
    std::vector<std::shared_ptr<BenchmarkNode<double>>> nodes;
    for (size_t i = 0; i < num_nodes; ++i) {
        auto node = std::make_shared<BenchmarkNode<double>>(
            "node_" + std::to_string(i), 
            100
        );
        nodes.push_back(node);
        graph.add_node(node);
        
        if (i > 0) {
            graph.add_edge(std::make_shared<Edge<double>>(nodes[i-1], nodes[i]));
        }
    }

    for (auto _ : state) {
        graph.execute().get();
    }

    state.SetComplexityN(state.range(0));
}

// Benchmark memory usage with fractal tree
static void BM_MemoryUsage(benchmark::State& state) {
    const size_t num_values = state.range(0);
    FractalTreeNode<double> tree(8, 0.001);

    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
    std::mt19937 rng(std::random_device{}());

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<double> values(num_values);
        for (size_t i = 0; i < num_values; ++i) {
            values[i] = dist(rng);
        }
        state.ResumeTiming();

        // Store values at different precision levels
        for (size_t i = 0; i < num_values; ++i) {
            size_t precision = i % 8;
            tree.store(values[i], precision);
        }

        // Force merge to measure compression effectiveness
        tree.merge_all();
    }

    state.SetComplexityN(state.range(0));
}

// Benchmark compression optimization
static void BM_CompressionOptimization(benchmark::State& state) {
    const size_t graph_size = state.range(0);
    Graph<double> graph;
    
    // Create a complex graph structure
    std::vector<std::shared_ptr<BenchmarkNode<double>>> nodes;
    for (size_t i = 0; i < graph_size; ++i) {
        auto node = std::make_shared<BenchmarkNode<double>>(
            "node_" + std::to_string(i), 
            100
        );
        node->set_precision_range(0, 8);
        node->adjust_precision(8); // Start with max precision
        nodes.push_back(node);
        graph.add_node(node);
    }

    // Add random edges (ensuring no cycles)
    std::uniform_int_distribution<size_t> dist(0, graph_size - 1);
    std::mt19937 rng(std::random_device{}());
    
    for (size_t i = 0; i < graph_size * 2; ++i) {
        size_t from = dist(rng);
        size_t to = dist(rng);
        if (from < to) { // Ensure no cycles
            try {
                graph.add_edge(std::make_shared<Edge<double>>(nodes[from], nodes[to]));
            } catch (...) {
                // Ignore edge addition failures (e.g., would create cycle)
            }
        }
    }

    auto optimizer = std::make_unique<CompressionOptimizationPass<double>>(0.8, 0.2);
    graph.add_optimization_pass(std::move(optimizer));

    for (auto _ : state) {
        graph.optimize();
        graph.execute().get();
    }

    state.SetComplexityN(state.range(0));
}

// Register benchmarks
BENCHMARK(BM_SingleNodePrecision)
    ->Range(0, 8)
    ->Complexity()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GraphOperations)
    ->Range(8, 64)
    ->Complexity()
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_MemoryUsage)
    ->Range(1<<10, 1<<16)
    ->Complexity()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_CompressionOptimization)
    ->Range(8, 64)
    ->Complexity()
    ->Unit(benchmark::kMillisecond);

} // namespace benchmark
} // namespace flowgraph