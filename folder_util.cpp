//
// Created by RGAA on 2024/3/29.
//

#include "folder_util.h"
#include <filesystem>
#include <iostream>
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

    void FolderUtil::VisitRecursiveFiles(const std::filesystem::path &path, int depth, int max_depth, const std::function<void(VisitResult&&)>& cbk, const std::string& filter_suffix) {
        // 如果达到最大深度，停止递归
        if (depth > max_depth) return;

        // 检查路径是否存在以及是否是目录
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            // 使用迭代器遍历当前目录
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                // 获取目录项的路径
                const auto& p = entry.path();
                // 输出目录项路径
                //std::cout << std::string(depth * 2, ' ') << "|-- " << p.filename() << '\n';
                if (fs::is_regular_file(p)) {
                    if (!filter_suffix.empty()) {
                        auto u8path = StringExt::ToUTF8(p.wstring());
                        auto suffix = StringExt::ToLowerCpy(FileExt::GetFileSuffix(u8path));
                        if (suffix == filter_suffix) {
                            cbk(VisitResult {
                                .name_ = p.filename().wstring(),
                                .path_ = p.wstring(),
                            });
                        }
                    } else {
                        cbk(VisitResult {
                            .name_ = p.filename().wstring(),
                            .path_ = p.wstring(),
                        });
                    }
                }
                // 如果是目录，递归遍历
                if (fs::is_directory(p)) {
                    VisitRecursiveFiles(p, depth + 1, max_depth, cbk, filter_suffix);
                }
            }
        }
    }

#ifdef WIN32
    void FolderUtil::VisitAllByQt(const std::string& path, std::function<void(VisitResult&&)>&& cbk, const std::string& filter_suffix) {
        QDirIterator it(QString::fromStdString(path), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot ,QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            auto file_info = it.fileInfo();
            if (file_info.isFile()) {
                cbk(VisitResult{
                    .name_ = file_info.fileName().toStdWString(),
                    .path_ = file_info.absoluteFilePath().toStdWString(),
                });
            }
        }
    }

    std::wstring FolderUtil::GetCurrentFilePath() {
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::wstring(path);
    }

    std::wstring FolderUtil::GetCurrentFolderPath() {
        const int maxPath = 4096;
        wchar_t szFullPath[maxPath] = { 0 };
        ::GetModuleFileNameW(NULL, szFullPath, maxPath);
        ::PathRemoveFileSpecW(szFullPath);
        return {szFullPath};
    }

    void FolderUtil::CreateDir(const std::string& path) {
        QDir dir(path.c_str());
        if (!dir.exists()) {
            dir.mkpath(path.c_str());
        }
    }

    void FolderUtil::OpenDir(const std::string& path) {
        auto target_path = std::format("file:///{}", path);
        QDesktopServices::openUrl(QUrl(target_path.c_str()));
    }

#endif
}