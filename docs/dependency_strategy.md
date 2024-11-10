# Dependency Resolution Strategy Analysis

## Current Issues
- Circular dependencies between core.hpp, base.hpp, and other core files
- Multiple redefinitions of base classes (GraphBase, NodeBase)
- Template specialization and concept requirement conflicts
- Inconsistent include hierarchies

## Potential Solutions

### 1. Pure Interface Approach
- Create a pure interface layer with only abstract base classes
- Move all implementation details to separate files
- Use PIMPL idiom to hide implementation details

Pros:
- Clean separation of interfaces and implementations
- Reduces compilation dependencies
- Makes the API more stable

Cons:
- Additional runtime overhead
- More complex implementation
- May require significant refactoring

### 2. Header-Only Template Library
- Consolidate all template code into header files
- Use explicit template instantiation
- Remove base classes in favor of concepts

Pros:
- Better compile-time optimization
- Simpler dependency management
- More flexible template usage

Cons:
- Longer compilation times
- Less runtime polymorphism
- May not fit all use cases

### 3. Layered Architecture
- Organize headers in strict layers:
  1. Core concepts and forward declarations
  2. Base interfaces
  3. Template implementations
  4. Concrete implementations

Pros:
- Clear dependency hierarchy
- Easier to maintain
- Better separation of concerns

Cons:
- May require more files
- Strict rules about dependencies
- Some duplication possible

### 4. Unified Header Approach
- Combine all core declarations into a single header
- Use internal namespaces for organization
- Split implementation into separate files

Pros:
- Simpler include structure
- No circular dependencies
- Easier to maintain

Cons:
- Less modular
- Larger compilation units
- Less granular control

### 5. Type Erasure Pattern
- Use type erasure to break circular dependencies
- Implement runtime polymorphism without inheritance
- Separate interface from implementation

Pros:
- Breaks dependency cycles
- More flexible design
- Better encapsulation

Cons:
- More complex implementation
- Potential runtime overhead
- Less intuitive API

## Recommended Solution: Layered Architecture

After analyzing the options, the Layered Architecture approach appears most suitable because:

1. It provides a clear structure for organizing the codebase
2. It explicitly prevents circular dependencies through layer rules
3. It maintains good separation of concerns
4. It's compatible with both template and non-template code
5. It's easier to maintain and extend

### Implementation Plan

1. Create strict layers:
   - Layer 1 (concepts.hpp): Core concepts and type traits
   - Layer 2 (forward_decl.hpp): Forward declarations
   - Layer 3 (base.hpp): Base interfaces
   - Layer 4 (impl/*.hpp): Template implementations
   - Layer 5 (*.hpp): Concrete implementations

2. Rules:
   - Each layer can only include headers from lower layers
   - No circular dependencies allowed between layers
   - Templates must be fully defined in header files
   - Implementation details go in impl/ subdirectory

3. File Organization:
```
include/flowgraph/
  core/
    concepts.hpp       # Layer 1: Core concepts
    forward_decl.hpp  # Layer 2: Forward declarations
    base.hpp         # Layer 3: Base interfaces
    impl/           # Layer 4: Template implementations
      node_impl.hpp
      graph_impl.hpp
      edge_impl.hpp
    node.hpp        # Layer 5: Public interface
    graph.hpp
    edge.hpp
```

This approach will help maintain a clean architecture while preventing circular dependencies and making the codebase more maintainable.
