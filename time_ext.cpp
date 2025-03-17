//
// Created by RGAA on 2023-12-17.
//

#include "time_ext.h"
#include "log.h"

namespace tc
{

    TimeDuration::TimeDuration(const std::string& name) {
        name_ = name;
        begin_ts_ = TimeExt::GetCurrentTimestamp();
    }

    TimeDuration::~TimeDuration() {
        auto end_ts_ = TimeExt::GetCurrentTimestamp();
        LOGI("[{}] used {}ms", name_, (end_ts_ - begin_ts_));
    }

}