// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <pthread.h>
#include <ioapi/iothread.hpp>

IoThread::IoThread(std::string name, int32_t priority,
                   std::function<void(void *ctxt)> threadFun,
                   void *ctxt) :
    m_name(name),
    m_priority(priority),
    m_threadFun(threadFun),
    m_usrContext(ctxt)
{
    setThreadPriority(priority);
}

IoThread::~IoThread()
{
    stop();
}

void IoThread::start()
{
    m_thread = std::thread(m_threadFun, m_usrContext);
    m_thread.detach();
}

void IoThread::stop()
{
    pthread_cancel(m_thread.native_handle());
}

std::string& IoThread::getThreadName()
{
    return m_name;
}

std::thread::id IoThread::getThreadId()
{
    return m_thread.get_id();
}

int32_t IoThread::setThreadPriority(int32_t priority)
{
    int32_t policy, ret;
    struct sched_param param;
    ret = pthread_getschedparam(m_thread.native_handle(), &policy, &param);
    if (ret) {
        std::cout << "IoThread::setThreadPriority: fail to get param" << std::endl;
        return -1;
    }

    param.sched_priority = priority;
    ret = pthread_setschedparam(m_thread.native_handle(), policy, &param);
    if (ret) {
        std::cout << "IoThread::setThreadPriority: fail to set param" << std::endl;
        return -1;
    }

    m_priority = priority;
    return 0;
}

int32_t IoThread::getThreadPriority()
{
    int32_t policy, ret;
    struct sched_param param;
    ret = pthread_getschedparam(m_thread.native_handle(), &policy, &param);
    if (ret) {
        std::cout << "IoThread::getThreadPriority: fail to get param" << std::endl;
        return m_priority;
    }

    m_priority = param.sched_priority;
    return m_priority;
}

int32_t IoThread::setCpuAffinity(size_t cpu_id)
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id % std::thread::hardware_concurrency(), &cpuset);
    int32_t ret = pthread_setaffinity_np(m_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (ret)
        std::cout << "IoThread::setCpuAffinity: set failure" << std::endl;

    return ret;
}

int32_t IoThread::getCpuAffinity(cpu_set_t *cpuset)
{
    int32_t ret = pthread_setaffinity_np(m_thread.native_handle(), sizeof(cpu_set_t), cpuset);
    if (ret)
        std::cout << "IoThread::getCpuAffinity: get failure" << std::endl;

    return ret;
}