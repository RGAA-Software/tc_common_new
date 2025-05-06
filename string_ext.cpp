//
// Created by RGAA on 2023-12-17.
//

#include "string_ext.h"
#include <qstring.h>

namespace tc {

	std::string StringExt::Trim(const std::string& str) {
		auto qstr = QString::fromStdString(str);
		auto trimed_qstr = qstr.trimmed();
		return trimed_qstr.toStdString();
	}
}