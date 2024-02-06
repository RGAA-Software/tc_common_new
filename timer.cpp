//
// Created by RGAA on 2024-02-05.
//

#include "timer.h"
#include <boost/bind.hpp>

namespace tc
{

    AsyncTimerPtr AsyncTimer::Make(const std::shared_ptr<boost::asio::io_context>& ctx, int ms) {
        return std::make_shared<AsyncTimer>(ctx, ms);
    }

    AsyncTimer::AsyncTimer(const std::shared_ptr<boost::asio::io_context>& ctx, int ms) :
            duration(ms),
            io_context_(ctx),
            boost_timer(*io_context_, boost::posix_time::milliseconds (duration)) {
    }

    AsyncTimer::~AsyncTimer() = default;

    void AsyncTimer::Timeout(const boost::system::error_code &) {
        if (exit) {
            return;
        }

        boost_timer.expires_at(boost_timer.expires_at() + boost::posix_time::milliseconds (duration));
        auto callback = boost::bind(&AsyncTimer::Timeout, this, boost::placeholders::_1);
        boost_timer.async_wait(callback);

        {
            std::lock_guard<std::mutex> guard(time_mtx);
            if (update_time_lapses) {
                time_lapses += duration;
            }
        }

        if (timeout_cb) {
            timeout_cb();
        }
    }

    void AsyncTimer::Start() {
        auto callback = boost::bind(&AsyncTimer::Timeout, this, boost::placeholders::_1);
        boost_timer.async_wait(callback);
    }

    void AsyncTimer::CancelTimer() {
        exit = true;
    }

    void AsyncTimer::RegisterTimeoutCallback(OnTimeoutCallback &&callback) {
        timeout_cb = std::move(callback);
    }

    long AsyncTimer::GetTimeLapses() {
        std::lock_guard<std::mutex> guard(time_mtx);
        return time_lapses;
    }

    long AsyncTimer::GetTimeLapsesInSecond() {
        std::lock_guard<std::mutex> guard(time_mtx);
        return time_lapses / 1000000;
    }

    void AsyncTimer::UpdateTimeLapses(long time_lapses) {
        std::lock_guard<std::mutex> guard(time_mtx);
        this->time_lapses = time_lapses * 1000000;
    }

    void AsyncTimer::EnableUpdateTimeLapses() {
        update_time_lapses = true;
    }

    void AsyncTimer::DisableUpdateTimeLapses() {
        update_time_lapses = false;
    }

}
