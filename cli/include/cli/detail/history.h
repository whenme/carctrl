// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_HISTORY_H_INCLUDED__
#define __CLI_DETAIL_HISTORY_H_INCLUDED__

#include <algorithm>
#include <cassert>
#include <deque>
#include <limits>
#include <string>
#include <vector>

namespace cli::detail
{
class History
{
public:
    explicit History(std::size_t size) : m_maxSize(size)
    {
    }

    // Insert a new item in the m_buffer, changing the m_current state to "inserting"
    // If we're browsing the history (eg with arrow keys) the new item overwrites
    // the m_current one.
    // Otherwise, the item is added to the front of the container
    void newCommand(const std::string& item)
    {
        ++m_commands;
        m_current = 0;
        if (m_mode == Mode::browsing)
        {
            assert(!m_buffer.empty());
            if (m_buffer.size() > 1 && m_buffer[1] == item)  // try to insert an element identical to last one
            {
                m_buffer.pop_front();
            }
            else  // the item was not identical
            {
                m_buffer[m_current] = item;
            }
        }
        else  // Mode::inserting
        {
            if (m_buffer.empty() || m_buffer[0] != item)  // insert an element not equal to last one
            {
                insert(item);
            }
        }
        m_mode = Mode::inserting;
    }

    // Return the previous item of the history, updating the m_current item and
    // changing the m_current state to "browsing"
    // If we're already browsing the history (eg with arrow keys) the edit line is inserted
    // to the front of the container.
    // Otherwise, the line overwrites the m_current item.
    std::string previous(const std::string& line)
    {
        if (m_mode == Mode::inserting)
        {
            insert(line);
            m_mode    = Mode::browsing;
            m_current = (m_buffer.size() > 1) ? 1 : 0;
        }
        else  // Mode::browsing
        {
            assert(!m_buffer.empty());
            m_buffer[m_current] = line;
            if (m_current != m_buffer.size() - 1)
            {
                ++m_current;
            }
        }
        assert(m_mode == Mode::browsing);
        assert(m_current < m_buffer.size());
        return m_buffer[m_current];
    }

    // Return the next item of the history, updating the m_current item.
    std::string next()
    {
        if (m_buffer.empty() || m_current == 0)
        {
            return {};
        }
        assert(m_current != 0);
        --m_current;
        assert(m_current < m_buffer.size());
        return m_buffer[m_current];
    }

    // Show the whole history on the given ostream
    void show(std::ostream& out) const
    {
        out << '\n';
        for (const auto& item : m_buffer)
        {
            out << item << '\n';
        }
        out << '\n' << std::flush;
    }

    // cmds[0] is the oldest command, cmds[size-1] the newer
    void loadCommands(const std::vector<std::string>& cmds)
    {
        for (const auto& cmd : cmds)
        {
            insert(cmd);
        }
    }

    // result[0] is the oldest command, result[size-1] the newer
    [[nodiscard]] std::vector<std::string> getCommands() const
    {
        auto numCmdsToReturn = std::min(m_commands, m_buffer.size());
        auto start           = m_buffer.begin();
        if (m_mode == Mode::browsing)
        {
            numCmdsToReturn = std::min(m_commands, m_buffer.size() - 1);
            start           = m_buffer.begin() + 1;
        }
        std::vector<std::string> result(numCmdsToReturn);
        assert(std::distance(start, m_buffer.end()) >= 0);
        assert(numCmdsToReturn <= static_cast<std::size_t>(std::distance(m_buffer.end(), start)));
        assert(numCmdsToReturn <= static_cast<unsigned long>(std::numeric_limits<long>::max()));
        std::reverse_copy(start, start + static_cast<long>(numCmdsToReturn), result.begin());
        return result;
    }

private:
    void insert(const std::string& item)
    {
        m_buffer.push_front(item);
        if (m_buffer.size() > m_maxSize)
        {
            m_buffer.pop_back();
        }
    }

    const std::size_t       m_maxSize;
    std::deque<std::string> m_buffer;
    std::size_t             m_current  = 0;
    std::size_t             m_commands = 0;  // number of commands issued
    enum class Mode
    {
        inserting,
        browsing
    };
    Mode m_mode = Mode::inserting;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_HISTORY_H_INCLUDED__
