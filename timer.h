//
// Created by RGAA on 2024-02-05.
//

#ifndef TC_APPLICATION_TIMER_H
#define TC_APPLICATION_TIMER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>

namespace tc
{

    typedef std::function<void()> OnTimeoutCallback;

    class AsyncTimer {

    public:

        static std::shared_ptr<AsyncTimer> Make(const std::shared_ptr<boost::asio::io_context>& ctx, int ms);

        explicit AsyncTimer(const std::shared_ptr<boost::asio::io_context>& ctx, int ms);

        ~AsyncTimer();

        void RegisterTimeoutCallback(OnTimeoutCallback &&callback);

        void Start();

        void CancelTimer();

        // in micro seconds
        long GetTimeLapses();

        long GetTimeLapsesInSecond();

        void UpdateTimeLapses(long time_lapses);

        void EnableUpdateTimeLapses();

        void DisableUpdateTimeLapses();

    private:

        void Timeout(const boost::system::error_code &);

    private:

        bool exit{false};
        // ms
        int duration{0};

        std::shared_ptr<boost::asio::io_context> io_context_ = nullptr;
        boost::asio::deadline_timer boost_timer;

        OnTimeoutCallback timeout_cb;

        std::mutex time_mtx;
        bool update_time_lapses = true;
        long time_lapses = 0;
    };


    typedef std::shared_ptr<AsyncTimer> AsyncTimerPtr;

}


#endif //TC_APPLICATION_TIMER_H
