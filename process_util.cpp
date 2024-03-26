//
// Created by Administrator on 2024/2/13.
//

#include "process_util.h"
#ifdef WIN32
#include "tc_common_new/log.h"

namespace tc
{

    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context) {
        typedef BOOL(__stdcall* SetProcessDpiAwarenessFunc)(DPI_AWARENESS_CONTEXT);
        bool res = false;
        HMODULE shCore = LoadLibraryA("User32.dll");
        if (shCore) {
            SetProcessDpiAwarenessFunc setProcessDpiAwareness =
                    (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwarenessContext");

            if (setProcessDpiAwareness != nullptr)
            {
                if (setProcessDpiAwareness(context)) {
                    res = true;
                }
            }
            FreeLibrary(shCore);
        }
        return res;
    }

    bool ProcessUtil::StartProcessAndWait(const std::string& exe_path, const std::vector<std::string>& args) {
        bp::ipstream pipe_stream;
        bp::child c(exe_path, bp::args(args), bp::std_out > pipe_stream);
        c.wait();
        int code = c.exit_code();
        if (code != 0) {
            LOGE("Start process code : {}", code);
        }
        return code == 0;
    }

    uint32_t ProcessUtil::StartProcess(const std::string& exe_path, const std::vector<std::string>& args) {
        bp::ipstream pipe_stream;
        bp::child c(exe_path, bp::args(args), bp::std_out > pipe_stream);
        return c.id();
    }

    std::vector<std::string> ProcessUtil::StartProcessAndOutput(const std::string& exe_path, const std::vector<std::string>& args) {
//        std::string err;

        if(!boost::filesystem::exists(exe_path)) {
            LOGE("StartProcessAndOutput exe_path is {}, but not exists.", exe_path);
            return {};
        }

        bp::ipstream out_stream, err_stream; // 输出和错误的流
        bp::child c(exe_path, bp::args(args), bp::std_out > out_stream, bp::std_err > err_stream);

        std::vector<std::string> output;
        std::string line;
        while (out_stream && std::getline(out_stream, line) && !line.empty()) {
            output.push_back(line);
        }
//
//        // 读取错误输出
//        while (err_stream && std::getline(err_stream, line) && !line.empty()) {
//            err += line + "\n";
//        }
//        LOGE("error: {}", err);
        c.wait();
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
        //LPSTR programPath = "E:\\source\\streamer\\out\\install\\x64-RelWithDebInfo\\bin\\DolitCloudApp.exe -o 3101 --run_in_system 1";
        //LPSTR work_dir = "E:\\source\\streamer\\out\\install\\x64-RelWithDebInfo\\bin";
        //LPSTR work_dir = "E:\\software\\bin";
        //LPSTR programPath = "E:\\software\\bin\\DolitCloudApp.exe -o 3101";
        // dolit_sys_test.exe
        //BOOL ok = SetCurrentDirectory("E:\\software\\bin");

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

}
#endif