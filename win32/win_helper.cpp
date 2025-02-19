//
// Created by RGAA on 2024/2/2.
//

#include "win_helper.h"

#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <winternl.h>
#include <filesystem>
#include <QProcess>
#include <QStringList>
#include <QString>
#include "tc_common_new/string_ext.h"

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "ntdll.lib")

constexpr auto kInjector32 = "";
constexpr auto kInjector64 = "";

constexpr auto kMaxTexBufSize = 1024;

namespace tc
{

    Response<bool, bool>
    WinHelper::IsDllInjected(uint32_t pid, const std::string &x86_dll_name, const std::string &x64_dll_name) {
        auto resp = Response<bool, bool>::Make(false, false);
        HANDLE hnd_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hnd_process == nullptr) {
            LOGE("IsAlreadyInject OpenProcess failed.");
            return resp;
        }

        BOOL x86;
        if (!IsWow64Process(hnd_process, &x86)) {
            LOGE("IsWow64Process failed with: {}", GetLastError());
            CloseHandle(hnd_process);
            return resp;
        }

        bool ret_val = false;
        HMODULE h_modules[1024];
        DWORD needed = sizeof(h_modules);
        if (EnumProcessModulesEx(hnd_process, h_modules, needed,
                                 &needed, x86 ? LIST_MODULES_32BIT : LIST_MODULES_64BIT)) {
            for (int i = 0; i < needed / sizeof(HMODULE); ++i) {
                char name[MAX_PATH] = {0};
                if (GetModuleBaseNameA(hnd_process, h_modules[i], name, sizeof(name) / sizeof(WCHAR))) {
                    //_strlwr(name);
                    if (x86) {
                        if (QString::compare(QString::fromStdString(x86_dll_name), QString::fromStdString(name), Qt::CaseInsensitive) == 0) {
                            ret_val = true;
                            break;
                        }
                    } else {
                        if (QString::compare(QString::fromStdString(x64_dll_name), QString::fromStdString(name), Qt::CaseInsensitive) == 0) {
                            ret_val = true;
                            break;
                        }
                    }
                } else {
                    LOGE("GetModuleBaseNameA failed with: {}", GetLastError());
                }
            }
        } else {
            LOGE("EnumProcessModulesEx failed with: {}", GetLastError());
        }

        CloseHandle(hnd_process);

        resp.ok_ = true;
        resp.value_ = ret_val;
        return resp;
    }

    Response<bool, bool>
    WinHelper::InjectDll(uint32_t pid, uint32_t tid, const std::string &x86_dll_name, const std::string &x64_dll_name) {
        auto resp = Response<bool, bool>::Make(false, false);
        auto is_x86 = IsX86Arch(pid);
        if (!is_x86.ok_) {
            LOGE("Check arch failed when InjectDll...");
            return resp;
        }

        std::string target_dll = is_x86.value_ ? x86_dll_name : x64_dll_name;
        std::string injector = is_x86.value_ ? kInjector32 : kInjector64;
        std::string cheat_anti = "0";
        std::string pid_str = std::to_string(pid);
        // todo: Test it.
        QStringList injector_args;
        injector_args << target_dll.c_str() << cheat_anti.c_str() << QString::number(pid);
        QProcess process;
        process.start(injector.c_str(), injector_args);
        process.waitForFinished();
        if (process.exitCode() == 0) {

        } else {

        }
#if 0
        try {
            bp::child child_process(injector, target_dll, cheat_anti, pid_str, bp::std_out > stdout);
            child_process.wait();
            if (child_process.exit_code() == 0) {
                LOGI("Inject exit success!");
            } else {
                LOGE("Inject exit failed!");
            }
        } catch (const std::exception &e) {
            LOGE("Create process for injecing failed: {}", e.what());
            return resp;
        }
#endif

        return resp;
    }

    Response<bool, bool> WinHelper::IsX86Arch(uint32_t pid) {
        auto resp = Response<bool, bool>::Make(false, false);
        HANDLE hnd_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hnd_process == nullptr) {
            return resp;
        }
        BOOL px86;
        if (!IsWow64Process(hnd_process, &px86)) {
            CloseHandle(hnd_process);
            return resp;
        }
        CloseHandle(hnd_process);
        resp.ok_ = true;
        resp.value_ = px86;
        return resp;
    }

    Response<bool, std::string> WinHelper::GetPathByHwnd(HWND hwnd) {
        auto ret = Response<bool, std::string>::Make(false, "");
        DWORD dwPid = -1;
        GetWindowThreadProcessId(hwnd, &dwPid);
        if (dwPid == -1)
            return ret;

        HANDLE handle = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, dwPid);
        if (handle == NULL)
            return ret;
        std::shared_ptr<void> hprocessAutoFree(NULL, [=](void *) {
            CloseHandle(handle);
        });

        wchar_t path[4096] = {0};
        DWORD len = sizeof(path) / sizeof(*path);
        if (!QueryFullProcessImageNameW(handle, 0, path, &len)) {
            LOGW("QueryFullProcessImageNameW failed.");
            return ret;
        }
        ret.ok_ = true;
        ret.value_ = StringExt::ToUTF8(path);
        return ret;
    }

    Response<bool, std::string> WinHelper::GetErrorStr(HRESULT hr) {
        wchar_t buffer[4096] = {0};
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buffer, sizeof(buffer) / sizeof(*buffer), NULL);
        return Response<bool, std::string>::Make(true, StringExt::ToUTF8(buffer));
    }

    Response<bool, std::string> WinHelper::GetExeName(DWORD pid) {
        auto ret = Response<bool, std::string>::Make(false, "");
        HANDLE handle = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, pid);
        if (handle == NULL) {
            return ret;
        }
        std::shared_ptr<void> dtor(NULL, [=](void *) {
            CloseHandle(handle);
        });
        wchar_t path[4096] = {0};
        DWORD len = sizeof(path) / sizeof(*path);
        std::string upath;
        if (!QueryFullProcessImageNameW(handle, 0, path, &len)) {
            LOGW("QueryFullProcessImageNameW failed.");
            return ret;
        }
        upath = StringExt::ToUTF8(path);
        if (upath.empty()) {
            return ret;
        }

        std::filesystem::path file_path(upath);
        ret.ok_ = true;
        ret.value_ = file_path.filename().string();
        return ret;
    }

    Response<bool, std::string> WinHelper::GetModuleName(HMODULE hModule) {
        const int maxPath = 4096;
        char szFullPath[maxPath] = { 0 };
        ::GetModuleFileNameA(hModule, szFullPath, maxPath);
        std::filesystem::path file_path(szFullPath);
        auto ret = Response<bool, std::string>::Make(false, "");
        if (file_path.filename().string().empty()) {
            return ret;
        }
        ret.ok_ = true;
        ret.value_ = file_path.filename().string();
        return ret;
    }

    Response<bool, std::wstring> WinHelper::GetModulePathW(HMODULE hModule) {
        const int maxPath = 4096;
        wchar_t szFullPath[maxPath] = { 0 };
        ::GetModuleFileNameW(hModule, szFullPath, maxPath);
        ::PathRemoveFileSpecW(szFullPath);
        return Response<bool, std::wstring>::Make(true, szFullPath);
    }

    Response<bool, std::string> WinHelper::GetModulePath(HMODULE hModule) {
        auto file_path = StringExt::ToUTF8(GetModulePathW(hModule).value_);
        return Response<bool, std::string>::Make(true, file_path);
    }

    Response<bool, std::string> WinHelper::Win32GetClassName(HWND hwnd) {
        auto ret = Response<bool, std::string>::Make(false, "");
        wchar_t clazzName[kMaxTexBufSize] = { 0 };
        if (GetClassNameW(hwnd, clazzName, kMaxTexBufSize) == 0) {
            LOGW("GetClassNameW failed with:%d", GetLastError());
            return ret;
        }
        ret.ok_ = true;
        ret.value_ = StringExt::ToUTF8(clazzName);
        return ret;
    }

    Response<bool, std::string> WinHelper::Win32GetWindowTitle(HWND hwnd) {
        auto ret = Response<bool, std::string>::Make(false, "");
        wchar_t text[kMaxTexBufSize] = { 0 };
        if (GetWindowTextW(hwnd, text, kMaxTexBufSize) <= 0) {
            LOGI("GetWindowTextW hwnd {} failed with:{}",(void*)hwnd, GetLastError());
            return ret;
        }
        ret.ok_ = true;
        ret.value_ = StringExt::ToUTF8(text);
        return ret;
    }

    Response<bool, HWND> WinHelper::FindHwndByPid(uint32_t pid) {
        auto ret = Response<bool, HWND>::Make(false, nullptr);
        HWND hWnd;
        DWORD dwProcessNowId;
        hWnd = GetTopWindow(NULL);
        while (hWnd) {
            GetWindowThreadProcessId(hWnd, &dwProcessNowId);
            if (dwProcessNowId == pid) {
                ret.ok_ = true;
                ret.value_ = hWnd;
                return ret;
            } else {
                hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
            }
        }
        return ret;
    }

    bool WinHelper::DontCareDPI() {
        typedef HRESULT* (__stdcall* SetProcessDpiAwarenessFunc)(DPI_AWARENESS_CONTEXT);
        bool res = true;
        HMODULE shCore = LoadLibraryA("User32.dll");
        if (shCore) {
            auto setProcessDpiAwareness = (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwarenessContext");
            if (setProcessDpiAwareness != nullptr)
            {
                setProcessDpiAwareness(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            }
            else {
                res = false;
            }
            FreeLibrary(shCore);
        }
        return res;
    }

}
