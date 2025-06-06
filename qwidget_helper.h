#pragma once
#ifdef WIN32
#include <windows.h>

class QWidget;

namespace tc  { 
    class QWidgetHelper {
    public:
        static void SetBorderInFullScreen(QWidget* window, bool has_border);
    private:
        static HWND GetHWND(QWidget* window);
    };
}

#endif //