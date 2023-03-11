#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::suffix_type = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const unordered_map<int, string> HttpResponse::code_status = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::code_path = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = src_dir_ = "";
    is_keep_alive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
};

HttpResponse::~HttpResponse() { UnmapFile(); }

void HttpResponse::Init(const string &src_dir, string &path, bool is_keep_alive,
                        int code) {
    assert(src_dir != "");
    if (mm_file_) {
        UnmapFile();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

void HttpResponse::MakeResponse(Buffer &buff) {
    /* 判断请求的资源文件 */
    if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 ||
        S_ISDIR(mm_file_stat_.st_mode)) {
        code_ = 404;
    } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if (code_ == -1) {
        code_ = 200;
    }
    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

char *HttpResponse::File() { return mm_file_; }

size_t HttpResponse::FileLen() const { return mm_file_stat_.st_size; }

void HttpResponse::ErrorHtml() {
    if (code_path.count(code_) == 1) {
        path_ = code_path.find(code_)->second;
        stat((src_dir_ + path_).data(), &mm_file_stat_);
    }
}

// 将状态写入缓冲区，等待写入 fd 中
void HttpResponse::AddStateLine(Buffer &buff) {
    string status;
    if (code_status.count(code_) == 1) {
        status = code_status.find(code_)->second;
    } else {
        code_ = 400;
        status = code_status.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

// 将 html 的头写入 缓冲区
void HttpResponse::AddHeader(Buffer &buff) {
    buff.Append("Connection: ");
    if (is_keep_alive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
}

// 将文件相关信息写入缓冲区
void HttpResponse::AddContent(Buffer &buff) {
    int ser_fd = open((src_dir_ + path_).data(), O_RDONLY);
    if (ser_fd < 0) {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (src_dir_ + path_).data());
    int *mmRet = (int *)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE,
                             ser_fd, 0);
    if (*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    mm_file_ = (char *)mmRet;
    close(ser_fd);
    buff.Append("Content-length: " + to_string(mm_file_stat_.st_size) +
                "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
    if (mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

string HttpResponse::GetFileType() {
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (suffix_type.count(suffix) == 1) {
        return suffix_type.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buff, string message) {
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (code_status.count(code_) == 1) {
        status = code_status.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
