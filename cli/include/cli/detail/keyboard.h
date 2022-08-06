// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_KEYBOARD_H_INCLUDED__
#define __CLI_DETAIL_KEYBOARD_H_INCLUDED__

#include <cassert>
#include <cstdio>
#include <memory>
#include <thread>

#include <termios.h>

#include "inputdevice.h"

namespace cli::detail
{

class InputSource
{
public:
    InputSource()
    {
        int pipes[2];          // NOLINT
        if (pipe(pipes) == 0)  // NOLINT
        {
            m_shutdownPipe = pipes[1];  // the write end
            m_readPipe     = pipes[0];  // the read end
        }
    }

    void waitKbHit() const
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(static_cast<uint64_t>(m_readPipe), &rfds);

        while (select(m_readPipe + 1, &rfds, nullptr, nullptr, nullptr) == 0)
        {
        }

        if (FD_ISSET(static_cast<uint64_t>(m_readPipe), &rfds))  // stop called
        {
            close(m_readPipe);
            throw std::runtime_error("InputSource stop");
        }

        if (FD_ISSET(STDIN_FILENO, &rfds))  // char from stdinput
        {
            return;
        }

        // cannot reach this point
        assert(false);
    }

    void stop()
    {
        write(m_shutdownPipe, " ", 1);
        close(m_shutdownPipe);
        m_shutdownPipe = -1;
    }

private:
    int m_shutdownPipe;
    int m_readPipe;
};

class Keyboard : public InputDevice
{
public:
    explicit Keyboard(Scheduler& sched) :
        InputDevice(sched),
        m_servant([this]() noexcept {
            read();
        })
    {
        toManualMode();
    }
    ~Keyboard() override
    {
        toStandardMode();
        m_is.stop();
        m_servant.join();
    }
    // non copyable
    Keyboard(const Keyboard&)            = delete;
    Keyboard& operator=(const Keyboard&) = delete;
    Keyboard(Keyboard&&)                 = delete;
    Keyboard& operator=(Keyboard&&)      = delete;

private:
    void read() noexcept
    {
        try
        {
            while (true)
            {
                auto key = get();
                notify(key);
            }
        }
        catch (const std::exception&)
        {
            // nothing to do: just exit
        }
    }

    std::pair<KeyType, char> get()
    {
        m_is.waitKbHit();

        int ch = std::getchar();
        switch (ch)
        {
            case EOF:
            case k_KeyEot:  // EOT
                return std::make_pair(KeyType::Eof, ' ');
            case k_KeyBackspace:
                return std::make_pair(KeyType::Backspace, ' ');
            case k_KeyRet:
                return std::make_pair(KeyType::Ret, ' ');
            case k_KeySymbol:  // symbol
                ch = std::getchar();
                if (ch == k_KeyArrow)  // arrow keys
                {
                    ch = std::getchar();
                    switch (ch)
                    {
                        case k_KeyWait:
                            ch = std::getchar();
                            if (ch == k_KeyCanc)
                            {
                                return std::make_pair(KeyType::Canc, ' ');
                            }
                            return std::make_pair(KeyType::Ignored, ' ');
                        case k_KeyUp:
                            return std::make_pair(KeyType::Up, ' ');
                        case k_KeyDown:
                            return std::make_pair(KeyType::Down, ' ');
                        case k_KeyLeft:
                            return std::make_pair(KeyType::Left, ' ');
                        case k_KeyRight:
                            return std::make_pair(KeyType::Right, ' ');
                        case k_KeyEnd:
                            return std::make_pair(KeyType::End, ' ');
                        case k_KeyHome:
                            return std::make_pair(KeyType::Home, ' ');
                        default:
                            return std::make_pair(KeyType::Ignored, ' ');
                    }
                }
                break;
            default:  // ascii
            {
                const char chr = static_cast<char>(ch);
                return std::make_pair(KeyType::Ascii, chr);
            }
        }
        return std::make_pair(KeyType::Ignored, ' ');
    }

    void toManualMode()
    {
        constexpr tcflag_t icanonFlag = ICANON;
        constexpr tcflag_t echoFlag   = ECHO;

        tcgetattr(STDIN_FILENO, &m_oldt);
        termios newt = m_oldt;
        newt.c_lflag &= ~(icanonFlag | echoFlag);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }

    void toStandardMode()
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &m_oldt);
    }

    termios     m_oldt{};
    InputSource m_is;
    std::thread m_servant;

    static constexpr int32_t k_KeyEot       = 4;
    static constexpr int32_t k_KeyBackspace = 127;
    static constexpr int32_t k_KeyRet       = 10;
    static constexpr int32_t k_KeySymbol    = 27;
    static constexpr int32_t k_KeyArrow     = 91;
    static constexpr int32_t k_KeyCanc      = 126;
    static constexpr int32_t k_KeyUp        = 65;
    static constexpr int32_t k_KeyDown      = 66;
    static constexpr int32_t k_KeyLeft      = 68;
    static constexpr int32_t k_KeyRight     = 67;
    static constexpr int32_t k_KeyHome      = 72;
    static constexpr int32_t k_KeyEnd       = 70;
    static constexpr int32_t k_KeyWait      = 51;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_KEYBOARD_H_INCLUDED__
