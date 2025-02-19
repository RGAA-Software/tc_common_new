//
// Created by RGAA on 2023/12/23.
//

#include "ipc_msg_queue.h"
#if 0
#include "log.h"

namespace tc
{

    IpcMsgQueue::IpcMsgQueue(const std::string& name, int msg_queue_size) {
        this->msg_queue_name_ = name;
        this->msg_queue_size_ = msg_queue_size;
    }

    IpcMsgQueue::~IpcMsgQueue() {
        RemoveQueue();
    }

    bool IpcMsgQueue::Create() {
        RemoveQueue();
        try {
            msg_queue_ = std::make_shared<message_queue>(create_only,
                                                         this->msg_queue_name_.c_str(),
                                                         msg_queue_size_,
                                                         sizeof(MsgQueueType));
            return true;
        } catch (std::exception& e) {
            LOGE("Create msg queue failed: {}, {}", this->msg_queue_name_, e.what());
            return false;
        }
    }

    bool IpcMsgQueue::Open() {
        try {
            msg_queue_ = std::make_shared<message_queue>(open_only, this->msg_queue_name_.c_str());
            return true;
        } catch (std::exception& e) {
            LOGE("Open msg queue failed: {}, {}", this->msg_queue_name_, e.what());
            return false;
        }
    }

    void IpcMsgQueue::Send(MsgQueueType value) {
        if (!msg_queue_) {
            return;
        }
        msg_queue_->send(&value, sizeof(MsgQueueType), 0);
    }

    MsgQueueType IpcMsgQueue::Receive() {
        if (!msg_queue_) {
            return kInvalidMsg;
        }
        MsgQueueType value;
        unsigned int priority;
        message_queue::size_type received_size;
        msg_queue_->receive(&value, sizeof(MsgQueueType), received_size, priority);
        return value;
    }

    void IpcMsgQueue::RemoveQueue() {
        message_queue::remove(this->msg_queue_name_.c_str());
    }

}
#endif