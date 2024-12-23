//
// Created by RGAA on 2024-02-23.
//

#ifndef TC_APPLICATION_CONCURRENT_HASHMAP_H
#define TC_APPLICATION_CONCURRENT_HASHMAP_H

#include <map>
#include <mutex>
#include <functional>
#include <unordered_map>

namespace tc
{

    template<class K, class V>
    class ConcurrentHashMap {
    public:

        void Insert(const K& k, const V& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            inner_[k] = v;
        }

        void Remove(const K& k) {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = inner_.find(k);
            if (it != inner_.end()) {
                inner_.erase(it);
            }
        }

        bool HasKey(const K& k) {
            std::lock_guard<std::mutex> lock(mtx_);
            return inner_.find(k) != inner_.end();
        }

        V Get(const K& k) {
            std::lock_guard<std::mutex> lock(mtx_);
            return inner_.at(k);
        }

        void Apply(const K& k, std::function<void(const V& v)>&& task) {
            std::lock_guard<std::mutex> lock(mtx_);
            if (inner_.find(k) != inner_.end()) {
                task(inner_.at(k));
            }
        }

        void ApplyAll(std::function<void(const K&k, const V& v)>&& task) {
            std::lock_guard<std::mutex> lock(mtx_);
            for (const auto& [k, v] : inner_) {
                task(k, v);
            }
        }

        void VisitAll(std::function<void(K k, V& v)>&& task) {
            std::lock_guard<std::mutex> lock(mtx_);
            for (auto& [k, v] : inner_) {
                task(k, v);
            }
        }

        void VisitAllCond(std::function<bool(K k, V& v)>&& task) {
            std::lock_guard<std::mutex> lock(mtx_);
            for (auto& [k, v] : inner_) {
                if (task(k, v)) {
                    break;
                }
            }
        }

        size_t Size() {
            std::lock_guard<std::mutex> lock(mtx_);
            return inner_.size();
        }

        void Clear() {
            std::lock_guard<std::mutex> lock(mtx_);
            inner_.clear();
        }

    private:
        std::mutex mtx_;
        std::unordered_map<K,V> inner_;
    };

}

#endif //TC_APPLICATION_CONCURRENT_HASHMAP_H
