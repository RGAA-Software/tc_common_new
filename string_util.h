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
#include <codecvt>
#include <cctype>
#include <iomanip>
#include <cstring>

#ifdef WIN32
#include <Windows.h>
#endif

namespace tc
{

    class StringUtil {
    public:

        // 
        static void Split(const std::string &s, std::vector<std::string> &sv, const char delim = ' ') {
            sv.clear();
            std::istringstream iss(s);
            std::string temp;
            while (std::getline(iss, temp, delim)) {
                sv.emplace_back(std::move(temp));
            }
        }

        static void Split(const std::string& s, std::vector<std::string>& res, const std::string& delimiter) {
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
            std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c){ return std::tolower(c); });
        }

        static std::string ToLowerCpy(const std::string& data) {
            std::string target = data;
            std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c) -> char { return std::tolower(c); });
            return target;
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
            size_t start_pos = 0;
            while ((start_pos = origin.find(from, start_pos)) != std::string::npos) {
                origin.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        }

        static inline std::wstring ToWString(const std::string& src) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(src);
        }

        static inline std::string ToUTF8(const std::wstring& src) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.to_bytes(src);
        }

#ifdef WIN32
        static std::string GetErrorStr(HRESULT hr) {
            wchar_t buffer[4096] = { 0 };
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL, hr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           buffer, sizeof(buffer) / sizeof(*buffer), NULL);
            std::wstring res = buffer;
            return ToUTF8(res);
        }

        static std::string StandardizeWinPath(const std::string& path) {
            std::string normalized = path;
            StringUtil::Replace(normalized, "\\", "/");
            // D: => D:/
            if (normalized.size() == 2 && normalized[1] == ':') {
                normalized += "/";
            }
            // "D:/video/" => "D:/video"
            if (normalized.size() >= 4 && normalized.back() == '/') {
                normalized.pop_back();
            }
            return normalized;
        }
#endif
        static std::string FormatSize(uint64_t byte_size) {
            static const char* suffixes[] = { "B", "KB", "MB", "GB" };
            const int numSuffixes = sizeof(suffixes) / sizeof(suffixes[0]);
            int suffixIndex = 0;
            while (byte_size >= 1024 && suffixIndex < numSuffixes - 1) {
                byte_size /= 1024;
                suffixIndex++;
            }
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(2) << byte_size << " " << suffixes[suffixIndex];
            return stream.str();
        }

        static std::string Trim(const std::string& str);

        static bool IsValidInteger(const std::string& str) {
            if (str.empty()) return false;
            for (char c : str) {
                if (!std::isdigit(c)) {
                    return false;
                }
            }
            return true;
        }

        static std::string ToHexString(const std::vector<uint8_t>& data);
    };

}

#endif //TC_APPLICATION_STRINGEXT_H
