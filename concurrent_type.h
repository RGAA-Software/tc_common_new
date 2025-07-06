//
// Created by RGAA on 6/07/2025.
//

#ifndef GAMMARAY_CONCURRENT_TYPE_H
#define GAMMARAY_CONCURRENT_TYPE_H

#include <mutex>
#include <string>

namespace tc
{
    template<typename T>
    class ConcurrentType {
    public:
        ConcurrentType() {

        }

        ConcurrentType(const ConcurrentType& other) {
            T other_value = other.Load();
            this->Update(other_value);
        }

        void Update(const T& s) {
            std::lock_guard<std::mutex> guard(mtx_);
            inner_ = s;
        }

        T Load() const {
            std::lock_guard<std::mutex> guard(mtx_);
            return inner_;
        }

        ConcurrentType<T>& operator=(const ConcurrentType<T>& other) {
            if (this != &other) {
                T other_value = other.Load();
                this->Update(other_value);
            }
            return *this;
        }

        ConcurrentType<T>& operator=(const T& other) {
            this->Update(other);
            return *this;
        }

        bool operator == (const T& other) {
            return this->Load() == other;
        }

        bool operator != (const T& other) {
            return this->Load() != other;
        }

    private:
        mutable std::mutex mtx_;
        T inner_;
    };

    using ConcurrentString = tc::ConcurrentType<std::string>;
}

#endif //GAMMARAY_CONCURRENT_TYPE_H
