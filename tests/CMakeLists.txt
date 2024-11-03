add_executable(flowgraph_tests main.cpp)
target_link_libraries(flowgraph_tests PRIVATE flowgraph)
add_test(NAME flowgraph_tests COMMAND flowgraph_tests)
