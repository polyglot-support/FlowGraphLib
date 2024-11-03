#pragma once
#include <memory>
#include "node.hpp"

namespace flowgraph {

template<NodeValue T>
class Edge {
public:
    Edge(std::shared_ptr<Node<T>> from, std::shared_ptr<Node<T>> to)
        : from_(from), to_(to) {}

    std::shared_ptr<Node<T>> from() const { return from_; }
    std::shared_ptr<Node<T>> to() const { return to_; }

private:
    std::shared_ptr<Node<T>> from_;
    std::shared_ptr<Node<T>> to_;
};

} // namespace flowgraph
