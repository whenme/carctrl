// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLIFILESESSION_H_INCLUDED__
#define __CLI_CLIFILESESSION_H_INCLUDED__

#include <iostream>
#include <stdexcept>  // std::invalid_argument
#include <string>
#include "cli.h"

namespace cli::detail
{

class CliFileSession : public CliSession
{
public:
    /// @throw std::invalid_argument if @c _in or @c out are invalid streams
    explicit CliFileSession(Cli& cli, std::istream& in = std::cin, std::ostream& out = std::cout) :
        CliSession(cli, out, 1),
        m_exit(false),
        m_in(in)
    {
        if (!m_in.good())
            throw std::invalid_argument("istream invalid");
        if (!out.good())
            throw std::invalid_argument("ostream invalid");
        ExitAction([this](std::ostream&) {
            m_exit = true;
        });
    }

    void Start()
    {
        while (!m_exit)
        {
            Prompt();
            std::string line;
            if (!m_in.good())
                Exit();
            std::getline(in, line);
            if (m_in.eof())
                Exit();
            else
                Feed(line);
        }
    }

private:
    bool          m_exit;
    std::istream& m_in;
};

}  // namespace cli::detail

#endif  // __CLI_CLIFILESESSION_H_INCLUDED__
