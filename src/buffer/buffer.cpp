#include "buffer.h"

Buffer::Buffer(int init_buff_size)
    : buffer_(init_buff_size), read_pos_(0), write_pos_(0) {}

size_t Buffer::ReadableBytes() const { return write_pos_ - read_pos_; }

size_t Buffer::WriteableBytes() const { return buffer_.size() - write_pos_; }
size_t Buffer::PrependableBytes() const { return read_pos_; }

const char *Buffer::Peek() const { return BeginPtr() + read_pos_; }
void Buffer::EnsureWriteable(size_t len) {
    if (WriteableBytes() < len) {
        MakeSpace(len);
    }
    assert(WriteableBytes() >= len);
}
void Buffer::HashWritten(size_t len) { write_pos_ += len; }
void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    read_pos_ += len;
}
void Buffer::RetrieveUntil(const char *end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}
const char *Buffer::BeginWriteConst() const { return BeginPtr() + write_pos_; }
char *Buffer::BeginWrite() { return BeginPtr() + write_pos_; }
void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void *data, size_t len) {
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const char *str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HashWritten(len);
}

void Buffer::Append(const Buffer &buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 读 fd 的内容
ssize_t Buffer::ReadFd(int fd, int *Errno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writeable = WriteableBytes();
    iov[0].iov_base = BeginPtr() + write_pos_;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    // readv 函数会将 fd 的所有内容写入到 iov 中，返回实际写入的 iov 中的字数。
    // 与 readv 相对的还有一个 writev ，这个函数是将 iov 中的内容写入到 fd
    // 中去，返回写入 fd 的字数
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {
        write_pos_ += len;
    } else {
        write_pos_ = buffer_.size();
        Append(buff, len - writeable);
    }
    return len;
}

// 将内容写入 fd
ssize_t Buffer::WriteFd(int fd, int *Errno) {
    size_t readable = ReadableBytes();
    ssize_t len = write(fd, Peek(), readable);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    read_pos_ += len;
    return len;
}

char *Buffer::BeginPtr() { return &*buffer_.begin(); }
const char *Buffer::BeginPtr() const { return &*buffer_.begin(); }
void Buffer::MakeSpace(size_t len) {
    if (WriteableBytes() + PrependableBytes() < len) {
        buffer_.resize(write_pos_ + len + 1);
    } else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr() + read_pos_, BeginPtr() + write_pos_, BeginPtr());
        read_pos_ = 0;
        write_pos_ = read_pos_ + readable;
        assert(readable == ReadableBytes());
    }
}