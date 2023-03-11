#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <assert.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
  public:
    explicit ThreadPool(size_t thread_count = 8)
        : pool_(std::make_shared<Pool>()) {
        assert(thread_count > 0);
        for (size_t i = 0; i < thread_count; i++) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> lock(pool->mtx_);
                while (true) {
                    if (!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        lock.unlock();
                        task();
                        lock.lock();
                    } else if (pool->is_close_)
                        break;
                    else
                        pool->cond_.wait(lock);
                }
            }).detach();
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> lock(pool_->mtx_);
                pool_->is_close_ = true;
            }
            pool_->cond_.notify_all();
        }
    }
    template <class F> void AddTask(F &&task) {
        {
            std::lock_guard<std::mutex> lock(pool_->mtx_);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond_.notify_one();
    }

  private:
    struct Pool {
        std::mutex mtx_;
        std::condition_variable cond_;
        bool is_close_;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

#endif //__THREADPOOL_H__