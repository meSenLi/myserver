#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include "../buffer/buffer.h"
#include "../log/log.h"

// 将响应 文件 写入到缓冲区中，等待写入到 fd 中
class HttpResponse {
  public:
    HttpResponse();
    ~HttpResponse();
    void Init(const std::string &srcDir, std::string &path,
              bool is_keep_alive = false, int code = -1);
    void MakeResponse(Buffer &buff);
    void UnmapFile();
    char *File();
    size_t FileLen() const;
    void ErrorContent(Buffer &buff, std::string message);
    int Code() const { return code_; }

  private:
    void AddStateLine(Buffer &buff);
    void AddHeader(Buffer &buff);
    void AddContent(Buffer &buff);
    void ErrorHtml();
    std::string GetFileType();
    int code_;
    bool is_keep_alive_;
    std::string path_;
    std::string src_dir_;
    char *mm_file_;
    struct stat mm_file_stat_;
    static const std::unordered_map<std::string, std::string> suffix_type;
    static const std::unordered_map<int, std::string> code_status;
    static const std::unordered_map<int, std::string> code_path;
};

#endif //__HTTPRESPONSE_H__