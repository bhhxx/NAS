#pragma once
// for json
#include "jsoncpp/json/json.h"

// for file operation
#include <cassert>
// for mutex
#include <memory>

// 压缩和解压缩
#include "bundle.h"

// 输入输出
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <fstream>

namespace storage
{
    //
    // 文件操作类
    // - bool Exist() : 判断文件是否存在
    // - int64_t FileSize() : 获取文件大小
    // - size_t GetLastAccessTime() : 获取文件最近访问时间
    // - size_t GetLastModifyTime() : 获取文件最近修改时间
    // - std::string GetFileName() : 从路径中解析出文件名
    // - bool GetPosLen(std::string *content, size_t pos, size_t len) : 从文件POS处获取len长度字符给content
    // - bool GetContent(std::string *content) : 获取文件内容
    // - bool SetContent(const char *content, size_t len) : 将文件内容写入到FileUtil对象的filename_
    // - bool Compress(const std::string &content, int format) : 压缩文件并写入到FileUtil对象的filename_
    // - bool UnCompress(std::string &download_path) : 解压文件
    class FileUtil
    {
    private:
        std::string filename_;
    public:
        FileUtil(const std::string &filename) : filename_(filename) {}

        //
        // 文件是否存在
        // - 存在返回true
        // - 不存在返回false
        // 
        bool Exist() {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                return false;
            }
            return true;
        }

        //
        //  获取文件大小
        // - 返回值为文件大小
        // - 如果文件不存在，返回-1
        int64_t FileSize() {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1) {
                return -1;
            }
            return s.st_size;
        }

        //
        // 获取文件最近访问时间
        // - 返回值为时间戳
        // - 如果文件不存在，返回-1
        // - 如果文件存在，返回文件的最近访问时间
        size_t GetLastAccessTime()
        {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                return -1;
            }
            return s.st_atime;
        }

        //
        // 获取文件最近修改时间
        // - 返回值为时间戳
        // - 如果文件不存在，返回-1
        // - 如果文件存在，返回文件的最近修改时间
        size_t GetLastModifyTime() {
            struct stat s;
            auto ret = stat(filename_.c_str(), &s);
            if (ret == -1)
            {
                return -1;
            }
            return s.st_mtime;
        }

        //
        // 从路径中解析出文件名
        // - 返回值为文件名
        // - 如果文件不存在，返回空字符串
        // - 如果文件存在，返回文件名
        std::string GetFileName() {
            auto pos = filename_.find_last_of("/");
            if (pos == std::string::npos)
                return filename_;
            return filename_.substr(pos + 1);
        }

        //
        // 从文件POS处获取len长度字符给content
        // - 返回值为true表示成功
        // - 返回值为false表示失败
        bool GetPosLen(std::string *content, size_t pos, size_t len) {
            // 判断要求数据内容是否符合文件大小
            if (pos + len > FileSize())
            {
                return false;
            }

            // 打开文件
            std::ifstream ifs;
            ifs.open(filename_.c_str(), std::ios::binary); // 以二进制方式打开文件 防止文件内容被修改
            if (ifs.is_open() == false)
            {
                return false;
            }

            // 读入content
            ifs.seekg(pos, std::ios::beg); // 更改文件指针的偏移量
            content->resize(len);
            ifs.read(&(*content)[0], len); // 从文件流读取到content中
            if (ifs.bad())
            {
                ifs.close();
                return false;
            }
            ifs.close();
            return true;
        }

        //
        // 获取文件内容
        // - 返回值为true表示成功
        // - 返回值为false表示失败
        bool GetContent(std::string *content) {
            return GetPosLen(content, 0, FileSize());
        }
        
        //
        // 将content写入到FileUtil对象的filename_
        //
        bool SetContent(const char *content, size_t len) {
            std::ofstream ofs;
            ofs.open(filename_.c_str(), std::ios::binary); // 如果文件不存在就创建文件
            if (ofs.is_open() == false) {
                return false;
            }
            ofs.write(content, len);
            if (ofs.bad()) {
                ofs.close();
                return false;
            }
            ofs.close();
            return true;
        }

        //
        // 压缩文件并写入到FileUtil对象的filename_
        //
        bool Compress(const std::string &content, int format) {
            std::string packed = bundle::pack(format, content);
            if (packed.size() == 0) {
                return false;
            }
            if (this->SetContent(packed.c_str(), packed.size()) == false) {
                return false;
            }
            return true;
        }

        //
        // 解压文件
        //
        bool UnCompress(std::string &path) {
            std::string compressed;
            if (this->GetContent(&compressed) == false) { // 获取压缩文件内容
                return false;
            }
            FileUtil tem_fu(path); // 创建一个临时文件
            std::string unpacked = bundle::unpack(compressed); // 解压缩
            if (tem_fu.SetContent(unpacked.c_str(), unpacked.size()) == false) {
                return false;
            }
            return true;
        }

        ///////////////////////////////////////////
        // 目录操作
        // 以下三个函数使用c++17中文件系统给的库函数实现
    };
    
    //
    // json序列化和反序列化类
    // - static bool Serialize(const Json::Value &root, std::string &content) : 将json对象序列化为字符串
    // - static bool UnSerialize(const std::string &jsonstr, Json::Value &root) : 将字符串反序列化为json对象
    class JSON_util
    {
    public:
        //
        // json序列化 即将json对象转换为字符串
        // - 返回值为true表示成功
        // - 返回值为false表示失败
        // - root为json对象
        // - content为序列化后的字符串
        static bool Serialize(const Json::Value &root, std::string &content)
        {
            Json::StreamWriterBuilder wbuilder;
            wbuilder["indentation"] = "\t";
            content = Json::writeString(wbuilder, root);
            return true;
        }

        //
        // json反序列化 即将字符串转换为json对象
        // - 返回值为true表示成功
        // - 返回值为false表示失败
        // - jsonstr为json字符串
        // - root为反序列化后的json对象
        static bool UnSerialize(const std::string &jsonstr, Json::Value &root)
        {
            Json::CharReaderBuilder builder;
            std::string errs;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            if (!reader->parse(jsonstr.c_str(), jsonstr.c_str() + jsonstr.size(), &root, &errs))
            {
                return false;
            }
            return true;
        }
    };
}