//
// Created by RGAA on 2024-02-05.
//

#ifndef TC_APPLICATION_PROCESS_HELPER_H
#define TC_APPLICATION_PROCESS_HELPER_H

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

#include "tc_common_new/response.h"

namespace tc
{

    struct ProcessInfo {
        std::string exe_name_{};
        bool is_x86_{};
        uint32_t pid_{};
        uint32_t ppid_{};
        uint32_t thread_id_{};
        HWND hwnd_{};

        [[nodiscard]] bool Valid() const {
            return pid_ > 0 && !exe_name_.empty();
        }
    };

    class WindowInfo {
    public:
        std::pair<int, int> GetWindowSize() const {
            if (!win_handle) {
                return std::make_pair(0, 0);
            }
            RECT rect;
            GetWindowRect(win_handle, &rect);
            return std::make_pair((rect.right - rect.left), (rect.bottom - rect.top));
        }

    public:
        DWORD pid;
        DWORD thread_id;
        HWND win_handle = nullptr;
        std::wstring title;
        std::wstring exe_name;
        std::wstring exe_path;
        std::string claxx;
    };

    class WindowInfos {
    public:

        int pid = 0;
        int filter_window_size = -1;
        std::vector<WindowInfo> infos;

    };

    class ProcessHelper {
    public:

        static RespBoolBool IsProcessX86Arch(uint32_t pid);
        static std::vector<ProcessInfo> GetProcessList();
        static bool CloseProcess(DWORD pid);
        static Response<bool, uint32_t> GetParentPid(uint32_t pid);
        static bool isChildOf(uint32_t child, uint32_t parent);
        static std::vector<uint32_t> FindAllChildProcess(uint32_t pid, const std::string& excludeProcessName = "");
        static uint32_t GetCurrentProcessId();
        static WindowInfos GetWindowInfoByPid(DWORD pid, int filter_window_size = 256);
        static bool GetWindowPositionByHwnd(HWND hwnd, RECT& rect);
    };

}

#endif //TC_APPLICATION_PROCESS_HELPER_H