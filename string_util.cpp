//
// Created by RGAA on 2023-12-17.
//

#include "string_util.h"
#ifdef WIN32
#include <qstring.h>
#endif

namespace tc {

	std::string StringUtil::Trim(const std::string& str) {
#ifdef WIN32
		auto qstr = QString::fromStdString(str);
		auto trimed_qstr = qstr.trimmed();
		return trimed_qstr.toStdString();
#else
        return str;
#endif
	}

    std::string ToHexString(const std::vector<uint8_t>& data) {
        std::ostringstream oss;
        for (const auto& byte : data) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }
}