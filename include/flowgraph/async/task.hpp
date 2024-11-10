#pragma once
#include <coroutine>
#include <exception>
#include <thread>
#include <atomic>
#include <memory>

namespace flowgraph {

template<typename T>
class Task {
public:
    struct SharedState {
        T result_;
        std::exception_ptr exception_;
        std::atomic<bool> promise_fulfilled_{false};
        std::coroutine_handle<> continuation_;
    };

    struct promise_type {
        inline Task<T> get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        inline std::suspend_never initial_suspend() { return {}; }
        
        struct final_awaiter {
            inline bool await_ready() noexcept { return false; }
            
            inline void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto& promise = h.promise();
                if (auto continuation = promise.shared_state_->continuation_) {
                    continuation.resume();
                }
            }
            
            inline void await_resume() noexcept {}
        };

        inline final_awaiter final_suspend() noexcept { return {}; }
        
        inline void return_value(T value) {
            shared_state_->result_ = std::move(value);
            shared_state_->promise_fulfilled_.store(true, std::memory_order_release);
        }
        
        inline void unhandled_exception() {
            shared_state_->exception_ = std::current_exception();
            shared_state_->promise_fulfilled_.store(true, std::memory_order_release);
        }

        inline T get_result() {
            while (!shared_state_->promise_fulfilled_.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (shared_state_->exception_) {
                std::rethrow_exception(shared_state_->exception_);
            }
            return std::move(shared_state_->result_);
        }

        inline promise_type() : shared_state_(std::make_shared<SharedState>()) {}

        std::shared_ptr<SharedState> shared_state_;
    };

    inline Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle)
        , shared_state_(handle.promise().shared_state_) {}

    inline Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
        , shared_state_(std::move(other.shared_state_)) {}

    inline Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            shared_state_ = std::move(other.shared_state_);
        }
        return *this;
    }

    inline ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    struct awaiter {
        inline bool await_ready() {
            return shared_state_->promise_fulfilled_.load(std::memory_order_acquire);
        }

        inline void await_suspend(std::coroutine_handle<> h) {
            shared_state_->continuation_ = h;
        }

        inline T await_resume() {
            if (shared_state_->exception_) {
                std::rethrow_exception(shared_state_->exception_);
            }
            return std::move(shared_state_->result_);
        }

        std::shared_ptr<SharedState> shared_state_;
    };

    inline awaiter operator co_await() noexcept {
        return awaiter{shared_state_};
    }

    // Synchronously get the result
    inline T get() {
        if (!shared_state_) {
            throw std::runtime_error("Task has no shared state");
        }
        while (!shared_state_->promise_fulfilled_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        if (shared_state_->exception_) {
            std::rethrow_exception(shared_state_->exception_);
        }
        return std::move(shared_state_->result_);
    }

private:
    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<SharedState> shared_state_;
};

// Specialization for void
template<>
class Task<void> {
public:
    struct SharedState {
        std::exception_ptr exception_;
        std::atomic<bool> promise_fulfilled_{false};
        std::coroutine_handle<> continuation_;
    };

    struct promise_type {
        inline Task<void> get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        inline std::suspend_never initial_suspend() { return {}; }
        
        struct final_awaiter {
            inline bool await_ready() noexcept { return false; }
            
            inline void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                auto& promise = h.promise();
                if (auto continuation = promise.shared_state_->continuation_) {
                    continuation.resume();
                }
            }
            
            inline void await_resume() noexcept {}
        };

        inline final_awaiter final_suspend() noexcept { return {}; }
        
        inline void return_void() {
            shared_state_->promise_fulfilled_.store(true, std::memory_order_release);
        }
        
        inline void unhandled_exception() {
            shared_state_->exception_ = std::current_exception();
            shared_state_->promise_fulfilled_.store(true, std::memory_order_release);
        }

        inline void get_result() {
            while (!shared_state_->promise_fulfilled_.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            if (shared_state_->exception_) {
                std::rethrow_exception(shared_state_->exception_);
            }
        }

        inline promise_type() : shared_state_(std::make_shared<SharedState>()) {}

        std::shared_ptr<SharedState> shared_state_;
    };

    inline Task(std::coroutine_handle<promise_type> handle)
        : handle_(handle)
        , shared_state_(handle.promise().shared_state_) {}

    inline Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
        , shared_state_(std::move(other.shared_state_)) {}

    inline Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            shared_state_ = std::move(other.shared_state_);
        }
        return *this;
    }

    inline ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    struct awaiter {
        inline bool await_ready() {
            return shared_state_->promise_fulfilled_.load(std::memory_order_acquire);
        }

        inline void await_suspend(std::coroutine_handle<> h) {
            shared_state_->continuation_ = h;
        }

        inline void await_resume() {
            if (shared_state_->exception_) {
                std::rethrow_exception(shared_state_->exception_);
            }
        }

        std::shared_ptr<SharedState> shared_state_;
    };

    inline awaiter operator co_await() noexcept {
        return awaiter{shared_state_};
    }

    // Synchronously get the result
    inline void get() {
        if (!shared_state_) {
            throw std::runtime_error("Task has no shared state");
        }
        while (!shared_state_->promise_fulfilled_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        if (shared_state_->exception_) {
            std::rethrow_exception(shared_state_->exception_);
        }
    }

private:
    std::coroutine_handle<promise_type> handle_;
    std::shared_ptr<SharedState> shared_state_;
};

} // namespace flowgraph
