#pragma once
#include <memory>
#include <mutex>
#include "Util.hpp"


namespace storage
{
    const char *Config_File = "Storage.conf";

    //
    // 配置文件读取类
    // 
    class Config
    {
    private:
        int server_port_;
        std::string server_ip_;
        std::string download_prefix_;
        std::string deep_storage_dir_;
        std::string low_storage_dir_;
        int bundle_format_;
        std::string storage_info_;
    public:
        static std::mutex _mutex;  // 声明（告诉编译器存在这个静态成员）
        static Config *_instance; // 声明 单例模式
        Config()
        {
            if (!ReadConfig()) // 读取配置文件
            {
            }
        }
    public:
        //
        // 读取配置文件 
        //
        bool ReadConfig()
        {
            
            Json::Value config_json;
            storage::FileUtil config_file(Config_File);
            std::string config_str;
            if (!config_file.GetContent(&config_str))
            {
                return false;
            }
            storage::JSON_util::UnSerialize(config_str, config_json);

            server_port_ = config_json["server_port"].asInt();
            server_ip_ = config_json["server_ip"].asString();
            download_prefix_ = config_json["download_prefix"].asString();
            deep_storage_dir_ = config_json["deep_storage_dir"].asString();
            low_storage_dir_ = config_json["low_storage_dir"].asString();
            bundle_format_ = config_json["bundle_format"].asInt();
            storage_info_ = config_json["storage_info"].asString();
            return true;
        }

    public:
        //
        // 获取配置文件内容
        //
        int GetServerPort() {
            return server_port_;
        }
        std::string GetServerIp() {
            return server_ip_;
        }
        std::string GetDownloadPrefix() {
            return download_prefix_;
        }
        int GetBundleFormat() {
            return bundle_format_;
        }
        std::string GetDeepStorageDir() {
            return deep_storage_dir_;
        }
        std::string GetLowStorageDir() {
            return low_storage_dir_;
        }
        std::string GetStorageInfoFile() {
            return storage_info_;
        }

        //
        // 单例模式
        //
        static Config *GetInstance() {
            if (_instance == nullptr) {
                _mutex.lock();
                if (_instance == nullptr) {
                    _instance = new Config();
                }
                _mutex.unlock();
            }
            return _instance;
        }

    };
    // 静态成员初始化，先写类型再写类域
    // 在 C++ 中，静态成员变量 属于类本身（而非类的某个对象），因此需要在类外进行定义和初始化。
    std::mutex Config::_mutex; // 定义（告诉编译器存在这个静态成员）
    Config *Config::_instance = nullptr;
}