#ifdef WIN32

#include "fft_32.h"
#include <fftw3.h>
#include <cmath>
#include <iostream>
#include "data.h"

namespace tc
{

    std::mutex FFT32::fft_mtx_;

    void FFT32::DoFFT(std::vector<double> &fft, const std::shared_ptr<Data>& one_channel_pcm_data, int bytes, bool pre_alloc_fft) {
        std::lock_guard<std::mutex> guard(fft_mtx_);
        int bytes_size = one_channel_pcm_data->Size();
        if (bytes_size > bytes) {
            bytes_size = bytes;
        }
        fftw_complex *din, *out;
        fftw_plan p;

        int divider_by = 2;
        int single_channel_double_size = bytes_size / divider_by;

        din = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * single_channel_double_size);
        out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * single_channel_double_size);

        auto data = one_channel_pcm_data->DataAddr();
        for (int i = 0; i < bytes_size; i = i + divider_by) {
            int32_t d1 = data[i];
            int32_t d2 = data[i + 1];
            auto val = (d2 << 8) + d1;
            auto scale_val = val * 1.0f / 32767 * 5;
            din[i / divider_by][0] = scale_val;
            din[i / divider_by][1] = 0;
        }

        p = fftw_plan_dft_1d(single_channel_double_size, din, out, FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(p);
        fftw_destroy_plan(p);
        fftw_cleanup();

        auto target_size = std::min(480, single_channel_double_size);
        for (int i = 0; i < target_size; i++) {
            auto re = out[i][0];
            auto im = out[i][1];
            double m = sqrt((re * re) + (im * im));
            auto val = 36 * log(m);
            if (pre_alloc_fft) {
                fft[i] = val;
            } else {
                fft.push_back(val);
            }
        }

        if (din != nullptr) {
            fftw_free(din);
        }
        if (out != nullptr) {
            fftw_free(out);
        }
    }
}
#endif