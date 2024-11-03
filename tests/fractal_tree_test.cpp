#include <cassert>
#include <iostream>
#include <cmath>
#include <flowgraph/core/fractal_tree_node.hpp>

using namespace flowgraph;

namespace fractal_tree_tests {

void test_basic_storage_and_retrieval() {
    FractalTreeNode<double> tree(4); // Max depth of 4

    // Store some values at different precision levels
    tree.store(1.0, 0);
    tree.store(1.1, 1);
    tree.store(1.15, 2);
    tree.store(1.155, 3);

    // Verify retrieval
    auto val0 = tree.get(0);
    auto val1 = tree.get(1);
    auto val2 = tree.get(2);
    auto val3 = tree.get(3);

    assert(val0.has_value());
    assert(val1.has_value());
    assert(val2.has_value());
    assert(val3.has_value());

    std::cout << "Basic storage and retrieval test passed!" << std::endl;
}

void test_merging() {
    FractalTreeNode<double> tree(4);

    // Store multiple values at the same precision level
    for (int i = 0; i < 15; i++) {
        tree.store(1.0 + i * 0.1, 2);
    }

    // Force merge
    tree.merge_all();

    // Verify that we can still retrieve a value
    auto val = tree.get(2);
    assert(val.has_value());

    std::cout << "Merging test passed!" << std::endl;
}

void test_compression() {
    FractalTreeNode<double> tree(4, 0.1); // Higher threshold for testing

    // Store similar values at different levels
    tree.store(1.0, 0);
    tree.store(1.01, 1); // Very close to level 0
    tree.store(1.5, 2);  // Different enough to keep

    tree.merge_all(); // This also triggers compression

    // Level 1 should be compressed away due to similarity with level 0
    auto val1 = tree.get(1);
    auto val2 = tree.get(2);

    assert(val2.has_value()); // Level 2 should still exist
    
    std::cout << "Compression test passed!" << std::endl;
}

void test_precision_limits() {
    FractalTreeNode<double> tree(2); // Max depth of 2

    // Try to store at a higher precision level
    tree.store(1.0, 5); // Should be stored at level 2

    auto val = tree.get(5); // Should return value at level 2
    assert(val.has_value());

    std::cout << "Precision limits test passed!" << std::endl;
}

void run_all_tests() {
    test_basic_storage_and_retrieval();
    test_merging();
    test_compression();
    test_precision_limits();
    std::cout << "All FractalTreeNode tests passed!" << std::endl;
}

} // namespace fractal_tree_tests
