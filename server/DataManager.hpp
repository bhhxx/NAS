#pragma once
#include "Config.hpp"
#include <unordered_map>
namespace storage
{
    //
    // 存储的文件需要包含的信息
    //     : 文件路径 修改时间 创建时间 url
    //
    struct StorageInfo
    {
        time_t mtime_;
        time_t ctime_;
        size_t fsize_;
        std::string storage_path_;
        std::string url_;
    };

    //
    // 文件管理器类
    //
    class DataManager
    {
    private:
        std::string storage_info_file_; // 保存存储文件信息的文件
        std::unordered_map<std::string, StorageInfo> storage_info_map_; // 保存存储文件信息的map
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
            storage::FileUtil storage_info_(storage_info_file_);
            if (!storage_info_.Exist()) {
                return false;
            }
            Json::Value root;
            std::string infostr;
            if (!storage_info_.GetContent(&infostr)) {
                return false;
            }
            JSON_util::UnSerialize(infostr, root);
            for (auto e : root) {
                StorageInfo info;
                info.storage_path_ = e["storage_path"].asString();
                info.mtime_ = e["mtime"].asUInt64();
                info.ctime_ = e["ctime"].asUInt64();
                info.url_ = e["url"].asString();
                Insert(info);
            }
            return true;
        }

        // 
        // 插入一条存储信息到map
        //
        bool Insert(const StorageInfo &info) {
            storage_info_map_[info.url_] = info;
            if (Store() == false) {
                return false;
            }
            return true;
        }

        //
        // 将被存储文件的信息存到文件中
        //
        bool Store() {
            std::vector<StorageInfo> infovec;
            if (!GetInfo(&infovec))
            {
                return false;
            }
            Json::Value root;
            for (auto e : infovec)
            {
                Json::Value item;
                item["storage_path_"] = e.storage_path_;
                item["mtime_"] = e.mtime_;
                item["ctime_"] = e.ctime_;
                item["url_"] = e.url_;
                item["storage_path_"] = e.storage_path_;
                root.append(item);
            }
            std::string infostr;
            JSON_util::Serialize(root, infostr);
            FileUtil f(storage_info_file_);
            return true;
        }

        // 
        // 读取map中存储的信息
        //
        bool GetInfo(std::vector<StorageInfo> *arry)
        {
            for (auto e : storage_info_map_)
            {
                arry->emplace_back(e.second);
            }
            return true;
        }
    };
}