#pragma once

// Layer 1: Core concepts and forward declarations
#include "forward_decl.hpp"
#include "concepts.hpp"

// Layer 2: Interfaces
#include "interfaces.hpp"

// Layer 3: Base classes
#include "base.hpp"
#include "error_state.hpp"

// Layer 4: Template implementations
#include "impl/compute_result_impl.hpp"
#include "impl/node_impl.hpp"

// Layer 5: Concrete implementations
#include "node.hpp"
#include "graph.hpp"
#include "edge.hpp"
