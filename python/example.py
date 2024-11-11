#!/usr/bin/env python3

import flowgraph

def simple_example():
    """Demonstrate basic usage of FlowGraphLib."""
    # Create a new graph
    graph = flowgraph.FlowGraph()
    
    # Create nodes
    node1 = graph.create_node("input1", 5.0)
    node2 = graph.create_node("input2", 3.0)
    node3 = graph.create_node("output", 0.0)
    
    # Connect nodes
    graph.connect_nodes(node1, node3)
    graph.connect_nodes(node2, node3)
    
    # Set precision
    graph.set_precision(node1, 4)
    graph.set_precision(node2, 4)
    graph.set_precision(node3, 4)
    
    # Execute and get results
    results = graph.execute()
    
    # Print results
    print("\nBasic example results:")
    for node_id, value in results.items():
        if isinstance(value, dict) and 'error' in value:
            print(f"Node {node_id}: Error - {value['error']} (from {value['source']})")
        else:
            print(f"Node {node_id}: {value}")

def optimized_example():
    """Demonstrate optimization features."""
    # Create a new graph
    graph = flowgraph.FlowGraph()
    
    # Create a chain of nodes
    nodes = []
    for i in range(5):
        nodes.append(graph.create_node(f"node{i}", i * 2.5))
    
    # Create connections
    for i in range(len(nodes) - 1):
        graph.connect_nodes(nodes[i], nodes[i + 1])
    
    # Enable optimization
    graph.enable_optimization(True, True)
    
    # Set varying precision levels
    for i, node in enumerate(nodes):
        graph.set_precision(node, i + 2)
    
    # Execute and get results
    results = graph.execute()
    
    # Print results
    print("\nOptimized graph results:")
    for node_id, value in results.items():
        if isinstance(value, dict) and 'error' in value:
            print(f"Node {node_id}: Error - {value['error']} (from {value['source']})")
        else:
            print(f"Node {node_id}: {value}")

def error_handling_example():
    """Demonstrate error handling."""
    graph = flowgraph.FlowGraph()
    
    # Create nodes
    node1 = graph.create_node("input", 5.0)
    
    # Try invalid operations
    print("\nError handling example:")
    
    # Invalid node connection
    result = graph.connect_nodes(node1, 999)
    print(f"Connect to invalid node: {result}")
    
    # Invalid precision setting
    result = graph.set_precision(999, 4)
    print(f"Set precision on invalid node: {result}")

if __name__ == "__main__":
    print("FlowGraphLib Python Examples")
    print("===========================")
    
    simple_example()
    optimized_example()
    error_handling_example()
