#pragma once

#include <string>

namespace tc {
    class DumpHelper
    {
    public:
        static void WatchDump();
        static void Snapshot(const std::string& name);
    };
}