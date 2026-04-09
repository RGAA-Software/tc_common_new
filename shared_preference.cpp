#include "shared_preference.h"

#include <filesystem>
#include <iostream>

#include "log.h"
#include "string_util.h"

namespace tc 
{

    bool SharedPreference::Init(const std::wstring& path, const std::string& name) {
        leveldb::Options options;
        options.create_if_missing = true;
        std::wstring target_path = path;
        if (path.empty()) {
            target_path = L".";
        }

        std::filesystem::path p(target_path);
        p /= name;

        std::u8string u8 = p.u8string();
        std::string str(
            reinterpret_cast<const char*>(u8.data()),
            u8.size()
        );
        LOGI("SharedPreference: {}", str);
        auto status = leveldb::DB::Open(options, str, &db_);
        return status.ok();
    }
    
    void SharedPreference::Release() const {
        if (db_) {
            delete db_;
        }
    }
    
    bool SharedPreference::Put(const std::string& key, const std::string& value) const {
        if (!db_) {
            return false;
        }
        auto st = db_->Put(leveldb::WriteOptions(), key, value);
        return st.ok();
    }

    bool SharedPreference::PutInt(const std::string& key, int value) const {
        if (!db_) {
            return false;
        }
        auto st = db_->Put(leveldb::WriteOptions(), key, std::to_string(value));
        return st.ok();
    }

    std::string SharedPreference::Get(const std::string& key) const {
        if (!db_) {
            return "";
        }
        std::string value;
        db_->Get(leveldb::ReadOptions(), key, &value);
        return value;
    }

    std::string SharedPreference::Get(const std::string& key, const std::string& def) const {
        if (!db_) {
            return def;
        }
        std::string value;
        auto status = db_->Get(leveldb::ReadOptions(), key, &value);
        if (status.ok()) {
            return value;
        } else {
            return def;
        }
    }

    int SharedPreference::GetInt(const std::string& key, int def) const {
        if (!db_) {
            return 0;
        }
        std::string value;
        auto status = db_->Get(leveldb::ReadOptions(), key, &value);
        if (status.ok()) {
            return std::atoi(value.c_str());
        } else {
            return def;
        }
    }

    bool SharedPreference::Remove(const std::string& key) const {
        if (!db_) {
            return false;
        }
        auto st = db_->Delete(leveldb::WriteOptions(), key);
        return st.ok();
    }
    
    void SharedPreference::Visit(IVisitListener&& listener) const {
        if (!listener) {
            return;
        }
        leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            listener(it->key().ToString(), it->value().ToString());
        }
        delete it;
    }

}
