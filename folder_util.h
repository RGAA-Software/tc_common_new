//
// Created by RGAA on 2024/3/29.
//

#ifndef TC_SERVER_STEAM_FOLDER_UTIL_H
#define TC_SERVER_STEAM_FOLDER_UTIL_H

#include <string>
#include <functional>
#include <filesystem>

#ifdef WIN32
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <Shlwapi.h>
#include <QDesktopServices>
#include <QUrl>
#endif

namespace fs = std::filesystem;

namespace tc
{
    class VisitResult {
    public:
        std::wstring name_;
        std::wstring path_;
    };

    class FolderUtil {
    public:

        static void VisitFiles(const std::string& path, std::function<void(VisitResult&&)>&&, const std::string& filter_suffix = "");
        static void VisitFolders(const std::string& path, std::function<void(VisitResult&&)>&&);
        static void VisitAll(const std::string& path, std::function<void(VisitResult&&)>&&, const std::string& filter_suffix = "");
        static void VisitRecursiveFiles(const std::filesystem::path& path, int depth, int max_depth, const std::function<void(VisitResult&&)>&, const std::string& filter_suffix = "");
        static bool CopyDirectory(const fs::path& source, const fs::path& destination, bool overwrite);
        static std::wstring GetProgramDataPath();
#ifdef WIN32
        static void VisitAllByQt(const std::string& path, std::function<void(VisitResult&&)>&&, const std::string& filter_suffix = "");
        static std::wstring GetCurrentFilePath();
        static std::wstring GetCurrentFolderPath();
        static void CreateDir(const std::string& path);
        static void OpenDir(const std::string& path);
#endif
    };
}

#endif //TC_SERVER_STEAM_FOLDER_UTIL_H
