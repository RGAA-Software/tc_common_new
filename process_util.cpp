//
// Created by Administrator on 2024/2/13.
//

#include "process_util.h"

#include "tc_common/log.h"

namespace tc {
#ifdef WIN32
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
#endif


    bool ProcessUtil::StartProcess(const std::string& exe_path, const std::vector<std::string>& args) {
        bp::ipstream pipe_stream;
        bp::child c(exe_path, bp::args(args), bp::std_out > pipe_stream);
        c.wait();
        int code = c.exit_code();
        if (code != 0) {
            LOGE("Start process code : {}", code);
        }
        return code == 0;
    }

}