// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLI_H_INCLUDED__
#define __CLI_CLI_H_INCLUDED__

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <ioapi/cmn_attribute.hpp>

#include "detail/fromstring.h"
#include "detail/history.h"
#include "detail/historystorage.h"
#include "detail/split.h"

namespace cli
{
    // inner class to provide a global output stream
    class OutStream : public std::basic_ostream<char>, public std::streambuf
    {
    public:
        OutStream() : std::basic_ostream<char>(this)
        {
        }

        // std::streambuf overrides
        std::streamsize xsputn(const char* stm, std::streamsize n) override
        {
            for (auto& os : m_ostreams)
            {
                os->rdbuf()->sputn(stm, n);
            }
            return n;
        }
        int overflow(int val) override
        {
            for (auto& os : m_ostreams)
            {
                *os << static_cast<char>(val);
            }
            return val;
        }

        void registerStream(std::ostream& os)
        {
            m_ostreams.push_back(&os);
        }
        void unRegisterStream(std::ostream& os)
        {
            m_ostreams.erase(std::remove(m_ostreams.begin(), m_ostreams.end(), &os), m_ostreams.end());
        }

    private:
        std::vector<std::ostream*> m_ostreams;
    };

// forward declarations
class Menu;
class CliSession;

class Cli
{
public:
    ~Cli() = default;

    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(Cli)

    /**
     * @brief Construct a new Cli object having a given root menu that contains the first level commands available.
     *
     * @param rootMenu is the @c Menu containing the first level commands available to the user.
     * @param historyStorage is the policy for the storage of the cli commands history. You must pass an istance of
     * a class derived from @c HistoryStorage. The library provides these policies:
     *   - @c VolatileHistoryStorage
     *   - @c FileHistoryStorage it's a persistent history. I.e., the command history is preserved after your
     * application is restarted.
     *
     * However, you can develop your own, just derive a class from @c HistoryStorage .
     */
    explicit Cli(std::unique_ptr<Menu>           rootMenu,
                 std::unique_ptr<HistoryStorage> historyStorage = std::make_unique<VolatileHistoryStorage>()) :
        m_globalHistoryStorage(std::move(historyStorage)),
        m_rootMenu(std::move(rootMenu)),
        m_exitAction{}
    {
    }

    /**
     * @brief Add a global exit action that is called every time a session (local or remote) gets the "exit" command.
     *
     * @param action the function to be called when a session exits, taking a @c std::ostream& parameter to write on
     * that session console.
     */
    void exitAction(const std::function<void(std::ostream&)>& action)
    {
        m_exitAction = action;
    }

    /**
     * @brief Add an handler that will be called when a @c std::exception (or derived) is thrown inside a command
     * handler.If an exception handler is not set, the exception will be logget on the session output stream.
     *
     * @param handler the function to be called when an exception is thrown, taking a @c std::ostream& parameter to
     * write on that session console and the exception thrown.
     */
    void stdExceptionHandler(
        const std::function<void(std::ostream&, const std::string& cmd, const std::exception&)>& handler)
    {
        m_exceptionHandler = handler;
    }

    /**
     * @brief Get a global out stream object that can be used to print on every session currently connected (local and
     * remote)
     * @return OutStream& the reference to the global out stream writing on every session console
     */
    static OutStream& cout()
    {
        return *CoutPtr();
    }

    Menu* getRootMenu()
    {
        return m_rootMenu.get();
    }

private:
    friend class CliSession;

    static std::shared_ptr<OutStream> CoutPtr()
    {
        static std::shared_ptr<OutStream> s = std::make_shared<OutStream>();
        return s;
    }

    void exitAction(std::ostream& out)
    {
        if (m_exitAction)
        {
            m_exitAction(out);
        }
    }

    void stdExceptionHandler(std::ostream& out, const std::string& cmd, const std::exception& err)
    {
        if (m_exceptionHandler)
        {
            m_exceptionHandler(out, cmd, err);
        }
        else
        {
            out << err.what() << '\n';
        }
    }

    static void registerStream(std::ostream& ostm)
    {
        cout().registerStream(ostm);
    }

    static void unRegisterStream(std::ostream& ostm)
    {
        cout().unRegisterStream(ostm);
    }

    void storeCommands(const std::vector<std::string>& cmds)
    {
        m_globalHistoryStorage->store(cmds);
    }

    [[nodiscard]] std::vector<std::string> getCommands() const
    {
        return m_globalHistoryStorage->commands();
    }

    std::unique_ptr<HistoryStorage>    m_globalHistoryStorage;
    std::unique_ptr<Menu>              m_rootMenu;  // just to keep it alive
    std::function<void(std::ostream&)> m_exitAction;
    std::function<void(std::ostream&, const std::string& cmd, const std::exception&)> m_exceptionHandler;
};


class Command
{
public:
    explicit Command(std::string name) : m_name(std::move(name))
    {
    }
    virtual ~Command() noexcept = default;

    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(Command)

    virtual void enable()
    {
        m_enabled = true;
    }
    virtual void disable()
    {
        m_enabled = false;
    }
    virtual bool exec(const std::vector<std::string>& cmdLine, CliSession& session) = 0;
    virtual void help(std::ostream& out) const                                      = 0;
    // Returns the collection of completions relatives to this command.
    // For simple commands, provides a base implementation that use the name of the command
    // for aggregate commands (i.e., Menu), the function is redefined to give the menu command
    // and the subcommand recursively
    [[nodiscard]] virtual std::vector<std::string> getCompletionRecursive(const std::string& line) const
    {
        if (!m_enabled)
        {
            return {};
        }

        if (m_name.rfind(line, 0) == 0)  // name starts_with line
        {
            return {m_name};
        }

        return {};
    }
    [[nodiscard]] const std::string& name() const
    {
        return m_name;
    }

protected:
    [[nodiscard]] bool isEnabled() const
    {
        return m_enabled;
    }

private:
    const std::string m_name;
    bool              m_enabled{true};
};

// ********************************************************************
// free utility function to get completions from a list of commands and the current line
inline std::vector<std::string> getCompletions(const std::shared_ptr<std::vector<std::shared_ptr<Command>>>& cmds,
                                               const std::string& currentLine)
{
    std::vector<std::string> result;
    std::for_each(cmds->begin(), cmds->end(), [&currentLine, &result](const auto& cmd) {
        auto comp = cmd->getCompletionRecursive(currentLine);
        result.insert(result.end(), std::make_move_iterator(comp.begin()), std::make_move_iterator(comp.end()));
    });
    return result;
}


class CliSession
{
public:
    CliSession(Cli& cli, std::ostream& out, std::size_t historySize = k_MHistorySizeDef);
    virtual ~CliSession() noexcept
    {
        m_coutPtr->unRegisterStream(m_out);
    }

    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(CliSession)

    void feed(const std::string& cmd);

    void prompt();

    void current(Menu* menu)
    {
        m_current = menu;
    }

    std::ostream& outStream()
    {
        return m_out;
    }

    void help() const;

    void exit()
    {
        m_exitAction(m_out);
        m_cli.exitAction(m_out);

        auto cmds = m_history.getCommands();
        m_cli.storeCommands(cmds);

        m_exit = true;  // prevent the prompt to be shown
    }

    void exitAction(const std::function<void(std::ostream&)>& action)
    {
        m_exitAction = action;
    }

    void showHistory() const
    {
        m_history.show(m_out);
    }

    std::string previousCmd(const std::string& line)
    {
        return m_history.previous(line);
    }

    std::string nextCmd()
    {
        return m_history.next();
    }

    [[nodiscard]] std::vector<std::string> getCompletions(std::string currentLine) const;

private:
    Cli&                               m_cli;
    std::shared_ptr<cli::OutStream>    m_coutPtr;
    Menu*                              m_current;
    std::unique_ptr<Menu>              m_globalScopeMenu;
    std::ostream&                      m_out;
    std::function<void(std::ostream&)> m_exitAction = [](std::ostream&) { };
    detail::History m_history;
    bool            m_exit{false};  // to prevent the prompt after exit command

    static constexpr std::size_t k_MHistorySizeDef = 100;
};


class CmdHandler
{
public:
    using CmdVec = std::vector<std::shared_ptr<Command>>;
    CmdHandler() : m_descriptor(std::make_shared<Descriptor>())
    {
    }
    CmdHandler(const std::weak_ptr<Command>& command, const std::weak_ptr<CmdVec>& vec) :
        m_descriptor(std::make_shared<Descriptor>(command, vec))
    {
    }
    void enable() const
    {
        if (m_descriptor)
        {
            m_descriptor->enable();
        }
    }
    void disable() const
    {
        if (m_descriptor)
        {
            m_descriptor->disable();
        }
    }
    void remove() const
    {
        if (m_descriptor)
        {
            m_descriptor->remove();
        }
    }

private:
    struct Descriptor
    {
        Descriptor() = default;
        Descriptor(std::weak_ptr<Command> command, std::weak_ptr<CmdVec> vec) :
            mCmd(std::move(command)),
            mCmds(std::move(vec))
        {
        }
        void enable() const
        {
            if (auto command = mCmd.lock())
            {
                command->enable();
            }
        }
        void disable() const
        {
            if (auto command = mCmd.lock())
            {
                command->disable();
            }
        }
        void remove() const
        {
            auto scmd  = mCmd.lock();
            auto scmds = mCmds.lock();
            if (scmd && scmds)
            {
                auto res = std::find_if(scmds->begin(), scmds->end(), [&](const auto& command) {
                    return command.get() == scmd.get();
                });
                if (res != scmds->end())
                {
                    scmds->erase(res);
                }
            }
        }
        std::weak_ptr<Command> mCmd;
        std::weak_ptr<CmdVec>  mCmds;
    };
    std::shared_ptr<Descriptor> m_descriptor;
};


class Menu : public Command
{
public:
    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(Menu)

    Menu() : Command({}), m_description(), m_cmds(std::make_shared<Cmds>())
    {
    }
    ~Menu() noexcept override = default;

    explicit Menu(const std::string& name, std::string desc = "(menu)") :
        Command(name),
        m_description(std::move(desc)),
        m_cmds(std::make_shared<Cmds>())
    {
    }

    template<typename R, typename... Args>
    CmdHandler insert(const std::string& cmdName,
                      R (*fun)(std::ostream&, Args...),
                      const std::string&              help,
                      const std::vector<std::string>& parDesc = {});

    template<typename F>
    CmdHandler insert(const std::string&              cmdName,
                      F                               fun,
                      const std::string&              help    = "",
                      const std::vector<std::string>& parDesc = {})
    {
        // dispatch to private Insert methods
        return insert(cmdName, help, parDesc, fun, &F::operator());
    }

    template<typename F>
    CmdHandler insert(const std::string&              cmdName,
                      const std::vector<std::string>& parDesc,
                      F                               fun,
                      const std::string&              help = "")
    {
        // dispatch to private Insert methods
        return insert(cmdName, help, parDesc, fun, &F::operator());
    }

    CmdHandler insert(std::unique_ptr<Command>&& cmd)
    {
        const std::shared_ptr<Command> scmd(std::move(cmd));
        CmdHandler                     hdl(scmd, m_cmds);
        m_cmds->push_back(scmd);
        return hdl;
    }

    CmdHandler insert(std::unique_ptr<Menu>&& menu)
    {
        const std::shared_ptr<Menu> smenu(std::move(menu));
        CmdHandler                  hdl(smenu, m_cmds);
        smenu->m_parent = this;
        m_cmds->push_back(smenu);
        return hdl;
    }

    bool exec(const std::vector<std::string>& cmdLine, CliSession& session) override
    {
        if (!isEnabled())
        {
            return false;
        }
        if (cmdLine[0] == name())
        {
            if (cmdLine.size() == 1)
            {
                session.current(this);
                return true;
            }
            else
            {
                // check also for subcommands
                const std::vector<std::string> subCmdLine(cmdLine.begin() + 1, cmdLine.end());
                for (auto& cmd : *m_cmds)
                {
                    if (cmd->exec(subCmdLine, session))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool scanCmds(const std::vector<std::string>& cmdLine, CliSession& session)
    {
        if (!isEnabled())
        {
            return false;
        }
        for (auto& cmd : *m_cmds)
        {
            if (cmd->exec(cmdLine, session))
            {
                return true;
            }
        }
        return (m_parent != nullptr && m_parent->exec(cmdLine, session));
    }

    [[nodiscard]] std::string prompt() const
    {
        return name();
    }

    void mainHelp(std::ostream& out)
    {
        if (!isEnabled())
        {
            return;
        }
        for (const auto& cmd : *m_cmds)
        {
            cmd->help(out);
        }
        if (m_parent != nullptr)
        {
            m_parent->help(out);
        }
    }

    void help(std::ostream& out) const override
    {
        if (!isEnabled())
        {
            return;
        }
        out << " - " << name() << "\n\t" << m_description << "\n";
    }

    // returns:
    // - the completions of this menu command
    // - the recursive completions of subcommands and parent menu
    [[nodiscard]] std::vector<std::string> getCompletions(const std::string& currentLine) const
    {
        auto result = cli::getCompletions(m_cmds, currentLine);
        if (m_parent != nullptr)
        {
            auto ch = m_parent->getCompletionRecursive(currentLine);
            result.insert(result.end(), std::make_move_iterator(ch.begin()), std::make_move_iterator(ch.end()));
        }
        return result;
    }

    // returns:
    // - the completion of this menu command
    // - the recursive completions of the subcommands
    [[nodiscard]] std::vector<std::string> getCompletionRecursive(const std::string& line) const override
    {
        if (line.rfind(name(), 0) == 0)  // line starts_with Name()
        {
            auto rest = line;
            rest.erase(0, name().size());
            // trim_left(rest);
            rest.erase(rest.begin(), std::find_if(rest.begin(), rest.end(), [](int ch) {
                           return !std::isspace(ch);
                       }));
            std::vector<std::string> result;
            for (const auto& cmd : *m_cmds)
            {
                auto cs = cmd->getCompletionRecursive(rest);
                for (const auto& ch : cs)
                {
                    result.push_back(name() + ' ' + ch);  // concat submenu with command
                }
            }
            return result;
        }
        return Command::getCompletionRecursive(line);
    }

private:
    template<typename F, typename R, typename... Args>
    CmdHandler insert(const std::string&              cmdName,
                      const std::string&              help,
                      const std::vector<std::string>& parDesc,
                      F&                              fun,
                      R (F::* /*unused*/)(std::ostream& out, Args...) const);

    template<typename F, typename R>
    CmdHandler insert(const std::string&              cmdName,
                      const std::string&              help,
                      const std::vector<std::string>& parDesc,
                      F&                              fun,
                      R (F::* /*unused*/)(std::ostream& out, const std::vector<std::string>&) const);

    template<typename F, typename R>
    CmdHandler insert(const std::string&              cmdName,
                      const std::string&              help,
                      const std::vector<std::string>& parDesc,
                      F&                              fun,
                      R (F::* /*unused*/)(std::ostream& out, std::vector<std::string>) const);

    Menu*             m_parent{nullptr};
    const std::string m_description;
    // using shared_ptr instead of unique_ptr to get a weak_ptr
    // for the CmdHandler::Descriptor
    using Cmds = std::vector<std::shared_ptr<Command>>;
    std::shared_ptr<Cmds> m_cmds;
};


template<typename... Args>
struct Select;

template<typename P, typename... Args>
struct Select<P, Args...>
{
    template<typename F, typename InputIt>
    static void exec(const F& fun, InputIt first, InputIt last)
    {
        assert(first != last);
        assert(std::distance(first, last) == 1 + sizeof...(Args));
        const P pun = detail::fromString<typename std::decay<P>::type>(*first);
        auto    gun = [&](auto... pars) {
            fun(pun, pars...);
        };
        Select<Args...>::exec(gun, std::next(first), last);
    }
};

template<>
struct Select<>
{
    template<typename F, typename InputIt>
    static void exec(const F& fun, InputIt first, InputIt last)
    {
        // silence the unused warning in release mode when assert is disabled
        static_cast<void>(first);
        static_cast<void>(last);

        assert(first == last);
        fun();
    }
};

template<typename F, typename... Args>
class VariadicFunctionCommand : public Command
{
public:
    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(VariadicFunctionCommand)

    VariadicFunctionCommand(const std::string& name, F fun, std::string desc, std::vector<std::string> parDesc) :
        Command(name),
        m_func(std::move(fun)),
        m_description(std::move(desc)),
        m_parameterDesc(std::move(parDesc))
    {
    }

    bool exec(const std::vector<std::string>& cmdLine, CliSession& session) override
    {
        if (!isEnabled())
        {
            return false;
        }
        const std::size_t paramSize = sizeof...(Args);
        if (cmdLine.size() != paramSize + 1)
        {
            return false;
        }
        if (name() == cmdLine[0])
        {
            try
            {
                auto fun = [&](auto... pars) {
                    m_func(session.outStream(), pars...);
                };
                Select<Args...>::exec(fun, std::next(cmdLine.begin()), cmdLine.end());
            }
            catch (std::bad_cast&)
            {
                return false;
            }
            return true;
        }
        return false;
    }

    void help(std::ostream& out) const override
    {
        if (!isEnabled())
        {
            return;
        }
        out << " - " << name();
        for (const auto& ch : m_parameterDesc)
        {
            out << " <" << ch << '>';
        }
        out << "\n\t" << m_description << "\n";
    }

private:
    const F                        m_func;
    const std::string              m_description;
    const std::vector<std::string> m_parameterDesc;
};

template<typename F>
class FreeformCommand : public Command
{
public:
    //class are neither copyable nor movable
    CMN_UNCOPYABLE_IMMOVABLE(FreeformCommand)

    FreeformCommand(const std::string& name, F fun, std::string desc, std::vector<std::string> parDesc) :
        Command(name),
        m_func(std::move(fun)),
        m_description(std::move(desc)),
        m_parameterDesc(std::move(parDesc))
    {
    }

    bool exec(const std::vector<std::string>& cmdLine, CliSession& session) override
    {
        if (!isEnabled())
        {
            return false;
        }
        assert(!cmdLine.empty());
        if (name() == cmdLine[0])
        {
            m_func(session.outStream(), std::vector<std::string>(std::next(cmdLine.begin()), cmdLine.end()));
            return true;
        }
        return false;
    }
    void help(std::ostream& out) const override
    {
        if (!isEnabled())
        {
            return;
        }
        out << " - " << name();

        for (const auto& ch : m_parameterDesc)
        {
            out << " <" << ch << '>';
        }
        out << "\n\t" << m_description << "\n";
    }

private:
    const F                        m_func;
    const std::string              m_description;
    const std::vector<std::string> m_parameterDesc;
};

// CliSession implementation
inline CliSession::CliSession(Cli& cli, std::ostream& out, std::size_t historySize) :
    m_cli(cli),
    m_coutPtr(Cli::CoutPtr()),
    m_current(m_cli.getRootMenu()),
    m_globalScopeMenu(std::make_unique<Menu>()),
    m_out(out),
    m_history(historySize)
{
    m_history.loadCommands(m_cli.getCommands());

    m_coutPtr->registerStream(out);
    m_globalScopeMenu->insert(
        "help",
        [this](std::ostream&) {
            help();
        },
        "This help message");
    m_globalScopeMenu->insert(
        "exit",
        [this](std::ostream&) {
            exit();
        },
        "Quit the session");
#ifdef CLI_HISTORY_CMD
    m_globalScopeMenu->insert(
        "history",
        [this](std::ostream&) {
            showHistory();
        },
        "Show the history");
#endif
}

inline void CliSession::feed(const std::string& cmd)
{
    std::vector<std::string> strs;
    detail::split(strs, cmd);
    if (strs.empty())  // just hit enter
    {
        return;
    }

    m_history.newCommand(cmd);  // add anyway to history
    try
    {
        // global cmds check
        bool found = m_globalScopeMenu->scanCmds(strs, *this);
        if (!found)  // root menu recursive cmds check
        {
            found = m_current->scanCmds(strs, *this);
        }

        if (!found)  // error msg if not found
        {
            m_out << "unsupported command: " << cmd << '\n';
        }
    }
    catch (const std::exception& e)
    {
        m_cli.stdExceptionHandler(m_out, cmd, e);
    }
    catch (...)
    {
        m_out << "Cli. Unknown exception caught handling command line \"" << cmd << "\"\n";
    }
}

inline void CliSession::prompt()
{
    if (m_exit)
    {
        return;
    }
    m_out << m_current->prompt() << "# " << std::flush;
}

inline void CliSession::help() const
{
    m_out << "Commands available:\n";
    m_globalScopeMenu->mainHelp(m_out);
    m_current->mainHelp(m_out);
}

inline std::vector<std::string> CliSession::getCompletions(std::string currentLine) const
{
    // trim_left(currentLine);
    currentLine.erase(currentLine.begin(), std::find_if(currentLine.begin(), currentLine.end(), [](int ch) {
                          return !std::isspace(ch);
                      }));
    auto v1 = m_globalScopeMenu->getCompletions(currentLine);
    auto v3 = m_current->getCompletions(currentLine);
    v1.insert(v1.end(), std::make_move_iterator(v3.begin()), std::make_move_iterator(v3.end()));

    // removes duplicates (std::unique requires a sorted container)
    std::sort(v1.begin(), v1.end());
    auto ip = std::unique(v1.begin(), v1.end());
    v1.resize(static_cast<std::size_t>(std::distance(v1.begin(), ip)));

    return v1;
}

// Menu implementation
template<typename R, typename... Args>
CmdHandler Menu::insert(const std::string& cmdName,
                        R (*fun)(std::ostream&, Args... args),
                        const std::string&              help,
                        const std::vector<std::string>& parDesc)
{
    using F = R (*)(std::ostream&, Args...);
    return insert(std::make_unique<VariadicFunctionCommand<F, Args...>>(cmdName, fun, help, parDesc));
}

template<typename F, typename R, typename... Args>
CmdHandler Menu::insert(const std::string&              cmdName,
                        const std::string&              help,
                        const std::vector<std::string>& parDesc,
                        F&                              fun,
                        R (F::* /*unused*/)(std::ostream& out, Args... args) const)
{
    return insert(std::make_unique<VariadicFunctionCommand<F, Args...>>(cmdName, fun, help, parDesc));
}

template<typename F, typename R>
CmdHandler Menu::insert(const std::string&              cmdName,
                        const std::string&              help,
                        const std::vector<std::string>& parDesc,
                        F&                              fun,
                        R (F::* /*unused*/)(std::ostream& out, const std::vector<std::string>& args) const)
{
    return insert(std::make_unique<FreeformCommand<F>>(cmdName, fun, help, parDesc));
}

template<typename F, typename R>
CmdHandler Menu::insert(const std::string&              cmdName,
                        const std::string&              help,
                        const std::vector<std::string>& parDesc,
                        F&                              fun,
                        R (F::* /*unused*/)(std::ostream& out, std::vector<std::string> args) const)
{
    return insert(std::make_unique<FreeformCommand<F>>(cmdName, fun, help, parDesc));
}
}  // namespace cli

#include "detail/clilocalsession.h"
#include "detail/standaloneasiolib.h"

#include "detail/genericasioremotecli.h"
#include "detail/genericasioscheduler.h"
#include "detail/genericcliasyncsession.h"

#endif  // __CLI_CLI_H_INCLUDED__
