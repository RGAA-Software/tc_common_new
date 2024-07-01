#include "base64.h"

#include "tc_3rdparty/asio2/3rd/cereal/external/base64.hpp"

namespace tc
{

    std::string Base64::Base64Encode(const uint8_t* data, std::size_t len) {
        return cereal::base64::encode(data, len);
    }

    std::string Base64::Base64Encode(const std::string& s) {
        return Base64Encode(reinterpret_cast <uint8_t const *> (s.data()), s.size());
    }

    std::string Base64::Base64Decode(const std::string& data) {
        return cereal::base64::decode(data);
    }

}
