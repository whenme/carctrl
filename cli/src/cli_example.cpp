// SPDX-License-Identifier: GPL-2.0

#include <cli_example.h>
#include <cli_impl.h>

namespace cli
{
void CliExample::initCliCommand(std::unique_ptr<Menu>& rootMenu)
{
    const std::string name    = getGroupName();
    auto              cliMenu = std::make_unique<Menu>(name);
    CmdHandler        colorCmd;
    CmdHandler        nocolorCmd;

    // setup cli
    cliMenu->Insert(
        "hello_everysession",
        [](std::ostream&) {
            Cli::cout() << "Hello, everybody" << std::endl;
        },
        "Print hello everybody on all open sessions");
    cliMenu->Insert("file",
                    [](std::ostream& out, int fd) {
                        out << "file descriptor: " << fd << "\n";
                    },
                    "Print the file descriptor specified",
                    {"file_descriptor"});
    cliMenu->Insert(
        "echo",
        {"string to echo", "string1 to echo"},
        [](std::ostream& out, const std::string& arg) {
            out << arg << "\n";
        },
        "Print the string passed as parameter");
    cliMenu->Insert(
        "error",
        [](std::ostream&) {
            Cli::cout() << "error handler..." << std::endl;
            throw std::logic_error("Error in cmd");
        },
        "Throw an exception in the command handler");
    cliMenu->Insert(
        "add",
        {"first_term", "second_term"},
        [](std::ostream& out, int xval, int yval) {
            out << xval << " + " << yval << " = " << (xval + yval) << "\n";
        },
        "Print the sum of the two numbers");
    colorCmd = cliMenu->Insert(
        "color",
        [&](std::ostream& out) {
            out << "Colors ON\n";
            colorCmd.Disable();
            nocolorCmd.Enable();
        },
        "Enable colors in the cli");
    nocolorCmd = cliMenu->Insert(
        "nocolor",
        [&](std::ostream& out) {
            out << "Colors OFF\n";
            colorCmd.Enable();
            nocolorCmd.Disable();
        },
        "Disable colors in the cli");
    nocolorCmd.Disable();  // start w/o colors, so we disable this command

    if (rootMenu != nullptr)
    {
        rootMenu->Insert(std::move(cliMenu));
    }
}

}  // namespace cli
