#pragma once
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/edge.hpp"

namespace flowgraph {
namespace wasm {

// JavaScript-friendly wrapper for flowgraph operations
class FlowGraphJS {
public:
    // Create a new node with given name and initial value
    int createNode(const std::string& name, double value) {
        auto node = std::make_shared<example::ArithmeticNode<double>>(name, value);
        nodes_[next_id_] = node;
        return next_id_++;
    }

    // Connect two nodes
    bool connectNodes(int from_id, int to_id) {
        auto from_it = nodes_.find(from_id);
        auto to_it = nodes_.find(to_id);
        if (from_it == nodes_.end() || to_it == nodes_.end()) {
            return false;
        }

        auto edge = std::make_shared<Edge<double>>(from_it->second, to_it->second);
        graph_.add_edge(edge);
        return true;
    }

    // Set precision for a node
    bool setPrecision(int node_id, int precision) {
        auto it = nodes_.find(node_id);
        if (it == nodes_.end()) {
            return false;
        }

        it->second->set_precision_range(0, precision);
        it->second->adjust_precision(precision);
        return true;
    }

    // Execute graph and return results
    emscripten::val execute() {
        auto result = graph_.execute().get();
        
        // Convert results to JavaScript object
        emscripten::val results = emscripten::val::object();
        for (const auto& [id, node] : nodes_) {
            auto compute_result = node->compute().get();
            if (compute_result.has_error()) {
                results.set(std::to_string(id), 
                    emscripten::val::object()
                    .set("error", compute_result.error().message())
                    .set("source", compute_result.error().source_node().value_or("unknown")));
            } else {
                results.set(std::to_string(id), compute_result.value());
            }
        }
        return results;
    }

    // Enable optimization
    void enableOptimization(bool enable_compression = true, bool enable_precision = true) {
        if (enable_compression) {
            graph_.add_optimization_pass(
                std::make_unique<CompressionOptimizationPass<double>>());
        }
        if (enable_precision) {
            graph_.add_optimization_pass(
                std::make_unique<PrecisionOptimizationPass<double>>());
        }
    }

private:
    Graph<double> graph_;
    std::unordered_map<int, std::shared_ptr<Node<double>>> nodes_;
    int next_id_ = 0;
};

} // namespace wasm
} // namespace flowgraph

// JavaScript bindings
EMSCRIPTEN_BINDINGS(flowgraph) {
    using namespace flowgraph::wasm;
    using namespace emscripten;

    class_<FlowGraphJS>("FlowGraph")
        .constructor<>()
        .function("createNode", &FlowGraphJS::createNode)
        .function("connectNodes", &FlowGraphJS::connectNodes)
        .function("setPrecision", &FlowGraphJS::setPrecision)
        .function("execute", &FlowGraphJS::execute)
        .function("enableOptimization", &FlowGraphJS::enableOptimization);
}
