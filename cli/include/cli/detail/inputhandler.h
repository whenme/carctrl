// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_INPUTHANDLER_H_INCLUDED__
#define __CLI_DETAIL_INPUTHANDLER_H_INCLUDED__

#include <functional>
#include <string>

#include "../cli.h"
#include "inputdevice.h"
#include "terminal.h"

namespace cli::detail
{

class InputHandler
{
public:
    InputHandler(CliSession& session, InputDevice& kb) : m_session(session), m_terminal(m_session.outStream())
    {
        kb.registerHandler([this](auto key) {
            this->keyPressed(key);
        });
    }

private:
    void keyPressed(std::pair<KeyType, char> key)
    {
        const std::pair<Symbol, std::string> str = m_terminal.keyPressed(key);
        newCommand(str);
    }

    void newCommand(const std::pair<Symbol, std::string>& str)
    {
        switch (str.first)
        {
            case Symbol::nothing:
                break;
            case Symbol::eof:
                m_session.exit();
                break;
            case Symbol::command:
                m_session.feed(str.second);
                m_session.prompt();
                break;
            case Symbol::down:
                m_terminal.setLine(m_session.nextCmd());
                break;
            case Symbol::up:
            {
                auto line = m_terminal.getLine();
                m_terminal.setLine(m_session.previousCmd(line));
                break;
            }
            case Symbol::tab:
            {
                auto line        = m_terminal.getLine();
                auto completions = m_session.getCompletions(line);

                if (completions.empty())
                {
                    break;
                }
                if (completions.size() == 1)
                {
                    m_terminal.setLine(completions[0] + ' ');
                    break;
                }

                auto prefix = commonPrefix(completions);
                if (prefix.size() > line.size())
                {
                    m_terminal.setLine(prefix);
                    break;
                }
                m_session.outStream() << '\n';
                std::string items;
                std::for_each(completions.begin(), completions.end(), [&items](auto& cmd) {
                    items += '\t' + cmd;
                });
                m_session.outStream() << items << '\n';
                m_session.prompt();
                m_terminal.resetCursor();
                m_terminal.setLine(line);
                break;
            }
        }
    }

    CliSession& m_session;
    Terminal    m_terminal;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_INPUTHANDLER_H_INCLUDED__
