//
// Created by RGAA on 2024/3/29.
//

#include "folder_util.h"
#include <filesystem>
#include <iostream>
#include "file_util.h"
#include "string_util.h"

#ifdef Q_OS_WIN
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <windows.h>
#include <shlobj.h>
#endif

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
                        auto u8path = StringUtil::ToUTF8(path.wstring());
                        auto suffix = FileUtil::GetFileSuffix(u8path);
                        StringUtil::ToLower(suffix);
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
                    auto u8path = StringUtil::ToUTF8(path.wstring());
                    auto suffix = FileUtil::GetFileSuffix(u8path);
                    StringUtil::ToLower(suffix);
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
                        auto u8path = StringUtil::ToUTF8(p.wstring());
                        auto suffix = StringUtil::ToLowerCpy(FileUtil::GetFileSuffix(u8path));
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

    std::wstring FolderUtil::GetProgramDataPath() {
#ifdef WIN32
        QString sharedPath;
        // Windows: 使用ProgramData
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) {
            sharedPath = QDir::fromNativeSeparators(QString::fromWCharArray(path));
        } else {
            sharedPath = "C:/ProgramData";
        }
        // 创建应用子目录
        QString appPath = sharedPath + "/GammaRay";// + QCoreApplication::applicationName();
        QDir dir;
        if (!dir.exists(appPath)) {
            dir.mkpath(appPath);
        }
        return appPath.toStdWString();
#endif

#ifdef UNIX
        sharedPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
#endif

#ifdef ANDROID
        return L"";
#endif

        return L"";
    }

    bool FolderUtil::CopyDirectory(const fs::path& source, const fs::path& destination, bool overwrite) {
        try {
            if (!fs::exists(source) || !fs::is_directory(source)) {
                std::cerr << "Source directory does not exist or is not a directory: " << source << std::endl;
                return false;
            }

            // 创建目标目录（如果不存在）
            if (!fs::exists(destination)) {
                fs::create_directories(destination);
                std::cout << "Created destination directory: " << destination << std::endl;
            }

            // 递归遍历源目录
            for (const auto& entry : fs::recursive_directory_iterator(
                    source,
                    fs::directory_options::skip_permission_denied)) {

                try {
                    // 计算目标路径
                    auto relative_path = fs::relative(entry.path(), source);
                    auto target_path = destination / relative_path;

                    if (fs::is_directory(entry.status())) {
                        // 创建目录
                        fs::create_directories(target_path);
                    } else if (fs::is_regular_file(entry.status())) {
                        // 拷贝文件
                        if (fs::exists(target_path)) {
                            if (overwrite) {
                                fs::remove(target_path); // 删除已存在的文件
                            } else {
                                std::cout << "Skipped (already exists): " << relative_path << std::endl;
                                continue;
                            }
                        }

                        // 拷贝文件内容
                        fs::copy_file(entry.path(), target_path, fs::copy_options::overwrite_existing);
                        std::cout << "Copied: " << relative_path << std::endl;
                    }
                } catch (const fs::filesystem_error& ex) {
                    std::cerr << "Error processing " << entry.path() << ": " << ex.what() << std::endl;
                    // 继续处理其他文件
                    continue;
                }
            }

            std::cout << "Directory copy completed: " << source << " -> " << destination << std::endl;
            return true;

        } catch (const fs::filesystem_error& ex) {
            std::cerr << "Filesystem error: " << ex.what() << std::endl;
            return false;
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
            return false;
        }
    }

}