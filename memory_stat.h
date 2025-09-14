//
// Created by RGAA on 14/09/2025.
//

#ifndef GAMMARAYPREMIUM_MEMORY_STAT_H
#define GAMMARAYPREMIUM_MEMORY_STAT_H

#include "concurrent_hashmap.h"

namespace tc
{

    class MemoryInfo {
    public:
        uint64_t id_ = 0;
        uint64_t size_ = 0;
        std::string module_;
        std::string name_;
    };

    class MemoryStatInfo {
    public:
        std::string Dump();

    public:
        uint64_t total_count_ = 0;
        uint64_t total_memory_size_ = 0;
        uint64_t total_memory_size_KB_ = 0;
        uint64_t total_memory_size_MB_ = 0;
        std::string total_readable_memory_size_;
    };

    class MemoryStat {
    public:
        static MemoryStat* Instance() {
            static MemoryStat stat;
            return &stat;
        }

        void AddMemInfo(uint64_t id, const std::shared_ptr<MemoryInfo>& info);
        void RemoveMemInfo(uint64_t id);
        MemoryStatInfo GetStatInfo();

    private:
        tc::ConcurrentHashMap<uint64_t, std::shared_ptr<MemoryInfo>> mem_info_;
    };

}

#endif //GAMMARAYPREMIUM_MEMORY_STAT_H
