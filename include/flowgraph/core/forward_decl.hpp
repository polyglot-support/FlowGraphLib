#pragma once
#include "concepts.hpp"

namespace flowgraph {

// Forward declarations for base classes
class GraphBase;
class NodeBase;

// Forward declarations for template classes
template<typename T>
    requires NodeValue<T>
class ComputeResult;

template<typename T>
    requires NodeValue<T>
class Node;

template<typename T>
    requires NodeValue<T>
class Graph;

template<typename T>
    requires NodeValue<T>
class FractalTreeNode;

// Forward declarations for optimization classes
class OptimizationBase;
class CompressionOptimization;
class PrecisionOptimization;

} // namespace flowgraph
