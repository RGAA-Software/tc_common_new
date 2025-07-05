//
// Created by RGAA on 2024/2/27.
//

#ifndef TC_APPLICATION_CONCURRENT_VECTOR_H
#define TC_APPLICATION_CONCURRENT_VECTOR_H

#include <mutex>
#include <vector>
#include <functional>

namespace tc
{

    template<typename T>
    class ConcurrentVector {
    public:
        void PushBack(const T& t) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.push_back(t);
        }

        int Size() {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.size();
        }

        T At(int idx) {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.at(idx);
        }

        void Visit(std::function<void(const T&)>&& cbk) {
            std::lock_guard<std::mutex> guard(mtx_);
            for (const auto& t : inner_) {
                cbk(t);
            }
        }

        void RemoveFirst() {
            std::lock_guard<std::mutex> guard(mtx_);
            if (!inner_.empty()) {
                inner_.erase(inner_.begin());
            }
        }

    private:
        std::mutex mtx_;
        std::vector<T> inner_;

    };

}

#endif //TC_APPLICATION_CONCURRENT_VECTOR_H
