#include "../include/flowgraph/core/node.hpp"
#include "../include/flowgraph/core/graph.hpp"
#include "../include/flowgraph/core/edge.hpp"
#include "../include/flowgraph/core/compute_result.hpp"
#include "../include/flowgraph/core/error_state.hpp"
#include "../include/flowgraph/core/optimization_base.hpp"
#include "../include/flowgraph/optimization/optimization_pass.hpp"
#include "../include/flowgraph/optimization/compression_optimization.hpp"
#include "../include/flowgraph/optimization/precision_optimization.hpp"
#include "../include/flowgraph/async/task.hpp"
#include "../include/flowgraph/async/thread_pool.hpp"

// This file exists to provide a compilation unit for the header-only library
namespace flowgraph {
    void ensure_linkage() {
        // This function exists to prevent the compiler from optimizing away this file
    }
}
