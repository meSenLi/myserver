#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "httprequest.h"
#include "httpresponse.h"
#include <arpa/inet.h>
#include <error.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
class HttpConn {
  public:
    HttpConn();

    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);

    ssize_t Read(int *saveErrno);

    ssize_t Write(int *saveErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char *GetIP() const;

    sockaddr_in GetAddr() const;

    bool Process();

    int ToWriteBytes() { return iov_[0].iov_len + iov_[1].iov_len; }

    bool IsKeepAlive() const { return request_.IsKeepAlive(); }

    // static 变量， 所有对象共享
    static bool is_ET_;
    static const char *src_dir_;
    static std::atomic<int> user_count_;

  private:
    int fd_;
    struct sockaddr_in addr_;

    bool is_close_;

    int iov_cnt_;
    struct iovec iov_[2];

    Buffer read_buff_;  // 读缓冲区
    Buffer write_buff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

#endif //__HTTPCONN_H__