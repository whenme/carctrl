// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_TERMINAL_H_INCLUDED__
#define __CLI_DETAIL_TERMINAL_H_INCLUDED__

#include <string>

#include "inputdevice.h"

namespace cli::detail
{

enum class Symbol
{
    Nothing,
    Command,
    Up,
    Down,
    Tab,
    Eof
};

class Terminal
{
public:
    explicit Terminal(std::ostream& out) : m_out(out)
    {
    }

    void resetCursor()
    {
        m_position = 0;
    }

    void setLine(const std::string& newLine)
    {
        m_out << std::string(m_position, '\b') << newLine << std::flush;

        // if newLine is shorter then currentLine, we have
        // to clear the rest of the string
        if (newLine.size() < m_currentLine.size())
        {
            m_out << std::string(m_currentLine.size() - newLine.size(), ' ');
            // and go back
            m_out << std::string(m_currentLine.size() - newLine.size(), '\b') << std::flush;
        }

        m_currentLine = newLine;
        m_position    = m_currentLine.size();
    }

    [[nodiscard]] std::string getLine() const
    {
        return m_currentLine;
    }

    std::pair<Symbol, std::string> keyPressed(std::pair<KeyType, char> key)
    {
        switch (key.first)
        {
            case KeyType::Eof:
                return std::make_pair(Symbol::Eof, std::string{});
                break;
            case KeyType::Backspace:
                if (m_position == 0)
                {
                    break;
                }
                --m_position;

                {
                    const auto pos = static_cast<std::string::difference_type>(m_position);
                    // remove the char from buffer
                    m_currentLine.erase(m_currentLine.begin() + pos);
                    // go back to the previous char
                    m_out << '\b';
                    // output the rest of the line
                    m_out << std::string(m_currentLine.begin() + pos, m_currentLine.end());
                }
                // remove last char
                m_out << ' ';
                // go back to the original position
                m_out << std::string(m_currentLine.size() - m_position + 1, '\b') << std::flush;
                break;
            case KeyType::Up:
                return std::make_pair(Symbol::Up, std::string{});
                break;
            case KeyType::Down:
                return std::make_pair(Symbol::Down, std::string{});
                break;
            case KeyType::Left:
                if (m_position > 0)
                {
                    m_out << '\b' << std::flush;
                    --m_position;
                }
                break;
            case KeyType::Right:
                if (m_position < m_currentLine.size())
                {
                    m_out << m_currentLine[m_position] << std::flush;
                    ++m_position;
                }
                break;
            case KeyType::Ret:
            {
                m_out << "\r\n";
                auto cmd = m_currentLine;
                m_currentLine.clear();
                m_position = 0;
                return std::make_pair(Symbol::Command, cmd);
            }
            break;
            case KeyType::Ascii:
            {
                const char chr = static_cast<char>(key.second);
                if (chr == '\t')
                {
                    return std::make_pair(Symbol::Tab, std::string());
                }

                const auto pos = static_cast<std::string::difference_type>(m_position);
                // output the new char:
                m_out << chr;
                // and the rest of the string:
                m_out << std::string(m_currentLine.begin() + pos, m_currentLine.end());
                // go back to the original position
                m_out << std::string(m_currentLine.size() - m_position, '\b') << std::flush;
                // update the buffer and cursor position:
                m_currentLine.insert(m_currentLine.begin() + pos, chr);
                ++m_position;
                break;
            }
            case KeyType::Canc:
                if (m_position == m_currentLine.size())
                {
                    break;
                }
                {
                    const auto pos = static_cast<std::string::difference_type>(m_position);
                    // output the rest of the line
                    m_out << std::string(m_currentLine.begin() + pos + 1, m_currentLine.end());
                    // remove last char
                    m_out << ' ';
                    // go back to the original position
                    m_out << std::string(m_currentLine.size() - m_position, '\b') << std::flush;
                    // remove the char from buffer
                    m_currentLine.erase(m_currentLine.begin() + pos);
                }
                break;
            case KeyType::End:
            {
                const auto pos = static_cast<std::string::difference_type>(m_position);
                m_out << std::string(m_currentLine.begin() + pos, m_currentLine.end()) << std::flush;
            }
                m_position = m_currentLine.size();
                break;
            case KeyType::Home:
                m_out << std::string(m_position, '\b') << std::flush;
                m_position = 0;
                break;
            case KeyType::Ignored:
                // TODO
                break;
        }

        return std::make_pair(Symbol::Nothing, std::string());
    }

private:
    std::string   m_currentLine;
    std::size_t   m_position = 0;  // next writing position in m_currentLine
    std::ostream& m_out;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_TERMINAL_H_INCLUDED__
