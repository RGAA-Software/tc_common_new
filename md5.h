//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_MD5_H
#define TC_APPLICATION_MD5_H

#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>

namespace tc
{

    class MD5 {
    public:

        static std::string Hex(const std::string& input) {
            boost::uuids::detail::md5 boost_md5;
            boost_md5.process_bytes(input.c_str(), input.size());
            boost::uuids::detail::md5::digest_type digest;
            boost_md5.get_digest(digest);
            const auto int_digest = reinterpret_cast<const int*>(&digest);
            std::string str_md5;
            boost::algorithm::hex(int_digest,
                                  int_digest + sizeof(boost::uuids::detail::md5::digest_type)/sizeof(int),
                                  std::back_inserter(str_md5));
            return str_md5;
        }

    };

}

#endif //TC_APPLICATION_MD5_H
