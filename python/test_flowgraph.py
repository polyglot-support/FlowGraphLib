#!/usr/bin/env python3
import unittest
import flowgraph

class TestFlowGraph(unittest.TestCase):
    def setUp(self):
        self.graph = flowgraph.FlowGraph()

    def test_node_creation(self):
        """Test node creation and retrieval."""
        node_id = self.graph.create_node("test", 5.0)
        self.assertIsInstance(node_id, int)

    def test_node_connection(self):
        """Test connecting nodes."""
        node1 = self.graph.create_node("input", 5.0)
        node2 = self.graph.create_node("output", 0.0)
        self.assertTrue(self.graph.connect_nodes(node1, node2))

    def test_invalid_connection(self):
        """Test connecting invalid nodes."""
        node1 = self.graph.create_node("input", 5.0)
        self.assertFalse(self.graph.connect_nodes(node1, 999))  # Invalid node ID

    def test_precision_setting(self):
        """Test setting node precision."""
        node = self.graph.create_node("test", 5.0)
        self.assertTrue(self.graph.set_precision(node, 4))

    def test_invalid_precision(self):
        """Test setting precision for invalid node."""
        self.assertFalse(self.graph.set_precision(999, 4))  # Invalid node ID

    def test_graph_execution(self):
        """Test graph execution and results."""
        # Create a simple graph
        node1 = self.graph.create_node("input1", 5.0)
        node2 = self.graph.create_node("input2", 3.0)
        node3 = self.graph.create_node("output", 0.0)

        # Connect nodes
        self.graph.connect_nodes(node1, node3)
        self.graph.connect_nodes(node2, node3)

        # Set precision
        self.graph.set_precision(node1, 4)
        self.graph.set_precision(node2, 4)
        self.graph.set_precision(node3, 4)

        # Execute
        results = self.graph.execute()
        
        # Verify results
        self.assertIsInstance(results, dict)
        self.assertEqual(len(results), 3)
        for node_id, value in results.items():
            if isinstance(value, dict):
                self.assertIn('error', value)
                self.assertIn('source', value)
            else:
                self.assertIsInstance(value, float)

    def test_optimization(self):
        """Test optimization settings."""
        # Create nodes
        nodes = []
        for i in range(5):
            nodes.append(self.graph.create_node(f"node{i}", i * 2.5))

        # Connect nodes
        for i in range(len(nodes) - 1):
            self.graph.connect_nodes(nodes[i], nodes[i + 1])

        # Enable optimization
        self.graph.enable_optimization(True, True)

        # Set precision levels
        for i, node in enumerate(nodes):
            self.graph.set_precision(node, i + 2)

        # Execute
        results = self.graph.execute()
        self.assertIsInstance(results, dict)
        self.assertEqual(len(results), len(nodes))

if __name__ == '__main__':
    unittest.main()
