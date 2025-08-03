// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "cmn_attribute.hpp"

namespace cmn
{
class thread_pool
{
public:
    explicit thread_pool(std::size_t thread_count = std::thread::hardware_concurrency())
    {
        if (thread_count < 1)
            thread_count = 1;

        m_workers.reserve(thread_count);

        for (std::size_t i = 0; i < thread_count; ++i) {
            // emplace_back can use the parameters to construct the std::thread object automictly
            // use lambda function as the thread proc function,lambda can has no parameters list
            m_workers.emplace_back([&] {
                for (;;) {
                    std::packaged_task<void()> task;
                    std::unique_lock<std::mutex> lock(m_mtx);
                    m_cv.wait(lock, [&] { return (m_stop || !m_tasks.empty()); });

                    if (m_stop && m_tasks.empty())
                        return;

                    task = std::move(m_tasks.front());
                    m_tasks.pop();

                    task();
                }
            });
        }
    }

    ~thread_pool()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_stop = true;

        m_cv.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable())
                worker.join();
        }
    }

    /**
     * @function : post a function object into the thread pool, then return immediately,
     * the function object will never be executed inside this function. Instead, it will
     * be executed asynchronously in the thread pool.
     * @param : fun - global function,static function,lambda,member function,std::function
     * @return : std::future<...>
     */
    template<typename Fun, typename... Args>
    auto post(Fun&& fun, Args&&... args) -> std::future<std::invoke_result_t<Fun, Args...>>
    {
        using return_type = std::invoke_result_t<Fun, Args...>;

        std::packaged_task<return_type()> task(
        std::bind(std::forward<Fun>(fun), std::forward<Args>(args)...));

        std::future<return_type> future = task.get_future();
        std::unique_lock<std::mutex> lock(m_mtx);

        // don't allow post after stopping the pool
        if (m_stop)
            throw std::runtime_error("post a task into thread pool but the pool is stopped");

        m_tasks.emplace(std::move(task));
        m_cv.notify_one();

        return future;
    }

    /**
     * @function : get thread count of the thread pool
     */
    inline std::size_t pool_size() const noexcept
    {
        return m_workers.size();
    }

    /**
     * @function : get remain task size
     */
    inline std::size_t task_size() const noexcept
    {
        return m_tasks.size();
    }

    /**
     * @function : Determine whether current code is running in the pool's threads.
     */
    inline bool running_in_threads() const noexcept
    {
        std::thread::id curr_tid = std::this_thread::get_id();
        for (auto& thread : m_workers) {
            if (curr_tid == thread.get_id())
                return true;
        }
        return false;
    }

    /**
     * @function : Determine whether current code is running in the thread by index
     */
    inline bool running_in_thread(std::size_t index) const noexcept
    {
        if (!(index < m_workers.size()))
            return false;

        return (std::this_thread::get_id() == m_workers[index].get_id());
    }

private:
    CMN_UNCOPYABLE(thread_pool)

protected:
    // need to keep track of threads so we can join them
    std::vector<std::thread> m_workers;

    // the task queue
    std::queue<std::packaged_task<void()>> m_tasks;

    // synchronization
    std::mutex m_mtx;
    std::condition_variable m_cv;

    // flag indicate the pool is stoped
    bool m_stop = false;
};

}  // namespace cmn
