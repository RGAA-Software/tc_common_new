#include "thread.h"
#include <iostream>
#include <sstream>
#include "log.h"
#include "memory_stat.h"

namespace tc
{

    ThreadPtr Thread::Make(const std::string& name, int max_task) {
        return std::make_shared<Thread>(name, max_task);
    }

    std::shared_ptr<Thread> Thread::MakeOnceTask(OnceTask&& task, const std::string& name, bool join) {
        auto t = std::make_shared<Thread>(std::move(task), name, join);
        return t;
    }

    Thread::Thread(const std::string &name, int max_task) : max_tasks_(max_task) {
        this->name_ = name;
    }

    Thread::Thread(OnceTask &&task, const std::string &name, bool join) {
        this->name_ = name;
        thread_ = std::make_shared<std::thread>([=, this]() {
#ifdef WIN32
            this->tid_ = GetCurrentThreadId();
#endif
            task();
        });
        
        if (join) {
            thread_->join();
        }
    }

    Thread::~Thread() {
        MemoryStat::Instance()->RemoveThread(GetGeneralId());
    }

    void Thread::Poll() {
        {
            std::lock_guard<std::mutex> guard(init_mtx_);
            if (init_) {
                return;
            }
            init_ = true;
        }

        thread_ = std::make_shared<std::thread>([this]() {
#ifdef WIN32
            this->tid_ = GetCurrentThreadId();
#endif
            TaskLoop();
        });

        MemoryStat::Instance()->AddThread(GetGeneralId(), shared_from_this());
    }


    void Thread::TaskLoop() {
        for (;;) {
            if (exit_) {
                LOGI("Ok, thread exit: {}", name_);
                return;
            }
            {
                std::unique_lock<std::mutex> lk(task_mtx_);
                take_var_.wait(lk, [this]() {
                    return !tasks_.empty() || exit_;
                });
            }

            ThreadTaskPtr task = nullptr;
            {
                std::lock_guard<std::mutex> guard(task_mtx_);
                if (!tasks_.empty()) {
                    task = tasks_.front();
                }
                if (task) {
                    tasks_.remove(task);
                }
            }
            if (task) {
                task->state_ = ThreadTaskState::kRunning;
                task->Run();
                task->state_ = ThreadTaskState::kReady;
                task_exec_count_++;
            }
        }

        exit_loop_ = true;
    }


    void Thread::Post(const ThreadTaskPtr &task) {
        if (exit_) {return;}
        std::lock_guard<std::mutex> guard(task_mtx_);
        if (max_tasks_ > 0 && (int)tasks_.size() >= max_tasks_) {
            tasks_.pop_front();
        }
        tasks_.push_back(task);
        take_var_.notify_all();
    }

    void Thread::Post(ThreadTaskPtr &&task) {
        if (exit_) {return;}
        std::lock_guard<std::mutex> guard(task_mtx_);
        if (max_tasks_ > 0 && (int)tasks_.size() >= max_tasks_) {
            tasks_.pop_front();
        }
        tasks_.push_back(std::move(task));
        take_var_.notify_all();
    }

    void Thread::Post(std::function<void()>&& task) {
        this->Post(SimpleThreadTask::Make(std::move(task)));
    }

    bool Thread::RemoveTask(uint64_t task_id) {
        std::lock_guard<std::mutex> guard(task_mtx_);
        for (auto it = tasks_.begin(); it != tasks_.end(); it++) {
            if ((*it)->task_id_ == task_id) {
                tasks_.erase(it);
                return true;
            }
        }
        return false;
    }

    bool Thread::TaskExists(uint64_t task_id) {
        std::lock_guard<std::mutex> guard(task_mtx_);
        for (auto it = tasks_.begin(); it != tasks_.end(); it++) {
            if ((*it)->task_id_ == task_id) {
                return true;
            }
        }
        return false;
    }

    std::list<ThreadTaskPtr> Thread::GetTasks() {
        return tasks_;
    }

    bool Thread::IsExit() {
        return exit_loop_;
    }

    bool Thread::IsLastTaskReturned() {
        return last_task_returned_;
    }

    void Thread::Exit() {
        exit_ = true;
        take_var_.notify_one();
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }

    bool Thread::HasTask() {
        return TaskSize() > 0;
    }

    int Thread::TaskSize() {
        std::lock_guard<std::mutex> guard(task_mtx_);
        return tasks_.size();
    }

    bool Thread::IsJoinable() {
        return thread_ && thread_->joinable();
    }

    void Thread::Join() {
        if (thread_) {
            thread_->join();
        }
    }

    unsigned long Thread::ExecCount() {
        return task_exec_count_;
    }

    int Thread::MaxTaskSize() {
        return max_tasks_;
    }

    uint32_t Thread::GetGeneralId() {
        auto id = thread_->get_id();
        std::hash<std::thread::id> hasher;
        size_t hash_value = hasher(id);
        int thread_id = static_cast<int>(hash_value);
        return thread_id;
    }

    uint32_t Thread::GetTid() {
        return tid_;
    }

    std::string Thread::GetThreadName() {
        return name_;
    }

}
