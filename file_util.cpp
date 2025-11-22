//
// Created by RGAA on 2024-02-06.
//

#include "file_util.h"
#include "string_util.h"

#ifdef WIN32
#include <QFile>
#include <QString>
#include <QProcess>
#endif

namespace tc
{

    std::string FileUtil::GetFileNameFromPath(const std::string& path) {
        std::string target_path = path;
        StringUtil::Replace(target_path, R"(\\)", "/");
        StringUtil::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        if (idx == std::string::npos) {
            return path;
        }
        return target_path.substr(idx + 1, target_path.size());
    }

    std::string FileUtil::GetFileNameFromPathNoSuffix(const std::string& path) {
        auto filename = GetFileNameFromPath(path);
        auto idx = filename.rfind('.');
        return filename.substr(0, idx);
    }

    std::string FileUtil::GetFileSuffix(const std::string& path) {
        auto idx = path.rfind('.');
        return path.substr(idx+1, path.size());
    }

    std::string FileUtil::GetFileFolder(const std::string& path) {
        std::string target_path = path;
        StringUtil::Replace(target_path, R"(\\)", "/");
        StringUtil::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        return target_path.substr(0, idx);
    }

    std::string FileUtil::GetFolderNameFormAbsFolderPath(const std::string& path) {
        std::string target_path = path;
        StringUtil::Replace(target_path, R"(\\)", "/");
        StringUtil::Replace(target_path, R"(\)", "/");
        auto idx = target_path.rfind('/');
        if (idx == path.size() - 1) {
            target_path = target_path.substr(0, idx);
        }
        return target_path.substr(idx + 1, target_path.size());
    }

    bool FileUtil::CopyFileExt(const std::string& from, const std::string& to, bool force_replace) {
#ifdef WIN32
        if (QFile::exists(to.c_str())) {
            if (force_replace) {
                QFile::remove(to.c_str());
            } else {
                return true;
            }
        }

        return QFile::copy(from.c_str(), to.c_str());

#endif
        return false;
    }

    void FileUtil::SelectFileInExplorer(const std::string& p) {
#ifdef WIN32
        QString path = QString::fromStdString(p);
        QString url = QString(R"(file:///%1)").arg(path.replace("/", "\\"));
        QString command = QString("explorer.exe /select,\"%1\"").arg(url);
        QProcess::startDetached(command);
#endif
    }

    bool FileUtil::ReName(const std::string& old_path, const std::string& new_path) {
        QFile file(QString::fromStdString(old_path));
        if (file.rename(QString::fromStdString(new_path))) {
            return true;
        }
        else {
            return false;
        }
    }

}