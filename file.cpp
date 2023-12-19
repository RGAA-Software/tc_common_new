#pragma execution_character_set("utf-8")

#include "file.h"

#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <boost/filesystem.hpp>

#include "string_ext.h"
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
    
    File::File(const std::string& path, const std::string& mode) {
        auto origin_path = path;
        StringExt::Replace(origin_path, "\\", "/");
        this->file_path_ = origin_path;
        fopen_s(&fp_, this->file_path_.c_str(), mode.c_str());
    }
    
    File::~File() {
        Close();
    }
    
    bool File::Exists() {
       return std::filesystem::exists(this->file_path_);
    }
    
    bool File::IsOpen() {
        return fp_ != nullptr;
    }
    
    DataPtr File::Read(uint64_t offset, uint64_t size, uint64_t& read_size) {
        if (!IsOpen()) {
            return nullptr;
        }
    
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
    }
    
    DataPtr File::ReadAll() {
        uint64_t read_size;
        return Read(0, Size(), read_size);
    }
    
    void File::ReadAll(std::function<void(uint64_t, DataPtr)>&& cbk) {
        uint64_t offset = 0;
        uint64_t file_size = Size();
        uint32_t block_size = 4 * 1024;
        while (offset < file_size) {
            uint64_t read_size = 0;
            auto data = Read(offset, block_size, read_size);
            if (data && read_size != 0) {
                cbk(offset, data);
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
    
        fseek(fp_, 0, SEEK_END);
        auto size = ftell(fp_);
        fseek(fp_, 0, SEEK_SET);
        return size;
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
    
        rewind(fp_);
    
        fseek(fp_, offset, SEEK_SET);
        return (int64_t)fwrite(data, 1, size, fp_);
    }
    
    void File::Close() {
        if (fp_) {
            fflush(fp_);
            fclose(fp_);
            fp_ = nullptr;
        }
    }
    
    int File::GetFileDescriptor() {
        if (fp_) {
            return _fileno(fp_);
        }
        return -1;
    }
}


















