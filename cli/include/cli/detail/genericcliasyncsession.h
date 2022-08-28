// SPDX-License-Identifier: GPL-2.0
#ifndef __CLI_DETAIL_GENERICCLIASYNCSESSION_H_INCLUDED__
#define __CLI_DETAIL_GENERICCLIASYNCSESSION_H_INCLUDED__

#include <string>

#include "../cli.h"  // CliSession
#include "genericasioscheduler.h"

namespace cli::detail
{

template<typename ASIOLIB>
class GenericCliAsyncSession : public CliSession
{
public:
    GenericCliAsyncSession(GenericAsioScheduler<ASIOLIB>& sched, Cli& cli) :
        CliSession(cli, std::cout, 1),
        m_input(sched.getAsioContext(), ::dup(STDIN_FILENO))
    {
        read();
    }
    ~GenericCliAsyncSession() noexcept override
    {
        try
        {
            m_input.close();
        }
        catch (const std::exception&)
        { /* do nothing */
        }
    }
    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(GenericCliAsyncSession)

private:
    void read()
    {
        prompt();
        // Read a line of input entered by the user.
        asiolib::async_read_until(
            m_input,
            m_inputBuffer,
            '\n',
            std::bind(&GenericCliAsyncSession::newLine, this, std::placeholders::_1, std::placeholders::_2));
    }

    void newLine(const asiolibec::error_code& error, std::size_t length)
    {
        if (!error || error == asiolib::error::not_found)
        {
            auto bufs = m_inputBuffer.data();
            auto size = static_cast<long>(length);
            if (!error)
            {
                --size;  // remove \n
            }
            std::string str(asiolib::buffers_begin(bufs), asiolib::buffers_begin(bufs) + size);
            m_inputBuffer.consume(length);

            feed(str);
            read();
        }
        else
        {
            m_input.close();
        }
    }

    asiolib::streambuf                m_inputBuffer;
    asiolib::posix::stream_descriptor m_input;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_GENERICCLIASYNCSESSION_H_INCLUDED__
