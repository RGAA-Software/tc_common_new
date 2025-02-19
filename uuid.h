//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_UUID_H
#define TC_APPLICATION_UUID_H

#include <string>
#include "tc_3rdparty/asio2/include/asio2/util/uuid.hpp"

#ifdef WIN32
#pragma comment(lib, "Bcrypt.lib")
#endif

namespace tc
{
    static std::string GetUUID() {
        asio2::uuid uuid;
        return uuid.generate().short_uuid(32);
    }
}

#endif //TC_APPLICATION_UUID_H
