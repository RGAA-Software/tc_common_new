#pragma once

#ifdef WIN32
#include "tc_common_new/data.h"

#include <mutex>
#include <vector>
#include <functional>

namespace tc
{

    class FFT32 {
    public:

        static void DoFFT(std::vector<double> &fft, const DataPtr& one_channel_pcm_data, int bytes = 0, bool pre_alloc_fft = false);
        static std::mutex fft_mtx_;

    };

}
#endif