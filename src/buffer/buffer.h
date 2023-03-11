#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <assert.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

// 以 vector<char> 为底层的缓冲区
class Buffer {
  public:
    Buffer(int init_buff_size = 1024);
    ~Buffer() = default;
    size_t WriteableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;
    const char *Peek() const;
    void EnsureWriteable(size_t len);
    void HashWritten(size_t len);
    void Retrieve(size_t len);
    void RetrieveUntil(const char *end);

    void RetrieveAll();
    std::string RetrieveAllToStr();
    const char *BeginWriteConst() const;
    char *BeginWrite();
    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const Buffer &buff);

    ssize_t ReadFd(int fd, int *Errno);
    ssize_t WriteFd(int fd, int *Errno);

  private:
    char *BeginPtr();
    const char *BeginPtr() const;
    void MakeSpace(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_pos_;
    std::atomic<std::size_t> write_pos_;
};

#endif //__BUFFER_H__