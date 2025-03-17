//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_MD5_H
#define TC_APPLICATION_MD5_H

#include "tc_3rdparty/asio2/include/asio2/util/md5.hpp"
#include "string_ext.h"

namespace tc
{

    class MD5 {
    public:

        static std::string Hex(const std::string& input) {
            return StringExt::ToLowerCpy(asio2::md5(input).str());
        }

    };

}

#endif //TC_APPLICATION_MD5_H
