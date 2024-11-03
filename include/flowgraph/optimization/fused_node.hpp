#pragma once
#include "../core/node.hpp"
#include <vector>

namespace flowgraph {

template<typename T>
class FusedNode : public Node<T> {
public:
    FusedNode(const std::vector<std::shared_ptr<Node<T>>>& chain)
        : Node<T>("fused_node"), chain_(chain) {}

protected:
    Task<T> compute_impl() override {
        T result;
        for (const auto& node : chain_) {
            result = co_await node->compute();
        }
        co_return result;
    }

private:
    std::vector<std::shared_ptr<Node<T>>> chain_;
};

} // namespace flowgraph
