#include "qwidget_helper.h"
#include <QWidget>
#ifdef WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif // WIN32

namespace tc {
   void QWidgetHelper::SetBorderInFullScreen(QWidget* window, bool hasBorder) {
#ifdef WIN32
        HWND hwnd = GetHWND(window);
        if (!hwnd) return;
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (hasBorder) {
            style |= WS_BORDER;
            MARGINS margins = { 0, 0, 0, 1 };
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
        else {
            style &= ~(WS_BORDER);
        }
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif
    }


    HWND QWidgetHelper::GetHWND(QWidget* window) {
#ifdef WIN32
        if (!window) return nullptr;
        return (HWND)window->winId();
#endif
        return nullptr;
    }
}