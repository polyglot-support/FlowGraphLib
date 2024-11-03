#pragma once
#include <coroutine>
#include <exception>

namespace flowgraph {

template<typename T>
class Task {
public:
    struct promise_type {
        Task<T> get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_value(T value) {
            result_ = std::move(value);
        }
        
        void unhandled_exception() {
            exception_ = std::current_exception();
        }

        T result_;
        std::exception_ptr exception_;
    };

    Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {}

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    T await_resume() {
        if (handle_.promise().exception_) {
            std::rethrow_exception(handle_.promise().exception_);
        }
        return std::move(handle_.promise().result_);
    }

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { h.resume(); }

private:
    std::coroutine_handle<promise_type> handle_;
};

// Specialization for void
template<>
class Task<void> {
public:
    struct promise_type {
        Task<void> get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        
        void return_void() {}
        
        void unhandled_exception() {
            exception_ = std::current_exception();
        }

        std::exception_ptr exception_;
    };

    Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {}

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    void await_resume() {
        if (handle_.promise().exception_) {
            std::rethrow_exception(handle_.promise().exception_);
        }
    }

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { h.resume(); }

private:
    std::coroutine_handle<promise_type> handle_;
};

} // namespace flowgraph
