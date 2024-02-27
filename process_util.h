//
// Created by Administrator on 2024/2/13.
//

#ifndef TC_APPLICATION_PROCESS_UTIL_H
#define TC_APPLICATION_PROCESS_UTIL_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <boost/process.hpp>
namespace bp = boost::process;

namespace tc
{
    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context);

    class ProcessUtil {
    public:

        static bool StartProcess(const std::string& exe_path, const std::vector<std::string>& args);
        static std::vector<std::string> StartProcessAndOutput(const std::string& exe_path, const std::vector<std::string>& args);
        static bool KillProcess(unsigned long pid);
    };

}
#endif
#endif //TC_APPLICATION_PROCESS_UTIL_H
