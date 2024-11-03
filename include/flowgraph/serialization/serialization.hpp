#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "../core/node.hpp"
#include "../core/edge.hpp"
#include "../core/graph.hpp"

namespace flowgraph {

// Forward declarations
template<typename T>
class Graph;

// Serialization interface
template<typename T>
class Serializable {
public:
    virtual ~Serializable() = default;
    virtual nlohmann::json to_json() const = 0;
    virtual void from_json(const nlohmann::json& j) = 0;
};

// Node serialization specialization
template<typename T>
nlohmann::json serialize_node(const Node<T>& node) {
    nlohmann::json j;
    j["name"] = node.name();
    return j;
}

template<typename T>
void deserialize_node(Node<T>& node, const nlohmann::json& j) {
    // Basic node properties
    if (j.contains("name")) {
        // Note: name is readonly after construction
    }
}

// Edge serialization
template<typename T>
nlohmann::json serialize_edge(const Edge<T>& edge) {
    nlohmann::json j;
    j["from"] = edge.from()->name();
    j["to"] = edge.to()->name();
    return j;
}

// Graph serialization
template<typename T>
nlohmann::json serialize_graph(const Graph<T>& graph) {
    nlohmann::json j;
    
    // Serialize nodes
    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& node : graph.get_nodes()) {
        nodes.push_back(serialize_node(*node));
    }
    j["nodes"] = nodes;
    
    // Serialize edges
    nlohmann::json edges = nlohmann::json::array();
    for (const auto& node : graph.get_nodes()) {
        for (const auto& edge : graph.get_outgoing_edges(node)) {
            edges.push_back(serialize_edge(*edge));
        }
    }
    j["edges"] = edges;
    
    return j;
}

} // namespace flowgraph
