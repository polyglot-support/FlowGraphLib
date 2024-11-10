#pragma once
#include <future>
#include <memory>
#include "task.hpp"
#include "../core/error_state.hpp"

namespace flowgraph {

template<typename T>
inline Task<T> make_task_from_future(std::future<T>&& future) {
    struct SharedState {
        std::future<T> future;
        explicit SharedState(std::future<T>&& f) : future(std::move(f)) {}
    };
    auto state = std::make_shared<SharedState>(std::move(future));

    struct awaiter {
        inline bool await_ready() { return false; }
        inline void await_suspend(std::coroutine_handle<> h) {
            // Create a new thread to wait for the future
            std::thread([this, h]() mutable {
                try {
                    result = state->future.get();
                    h.resume();
                } catch (...) {
                    exception = std::current_exception();
                    h.resume();
                }
            }).detach();
        }
        inline T await_resume() {
            if (exception) {
                std::rethrow_exception(exception);
            }
            return std::move(result);
        }
        std::shared_ptr<SharedState> state;
        T result;
        std::exception_ptr exception;
    };

    co_return co_await awaiter{std::move(state)};
}

// Specialization for ComputeResult
template<typename T>
inline Task<ComputeResult<T>> make_task_from_future(std::future<ComputeResult<T>>&& future) {
    struct SharedState {
        std::future<ComputeResult<T>> future;
        explicit SharedState(std::future<ComputeResult<T>>&& f) : future(std::move(f)) {}
    };
    auto state = std::make_shared<SharedState>(std::move(future));

    struct awaiter {
        inline bool await_ready() { return false; }
        inline void await_suspend(std::coroutine_handle<> h) {
            // Create a new thread to wait for the future
            std::thread([this, h]() mutable {
                try {
                    result = state->future.get();
                    h.resume();
                } catch (...) {
                    auto error = ErrorState::computation_error("Future execution failed");
                    result = ComputeResult<T>(std::move(error));
                    h.resume();
                }
            }).detach();
        }
        inline ComputeResult<T> await_resume() {
            return std::move(result);
        }
        std::shared_ptr<SharedState> state;
        ComputeResult<T> result;
    };

    co_return co_await awaiter{std::move(state)};
}

// Specialization for void
template<>
inline Task<void> make_task_from_future(std::future<void>&& future) {
    struct SharedState {
        std::future<void> future;
        explicit SharedState(std::future<void>&& f) : future(std::move(f)) {}
    };
    auto state = std::make_shared<SharedState>(std::move(future));

    struct awaiter {
        inline bool await_ready() { return false; }
        inline void await_suspend(std::coroutine_handle<> h) {
            // Create a new thread to wait for the future
            std::thread([this, h]() mutable {
                try {
                    state->future.get();
                    h.resume();
                } catch (...) {
                    exception = std::current_exception();
                    h.resume();
                }
            }).detach();
        }
        inline void await_resume() {
            if (exception) {
                std::rethrow_exception(exception);
            }
        }
        std::shared_ptr<SharedState> state;
        std::exception_ptr exception;
    };

    co_await awaiter{std::move(state)};
    co_return;
}

} // namespace flowgraph
