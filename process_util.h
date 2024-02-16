//
// Created by Administrator on 2024/2/13.
//

#ifndef TC_APPLICATION_PROCESS_UTIL_H
#define TC_APPLICATION_PROCESS_UTIL_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <boost/process.hpp>

namespace bp = boost::process;

namespace tc {
#ifdef WIN32
    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context);
#endif

    class ProcessUtil {
    public:

        static bool StartProcess(const std::string& exe_path, const std::vector<std::string>& args);

    };

}

#endif //TC_APPLICATION_PROCESS_UTIL_H
