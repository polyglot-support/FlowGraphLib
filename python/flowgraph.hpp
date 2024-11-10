#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/edge.hpp"

namespace py = pybind11;

namespace flowgraph {
namespace python {

// Python-friendly wrapper for flowgraph operations
class FlowGraphPython {
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
    py::dict execute() {
        auto result = graph_.execute().get();
        
        // Convert results to Python dictionary
        py::dict results;
        for (const auto& [id, node] : nodes_) {
            auto compute_result = node->compute().get();
            if (compute_result.has_error()) {
                py::dict error_info;
                error_info["error"] = compute_result.error().message();
                error_info["source"] = compute_result.error().source_node().value_or("unknown");
                results[py::str(std::to_string(id))] = error_info;
            } else {
                results[py::str(std::to_string(id))] = compute_result.value();
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

// Python module definition
PYBIND11_MODULE(flowgraph, m) {
    m.doc() = "FlowGraphLib Python bindings"; // Module docstring

    py::class_<FlowGraphPython>(m, "FlowGraph")
        .def(py::init<>())
        .def("create_node", &FlowGraphPython::createNode,
             "Create a new node with given name and initial value")
        .def("connect_nodes", &FlowGraphPython::connectNodes,
             "Connect two nodes by their IDs")
        .def("set_precision", &FlowGraphPython::setPrecision,
             "Set precision level for a node")
        .def("execute", &FlowGraphPython::execute,
             "Execute the graph and return results")
        .def("enable_optimization", &FlowGraphPython::enableOptimization,
             "Enable optimization passes",
             py::arg("enable_compression") = true,
             py::arg("enable_precision") = true);
}

} // namespace python
} // namespace flowgraph
