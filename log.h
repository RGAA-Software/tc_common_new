#ifndef __SK_LOG__
#define __SK_LOG__

#include <stdio.h>
#include <stdarg.h>

#define SPDLOG_NAME         "spd.log"
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <iostream>

#define LOG_FILE 1

namespace tc
{

    #define LOGD_ SPDLOG_LOGGER_DEBUG
    #define LOGI_ SPDLOG_LOGGER_INFO
    #define LOGW_ SPDLOG_LOGGER_WARN
    #define LOGE_ SPDLOG_LOGGER_ERROR

    class Logger {
    public:

        static bool InitLog(const std::string& path, bool save_to_file = false) {
            if (save_to_file) {
                std::string strFilePath = path;
                try {
                    std::shared_ptr<spdlog::logger> logger;
                    logger = spdlog::rotating_logger_mt(SPDLOG_NAME, strFilePath, 1024 * 1024 * 3, 5, false);
                    spdlog::set_default_logger(logger);
                    logger->set_level(spdlog::level::debug);
                    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%s:%#,%!][%l] : %v");
                }
                catch (const spdlog::spdlog_ex& ex) {
                    std::cout << "Log initialization failed: " << ex.what() << std::endl;
                    return false;
                }
            }
            else {
                //spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%s:%#,%!][%l] : %v");
                spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e][thread %t][%s:%#][%l] : %v");
            }
            return true;
        }
    };

    #define LOGI(...) LOGI_(spdlog::default_logger_raw(), __VA_ARGS__)
    #define LOGD(...) LOGD_(spdlog::default_logger_raw(), __VA_ARGS__)
    #define LOGW(...) LOGW_(spdlog::default_logger_raw(), __VA_ARGS__)
    #define LOGE(...) LOGE_(spdlog::default_logger_raw(), __VA_ARGS__)
}


#endif
