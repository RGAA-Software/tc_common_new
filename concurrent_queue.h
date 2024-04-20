//
// Created by RGAA on 2024/2/27.
//

#ifndef TC_APPLICATION_CONCURRENT_QUEUE_H
#define TC_APPLICATION_CONCURRENT_QUEUE_H

#include <mutex>
#include <queue>
#include <functional>

namespace tc
{

    template<typename T>
    class ConcurrentQueue {
    public:

        const T& Front() {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.front();
        }

        const T& Back() {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.back();
        }

        bool Empty() {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.empty();
        }

        size_t Size() {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_.size();
        }

        void Push(const T& e) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.push(e);
        }

        void Push(T&& e) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.push(std::move(e));
        }

        void Pop() {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.pop();
        }

    private:
        std::mutex mtx_;
        std::queue<T> inner_;
    };

}

#endif //TC_APPLICATION_CONCURRENT_QUEUE_H
