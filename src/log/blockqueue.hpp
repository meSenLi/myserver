#ifndef __BLOCKQUEUE_HPP__
#define __BLOCKQUEUE_HPP__

#include <condition_variable>
#include <deque>
#include <mutex>
#include <sys/time.h>

template <class T> class BlockDeque {
  public:
    explicit BlockDeque(size_t capacity = 1000) : capacity_(capacity) {
        assert(capacity > 0);
        is_close_ = false;
    }
    ~BlockDeque() { close(); }
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        deque_.clear();
    }
    bool empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.empty();
    }
    bool full() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.size() >= capacity_;
    }
    void close() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            deque_.clear();
            is_close_ = true;
        }
        cond_producer_.notify_all();
        cond_consumer_.notify_all();
    }
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.size();
    }
    size_t capacity() {
        std::lock_guard<std::mutex> lock(mtx_);
        return capacity_;
    }
    T front() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.front();
    }
    T back() {
        std::lock_guard<std::mutex> lock(mtx_);
        return deque_.back();
    }
    void push_back(const T &item) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.size() >= capacity_) {
            cond_producer_.wait(lock);
        }
        deque_.push_back(item);
        cond_consumer_.notify_one();
    }
    void push_front(const T &item) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.size() >= capacity_) {
            cond_producer_.wait(lock);
        }
        deque_.push_front(item);
        cond_consumer_.notify_one();
    }
    bool pop(T &item) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.empty()) {
            cond_consumer_.wait(lock);
            if (is_close_)
                return false;
        }
        item = deque_.front();
        deque_.pop_front();
        cond_producer_.notify_one();
        return true;
    }
    bool pop(T &item, int timeout) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (deque_.empty()) {
            if (cond_consumer_.wait_for(lock, std::chrono::seconds(timeout)) ==
                std::cv_status::timeout) {
                return false;
            }
            if (is_close_) {
                return false;
            }
            /* code */
        }
        item = deque_.front();
        deque_.pop_front();
        cond_producer_.notify_one();
        return true;
    }
    void flush() { cond_consumer_.notify_one(); }

  private:
    std::deque<T> deque_;
    size_t capacity_;
    std::mutex mtx_;
    bool is_close_;
    std::condition_variable cond_consumer_;
    std::condition_variable cond_producer_;
};

#endif //__BLOCKQUEUE_HPP__