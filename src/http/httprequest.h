#ifndef __HTPPREQUEST_H__
#define __HTPPREQUEST_H__

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include <errno.h>
#include <mysql/mysql.h>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>

// 主要实现了对请求内容的解析
class HttpRequest {
  public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    void Init();
    bool Pares(Buffer &buff);
    std::string Path() const;
    std::string &Path();
    std::string Method() const;
    std::string Version() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;
    bool IsKeepAlive() const;

  private:
    bool ParseRequestLine(const std::string &line);
    void ParseHeader(const std::string &line);
    void ParseBody(const std::string &line);
    void ParsePath();
    void ParsePost();
    void ParseFromUrlEncoded();
    static bool UserVerify(const std::string &name, const std::string &pwd,
                           bool is_login);
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    static const std::unordered_set<std::string> default_html;
    static const std::unordered_map<std::string, int> default_html_tag;
    static int ConverHex(char ch);
};

#endif //__HTPPREQUEST_H__