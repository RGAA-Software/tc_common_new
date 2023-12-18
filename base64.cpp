#include "base64.h"

#include <boost/beast/core/detail/base64.hpp>

namespace tc
{

    std::string Base64::Base64Encode(const uint8_t* data, std::size_t len) {
        std::string dest;
        dest.resize(boost::beast::detail::base64::encoded_size(len));
        dest.resize(boost::beast::detail::base64::encode(&dest[0], data, len));
        return dest;
    }

    std::string Base64::Base64Encode(const std::string& s) {
        return Base64Encode(reinterpret_cast <uint8_t const *> (s.data()), s.size());
    }

    std::string Base64::Base64Decode(const std::string& data) {
        std::string dest;
        dest.resize(boost::beast::detail::base64::decoded_size(data.size()));
        auto const result = boost::beast::detail::base64::decode(
                &dest[0], data.data(), data.size());
        dest.resize(result.first);
        return dest;
    }

}
