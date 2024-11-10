#pragma once
#include "forward_decl.hpp"
#include "concepts.hpp"
#include "base.hpp"
#include "../async/task.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace flowgraph {

template<typename T>
    requires NodeValue<T>
class Node : public NodeBase {
public:
    using value_type = T;
    using callback_type = std::function<void(const ComputeResult<T>&)>;

    Node(std::string name, size_t max_precision_depth = 8, double compression_threshold = 0.001);
    
    // Implement NodeBase interface
    const std::string& name() const override;
    void set_parent_graph(IGraph* graph) override;
    size_t current_precision_level() const override;
    size_t max_precision_level() const override;
    size_t min_precision_level() const override;
    void set_precision_range(size_t min_level, size_t max_level) override;
    void adjust_precision(size_t target_level) override;
    void merge_updates() override;

    // Node-specific methods
    [[nodiscard]] Task<ComputeResult<T>> compute(size_t precision_level = 0);
    void add_completion_callback(callback_type callback);

protected:
    virtual Task<ComputeResult<T>> compute_impl(size_t precision_level) = 0;

private:
    bool should_merge_updates();

    std::string name_;
    std::mutex mutex_;
    FractalTreeNode<T> value_storage_;
    std::vector<callback_type> completion_callbacks_;
    size_t current_precision_level_;
    size_t min_precision_level_;
    size_t max_precision_level_;
    size_t computation_count_ = 0;
    IGraph* parent_graph_ = nullptr;
};

} // namespace flowgraph

#include "impl/node_impl.hpp"
