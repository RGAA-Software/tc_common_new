//
// Created by RGAA on 2024-02-06.
//

#include "file_ext.h"
#include "string_ext.h"

namespace tc
{

    std::string FileExt::GetFileNameFromPath(const std::string& path) {
        std::string target_path = path;
        StringExt::Replace(target_path, R"(\\)", "/");
        StringExt::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        if (idx == std::string::npos) {
            return path;
        }
        return target_path.substr(idx + 1, target_path.size());
    }

    std::string FileExt::GetFileNameFromPathNoSuffix(const std::string& path) {
        auto filename = GetFileNameFromPath(path);
        auto idx = filename.rfind('.');
        return filename.substr(0, idx);
    }

    std::string FileExt::GetFileSuffix(const std::string& path) {
        auto idx = path.rfind('.');
        return path.substr(idx+1, path.size());
    }

    std::string FileExt::GetFileFolder(const std::string& path) {
        std::string target_path = path;
        StringExt::Replace(target_path, R"(\\)", "/");
        StringExt::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        return target_path.substr(0, idx);
    }

    std::string FileExt::GetFolderNameFormAbsFolderPath(const std::string& path) {
        std::string target_path = path;
        StringExt::Replace(target_path, R"(\\)", "/");
        StringExt::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        if (idx == path.size() - 1) {
            target_path = target_path.substr(0, idx);
        }
        return target_path.substr(idx + 1, target_path.size());
    }
}