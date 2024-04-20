//
// Created by RGAA on 2023/12/23.
//

#ifndef TC_APPLICATION_IPC_MSQ_QUEUE_H
#define TC_APPLICATION_IPC_MSQ_QUEUE_H

#if 0

#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

using namespace boost::interprocess;

namespace tc
{

    using MsgQueueType = int;

    constexpr auto kInvalidMsg = -10203040;

    class IpcMsgQueue {
    public:

        explicit IpcMsgQueue(const std::string& name, int msg_queue_size);
        ~IpcMsgQueue();

        bool Create();
        bool Open();
        void Send(MsgQueueType value);
        MsgQueueType Receive();

    private:

        void RemoveQueue();

    private:

        std::string msg_queue_name_;
        int msg_queue_size_;

        std::shared_ptr<message_queue> msg_queue_ = nullptr;
    };

}

#endif

#endif //TC_APPLICATION_IPC_MSQ_QUEUE_H
