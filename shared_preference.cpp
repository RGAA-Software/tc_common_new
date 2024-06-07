#include "shared_preference.h"


#include <iostream>

namespace tc 
{

    bool SharedPreference::Init(const std::string& path, const std::string& name) {
        leveldb::Options options;
        options.create_if_missing = true;
        std::string target_path = path;
        if (path.empty()) {
            target_path = ".";
        }
        auto status = leveldb::DB::Open(options, target_path + "/" + name, &db_);
        return status.ok();
    }
    
    void SharedPreference::Release() {
        delete db_;
    }
    
    bool SharedPreference::Put(const std::string& key, const std::string& value) {
        auto st = db_->Put(leveldb::WriteOptions(), key, value);
        return st.ok();
    }

    bool SharedPreference::PutInt(const std::string& key, int value) {
        auto st = db_->Put(leveldb::WriteOptions(), key, std::to_string(value));
        return st.ok();
    }

    std::string SharedPreference::Get(const std::string& key) {
        std::string value;
        db_->Get(leveldb::ReadOptions(), key, &value);
        return value;
    }

    std::string SharedPreference::Get(const std::string& key, const std::string& def) {
        std::string value;
        auto status = db_->Get(leveldb::ReadOptions(), key, &value);
        if (status.ok()) {
            return value;
        } else {
            return def;
        }
    }

    int SharedPreference::GetInt(const std::string& key, int def) {
        std::string value;
        auto status = db_->Get(leveldb::ReadOptions(), key, &value);
        if (status.ok()) {
            return std::atoi(value.c_str());
        } else {
            return def;
        }
    }

    bool SharedPreference::Remove(const std::string& key) {
        auto st = db_->Delete(leveldb::WriteOptions(), key);
        return st.ok();
    }
    
    void SharedPreference::Visit(IVisitListener&& listener) {
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
