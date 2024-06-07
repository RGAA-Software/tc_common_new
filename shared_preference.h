#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <functional>
#include <leveldb/db.h>

namespace tc
{

    typedef std::function<void(const std::string& key, const std::string& val)> IVisitListener;

    // 存储Key - Value值
    class SharedPreference {
    public:

        static SharedPreference* Instance() {
            static SharedPreference instance;
            return &instance;
        }

        SharedPreference() = default;

        bool Init(const std::string& path, const std::string& name);
        void Release();

        bool Put(const std::string& key, const std::string& value);
        bool PutInt(const std::string& key, int value);
        std::string Get(const std::string& key);
        std::string Get(const std::string& key, const std::string& def);
        int GetInt(const std::string& key, int def = 0);
        bool Remove(const std::string& key);

        void Visit(IVisitListener&& listener);

    private:

        leveldb::DB* db_;
};

}

#endif // DATABASE_H
