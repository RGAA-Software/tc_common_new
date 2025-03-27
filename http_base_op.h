//
// Created by RGAA on 27/03/2025.
//

#ifndef GAMMARAY_HTTP_BASE_OP_H
#define GAMMARAY_HTTP_BASE_OP_H

#include <string>
#include "expt/expected.h"

namespace tc
{
    class HttpBaseOp {
    public:
        static Result<std::string, bool> CanPingServer(const std::string& host, const std::string& port);
    };
}

#endif //GAMMARAY_HTTP_BASE_OP_H
