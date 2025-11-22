#pragma once

#include <string>

namespace tc
{
    void CaptureDump();

    class BreakpadContext {
    public:
        std::string version_;
        std::string app_name_;
    };

    void CaptureDumpByBreakpad(BreakpadContext* bc);
}