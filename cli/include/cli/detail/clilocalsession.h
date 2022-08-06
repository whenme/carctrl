// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_CLILOCALSESSION_H_INCLUDED__
#define __CLI_CLILOCALSESSION_H_INCLUDED__

#include <cli/cli.h>
#include <cli/detail/inputhandler.h>
#include <cli/detail/keyboard.h>
#include <ostream>

namespace cli::detail
{

class Scheduler;

/**
 * @brief CliLocalSession represents a local session
 * You should instantiate it to start an interactive prompt on the standard
 * input/output of your application
 * The handlers of the commands will be invoked in the same thread the @c Scheduler runs
 */
class CliLocalTerminalSession : public CliSession
{
public:
    /**
     * @brief Construct a new Cli Local Terminal Session object that uses the specified @c std::ostream
     * for output. You can also specify a size for the command history
     *
     * @param cli The cli object that defines the menu hierarchy for this session
     * @param scheduler The scheduler that will process the command handlers
     * @param out the output stream where command output will be printed
     * @param historySize the size of the command history
     */
    CliLocalTerminalSession(Cli&          cli,
                            Scheduler&    scheduler,
                            std::ostream& out,
                            std::size_t   historySize = k_MHistorySizeDef) :
        CliSession(cli, out, historySize),
        m_kb(scheduler),
        m_ih(*this, m_kb)
    {
        prompt();
    }

private:
    Keyboard     m_kb;
    InputHandler m_ih;

    static constexpr std::size_t k_MHistorySizeDef = 100;
};

using CliLocalSession = CliLocalTerminalSession;
}  // namespace cli::detail

#endif  // __CLI_CLILOCALSESSION_H_INCLUDED__
