//
// Created by hy on 2024/3/29.
//

#include "folder_util.h"
#include <filesystem>
#include "file_ext.h"
#include "string_ext.h"

namespace fs = std::filesystem;

namespace tc
{

    void FolderUtil::VisitFiles(const std::string& path, std::function<void(VisitResult&&)>&& cbk, const std::string& filter_suffix) {
        auto current_path = std::filesystem::u8path(path);
        if (fs::is_directory(current_path)) {
            for (const auto& entry : fs::directory_iterator(current_path)) {
                auto path = entry.path();
                if (entry.is_regular_file()) {
                    if (!filter_suffix.empty()) {
                        auto u8path = StringExt::ToUTF8(path.wstring());
                        auto suffix = FileExt::GetFileSuffix(u8path);
                        StringExt::ToLower(suffix);
                        if (filter_suffix != suffix) {
                            continue;
                        }
                    }
                    VisitResult result{
                        .name_ = path.filename().wstring(),
                        .path_ = path.wstring(),
                    };
                    cbk(std::move(result));
                }
            }
        }
    }

    void FolderUtil::VisitFolders(const std::string& path, std::function<void(VisitResult&&)>&& cbk) {
        auto current_path = std::filesystem::u8path(path);
        if (fs::is_directory(current_path)) {
            for (const auto& entry : fs::directory_iterator(current_path)) {
                auto path = entry.path();
                if (entry.is_directory()) {
                    VisitResult result{
                            .name_ = path.filename().wstring(),
                            .path_ = path.wstring(),
                    };
                    cbk(std::move(result));
                }
            }
        }
    }

    void FolderUtil::VisitAll(const std::string& path, std::function<void(VisitResult&&)>&& cbk, const std::string& filter_suffix) {
        auto current_path = std::filesystem::u8path(path);
        if (fs::is_directory(current_path)) {
            for (const auto& entry : fs::directory_iterator(current_path)) {
                auto path = entry.path();
                if (!filter_suffix.empty()) {
                    auto u8path = StringExt::ToUTF8(path.wstring());
                    auto suffix = FileExt::GetFileSuffix(u8path);
                    StringExt::ToLower(suffix);
                    if (filter_suffix != suffix) {
                        continue;
                    }
                }
                VisitResult result{
                        .name_ = path.filename().wstring(),
                        .path_ = path.wstring(),
                };
                cbk(std::move(result));
            }
        }
    }
}