//
// Created by RGAA on 2023-12-17.
//

#ifndef TC_APPLICATION_STRINGEXT_H
#define TC_APPLICATION_STRINGEXT_H

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <locale>
#include <string>
#include <codecvt>
#include <cctype>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace tc
{

    class StringExt {
    public:

        // 
        static void Split(const std::string &s,
                          std::vector<std::string> &sv,
                          const char delim = ' ') {
            sv.clear();
            std::istringstream iss(s);
            std::string temp;

            while (std::getline(iss, temp, delim)) {
                sv.emplace_back(std::move(temp));
            }
        }


        static void Split(const std::string& s,
                          std::vector<std::string>& res,
                          const std::string& delimiter) {
            size_t pos_start = 0, pos_end, delim_len = delimiter.length();
            std::string token;

            while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
                token = s.substr (pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                res.push_back (token);
            }

            res.push_back (s.substr (pos_start));
        }

        static void ToLower(std::string& data) {
            std::transform(data.begin(), data.end(), data.begin(),
                           [](unsigned char c){ return std::tolower(c); });
        }

        static std::string ToLowerCpy(const std::string& data) {
            return boost::algorithm::to_lower_copy(data);
        }

        static bool StartWith(const std::string& input, const std::string& find) {
            if (input.rfind(find, 0) == 0) {
                return true;
            }
            return false;
        }

        static std::string CopyStr(const std::string& origin) {
            std::string copy;
            copy.resize(origin.size());
            memcpy(copy.data(), origin.data(), origin.size());
            return copy;
        }

        static void Replace(std::string& origin, const std::string& from, const std::string& to) {
            boost::replace_all(origin, from, to);
        }

    };

}

#endif //TC_APPLICATION_STRINGEXT_H
