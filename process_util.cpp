//
// Created by Administrator on 2024/2/13.
//

#include "process_util.h"

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
}