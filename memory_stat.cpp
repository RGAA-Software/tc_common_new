//
// Created by RGAA on 14/09/2025.
//

#include "memory_stat.h"
#include <sstream>
#include "num_formatter.h"

namespace tc
{

    std::string MemoryStatInfo::Dump() {
        std::stringstream ss;
        ss << std::endl;
        ss << std::format("History alloc size: {}", NumFormatter::FormatStorageSize(alloc_size_)) << std::endl;
        ss << std::format("Total count: {}", total_count_) << std::endl;
        ss << std::format("Total memory size: {}", total_memory_size_) << std::endl;
        ss << std::format("Total memory size: {} KB", total_memory_size_KB_) << std::endl;
        ss << std::format("Total memory size: {} MB", total_memory_size_MB_) << std::endl;
        return ss.str();
    }

    void MemoryStat::AddMemInfo(uint64_t id, const std::shared_ptr<MemoryInfo>& info) {
        mem_info_.Insert(id, info);
        alloc_size_ += info->size_;
    }

    void MemoryStat::RemoveMemInfo(uint64_t id) {
        mem_info_.Remove(id);
    }

    MemoryStatInfo MemoryStat::GetStatInfo() {
        MemoryStatInfo stat_info;
        stat_info.alloc_size_ = alloc_size_;
        stat_info.total_count_ = mem_info_.Size();
        mem_info_.VisitAll([&](uint64_t key, std::shared_ptr<MemoryInfo>& info){
            stat_info.total_memory_size_ += info->size_;
        });
        stat_info.total_memory_size_KB_ = stat_info.total_memory_size_ / 1024;
        stat_info.total_memory_size_MB_ = stat_info.total_memory_size_ / 1024 / 1024;
        return stat_info;
    }

}