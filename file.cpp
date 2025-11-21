#pragma execution_character_set("utf-8")

#include "file.h"

#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include "string_util.h"
#include "log.h"

namespace tc 
{

    std::shared_ptr<File> File::OpenForRead(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "r");
    }
    
    std::shared_ptr<File> File::OpenForWrite(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "w");
    }
    
    std::shared_ptr<File> File::OpenForRW(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "w+");
    }
    
    std::shared_ptr<File> File::OpenForAppend(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "a+");
    }
    
    std::shared_ptr<File> File::OpenForReadB(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "rb");
    }
    
    std::shared_ptr<File> File::OpenForWriteB(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "wb");
    }
    
    std::shared_ptr<File> File::OpenForRWB(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "wb+");
    }
    
    std::shared_ptr<File> File::OpenForAppendB(const std::string& path) {
        return std::make_shared<File>(path.c_str(), "ab+");
    }

    bool File::IsFolder(const std::string& path) {
#ifdef WIN32
        QFileInfo file_info(path.c_str());
        return file_info.isDir();
#else
      return false;
#endif
    }

    bool File::Exists(const std::string& path) {
#ifdef WIN32
        return QFile::exists(path.c_str());
#else
        return std::filesystem::exists(path);
#endif
    }

    int64_t File::Size(const std::string& path) {
#ifdef WIN32
        QFileInfo fi(path.c_str());
        if (!fi.exists()) {
            return -1;
        }
        return (int64_t)fi.size();
#else
        uintmax_t size = std::filesystem::file_size(path);
        return (int64_t)size;
#endif
    }

    File::File(const std::string& path, const std::string& mode) {
        auto origin_path = path;
        StringUtil::Replace(origin_path, "\\", "/");
        this->file_path_ = origin_path;
#ifdef WIN32
        file_ = std::make_shared<QFile>(path.c_str());
        if (mode == "a+" || mode == "ab+") {
            if (!file_->open(QIODeviceBase::OpenModeFlag::Append)) {
                LOGE("Open Append failed: {}", path);
                return;
            }
        } else {
            if (mode == "rb") {
                if (!file_->open(QIODeviceBase::OpenModeFlag::ReadOnly)) {
                    LOGE("Read only failed: {}", path);
                    return;
                }
            }
            else {
                if (!file_->open(QIODeviceBase::OpenModeFlag::ReadWrite)) {
                    LOGE("Open Read-Write failed, try readonly: {}", path);
                    if (!file_->open(QIODeviceBase::OpenModeFlag::ReadOnly)) {
                        LOGE("Open Read-Write failed: {}", path);
                        return;
                    }
                }
            }
        }
        file_info_ = QFileInfo(path.c_str());
#else
        fopen(path.c_str(), mode.c_str());
#endif
    }
    
    File::~File() {
        Close();
    }

    bool File::Delete(const std::string& path) {
#ifdef WIN32
        return QFile::remove(path.c_str());
#else
        return false;
#endif
    }

    bool File::Exists() {
#ifdef WIN32
        return QFile::exists(this->file_path_.c_str());
#else
       return std::filesystem::exists(this->file_path_);
#endif
    }
    
    bool File::IsOpen() {
#ifdef WIN32
        return this->file_ && this->file_->isOpen();
#else
        return fp_ != nullptr;
#endif
    }

    std::string File::FileName() {
#ifdef WIN32
        return file_info_.fileName().toStdString();
#else
        return "";
#endif
    }

    DataPtr File::Read(uint64_t offset, uint64_t size, uint64_t& read_size) {
        if (!IsOpen()) {
            return nullptr;
        }
#ifdef WIN32
        if (!file_->seek((qint64)offset)) {
            LOGE("file seek failed, offset: {}, file: {}", offset, this->file_path_);
            return nullptr;
        }
        auto bytes = file_->read((qint64)size);
        read_size = bytes.size();
        if (read_size <= 0) {
            return nullptr;
        }
        return Data::Make(bytes.data(), (int)bytes.size());
#else
        rewind(fp_);
    
        char* read_data = (char*)malloc(size);
        fseek(fp_, (long)offset, SEEK_SET);
        read_size = fread(read_data, 1, size, fp_);
        if (read_size <= 0) {
            free(read_data);
            return nullptr;
        }
    
        auto data = Data::Make(read_data, read_size);
        free(read_data);
        return data;
#endif
    }
    
    DataPtr File::ReadAll() {
        uint64_t read_size;
        return Read(0, Size(), read_size);
    }
    
    void File::ReadAll(std::function<bool(uint64_t, DataPtr&&)>&& cbk, int buffer_size) {
        uint64_t offset = 0;
        uint64_t file_size = Size();
        uint32_t block_size = buffer_size;
        while (offset < file_size) {
            uint64_t read_size = 0;
            auto data = Read(offset, block_size, read_size);
            if (data && read_size != 0) {
                if (cbk(offset, std::move(data))) {
                    break;
                }
                offset += read_size;
            }
        }
    }
    
    std::string File::ReadAllAsString() {
        auto data = ReadAll();
        if (data) {
            return data->AsString();
        }
        return "";
    }
    
    uint64_t File::Size() {
        if (!IsOpen()) {
            return 0;
        }
#ifdef WIN32
        return file_info_.size();
#else
        fseek(fp_, 0, SEEK_END);
        auto size = ftell(fp_);
        fseek(fp_, 0, SEEK_SET);
        return size;
#endif
    }
    
    int64_t File::Write(uint64_t offset, const DataPtr& data) {
        if (!data) {
            return -1;
        }
        return Write(offset, data->CStr(), data->Size());
    }
    
    int64_t File::Write(uint64_t offset, const std::string& data) {
        if (data.empty()) {
            return -1;
        }
        return Write(offset, data.c_str(), data.size());
    }
    
    int64_t File::Write(uint64_t offset, const char* data, uint64_t size) {
        if (!IsOpen()) {
            return -1;
        }
#ifdef WIN32
        if (!file_->seek((qint64)offset)) {
            LOGE("seek failed for writing data, offset: {}, file: {}", offset, file_path_);
            return -1;
        }
        return file_->write(data, (qint64)size);
#else
        rewind(fp_);
        fseek(fp_, offset, SEEK_SET);
        return (int64_t)fwrite(data, 1, size, fp_);
#endif
    }

    int64_t File::Append(const DataPtr& data) {
        return this->Append(data->CStr(), data->Size());
    }

    int64_t File::Append(const std::string& data) {
        return this->Append(data.c_str(), data.size());
    }

    int64_t File::Append(const char* data, uint64_t size) {
        if (!IsOpen()) {
            return -1;
        }
#ifdef WIN32
        return file_->write(data, (qint64)size);
#else
        return (int64_t)fwrite(data, 1, size, fp_);
#endif
    }

    void File::Close() {
#ifdef WIN32
        if (file_) {
            file_->close();
            file_ = nullptr;
        }
#else
        if (fp_) {
            fflush(fp_);
            fclose(fp_);
            fp_ = nullptr;
        }
#endif
    }
}


















