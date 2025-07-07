//
// Created by RGAA on 2024/2/27.
//

#ifndef TC_APPLICATION_CONCURRENT_QUEUE_H
#define TC_APPLICATION_CONCURRENT_QUEUE_H

#include <mutex>
#include <queue>
#include <vector>
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

        void PushBack(const T& e) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.push_back(e);
        }

        void PushBack(T&& e) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.push_back(std::move(e));
        }

        void PopFront() {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.pop_front();
        }

        void PopBack() {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_.pop_back();
        }

        std::vector<T> ToVector() {
            std::lock_guard<std::mutex> guard(mtx_);
            std::vector<T> r;
            for (const auto& v : inner_) {
                r.push_back(v);
            }
            return r;
        }

    private:
        std::mutex mtx_;
        std::deque<T> inner_;
    };

}

#endif //TC_APPLICATION_CONCURRENT_QUEUE_H
