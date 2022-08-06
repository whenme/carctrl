// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_GENERICASIOSCHEDULER_H_INCLUDED__
#define __CLI_DETAIL_GENERICASIOSCHEDULER_H_INCLUDED__

#include <functional>
#include <memory>

namespace cli::detail
{
/**
 * A `Scheduler` represents an engine capable of running a task.
 * Its method `Post` can be safely called from any thread to submit the task
 * that will execute in an unspecified thread of execution as soon as possible
 * (but in any case after the call to `Post` is terminated).
 */
class Scheduler
{
public:
    Scheduler()          = default;
    virtual ~Scheduler() = default;

    // non copyable
    Scheduler(const Scheduler&)            = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&)                 = delete;
    Scheduler& operator=(Scheduler&&)      = delete;

    /// Submits a completion token or function object for execution.
    virtual void post(const std::function<void()>& fun) = 0;
};

template<typename ASIOLIB>
class GenericAsioScheduler : public Scheduler
{
public:
    using ContextType = typename ASIOLIB::ContextType;
    using WorkGuard   = typename ASIOLIB::WorkGuard;

    explicit GenericAsioScheduler() :
        m_owned{true},
        m_context{new ContextType()},
        m_executor{*m_context},
        m_work{std::make_unique<WorkGuard>(ASIOLIB::makeWorkGuard(*m_context))}
    {
    }

    explicit GenericAsioScheduler(ContextType& context) : m_context{&context}, m_executor{*m_context}
    {
    }

    ~GenericAsioScheduler() override
    {
        if (m_owned)
        {
            m_work.reset();  // work uses context, so it must be deleted before context
            delete m_context;
        }
    }

    // non copyable
    GenericAsioScheduler(const GenericAsioScheduler&)            = delete;
    GenericAsioScheduler& operator=(const GenericAsioScheduler&) = delete;
    GenericAsioScheduler(GenericAsioScheduler&&)                 = delete;
    GenericAsioScheduler& operator=(GenericAsioScheduler&&)      = delete;

    void stop()
    {
        if (m_work)
        {
            m_work->reset();
        }
        m_context->stop();
    }

    void run()
    {
        m_context->run();
    }

    [[nodiscard]] bool stopped() const
    {
        return m_context->stopped();
    }

    void execOne()
    {
        m_context->run_one();
    }

    void pollOne()
    {
        m_context->poll_one();
    }

    void post(const std::function<void()>& fun) override
    {
        m_executor.post(fun);
    }

    ContextType& getAsioContext()
    {
        return *m_context;
    }

private:
    using ExecutorType = typename ASIOLIB::Executor;

    bool                       m_owned = false;
    ContextType*               m_context;
    ExecutorType               m_executor;
    std::unique_ptr<WorkGuard> m_work;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_GENERICASIOSCHEDULER_H_INCLUDED__
