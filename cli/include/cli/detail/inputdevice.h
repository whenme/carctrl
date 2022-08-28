// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_INPUTDEVICE_H_INCLUDED__
#define __CLI_DETAIL_INPUTDEVICE_H_INCLUDED__

#include <functional>
#include <string>

#include "genericasioscheduler.h"

namespace cli::detail
{
enum class KeyType
{
    Ascii,
    Up,
    Down,
    Left,
    Right,
    Backspace,
    Canc,
    Home,
    End,
    Ret,
    Eof,
    Ignored
};

class InputDevice
{
public:
    using Handler = std::function<void(std::pair<KeyType, char>)>;

    explicit InputDevice(Scheduler& sched) : m_scheduler(sched)
    {
    }
    virtual ~InputDevice() = default;
    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(InputDevice)

    template<typename H>
    void registerHandler(H&& hdl)
    {
        m_handler = std::forward<H>(hdl);
    }

protected:
    void notify(std::pair<KeyType, char> key)
    {
        m_scheduler.post([this, key]() {
            if (m_handler)
            {
                m_handler(key);
            }
        });
    }

private:
    Scheduler& m_scheduler;
    Handler    m_handler;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_INPUTDEVICE_H_INCLUDED__
