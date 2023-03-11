#include "heaptimer.h"

void HeapTimer::ShiftUp(size_t index) {
    assert(index >= 0 && index < heap_.size());
    size_t j = (index - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[index]) {
            break;
        }
        SwapNode(index, j);
        index = j;
        j = (index - 1) / 2;
    }
}

void HeapTimer::SwapNode(size_t index, size_t j) {
    assert(index >= 0 && index < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[index], heap_[j]);
    ref_[heap_[index].id] = index;
    ref_[heap_[j].id] = j;
}

bool HeapTimer::ShitfDown(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j])
            j++;
        if (heap_[i] < heap_[j])
            break;
        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::add(int id, int time_out, const TimeoutCallBack &cb) {
    assert(id >= 0);
    size_t i;
    if (ref_.count(id) == 0) {
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(time_out), cb});
        ShiftUp(i);
    } else {
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(time_out);
        heap_[i].cb = cb;
        if (!ShitfDown(i, heap_.size())) {
            ShiftUp(i);
        }
    }
}

void HeapTimer::Delete(size_t index) {
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        SwapNode(i, n);
        if (!ShitfDown(i, n)) {
            ShiftUp(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now())
                .count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    Delete(0);
}
void HeapTimer::adjust(int id, int timeout) {
    // 调整指定id的结点
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    ;
    ShitfDown(ref_[id], heap_.size());
}
void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res =
            std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now())
                .count();
        res = res > 0 ? res : 0;
    }
    return res;
}