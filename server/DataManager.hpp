#pragma once
#include "Config.hpp"
#include <unordered_map>
namespace storage
{
    //
    // 存储信息结构体
    //
    struct StorageInfo
    {
        time_t mtime_;
        time_t atime_;
        size_t fsize_;
        std::string storage_path_; // 文件存储路径
        std::string url_;          // 请求URL中的资源路径 
    };

    //
    // 文件管理器类
    // 传入一个文件名，创建一个DataManager对象，该对象可以对该文件进行操作
    // - bool Insert(const StorageInfo &info) 插入一条存储信息到文件管理器类初始化时的storage_info_file_中
    // - bool Store() 将文件管理器类中的信息存到storage_info_file_中
    // - bool GetInfo(std::vector<StorageInfo> *arry) 读取文件管理器类中的信息
    // - bool InitLoad() 初始化文件管理器类

    class DataManager
    {
    private:
        std::string storage_info_file_; // 保存存储文件信息的文件
        std::unordered_map<std::string, StorageInfo> storage_info_map_; // 保存存储文件信息的map key为url value为StorageInfo对象
    public:
        //
        // 类构造
        //
        DataManager() {
            storage_info_file_ = storage::Config::GetInstance()->GetStorageInfoFile();
            InitLoad();
        }
        ~DataManager(){}
        
        //
        // 初始化文件管理器
        //
        bool InitLoad() {
            storage::FileUtil storage_info_util(storage_info_file_);
            if (!storage_info_util.Exist()) { // 文件操作类绑定的文件不存在
                return false;
            }
            Json::Value root;
            std::string infoStr;
            if (!storage_info_util.GetContent(&infoStr)) { // 获取文件内容
                return false;
            }
            JSON_util::UnSerialize(infoStr, root); // 将文件内容反序列化为json对象
            for (auto e : root) { // 遍历json对象
                StorageInfo info;
                info.storage_path_ = e["storage_path_"].asString();
                info.mtime_ = e["mtime_"].asUInt64();
                info.atime_ = e["atime_"].asUInt64();
                info.url_ = e["url_"].asString();
                info.fsize_ = e["fsize_"].asUInt64();
                FileUtil file_util(info.storage_path_);
                if (!file_util.Exist()) { // 文件不存在
                    continue;
                }
                Insert(info); // 将json对象中的信息插入到map中
            }
            return true;
        }

        // 
        // 插入一条存储信息到map并将map的信息存到storage_info_file_中
        //
        bool Insert(const StorageInfo &info) {
            storage_info_map_[info.url_] = info;
            if (Store() == false) {
                return false;
            }
            return true;
        }

        //
        // 将map的信息存到storage_info_file_中
        //
        bool Store() {
            std::vector<StorageInfo> infoVector;
            if (!GetInfo(&infoVector)) { // 获取map中存储的信息
                return false;
            }
            Json::Value root;
            for (auto e : infoVector) { // 将map中的信息转换为json对象
                Json::Value item;
                item["storage_path_"] = e.storage_path_;
                item["mtime_"] = e.mtime_;
                item["atime_"] = e.atime_;
                item["url_"] = e.url_;
                item["fsize_"] = e.fsize_;
                root.append(item);
            }
            std::string infoStr; // 存储json对象的字符串
            JSON_util::Serialize(root, infoStr); // 将json对象序列化为字符串
            FileUtil storage_info_util(storage_info_file_);
            if (!storage_info_util.SetContent(infoStr.c_str(), infoStr.size())) { // 将字符串写入文件
                return false;
            }
            return true;
        }

        // 
        // 根据URL获取文件存储信息
        //
        bool GetOneByURL(const std::string &key, StorageInfo *info)
        {
            // URL是key，所以直接find()找
            if (storage_info_map_.find(key) == storage_info_map_.end()) {
                return false;
            }
            *info = storage_info_map_[key]; // 获取url对应的文件存储信息
            return true;
        }

        // 
        // 读取map中存储的信息
        //
        bool GetInfo(std::vector<StorageInfo> *arry) {
            for (auto e : storage_info_map_) {
                arry->emplace_back(e.second);
            }
            return true;
        }
    };
}