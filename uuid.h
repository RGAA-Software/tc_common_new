//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_UUID_H
#define TC_APPLICATION_UUID_H

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#ifdef WIN32
#pragma comment(lib, "Bcrypt.lib")
#endif

namespace tc
{
    static std::string GetUUID() {
        boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
        std::string tmp_uuid = boost::uuids::to_string(a_uuid);
        return tmp_uuid;
    }
}

#endif //TC_APPLICATION_UUID_H
