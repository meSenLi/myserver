#ifndef __HEAPTIME_H__
#define __HEAPTIME_H__

#include "../log/log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <assert.h>
#include <chrono>
#include <functional>
#include <queue>
#include <time.h>
#include <unordered_map>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t) { return expires < t.expires; }
};

class HeapTimer {
  public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }
    void adjust(int id, int newExpires);
    void add(int id, int time_out, const TimeoutCallBack &cb);
    void DoWork(int id);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

  private:
    void Delete(size_t index);
    void ShiftUp(size_t index);
    bool ShitfDown(size_t index, size_t n);
    void SwapNode(size_t index, size_t j);
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};

#endif //__HEAPTIME_H__
