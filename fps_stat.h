//
// Created by RGAA on 2024/1/8.
//

#ifndef PLC_CONTROLLER_FPS_STAT_H
#define PLC_CONTROLLER_FPS_STAT_H

#include<cstdint>
#include <queue>
#include <chrono>

namespace tc
{

    class FpsStat {
    public:
        FpsStat() {}

        ~FpsStat() {}

        void Tick() {
            auto now = std::chrono::steady_clock::now();
            while (!tick_points_.empty() && tick_points_.front() < (now - std::chrono::seconds(1))) {
                tick_points_.pop();
            }
            tick_points_.push(now);
        }

        int value() {
            int count = tick_points_.size();
            while (!tick_points_.empty()) {
                tick_points_.pop();
            }
            return count;
        }

    private:
        std::queue<std::chrono::steady_clock::time_point> tick_points_; // class FpsStat
    };
}

#endif //PLC_CONTROLLER_FPS_STAT_H
