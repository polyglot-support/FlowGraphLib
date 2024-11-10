#pragma once
#include <memory>
#include "base.hpp"
#include "concepts.hpp"

namespace flowgraph {

template<typename T>
class Edge;

template<typename T>
    requires NodeValue<T>
class Edge<T> {
public:
    Edge(std::shared_ptr<NodeBase> from, std::shared_ptr<NodeBase> to)
        : from_(from), to_(to) {}

    std::shared_ptr<NodeBase> from() const { return from_; }
    std::shared_ptr<NodeBase> to() const { return to_; }

private:
    std::shared_ptr<NodeBase> from_;
    std::shared_ptr<NodeBase> to_;
};

} // namespace flowgraph
