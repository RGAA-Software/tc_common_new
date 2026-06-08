//
// Created by RGAA  on 2024/2/13.
//

#include "process_util.h"
#ifdef WIN32
#include "tc_common_new/log.h"
#include "tc_common_new/string_util.h"
#include <UserEnv.h>
#include <TlHelp32.h>
#include <wtsapi32.h>
#include <ShlObj_core.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <sstream>
#include <filesystem>

namespace tc
{

    namespace {
        std::wstring EscapeArg(const std::wstring& arg) {
            if (arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
                return arg;
            }
            std::wstring escaped = L"\"";
            for (size_t i = 0; i < arg.size(); ++i) {
                size_t backslashCount = 0;
                while (i < arg.size() && arg[i] == L'\\') {
                    backslashCount++;
                    i++;
                }
                if (i == arg.size()) {
                    escaped.append(backslashCount * 2, L'\\');
                    break;
                } else if (arg[i] == L'"') {
                    escaped.append(backslashCount * 2 + 1, L'\\');
                    escaped += L'"';
                } else {
                    escaped.append(backslashCount, L'\\');
                    escaped += arg[i];
                }
            }
            escaped += L'"';
            return escaped;
        }

        std::wstring BuildCommandLine(const std::string& exe_path, const std::vector<std::string>& args) {
            std::wstring cmdline;
            auto wexe = StringUtil::ToWString(exe_path);
            cmdline += EscapeArg(wexe);
            for (const auto& arg : args) {
                cmdline += L' ';
                cmdline += EscapeArg(StringUtil::ToWString(arg));
            }
            return cmdline;
        }
    }

    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context) {
        typedef BOOL(__stdcall* SetProcessDpiAwarenessFunc)(DPI_AWARENESS_CONTEXT);
        bool res = false;
        HMODULE shCore = LoadLibraryA("User32.dll");
        if (shCore) {
            auto setProcessDpiAwareness = (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwarenessContext");
            if (setProcessDpiAwareness != nullptr) {
                if (setProcessDpiAwareness(context)) {
                    res = true;
                }
            }
            FreeLibrary(shCore);
        }
        return res;
    }

    bool ProcessUtil::StartProcessAndWait(const std::string& exe_path, const std::vector<std::string>& args) {
        auto cmdline = BuildCommandLine(exe_path, args);
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        if (!CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            LOGE("StartProcessAndWait failed: {}, err: {}", exe_path, GetLastError());
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode != 0) {
            LOGE("Start process: {}, exit code: {}", exe_path, exitCode);
            return false;
        }
        return true;
    }

    uint32_t ProcessUtil::StartProcess(const std::string& exe_path, const std::vector<std::string>& args, bool detach, bool wait) {
        auto cmdline = BuildCommandLine(exe_path, args);
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        DWORD creationFlags = 0;
        if (detach) {
            creationFlags |= CREATE_NEW_CONSOLE;
        }
        if (!CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, creationFlags, nullptr, nullptr, &si, &pi)) {
            LOGE("StartProcess failed: {}, err: {}", exe_path, GetLastError());
            return 0;
        }
        if (wait) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
        DWORD pid = pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return pid;
    }

    std::vector<std::string> ProcessUtil::StartProcessAndOutput(const std::string& exe_path, const std::vector<std::string>& args) {
        std::vector<std::string> output;

        SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
        HANDLE hStdOutRead = nullptr, hStdOutWrite = nullptr;
        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
            LOGE("CreatePipe failed: {}", GetLastError());
            return output;
        }
        if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
            LOGE("SetHandleInformation failed: {}", GetLastError());
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            return output;
        }

        auto cmdline = BuildCommandLine(exe_path, args);
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite;
        si.hStdError = hStdOutWrite;
        PROCESS_INFORMATION pi = {};

        if (!CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
            LOGE("StartProcessAndOutput failed: {}, err: {}", exe_path, GetLastError());
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            return output;
        }

        CloseHandle(hStdOutWrite);

        char buffer[4096];
        DWORD bytesRead;
        std::string output_str;
        while (true) {
            BOOL success = ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!success || bytesRead == 0) {
                break;
            }
            output_str.append(buffer, bytesRead);
        }

        std::istringstream iss(output_str);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (!line.empty()) {
                output.push_back(line);
            }
        }

        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(hStdOutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return output;
    }

    bool ProcessUtil::KillProcess(unsigned long pid) {
        HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (processHandle == NULL) {
            std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
            return false;
        }
        BOOL result = TerminateProcess(processHandle, 1);
        CloseHandle(processHandle);
        return result != 0;
    }

    int ProcessUtil::GetPidByExeName(const std::string& exe_name) {
        int find_pid = 0;
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(pe32);
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return 0;
        }
        int cur_ses_id = GetCurrentSessionId();
        if(cur_ses_id == -1) {
            CloseHandle(hProcessSnap);
            return 0;
        }
        std::wstring wname = StringUtil::ToWString(exe_name);
        BOOL bResult = Process32FirstW(hProcessSnap, &pe32);
        while (bResult) {
            bResult = Process32NextW(hProcessSnap, &pe32);
            if (_wcsicmp(pe32.szExeFile, wname.c_str()) == 0) {
                int ses_id = GetProcessSessionId(pe32.th32ProcessID);
                if(ses_id == cur_ses_id) {
                    find_pid = pe32.th32ProcessID;
                    break;
                }
            }
        }
        CloseHandle(hProcessSnap);
        return find_pid;
    }


    HANDLE ProcessUtil::DupAdminToken() {
        int find_pid = GetPidByExeName(kExploreName);
        if(find_pid == 0)
            return nullptr;
        HANDLE hProcess = INVALID_HANDLE_VALUE;
        HANDLE hToken = INVALID_HANDLE_VALUE;
        HANDLE hDuplicatedToken = INVALID_HANDLE_VALUE;
        do
        {
            hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, find_pid);
            if (!hProcess) {
                break;
            }
            bool ret = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken);
            if (!ret) {
                break;
            }
            ret = DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDuplicatedToken);
            if (!ret) {
                break;
            }
        } while (0);
        if(hProcess)
            CloseHandle(hProcess);
        if(hToken)
            CloseHandle(hToken);
        return hDuplicatedToken;
    }

    bool ProcessUtil::StartProcessInWorkDir(const std::string &work_dir, const std::string &cmdline,
                                            const std::vector<std::string> &args) {
        STARTUPINFOW si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        auto w_cmdline = StringUtil::ToWString(cmdline);
        auto w_work_dir = StringUtil::ToWString(work_dir);

        if (IsUserAnAdmin()) {
            HANDLE user_token = DupAdminToken();
            if (user_token == INVALID_HANDLE_VALUE) {
                LOGE("DuplicateLimitPrivilegeToken failed.");
                return false;
            }
            LOGI("IsUserAnAdmin，create process with token.");

            bool suspend = false;
            DWORD create_flag = suspend ? CREATE_SUSPENDED : 0;
            create_flag |= ABOVE_NORMAL_PRIORITY_CLASS;

            void *lpEnvironment = NULL;
            CreateEnvironmentBlock(&lpEnvironment, user_token, TRUE);
            auto ret = CreateProcessWithTokenW(user_token, 0, NULL, (wchar_t*)w_cmdline.c_str(),
                                          create_flag | CREATE_UNICODE_ENVIRONMENT, lpEnvironment, w_work_dir.c_str(),
                                          &si, &pi);
            LOGI("CreateProcessWithTokenW: {}", ret);
            CloseHandle(user_token);
            DestroyEnvironmentBlock(lpEnvironment);
        }
        else {
            if (!CreateProcessW(
                    NULL,
                    (wchar_t *) w_cmdline.c_str(),
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    (wchar_t *) w_work_dir.c_str(),
                    &si,
                    &pi
            )) {
                LOGE("CreateProcessW failed: {}", GetLastError());
                return false;
            }
        }

        LOGI("==> CreateProcessSuccess...{} {}", pi.dwProcessId, pi.dwThreadId);

        std::cout << "pid:" << pi.dwProcessId << " tid:" << pi.dwThreadId << std::endl;

        WaitForSingleObject(pi.hProcess, INFINITE);
        LOGI("==> process exit....");
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    bool ProcessUtil::StartProcessInSameUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait) {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
            LOGE("StartProcessInSameUser OpenProcessToken failed.");
            return false;
        }

        HANDLE hTokenDup = NULL;
        bool bRet = DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &hTokenDup);
        if (!bRet || hTokenDup == NULL) {
            LOGE("DuplicateTokenEx failed.");
            CloseHandle(hToken);
            return false;
        }

        DWORD dwSessionId = WTSGetActiveConsoleSessionId();

        if (0xFFFFFFFF == dwSessionId) {
            return false;
        }

        if (!SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD))) {
            DWORD nErr = GetLastError();
            CloseHandle(hTokenDup);
            CloseHandle(hToken);
            LOGE("SetTokenInformation failed.");
            return false;
        }

        STARTUPINFOW si;
        ZeroMemory(&si, sizeof(STARTUPINFOW));

        si.cb = sizeof(STARTUPINFOW);
        si.lpDesktop = (wchar_t*)L"WinSta0\\Default";
        si.wShowWindow = SW_SHOW;
        si.dwFlags = STARTF_USESHOWWINDOW;

        LPVOID pEnv = NULL;
        bRet = CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE);
        if (!bRet) {
            CloseHandle(hTokenDup);
            CloseHandle(hToken);
            LOGE("GetEnvironmentBlock failed.");
            return false;
        }

        if (pEnv == NULL) {
            CloseHandle(hTokenDup);
            CloseHandle(hToken);
            LOGE("Not have env .");
            return false;
        }

        PROCESS_INFORMATION processInfo;
        ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
        DWORD dwCreationFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

        LOGI("workdir: {}", StringUtil::ToUTF8(work_dir));
        LOGI("cmdline: {}", StringUtil::ToUTF8(cmdline));

        if (CreateProcessAsUserW(hTokenDup, NULL, (wchar_t*)cmdline.c_str(), NULL, NULL, FALSE, dwCreationFlag, pEnv, (wchar_t*)work_dir.c_str(), &si, &processInfo) == 0) {
            DWORD nRet = GetLastError();
            CloseHandle(hTokenDup);
            CloseHandle(hToken);
            LOGE("CreateProcess Failed failed. error code : {}", nRet);
            return false;
        }
        if (wait) {
            auto wait_result = ::WaitForSingleObject(processInfo.hProcess, INFINITE);
            LOGI("wait result: {:x}, last error: {:x}", wait_result, GetLastError());
        }

        DestroyEnvironmentBlock(pEnv);
        CloseHandle(hTokenDup);
        CloseHandle(hToken);

        return true;
    }

    static BOOL GetTokenByName(HANDLE& hToken, const std::string& lpName)
    {
        HANDLE hProcessSnap = NULL;
        BOOL bRet = FALSE;
        PROCESSENTRY32 pe32 = {0};

        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
            return (FALSE);

        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hProcessSnap, &pe32)) {
            do {
                std::wstring exe_file(pe32.szExeFile);
                std::wstring lpname = StringUtil::ToWString(lpName);
                if(CompareStringOrdinal(exe_file.c_str(), -1, lpname.c_str(), -1, TRUE) == CSTR_EQUAL) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE,pe32.th32ProcessID);
                    bRet = OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);
                    CloseHandle (hProcessSnap);
                    LOGI("Found it : {}", lpName);
                    return (bRet);
                }
            }
            while (Process32Next(hProcessSnap, &pe32));
            bRet = TRUE;
        }
        else {
            bRet = FALSE;
        }

        CloseHandle (hProcessSnap);
        return (bRet);
    }

    bool ProcessUtil::StartProcessInCurrentUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait) {
        DWORD dwSessionId = WTSGetActiveConsoleSessionId();
        if (dwSessionId == 0xFFFFFFFF) {
            LOGE("StartProcessInCurrentUser, WTSGetActiveConsoleSessionId failed");
            return FALSE;
        }

        HANDLE hUserToken = NULL;
        if (!WTSQueryUserToken(dwSessionId, &hUserToken)) {
            LOGE("StartProcessInCurrentUser, WTSQueryUserToken failed");
            return FALSE;
        }

        HANDLE token;
        auto r = GetTokenByName(token, "EXPLORER.EXE");
        if (!r) {
            LOGE("Can't get token for: explorer.exe");
            return FALSE;
        }

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        BOOL bSuccess = CreateProcessAsUserW(
                token,
                NULL,
                (LPWSTR)cmdline.c_str(),
                NULL,
                NULL,
                FALSE,
                CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS,
                NULL,
                (LPWSTR)work_dir.c_str(),
                &si,
                &pi
        );
        if (!bSuccess) {
            LOGE("**CreateProcessAsUser failed, error: {:x}", GetLastError());
        }

        if (token) {
            CloseHandle(token);
        }
        if (bSuccess) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return bSuccess;
    }

    uint32_t ProcessUtil::GetCurrentSessionId() {
        return GetProcessSessionId(GetCurrentProcessId());
    }

    uint32_t ProcessUtil::GetProcessSessionId(uint32_t pid)
    {
        DWORD sessionId = -1;
        if (ProcessIdToSessionId(pid, &sessionId))
            return sessionId;
        return -1;
    }

    int ProcessUtil::GetThreadCount() {
#ifdef WIN32
        DWORD threadCount = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            LOGE("Failed to create snapshot!");
            return 0;
        }

        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);
        DWORD currentPid = GetCurrentProcessId();

        if (Thread32First(hSnapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == currentPid) {
                    threadCount++;
                }
            } while (Thread32Next(hSnapshot, &te32));
        }

        CloseHandle(hSnapshot);
        return threadCount;
#else
        int count = 0;
        for (const auto& entry : std::filesystem::directory_iterator("/proc/self/task")) {
            if (entry.is_directory()) {
                auto name = entry.path().filename().string();
                if (name != "." && name != "..") {
                    count++;
                }
            }
        }
        return count;
#endif
    }

    void ProcessUtil::SetProcessInHighLevel() {
#ifdef WIN32
        HANDLE hProcess = GetCurrentProcess();
        BOOL result = SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
        if (!result) {
            LOGE("SetPriorityClass failed, err: {}", GetLastError());
        }
#endif
    }

    bool ProcessUtil::RunAsAdminWithShell(const std::wstring& exePath, const std::wstring& parameters)
    {
        SHELLEXECUTEINFO sei = { sizeof(sei) };

        sei.lpVerb = L"runas";
        sei.lpFile = exePath.c_str();
        sei.lpParameters = parameters.empty() ? NULL : parameters.c_str();
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (!ShellExecuteEx(&sei)) {
            DWORD err = GetLastError();
            if (err == ERROR_CANCELLED) {
                LOGE("UAC denied");
            } else {
                LOGE("UAC error: {}", err);
            }
            return false;
        }

        if (sei.hProcess) {
            CloseHandle(sei.hProcess);
        }
        return true;
    }

}
#endif
