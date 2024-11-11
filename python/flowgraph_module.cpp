#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "flowgraph.hpp"

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
