//
// Created by RGAA on 23/11/2024.
//

#include "url_helper.h"

namespace tc
{

#ifdef WIN32
    // 解码 URL 编码的特殊字符
    static std::string UrlDecode(const std::string& str) {
        std::string result;
        char ch;
        int i = 0;
        while (i < str.length()) {
            if (str[i] == '%') {
                sscanf(str.substr(i + 1, 2).c_str(), "%x", &ch);
                result += ch;
                i += 3;
            } else if (str[i] == '+') {
                result += ' ';
                i++;
            } else {
                result += str[i];
                i++;
            }
        }
        return result;
    }

    // 解析查询参数字符串并存储在 std::map 中
    std::unordered_map<std::string, std::string> UrlHelper::ParseQueryString(const std::string& queryString) {
        std::unordered_map<std::string, std::string> params;

        // 分割参数对
        std::istringstream iss(queryString);
        std::string param;
        while (std::getline(iss, param, '&')) {
            size_t pos = param.find('=');
            if (pos != std::string::npos) {
                std::string key = param.substr(0, pos);
                std::string value = param.substr(pos + 1);

                // 解码参数值
                key = UrlDecode(key);
                value = UrlDecode(value);

                // 存储参数
                params[key] = value;
            }
        }

        return params;
    }
#endif
}