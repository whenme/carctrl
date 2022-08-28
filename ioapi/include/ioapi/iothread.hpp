// SPDX-License-Identifier: GPL-2.0
#ifndef __IO_THREAD_HPP__
#define __IO_THREAD_HPP__

#include <functional>
#include <string>
#include <thread>

#include <asio.hpp>

class IoThread
{
public:
    IoThread(std::string& name, int32_t priority,
             std::function<void(void *ctxt)> threadFun,
             void *ctxt = nullptr);
    virtual ~IoThread();

    void start();
    void stop();

    std::string&    getThreadName();
    std::thread::id getThreadId();
    int32_t         getThreadPriority();
    int32_t         setThreadPriority(int32_t priority);
    int32_t         setCpuAffinity(size_t cpu_id);

    constexpr static int32_t ThreadPriorityLowest      = 1;
    constexpr static int32_t ThreadPriorityBelowNormal = 16;
    constexpr static int32_t ThreadPriorityNormal      = 32;
    constexpr static int32_t ThreadPriorityAboveNormal = 66;
    constexpr static int32_t ThreadPriorityHighest     = 99;

private:
    std::thread m_thread;
    std::string m_name;
    int32_t m_priority;
    void *m_usrContext;
    std::function<void(void *ctxt)> m_threadFun;
};


#endif
