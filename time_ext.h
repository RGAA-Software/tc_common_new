//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_TIMEEXT_H
#define TC_APPLICATION_TIMEEXT_H

#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>

namespace tc
{

    class TimeExt {
    public:

        static uint64_t GetCurrentTimestamp() {
            std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp
                    = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            std::time_t timestamp = tp.time_since_epoch().count();
            return timestamp;
        }

        static std::string FormatTimestamp(uint64_t time, bool with_ms = false) {
            time_t seconds = time / 1000;
            std::tm timeinfo = *std::localtime(&seconds);
            char buffer[80];
            std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", &timeinfo);
            return std::string(buffer) + (with_ms ? ("." + std::to_string(time % 1000)) : "");
        }

        static uint64_t GetCurrentTimePointUS() {
            auto now = std::chrono::high_resolution_clock::now();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
            return microseconds;
        }
    };

    class TimeDuration {
    public:
        TimeDuration(const std::string& name);
        ~TimeDuration();

    private:
        uint64_t begin_ts_ = 0;
        std::string name_;
    };

}

#endif //TC_APPLICATION_TIMEEXT_H
