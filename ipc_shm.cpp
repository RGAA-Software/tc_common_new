//
// Created by RGAA on 2023/12/23.
//

#include "ipc_shm.h"
#include "log.h"
#include "data.h"

#if 0
namespace tc
{

    IpcShm::IpcShm(const std::string& name) {
        this->shm_ipc_name_ = name;
    }

    IpcShm::~IpcShm() {
        RemoveShm();
    }

    bool IpcShm::Open() {
        RemoveShm();
        shm_object_ = shared_memory_object(open_or_create, this->shm_ipc_name_.c_str(), read_write);
        ObtainBuffer();
        return true;
    }

    void IpcShm::ObtainBuffer() {
        shm_object_.truncate(sizeof(IpcBuffer));
        mapped_region_ = mapped_region(shm_object_, read_write);
        mapped_addr_ = mapped_region_.get_address();
        ipc_buffer_ = new(mapped_addr_)IpcBuffer;
    }

    void IpcShm::RemoveShm() {
        shared_memory_object::remove(this->shm_ipc_name_.c_str());
    }

    std::shared_ptr<Data> IpcShm::ReadBuffer() {
        if (!ipc_buffer_) {
            LOGE("Not have ipc buffer...");
            return nullptr;
        }
        if (ipc_buffer_->msg_buffer_size > kIpcBufferSize) {
            return nullptr;
        }
        scoped_lock<interprocess_mutex> lock(ipc_buffer_->buffer_mtx_);
        return Data::Make(ipc_buffer_->buffer_, ipc_buffer_->msg_buffer_size);
    }

}
#endif