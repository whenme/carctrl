// SPDX-License-Identifier: GPL-2.0
#include <iostream>
#include <pthread.h>
#include <ioapi/easylog.hpp>
#include <ioapi/iothread.hpp>

IoThread::IoThread(std::string name, int32_t priority,
                   std::function<void(void *ctxt)> threadFun,
                   void *ctxt) :
    m_name(name),
    m_priority(priority),
    m_threadFun(threadFun),
    m_usrContext(ctxt)
{
}

IoThread::~IoThread()
{
    stop();
}

void IoThread::start()
{
    m_thread = std::thread(m_threadFun, m_usrContext);

    //after detach, native_handle() will changed. save it before detach
    m_threadId = m_thread.native_handle();
    m_thread.detach();

    setThreadPriority(m_priority);
    m_runState = true;
}

void IoThread::stop()
{
    if (m_runState)
        pthread_cancel(m_threadId);

    m_runState = false;
}

std::string& IoThread::getThreadName()
{
    return m_name;
}

std::thread::id IoThread::getThreadId()
{
    return m_thread.get_id();
}

bool IoThread::getRunState()
{
    return m_runState;
}

int32_t IoThread::setThreadPriority(int32_t priority)
{
    int32_t policy, ret;
    struct sched_param param;
    ret = pthread_getschedparam(m_threadId, &policy, &param);
    if (ret) {
        apilog::warn("fail to get param");
        return -1;
    }

    int32_t maxPrio = sched_get_priority_max(policy);
    int32_t minPrio = sched_get_priority_min(policy);
    if (priority > maxPrio) {
        priority = maxPrio;
    } else if (priority < minPrio) {
        priority = minPrio;
    }
    param.sched_priority = priority;
    ret = pthread_setschedparam(m_threadId, policy, &param);
    if (ret) {
        apilog::warn("fail to set param {}", ret);
        return -1;
    }

    m_priority = priority;
    return 0;
}

int32_t IoThread::getThreadPriority()
{
    int32_t policy, ret;
    struct sched_param param;
    ret = pthread_getschedparam(m_threadId, &policy, &param);
    if (ret) {
        apilog::warn("fail to get param");
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
    int32_t ret = pthread_setaffinity_np(m_threadId, sizeof(cpu_set_t), &cpuset);
    if (ret)
        apilog::warn("setCpuAffinity failed");

    return ret;
}

int32_t IoThread::getCpuAffinity(cpu_set_t *cpuset)
{
    int32_t ret = pthread_setaffinity_np(m_threadId, sizeof(cpu_set_t), cpuset);
    if (ret)
        apilog::warn("getCpuAffinity failed");

    return ret;
}
