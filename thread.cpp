#include "thread.h"
#include <iostream>
#include <sstream>

namespace tc
{

    ThreadPtr Thread::Make(const std::string& name, int max_task) {
        return std::make_shared<Thread>(name, max_task);
    }

    Thread::Thread(const std::string &name, int max_task) : max_tasks_(max_task) {
        this->name_ = name;
    }

    Thread::Thread(OnceTask &&task, const std::string &name, bool join) {
        this->name_ = name;
        thread_ = std::make_shared<std::thread>(task);
        if (join) {
            thread_->join();
        } else {
            thread_->detach();
        }
    }

    Thread::~Thread() {
        Exit();
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
            TaskLoop();
        });
        thread_->detach();
    }


    void Thread::TaskLoop() {
        for (;;) {
            {
                std::unique_lock<std::mutex> lk(task_mtx_);
                take_var_.wait(lk, [this]() {
                    return !tasks_.empty() || exit_;
                });
            }
            if (exit_) {
                break;
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
                task->Run();
                task_exec_count_++;
            }
        }

        exit_loop_ = true;
    }


    void Thread::Post(const ThreadTaskPtr &task) {
        std::lock_guard<std::mutex> guard(task_mtx_);
        if (max_tasks_ > 0 && (int)tasks_.size() >= max_tasks_) {
            tasks_.pop_front();
        }
        tasks_.push_back(task);
        take_var_.notify_all();
    }

    void Thread::Post(ThreadTaskPtr &&task) {
        std::lock_guard<std::mutex> guard(task_mtx_);
        if (max_tasks_ > 0 && (int)tasks_.size() >= max_tasks_) {
            tasks_.pop_front();
        }
        tasks_.push_back(std::move(task));
        take_var_.notify_all();
    }

    bool Thread::IsExit() {
        return exit_loop_;
    }

    bool Thread::IsLastTaskReturned() {
        return last_task_returned_;
    }

    void Thread::Exit() {
        exit_ = true;
        take_var_.notify_all();
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

    std::string Thread::GetId() {
        std::stringstream ss;
        //ss << std::this_thread::get_id();
        return ss.str();
    }

}
