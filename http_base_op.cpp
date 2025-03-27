//
// Created by RGAA on 27/03/2025.
//

#include "http_base_op.h"
#include "http_client.h"
#include "log.h"
#include "json/json.hpp"
#include <format>

using namespace nlohmann;

namespace tc
{

    Result<std::string, bool> HttpBaseOp::CanPingServer(const std::string& host, const std::string& port) {
        auto client =
                HttpClient::Make(std::format("{}:{}", host, port), "/ping", 2);
        auto resp = client->Request();
        if (resp.status != 200 || resp.body.empty()) {
            LOGE("Request new device failed.");
            return TRError(false);
        }

        try {
            auto obj = json::parse(resp.body);
            if (obj["code"].get<int>() != 200) {
                return TRError(false);
            }
            return resp.body;
        } catch (...) {
            return TRError(false);
        }
    }

}