//
// Created by RGAA on 2024-02-23.
//

#ifndef TC_APPLICATION_CONCURRENT_HASHMAP_H
#define TC_APPLICATION_CONCURRENT_HASHMAP_H

#include <map>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <optional>
#include "log.h"

namespace tc
{

    template<class K, class V>
    class ConcurrentHashMap {
    public:

        void Insert(const K& k, const V& v) {
            std::lock_guard<std::mutex> lock(mtx_);
            inner_[k] = v;
        }

        std::optional<V> Remove(const K& k) {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = inner_.find(k);
            if (it != inner_.end()) {
                auto v = inner_[k];
                inner_.erase(it);
                return v;
            }
            return std::nullopt;
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

        void ApplyAllCond(std::function<bool(const K& k, const V& v)>&& task) {
            std::lock_guard<std::mutex> lock(mtx_);
            for (auto& [k, v] : inner_) {
                if (task(k, v)) {
                    break;
                }
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

        // 0-based
        std::optional<std::vector<V>> QueryRange(int begin, int end) {
            std::lock_guard<std::mutex> lock(mtx_);
            std::vector<V> values;
            // overflow
            if (begin >= inner_.size()) {
                //LOGI("Overflow, begin: {}, total: {}", begin, inner_.size());
                return std::nullopt;
            }

            auto it_beg = inner_.begin();
            std::advance(it_beg, begin);

            decltype(it_beg) it_end;

            if (begin < inner_.size() && end >= inner_.size()) {
                // portion
                it_end = inner_.end();
                //LOGI("Portion, {} -> {}", begin, inner_.size());
            }
            else {
                // whole
                it_end = it_beg;
                std::advance(it_end, end - begin);
                //LOGI("Whole");
            }
            for (; it_beg != it_end; ++it_beg) {
                auto pair = *it_beg;
                values.push_back(pair.second);
                //LOGI("--> {}", pair.first);
            }
            return values;
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
        std::map<K,V> inner_;
    };

}

#endif //TC_APPLICATION_CONCURRENT_HASHMAP_H
