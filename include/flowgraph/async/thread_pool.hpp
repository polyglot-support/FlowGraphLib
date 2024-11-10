#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <coroutine>
#include "task.hpp"

namespace flowgraph {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    template<typename F, typename... Args>
    inline auto enqueue(F&& f, Args&&... args) {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        
        std::future<return_type> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }

    // Specialization for Task<T>
    template<typename T>
    inline auto enqueue_task(std::function<Task<T>()> f) -> std::future<T> {
        auto promise = std::make_shared<std::promise<T>>();
        auto future = promise->get_future();

        auto task = [f = std::move(f), promise = std::move(promise)]() mutable {
            try {
                auto coroutine = f();
                // Create a shared state to keep the coroutine alive
                auto shared_coroutine = std::make_shared<Task<T>>(std::move(coroutine));
                // Get the result synchronously
                auto result = shared_coroutine->get();
                promise->set_value(std::move(result));
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        };

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
            }
            tasks_.emplace(std::move(task));
        }
        condition_.notify_one();
        return future;
    }

    // Specialization for Task<void>
    inline auto enqueue_task(std::function<Task<void>()> f) -> std::future<void> {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        auto task = [f = std::move(f), promise = std::move(promise)]() mutable {
            try {
                auto coroutine = f();
                // Create a shared state to keep the coroutine alive
                auto shared_coroutine = std::make_shared<Task<void>>(std::move(coroutine));
                // Get the result synchronously
                shared_coroutine->get();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        };

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
            }
            tasks_.emplace(std::move(task));
        }
        condition_.notify_one();
        return future;
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

    inline size_t thread_count() const {
        return workers_.size();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

} // namespace flowgraph
