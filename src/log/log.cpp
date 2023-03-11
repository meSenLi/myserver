#include "log.h"

Log::Log() {
    line_count_ = 0;
    is_async_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (write_thread_ && write_thread_->joinable()) {
        while (!deque_->empty()) {
            deque_->flush();
        }
        deque_->close();
        write_thread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> lock(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    std::lock_guard<std::mutex> lock(mtx_);
    return level_;
}
void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> lock(mtx_);
    level_ = level;
}

void Log::init(int level, const char *path, const char *suffix,
               int max_queue_size) {
    is_open_ = true;
    level_ = level;
    if (max_queue_size > 0) {
        is_async_ = true;
        if (!deque_) {
            deque_ = std::make_unique<BlockDeque<std::string>>();
            write_thread_ = std::make_unique<std::thread>(FlushLogThread);
        }
    } else {
        is_async_ = false;
    }
    line_count_ = 0;
    time_t timer = time(nullptr);
    struct tm *sys_time = std::localtime(&timer);
    struct tm t = *sys_time;
    path_ = path;
    suffix_ = suffix;
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_,
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_.RetrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(file_name, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(file_name, "a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm *sys_time = localtime(&t_sec);
    struct tm t = *sys_time;
    va_list va_list_local;
    if (today_ != t.tm_mday ||
        (line_count_ && (line_count_ % MAX_LINES == 0))) {
        std::unique_lock<std::mutex> lock(mtx_);
        lock.unlock();
        char new_file[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
                 t.tm_mday);
        if (today_ != t.tm_mday) {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail,
                     suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        } else {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
                     (line_count_ / MAX_LINES), suffix_);
        }
        lock.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(new_file, "a");
        assert(fp_ != nullptr);
    }
    {
        std::unique_lock<std::mutex> lock(mtx_);
        line_count_++;
        int n = snprintf(buffer_.BeginWrite(), 28,
                         "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900,
                         t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                         now.tv_usec);
        buffer_.HashWritten(n);
        AppendLogLevelTitle(level);
        va_start(va_list_local, format);
        int m = vsnprintf(buffer_.BeginWrite(), buffer_.WriteableBytes(),
                          format, va_list_local);
        va_end(va_list_local);

        buffer_.HashWritten(m);
        buffer_.Append("\n\0", 2);
        if (is_async_ && deque_ && !deque_->full()) {
            deque_->push_back(buffer_.RetrieveAllToStr());
        } else {
            fputs(buffer_.Peek(), fp_);
        }
        buffer_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle(int level) {
    switch (level) {
    case 0:
        buffer_.Append("[debug]:", 9);
        break;
    case 1:
        buffer_.Append("[info]:", 9);
        break;
    case 2:
        buffer_.Append("[warn]:", 9);
        break;
    case 3:
        buffer_.Append("[error]:", 9);
        break;
    default:
        buffer_.Append("[info]:", 9);
        break;
    }
}

void Log::flush() {
    if (!is_async_) {
        deque_->flush();
    }
    fflush(fp_);
}
void Log::AsyncWrite() {
    std::string str = "";
    while (deque_->pop(str)) {
        std::lock_guard<std::mutex> lock(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log *Log::Instance() {
    static Log inst;
    return &inst;
}
void Log::FlushLogThread() { Log::Instance()->AsyncWrite(); }