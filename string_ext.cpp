//
// Created by RGAA on 2023-12-17.
//

#include "string_ext.h"
#ifdef WIN32
#include <qstring.h>
#endif

namespace tc {

	std::string StringExt::Trim(const std::string& str) {
#ifdef WIN32
		auto qstr = QString::fromStdString(str);
		auto trimed_qstr = qstr.trimmed();
		return trimed_qstr.toStdString();
#else
        return str;
#endif
	}
}