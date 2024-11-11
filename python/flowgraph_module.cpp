#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Core includes
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/edge.hpp"
#include "../include/flowgraph/core/compute_result.hpp"
#include "../include/flowgraph/core/error_state.hpp"

// Optimization includes
#include "../include/flowgraph/optimization/compression_optimization.hpp"
#include "../include/flowgraph/optimization/precision_optimization.hpp"

// Async includes
#include "../include/flowgraph/async/task.hpp"
#include "../include/flowgraph/async/thread_pool.hpp"

// Local includes
#include "arithmetic_node.hpp"

// Standard includes
#include <memory>
#include <string>
#include <unordered_map>

namespace py = pybind11;

namespace flowgraph {
namespace python {

// Explicit template instantiations
template class ArithmeticNode<double>;
template class Graph<double>;
template class Node<double>;
template class Edge<double>;
template class ComputeResult<double>;
template class CompressionOptimization<double>;
template class PrecisionOptimization<double>;

class FlowGraphPython {
public:
    // Create a new node with given name and initial value
    int createNode(const std::string& name, double value) {
        auto node = std::make_shared<ArithmeticNode<double>>(name, value);
        nodes_[next_id_] = node;
        graph_.add_node(node);
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
        graph_.execute().get();
        
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
            auto pass = std::make_unique<CompressionOptimization<double>>();
            graph_.add_optimization_pass(std::move(pass));
        }
        if (enable_precision) {
            auto pass = std::make_unique<PrecisionOptimization<double>>();
            graph_.add_optimization_pass(std::move(pass));
        }
    }

private:
    Graph<double> graph_;
    std::unordered_map<int, std::shared_ptr<Node<double>>> nodes_;
    int next_id_ = 0;
};

} // namespace python
} // namespace flowgraph

PYBIND11_MODULE(flowgraph, m) {
    m.doc() = "FlowGraphLib Python bindings";

    py::class_<flowgraph::python::FlowGraphPython>(m, "FlowGraph")
        .def(py::init<>())
        .def("create_node", &flowgraph::python::FlowGraphPython::createNode,
             "Create a new node with given name and initial value")
        .def("connect_nodes", &flowgraph::python::FlowGraphPython::connectNodes,
             "Connect two nodes by their IDs")
        .def("set_precision", &flowgraph::python::FlowGraphPython::setPrecision,
             "Set precision level for a node")
        .def("execute", &flowgraph::python::FlowGraphPython::execute,
             "Execute the graph and return results")
        .def("enable_optimization", &flowgraph::python::FlowGraphPython::enableOptimization,
             "Enable optimization passes",
             py::arg("enable_compression") = true,
             py::arg("enable_precision") = true);
}
