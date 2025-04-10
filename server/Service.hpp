#pragma once
#include "DataManager.hpp"
#include <dirent.h>
#include <cctype>
// libevent
#include <signal.h>
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <evhttp.h>
#include <regex>
#include "base64.h" // 来自 cpp-base64 库
#include <sys/queue.h>
#include <fcntl.h>
namespace storage
{
    DataManager data_;
    //
    // 服务器端
    //
    class Service
    {
    private:
        int server_port_;
        std::string server_ip_;
        std::string download_prefix_;
    public:
        Service() {
            server_port_ = Config::GetInstance()->GetServerPort();
            server_ip_ = Config::GetInstance()->GetServerIp();
            download_prefix_ = Config::GetInstance()->GetDownloadPrefix();
        }

        static void
        signal_cb(evutil_socket_t fd, short event, void *arg)
        {
            printf("%s signal received\n", strsignal(fd));
            struct event_base *event_base = (struct event_base *)arg;
            event_base_loopbreak(event_base); // 退出事件循环
            // 如果不调用event_base_loopbreak，则会导致程序一直处于阻塞状态，无法退出
            // 这里的意思是，接收到信号后，退出事件循环
            // 也就是退出event_base_dispatch函数
            // 这样就可以正常退出程序了
        }
        // 
        // 远程能够通过浏览器访问的接口
        // - 基于事件驱动的http服务器
        // 
        bool StartServer()
        {
            // 创建事件库
            // 创建基于该事件库的信号
            // 创建基于该事件库的http服务器
            //     http服务器绑定端口
            //     设置http服务器的回调函数
            //     设置信号的回调函数
            // 事件循环
            // 释放资源
            //     释放http服务器
            //     释放事件库
            //     释放信号

            struct event_base* base = event_base_new();
            if (base == nullptr)
            {
                return false;
            }

            // 创建http服务器
            struct evhttp* http_server = evhttp_new(base);
            if (http_server == nullptr) {
                return false;
            }

            // 绑定信号
            struct event *sig_int = evsignal_new(base, SIGINT, signal_cb, base);
            if (sig_int == nullptr) {
                return false;
            }
            event_add(sig_int, NULL);

            // 绑定端口 如果返回-1则表示绑定失败
            if (evhttp_bind_socket(http_server, server_ip_.c_str(), server_port_) != 0) {
                return false;
            }

            // 设置回调函数
            evhttp_set_gencb(http_server, HttpCallback, NULL);

            // 设置事件循环
            if (base) {
                if (-1 == event_base_dispatch(base)) {}
            }
            if (base) {
                event_base_free(base);
            }
            if (http_server) {
                evhttp_free(http_server);
            }
            return true;
        }

    private:
        //
        // 
        //
        static unsigned char FromHex(unsigned char x) {
            unsigned char y;
            if (x >= 'A' && x <= 'Z')
                y = x - 'A' + 10;
            else if (x >= 'a' && x <= 'z')
                y = x - 'a' + 10;
            else if (x >= '0' && x <= '9')
                y = x - '0';
            else
                assert(0);
            return y;
        }

        //
        // URL解码
        // - str: URL编码的字符串
        // - 返回值: 解码后的字符串
        //
        static std::string UrlDecode(const std::string &str) {
            std::string strTemp = "";
            size_t length = str.length();
            for (size_t i = 0; i < length; i++) {
                if (str[i] == '%') {
                    assert(i + 2 < length);
                    unsigned char high = FromHex((unsigned char)str[++i]);
                    unsigned char low = FromHex((unsigned char)str[++i]);
                    strTemp += high * 16 + low;
                }
                else
                    strTemp += str[i];
            }
            return strTemp;
        }

        //
        // 格式化文件大小
        // - bytes: 文件大小
        // - 返回值: 格式化后的字符串
        // 
        static std::string formatSize(uint64_t bytes) {
            const char *units[] = {"B", "KB", "MB", "GB"};
            int unit_index = 0;
            double size = bytes;

            while (size >= 1024 && unit_index < 3) {
                size /= 1024;
                unit_index++;
            }

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
            return ss.str();
        }

        //
        // 格式化时间
        // - t: 时间戳
        // - 返回值: 格式化后的字符串
        //
        static std::string TimetoStr(time_t t) {
            std::string tmp = std::ctime(&t);
            return tmp;
        }

        //
        // 文件列表格式化为HTML
        // - files: 文件列表
        // - 返回值: 处理后的字符串
        //
        static std::string generateModernFileList(const std::vector<StorageInfo> &files)
        {
            std::stringstream ss;
            ss << "<div class='file-list'><h3>已上传文件</h3>";

            for (const auto &file : files) {
                std::string filename = FileUtil(file.storage_path_).GetFileName();

                // 从路径中解析存储类型（示例逻辑，需根据实际路径规则调整）
                std::string storage_type = "low";
                if (file.storage_path_.find("/deep/") != std::string::npos) {
                    storage_type = "deep";
                }

                ss << "<div class='file-item'>"
                   << "<div class='file-info'>"
                   << "<span>📄" << filename << "</span>"
                   << "<span class='file-type'>"
                   << (storage_type == "deep" ? "深度存储" : "普通存储")
                   << "</span>"
                   << "<span>" << formatSize(file.fsize_) << "</span>"
                   << "<span>" << TimetoStr(file.mtime_) << "</span>"
                   << "</div>"
                   << "<button onclick=\"window.location='" << file.url_ << "'\">⬇️ 下载</button>"
                   << "</div>";
            }

            ss << "</div>";
            return ss.str();
        }

        //
        // 服务器端处理请求的回调函数
        // - 处理请求
        // - 处理请求的参数
        //   show
        //   download
        //   upload
        //   notfound
        //
        static void HttpCallback(struct evhttp_request* req, void* arg) {
            std::string path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            path = UrlDecode(path); // 中文解码
            if (path.find("/download/") != std::string::npos) {
                Download(req, arg);
            }
            else if (path == "/upload") {
                Upload(req, arg);
            }
            else if (path == "/") {
                ListShow(req, arg);
            }
            else {
                evhttp_send_reply(req, HTTP_NOTFOUND, "Not Found", NULL);
            }
        }

        //
        // 获取文件的etag
        // - 自定义etag :  filename-fsize-mtime
        //
        static std::string GetETag(const StorageInfo &info) {
            
            FileUtil fu(info.storage_path_);
            std::string etag = fu.GetFileName();
            etag += "-";
            etag += std::to_string(info.fsize_);
            etag += "-";
            etag += std::to_string(info.mtime_);
            return etag;
        }

        //
        // 下载文件
        //
        static void Download(struct evhttp_request *req, void *arg) {
            StorageInfo info;
            std::string resource_path = evhttp_uri_get_path(evhttp_request_get_evhttp_uri(req));
            resource_path = UrlDecode(resource_path);
            data_.GetOneByURL(resource_path, &info);
            std::string download_path = info.storage_path_;
            FileUtil fileutil(download_path);
            if (fileutil.Exist() == false && info.storage_path_.find("deep_storage") != std::string::npos) {
                evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);
            }

            bool retrans = false;
            std::string old_etag;
            auto if_range = evhttp_find_header(req->input_headers, "If-Range");
            if (NULL != if_range) {
                old_etag = if_range;
                if (old_etag == GetETag(info)) {
                    retrans = true;
                }
            }
            evbuffer *out_buffer = evhttp_request_get_output_buffer(req);
            int fd = open(download_path.c_str(), O_RDONLY);
            if (fd == -1) {
                evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);
                return;
            }
            if (-1 == evbuffer_add_file(out_buffer, fd, 0, fileutil.FileSize())) {
            }
            evhttp_add_header(req->output_headers, "Accept-Ranges", "bytes");
            evhttp_add_header(req->output_headers, "ETag", GetETag(info).c_str());
            evhttp_add_header(req->output_headers, "Content-Type", "application/octet-stream");
            if (retrans == false) {
                evhttp_send_reply(req, HTTP_OK, "Success", NULL);
            }
            else {
                evhttp_send_reply(req, 206, "breakpoint continuous transmission", NULL); // 区间请求响应的是206
            }
        }

        //
        // 上传文件
        //
        static void Upload(struct evhttp_request *req, void *arg) {
            struct evbuffer *buf = evhttp_request_get_input_buffer(req);
            if (buf == NULL) {
                evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
                return;
            }
            size_t len = evbuffer_get_length(buf); // 获取请求体的长度
            if (len == 0) {
                evhttp_send_reply(req, HTTP_BADREQUEST, "file empty", NULL);
                return;
            }
            std::string content(len, 0);
            if (evbuffer_copyout(buf, &content[0], len) == -1) {
                evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
                return;
            }
            std::string filename = evhttp_find_header(req->input_headers, "Filename");
            filename = base64_decode(filename);
            std::string filetype = evhttp_find_header(req->input_headers, "StorageType");
            std::string storage_path;
            if (filetype == "deep") {
                storage_path = Config::GetInstance()->GetDeepStorageDir();
            }
            else if (filetype == "low") {
                storage_path = Config::GetInstance()->GetLowStorageDir();
            }
            else {
                evhttp_send_reply(req, HTTP_BADREQUEST, "Bad Request", NULL);
                return;
            }
            storage_path += filename;
            FileUtil fileutil(storage_path);
            if (fileutil.SetContent(content.c_str(), content.size()) == false) {
                evhttp_send_reply(req, HTTP_INTERNAL, "Internal Error", NULL);
                return;
            }
            StorageInfo info;
            info.storage_path_ = storage_path;
            info.mtime_ = fileutil.GetLastModifyTime();
            info.atime_ = fileutil.GetLastAccessTime();
            info.fsize_ = fileutil.FileSize();
            Config *config = Config::GetInstance();
            info.url_ = config->GetDownloadPrefix() + fileutil.GetFileName();
            if (data_.Insert(info) == false) {
                evhttp_send_reply(req, HTTP_INTERNAL, "Internal Error", NULL);
                return;
            }
            evhttp_send_reply(req, HTTP_OK, "Success", NULL);
        }
        
        //
        // 显示文件列表
        //
        static void ListShow(struct evhttp_request *req, void *arg) {
            // 读取文件管理器文件
            std::vector<StorageInfo> arry;
            data_.GetInfo(&arry);

            // 读取模板文件
            std::ifstream templateFile("index.html");
            std::string templateContent(
                (std::istreambuf_iterator<char>(templateFile)),
                std::istreambuf_iterator<char>());
            
            // 替换模板中的占位符
            templateContent = std::regex_replace(templateContent,
                std::regex("\\{\\{FILE_LIST\\}\\}"),
                generateModernFileList(arry));
            // 替换模板中的占位符
            templateContent = std::regex_replace(templateContent,
                std::regex("\\{\\{BACKEND_URL\\}\\}"),
                "http://"+storage::Config::GetInstance()->GetServerIp()+":"+std::to_string(storage::Config::GetInstance()->GetServerPort()));
            
            // 获取请求的输出evbuffer
            struct evbuffer *buf = evhttp_request_get_output_buffer(req);
            evbuffer_add(buf, (const void *)templateContent.c_str(), templateContent.size());
            evhttp_add_header(req->output_headers, "Content-Type", "text/html;charset=utf-8");
            evhttp_send_reply(req, HTTP_OK, NULL, NULL);
        }
    };
}