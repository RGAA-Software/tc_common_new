//
// Created by RGAA on 2024-02-06.
//

#ifndef TC_APPLICATION_FILE_EXT_H
#define TC_APPLICATION_FILE_EXT_H

#include <string>

namespace tc
{

    class FileExt {
    public:

        static std::string GetFileNameFromPath(const std::string& path);
        static std::string GetFileNameFromPathNoSuffix(const std::string& path);
        static std::string GetFileSuffix(const std::string& path);
        static std::string GetFileFolder(const std::string& path);
        static std::string GetFolderNameFormAbsFolderPath(const std::string& path);

    };

}

#endif //TC_APPLICATION_FILE_EXT_H
