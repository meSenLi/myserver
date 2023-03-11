
#include "httpconn.h"

using namespace std;

const char *HttpConn::src_dir_;
std::atomic<int> HttpConn::user_count_;
bool HttpConn::is_ET_;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    is_close_ = true;
};

HttpConn::~HttpConn() { Close(); };

// 新的连接到来，初始化一个 httpconn
void HttpConn::Init(int fd, const sockaddr_in &addr) {
    assert(fd > 0);
    user_count_++;
    addr_ = addr;
    fd_ = fd;
    write_buff_.RetrieveAll();
    read_buff_.RetrieveAll();
    is_close_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(),
             (int)user_count_);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (is_close_ == false) {
        is_close_ = true;
        user_count_--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(),
                 GetPort(), (int)user_count_);
    }
}

int HttpConn::GetFd() const { return fd_; };

struct sockaddr_in HttpConn::GetAddr() const { return addr_; }

const char *HttpConn::GetIP() const { return inet_ntoa(addr_.sin_addr); }

int HttpConn::GetPort() const { return addr_.sin_port; }

// 从 fd 中读取内容到缓冲区 read_buff_
ssize_t HttpConn::Read(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = read_buff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (is_ET_);
    return len;
}

// 将 iov 中的内容写入到 fd 中
ssize_t HttpConn::Write(int *saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_cnt_);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base =
                (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                write_buff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            write_buff_.Retrieve(len);
        }
    } while (is_ET_ || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::Process() {
    // 前面已经将 fd 的请求内容写入到了 read_buff_ 中
    request_.Init();
    if (read_buff_.ReadableBytes() <= 0) {
        return false;
    } else if (request_.Pares(read_buff_)) {
        LOG_DEBUG("%s", request_.Path().c_str());
        response_.Init(src_dir_, request_.Path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(src_dir_, request_.Path(), false, 400);
    }

    // 将响应内容写入到 write_buff_ 中

    response_.MakeResponse(write_buff_);

    // 将 iov 指向 write_buff_ , 后面直接使用 writev 写入到 fd 中
    iov_[0].iov_base = const_cast<char *>(write_buff_.Peek());
    iov_[0].iov_len = write_buff_.ReadableBytes();
    iov_cnt_ = 1;

    // 将文件写入映射到 iov 中
    if (response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iov_cnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen(), iov_cnt_,
              ToWriteBytes());
    return true;
}
