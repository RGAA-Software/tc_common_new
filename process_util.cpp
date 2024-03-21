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

}
#endif