// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_HISTORYSTORAGE_H_INCLUDED__
#define __CLI_HISTORYSTORAGE_H_INCLUDED__

#include <deque>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace cli
{
class HistoryStorage
{
public:
    HistoryStorage()          = default;
    virtual ~HistoryStorage() = default;

    // non copyable
    HistoryStorage(const HistoryStorage&)            = delete;
    HistoryStorage& operator=(const HistoryStorage&) = delete;
    HistoryStorage(HistoryStorage&&)                 = delete;
    HistoryStorage& operator=(HistoryStorage&&)      = delete;

    // Store a vector of commands in the history storage
    virtual void store(const std::vector<std::string>& commands) = 0;
    // Returns all the commands stored
    [[nodiscard]] virtual std::vector<std::string> commands() const = 0;
    // Clear the whole content of the storage
    // After calling this method, Commands() returns the empty vector
    virtual void clear() = 0;
};

class FileHistoryStorage : public HistoryStorage
{
public:
    explicit FileHistoryStorage(std::string fileName, std::size_t size = k_MHistorySizeDef) :
        m_maxSize(size),
        m_fileName(std::move(fileName))
    {
    }

    void store(const std::vector<std::string>& cmds) override
    {
        using Dt = std::vector<std::string>::difference_type;
        auto cmd = commands();
        cmd.insert(cmd.end(), cmds.begin(), cmds.end());
        if (cmd.size() > m_maxSize)
        {
            cmd.erase(cmd.begin(), cmd.begin() + static_cast<Dt>(cmd.size() - m_maxSize));
        }
        std::ofstream of(m_fileName, std::ios_base::out);
        for (const auto& line : cmd)
        {
            of << line << '\n';
        }
    }

    [[nodiscard]] std::vector<std::string> commands() const override
    {
        std::vector<std::string> cmds;
        std::ifstream            in(m_fileName);
        if (in)
        {
            std::string line;
            while (std::getline(in, line))
            {
                cmds.push_back(line);
            }
        }
        return cmds;
    }

    void clear() override
    {
        const std::ofstream of(m_fileName, std::ios_base::out | std::ios_base::trunc);
    }

private:
    const std::size_t m_maxSize;
    const std::string m_fileName;

    static constexpr std::size_t k_MHistorySizeDef = 1000;
};

class VolatileHistoryStorage : public HistoryStorage
{
public:
    explicit VolatileHistoryStorage(std::size_t size = k_MHistorySizeDef) : m_maxSize(size)
    {
    }

    void store(const std::vector<std::string>& cmds) override
    {
        using Dt = std::deque<std::string>::difference_type;
        m_commands.insert(m_commands.end(), cmds.begin(), cmds.end());
        if (m_commands.size() > m_maxSize)
        {
            m_commands.erase(m_commands.begin(), m_commands.begin() + static_cast<Dt>(m_commands.size() - m_maxSize));
        }
    }

    [[nodiscard]] std::vector<std::string> commands() const override
    {
        auto cmds = std::vector<std::string>(m_commands.begin(), m_commands.end());
        return cmds;
    }

    void clear() override
    {
        m_commands.clear();
    }

private:
    const std::size_t       m_maxSize;
    std::deque<std::string> m_commands;

    static constexpr std::size_t k_MHistorySizeDef = 1000;
};

}  // namespace cli

#endif  // __CLI_HISTORYSTORAGE_H_INCLUDED__
