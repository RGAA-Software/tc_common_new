//
// Created by RGAA  on 2024/2/13.
//

#include "process_util.h"
#ifdef WIN32
#include "tc_common_new/log.h"
#include "tc_common_new/string_util.h"
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QFile>
#include <QApplication>
#include <UserEnv.h>
#include <TlHelp32.h>
#include <wtsapi32.h>

namespace tc
{

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
        QProcess process;
        QStringList exe_args;
        for (const auto& arg : args) {
            exe_args << QString::fromStdString(arg);
        }
        process.start(QString::fromStdString(exe_path), exe_args);
        process.waitForFinished();
        if (process.exitCode() != 0) {
            LOGE("Start process: {}, exit code: {}", exe_path, process.exitCode());
            return false;
        }
        return true;
    }

    uint32_t ProcessUtil::StartProcess(const std::string& exe_path, const std::vector<std::string>& args, bool detach, bool wait) {
        QStringList exe_args;
        for (const auto& arg : args) {
            exe_args << QString::fromStdString(arg);
        }
        qint64 pid;
        if (detach) {
            QProcess::startDetached(QString::fromStdString(exe_path), exe_args, "", &pid);
        }
        if (wait) {
            QProcess process;
            process.start(QString::fromStdString(exe_path), exe_args);
            process.waitForFinished();
            return 0;
        }
        return pid;
    }

    std::vector<std::string> ProcessUtil::StartProcessAndOutput(const std::string& exe_path, const std::vector<std::string>& args) {
//        if(!QFile::exists(exe_path.c_str())) {
//            LOGE("StartProcessAndOutput exe_path is {}, but not exists.", exe_path);
//            return {};
//        }
        std::vector<std::string> output;

//        bp::ipstream out_stream, err_stream;
//        bp::child c(exe_path, bp::args(args), bp::std_out > out_stream, bp::std_err > err_stream);
//
//        std::string line;
//        while (out_stream && std::getline(out_stream, line) && !line.empty()) {
//            output.push_back(line);
//            LOGE("StartProcess info: {}", line);
//        }
//        while (err_stream && std::getline(err_stream, line) && !line.empty()) {
//            LOGE("StartProcess error: {}", line);
//        }
//        c.wait();

        QProcess process;
        QObject::connect(&process, &QProcess::readyReadStandardOutput, [&]() {
            QByteArray info = process.readAllStandardOutput();
            output.push_back(info.toStdString());
        });

        QStringList exe_args;
        for (const auto& arg : args) {
            exe_args << QString::fromStdString(arg);
        }
        process.start(QString::fromStdString(exe_path), exe_args);
        process.waitForFinished();
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

    bool ProcessUtil::StartProcessInWorkDir(const std::string &work_dir, const std::string &cmdline,
                                            const std::vector<std::string> &args) {
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(
                NULL,           // 模块名，NULL意味着使用命令行
                (char*)cmdline.c_str(),    // 命令行
                NULL,           // 进程安全属性
                NULL,           // 线程安全属性
                FALSE,          // 句柄继承选项
                0,              // 创建标志
                NULL,           // 使用父进程的环境块
                (char*)work_dir.c_str(), // 设置子进程的工作目录
                &si,            // 指向 STARTUPINFO 结构体
                &pi             // 指向 PROCESS_INFORMATION 结构体
        )) {
            LOGE("CreateProcess failed: {}", GetLastError());
            return false;
        }

        LOGI("==> CreateProcessSuccess...{} {}", pi.dwProcessId, pi.dwThreadId);

        std::cout << "pid:" << pi.dwProcessId << " tid:" << pi.dwThreadId << std::endl;

        // 等待子进程结束
        WaitForSingleObject(pi.hProcess, INFINITE);
        LOGI("==> process exit....");
        // 关闭进程和线程句柄
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

        if (0xFFFFFFFF == dwSessionId) { //如果物理控制台会话正在附加或分离
            return false;
        }

        //把服务hToken的SessionId替换成当前活动的Session(即替换到可与用户交互的winsta0下)
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
        si.dwFlags = STARTF_USESHOWWINDOW /*|STARTF_USESTDHANDLES*/;

        //创建进程环境块
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

        //在活动的Session下创建进程
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
            // Wait until child process exits.
            auto wait_result = ::WaitForSingleObject(processInfo.hProcess, INFINITE);
            LOGI("wait result: {:x}, last error: {:x}", wait_result, GetLastError());
        }

        DestroyEnvironmentBlock(pEnv);
        CloseHandle(hTokenDup);
        CloseHandle(hToken);

        return true;
    }

    bool ProcessUtil::StartProcessInCurrentUser(const std::wstring& cmdline, const std::wstring& work_dir, bool wait) {
        DWORD dwSessionId = WTSGetActiveConsoleSessionId();
        if (dwSessionId == 0xFFFFFFFF) {
            LOGE("StartProcessInCurrentUser, WTSGetActiveConsoleSessionId failed");
            return FALSE; // 没有用户登录
        }

        HANDLE hUserToken = NULL;
        if (!WTSQueryUserToken(dwSessionId, &hUserToken)) {
            LOGE("StartProcessInCurrentUser, WTSQueryUserToken failed");
            return FALSE; // 获取用户 Token 失败
        }

        // 复制 Token（避免权限问题）
        HANDLE hDuplicatedToken = NULL;
        if (!DuplicateTokenEx(
                hUserToken,
                TOKEN_ALL_ACCESS,
                NULL,
                SecurityImpersonation,
                TokenPrimary,
                &hDuplicatedToken
        )) {
            CloseHandle(hUserToken);
            LOGE("StartProcessInCurrentUser, DuplicateTokenEx failed");
            return FALSE;
        }

        // 设置环境变量（可选）
        LPVOID lpEnvironment = NULL;
        if (!CreateEnvironmentBlock(&lpEnvironment, hDuplicatedToken, FALSE)) {
            CloseHandle(hDuplicatedToken);
            CloseHandle(hUserToken);
            LOGE("StartProcessInCurrentUser, CreateEnvironmentBlock failed");
            return FALSE;
        }

        // 启动进程
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        BOOL bSuccess = CreateProcessAsUserW(
                hDuplicatedToken,          // 用户 Token
                NULL,         // 应用程序路径
                (LPWSTR)cmdline.c_str(),     // 命令行参数
                NULL,                      // 进程安全属性
                NULL,                      // 线程安全属性
                FALSE,                     // 不继承句柄
                CREATE_UNICODE_ENVIRONMENT | NORMAL_PRIORITY_CLASS, // 标志
                lpEnvironment,             // 环境变量
                (LPWSTR)work_dir.c_str(),                      // 当前目录（NULL 表示使用父进程目录）
                &si,                       // 启动信息
                &pi                        // 进程信息
        );
        if (!bSuccess) {
            LOGE("CreateProcessAsUser failed, error: {:x}", GetLastError());
        }

        // 清理资源
        if (lpEnvironment) {
            DestroyEnvironmentBlock(lpEnvironment);
        }
        if (hDuplicatedToken) {
            CloseHandle(hDuplicatedToken);
        }
        if (hUserToken) {
            CloseHandle(hUserToken);
        }
        if (bSuccess) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return bSuccess;
    }

    uint32_t ProcessUtil::GetCurrentSessionId()
    {
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
#ifdef Q_OS_WIN
        DWORD threadCount = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            LOGE("Failed to create snapshot!");
            return 0;
        }

        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(hSnapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == QCoreApplication::applicationPid()) {
                    threadCount++;
                }
            } while (Thread32Next(hSnapshot, &te32));
        }

        CloseHandle(hSnapshot);
        return threadCount;
#else
        QDir dir(QString("/proc/%1/task").arg(QCoreApplication::applicationPid()));
        return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
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

}
#endif