//
// Created by Administrator on 2024/2/13.
//

#ifndef TC_APPLICATION_PROCESS_UTIL_H
#define TC_APPLICATION_PROCESS_UTIL_H

#ifdef WIN32
#include <Windows.h>
#endif

namespace tc {
#ifdef WIN32
    bool SetDpiAwarenessContext(DPI_AWARENESS_CONTEXT context);


#endif
}

#endif //TC_APPLICATION_PROCESS_UTIL_H
