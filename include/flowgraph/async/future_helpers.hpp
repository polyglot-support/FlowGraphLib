#pragma once
#include <future>
#include "task.hpp"

namespace flowgraph {

template<typename T>
Task<T> make_task_from_future(std::future<T>&& future) {
    try {
        T result = future.get();
        co_return result;
    } catch (...) {
        std::rethrow_exception(std::current_exception());
    }
}

} // namespace flowgraph
