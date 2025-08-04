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

const std::string kExploreName = "explorer.exe";

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
        // Will run in the same user with the executor
        static bool StartProcessInSameUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait);
        // Will run in current user
        static bool StartProcessInCurrentUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait);
        static uint32_t GetCurrentSessionId();
        static uint32_t GetProcessSessionId(uint32_t pid);
        static int GetThreadCount();
        static void SetProcessInHighLevel();
        static int GetPidByExeName(const std::string& exe_name);
        // By explorer.exe
        static HANDLE DupAdminToken();
        static bool RunAsAdminWithShell(const std::wstring& path, const std::wstring& args = L"");
    };

}
#endif
#endif //TC_APPLICATION_PROCESS_UTIL_H
