#pragma once
#include <string>
#include <memory>
#include "../core/node.hpp"
#include "../core/edge.hpp"
#include "../core/concepts.hpp"

namespace flowgraph {

// Forward declarations
template<typename T>
class Graph;

// Basic serialization functions
template<typename T>
std::string serialize_node(const Node<T>& node) {
    return node.name();
}

template<typename T>
void deserialize_node(Node<T>& node, const std::string& s) {
    // Note: name is readonly after construction
}

template<typename T>
std::string serialize_edge(const Edge<T>& edge) {
    return edge.from()->name() + "->" + edge.to()->name();
}

template<typename T>
std::string serialize_graph(const Graph<T>& graph) {
    std::string result;
    
    // Serialize nodes
    result += "Nodes:\n";
    for (const auto& node : graph.get_nodes()) {
        result += serialize_node(*node) + "\n";
    }
    
    // Serialize edges
    result += "Edges:\n";
    for (const auto& node : graph.get_nodes()) {
        for (const auto& edge : graph.get_outgoing_edges(node)) {
            result += serialize_edge(*edge) + "\n";
        }
    }
    
    return result;
}

} // namespace flowgraph
