#pragma once
#include <memory>
#include <mutex>
#include "Util.hpp"


namespace storage
{
    const char *config_path = "Storage.conf";

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
        int bundle_format_; // 压缩格式
        std::string storage_info_;
    public:
        static std::mutex _mutex;  // 声明（告诉编译器存在这个静态成员）
        static Config *_instance; // 声明 单例模式
        Config()
        { 
            // 读取配置文件
            if (!ReadConfig()) {
            }
        }
    public:
        //
        // 读取配置文件 
        //
        bool ReadConfig() {
            Json::Value config_json;
            storage::FileUtil config_file_util(config_path);
            std::string config_str;
            if (!config_file_util.GetContent(&config_str)) // 读取配置文件的字符串
            {
                return false;
            }
            storage::JSON_util::UnSerialize(config_str, config_json); // 将字符串反序列化为json对象

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

        // 获取服务器端口
        int GetServerPort() {
            return server_port_;
        }

        // 获取服务器IP
        std::string GetServerIp() {
            return server_ip_;
        }

        // 获取下载前缀
        std::string GetDownloadPrefix() {
            return download_prefix_;
        }

        // 获取压缩格式
        int GetBundleFormat() {
            return bundle_format_;
        }

        // 获取深度存储目录
        std::string GetDeepStorageDir() {
            return deep_storage_dir_;
        }

        // 获取低存储目录
        std::string GetLowStorageDir() {
            return low_storage_dir_;
        }

        // 获取存储信息文件路径
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