//
// Created by RGAA on 2023/12/23.
//

#ifndef TC_APPLICATION_IPC_SHM_H
#define TC_APPLICATION_IPC_SHM_H
#if 0
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <cstring>
#include <iostream>

constexpr auto kIpcBufferSize = 2048;

using namespace boost::interprocess;

namespace tc
{

    class Data;

    // Buffer
    struct IpcBuffer {
        uint32_t msg_buffer_size = 0;
        char buffer_[kIpcBufferSize];
        interprocess_mutex buffer_mtx_;
    };

    // IPC
    class IpcShm {
    public:

        explicit IpcShm(const std::string& name);
        ~IpcShm();

        bool Open();
        std::shared_ptr<Data> ReadBuffer();

    private:
        void RemoveShm();
        void ObtainBuffer();

    private:
        bool exit_ = false;
        std::string shm_ipc_name_;

        shared_memory_object shm_object_;
        mapped_region mapped_region_;
        void* mapped_addr_ = nullptr;
        IpcBuffer* ipc_buffer_ = nullptr;
    };

}

#endif

#endif //TC_APPLICATION_IPC_SHM_H
