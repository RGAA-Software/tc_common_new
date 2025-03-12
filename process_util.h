//
// Created by RGAA  on 2024/2/13.
//

#ifndef TC_APPLICATION_PROCESS_UTIL_H
#define TC_APPLICATION_PROCESS_UTIL_H

#ifdef WIN32
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace tc
{
    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context);

    class ProcessUtil {
    public:
        static bool StartProcessAndWait(const std::string& exe_path, const std::vector<std::string>& args);
        static uint32_t StartProcess(const std::string& exe_path, const std::vector<std::string>& args, bool detach, bool wait);
        static std::vector<std::string> StartProcessAndOutput(const std::string& exe_path, const std::vector<std::string>& args);
        static bool StartProcessInWorkDir(const std::string& work_dir, const std::string& cmdline, const std::vector<std::string>& args);
        static bool KillProcess(unsigned long pid);
        static bool StartProcessAsUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait);
        static uint32_t GetCurrentSessionId();
        static uint32_t GetProcessSessionId(uint32_t pid);
    };

}
#endif
#endif //TC_APPLICATION_PROCESS_UTIL_H
