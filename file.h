#ifndef FILE_H
#define FILE_H

#include <functional>
#include <filesystem>
#include "data.h"

namespace tc 
{

    class File {
    public:
    
        static std::shared_ptr<File> OpenForRead(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForWrite(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForRW(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForAppend(const std::filesystem::path& path);
    
        static std::shared_ptr<File> OpenForReadB(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForWriteB(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForRWB(const std::filesystem::path& path);
        static std::shared_ptr<File> OpenForAppendB(const std::filesystem::path& path);

        static bool IsFolder(const std::filesystem::path& path);
        static bool Exists(const std::filesystem::path& path);
        static int64_t Size(const std::filesystem::path& path);

        File(const std::filesystem::path& path, const std::string& mode);
        ~File();
        static bool Delete(const std::filesystem::path& path);
        uint64_t Size();
        bool Exists();
        bool IsOpen();
        void Close();
        std::string FileName();

        DataPtr Read(uint64_t offset, uint64_t size, uint64_t& read_size);
        DataPtr ReadAll();
        void ReadAll(std::function<bool(uint64_t offset, DataPtr&&)>&& cbk, int buffer_size = 4096);
        std::string ReadAllAsString();
    
        int64_t Write(uint64_t offset, const DataPtr& data);
        int64_t Write(uint64_t offset, const std::string& data);
        int64_t Write(uint64_t offset, const char* data, uint64_t size);
        int64_t Append(const DataPtr& data);
        int64_t Append(const std::string& data);
        int64_t Append(const char* data, uint64_t size);

    private:
        std::filesystem::path file_path_;
        FILE* fp_ = nullptr;
        int64_t current_offset_ = 0;
    };
    
    typedef std::shared_ptr<File> FilePtr;

}

#endif // FILE_H
